/* extractpak (src/tools/extractpak.c): upstream's standalone PAK extractor CLI. Its main() is
 * compiled in under another name and run as `extractpak --outdir <out> <root>/in.pak`.
 *
 * The tool writes every entry to <out>/in/<entry name>, so per input it writes files whose names
 * and sizes come from the input. To keep each input independent of the ones before it:
 *  - <out> is nested OUT_DEPTH directories below <root>. An entry name is at most 255 bytes, so a
 *    "../../.." name climbs at most 85 levels and always lands inside <root>; after each input
 *    everything under <root> is removed except in.pak and the directory chain down to <out>.
 *  - File sizes are capped with RLIMIT_FSIZE (SIGXFSZ ignored, so an oversized write fails with
 *    EFBIG instead of killing the process). A wrapped entry size makes extractFile() loop over up
 *    to 4 GB; with the cap it no longer fills the disk (see fz_fwrite below for the time it takes).
 *  - extractPak() does not fclose() the PAK on success; any fd the call left open is closed after
 *    it (the fds open before the call are recorded from /proc/self/fd), so the process never runs
 *    out of descriptors. The leak itself is a property of the tool, not hidden. */
#define _GNU_SOURCE
#include <strings.h>
#include <ftw.h>
#include <dirent.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

/* The disk the tool writes to is mocked as a size-capped one: once a write to a stream has failed
 * (EFBIG past RLIMIT_FSIZE), further writes to it fail at once instead of copying 1 KB and
 * retrying the write(2) on every pass of extractFile()'s loop. Upstream ignores both return values,
 * so its control flow is unchanged; only the wasted syscalls go (a wrapped size took ~1.5 s). */
static size_t fz_fwrite(const void *p, size_t sz, size_t n, FILE *f)
{
	if (ferror_unlocked(f)) return 0;
	return fwrite(p, sz, n, f);
}

/* glibc's EOF is sticky (a read at EOF returns 0 without reading): the same, minus the locking */
static size_t fz_fread(void *p, size_t sz, size_t n, FILE *f)
{
	if (feof_unlocked(f)) return 0;
	return fread(p, sz, n, f);
}

#undef main
#define main extractpak_main
#define fwrite fz_fwrite
#define fread fz_fread
#include "../../src/tools/extractpak.c"
#undef fwrite
#undef fread
#undef main

#define OUT_DEPTH 100
#define FSIZE_CAP (1 << 20)
#define MAX_FDS 1024

static char root[2048], outd[2400], inp[2100];
static size_t root_len;

/* keep in.pak and every directory on the chain <root>/d/.../d/out; remove everything else */
static int rm_cb(const char *path, const struct stat *sb, int flag, struct FTW *ftw)
{
	size_t len = strlen(path);
	(void)sb; (void)flag;
	if (ftw->level == 0) return 0;
	if (strcmp(path, inp) == 0) return 0;
	if (strncmp(outd, path, len) == 0 && (outd[len] == '/' || outd[len] == '\0')) return 0;
	remove(path);
	return 0;
}

static void open_fds(unsigned char *set)
{
	DIR *d = opendir("/proc/self/fd");
	struct dirent *e;
	int self;
	memset(set, 0, MAX_FDS);
	if (d == NULL) return;
	self = dirfd(d);
	while ((e = readdir(d)) != NULL) {
		int fd = atoi(e->d_name);
		if (e->d_name[0] < '0' || e->d_name[0] > '9' || fd == self) continue;
		if (fd >= 0 && fd < MAX_FDS) set[fd] = 1;
	}
	closedir(d);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	static int inited = 0;
	unsigned char before[MAX_FDS], after[MAX_FDS];
	char *argv[5];
	FILE *f;
	int i;
	if (!inited) {
		const char *tmp = getenv("TMPDIR");
		struct rlimit rl;
		size_t n;
		if (tmp == NULL || *tmp == '\0') tmp = "/tmp";
		snprintf(root, sizeof(root), "%s/extractpak-fuzz-XXXXXX", tmp);
		if (mkdtemp(root) == NULL) abort();
		root_len = strlen(root);
		memcpy(outd, root, root_len + 1);
		n = root_len;
		for (i = 0; i < OUT_DEPTH; i++) {
			memcpy(outd + n, "/d", 3); n += 2;
			if (mkdir(outd, 0777) != 0) abort();
		}
		memcpy(outd + n, "/out", 5);
		if (mkdir(outd, 0777) != 0) abort();
		snprintf(inp, sizeof(inp), "%s/in.pak", root);
		signal(SIGXFSZ, SIG_IGN);
		rl.rlim_cur = rl.rlim_max = FSIZE_CAP;
		if (setrlimit(RLIMIT_FSIZE, &rl) != 0) abort();
		inited = 1;
	}
	f = fopen(inp, "wb");
	if (f == NULL) abort();
	if (size != 0) fwrite(data, 1, size, f);
	fclose(f);
	argv[0] = "extractpak"; argv[1] = "--outdir"; argv[2] = outd; argv[3] = inp; argv[4] = NULL;
	outdir = NULL;
	open_fds(before);
	extractpak_main(4, argv);
	open_fds(after);
	for (i = 0; i < MAX_FDS; i++) if (after[i] && !before[i]) close(i);
	nftw(root, rm_cb, 16, FTW_DEPTH | FTW_PHYS);
	return 0;
}
