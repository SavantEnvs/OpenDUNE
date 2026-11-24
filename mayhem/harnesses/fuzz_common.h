/* Shared scaffolding for the OpenDUNE fuzz harnesses.
 *
 * The engine reads every game file through src/file.c, which indexes the data directory once in
 * File_Init() and then opens files by fixed names. Each harness therefore owns a private scratch
 * directory under $TMPDIR (default /tmp): HOME points there, so the engine's personal dir and
 * opendune.ini (~/.config/opendune/opendune.ini, which sets datadir= to <scratch>/data) live there.
 * Per input: wipe <scratch>/data, write the input under the file name the loader expects,
 * File_Init(), run the loader, free everything, File_Uninit(). Nothing survives between inputs
 * except the read-only opendune.ini and the directories. */
#ifndef FUZZ_COMMON_H
#define FUZZ_COMMON_H

#define _XOPEN_SOURCE 700
#include <dirent.h>
#include <errno.h>
#include <ftw.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "types.h"
#include "file.h"
#include "inifile.h"

static char fz_root[2048];
static void fz_cleanup(void);
static char fz_data[2100];

static int fz_rm_cb(const char *path, const struct stat *sb, int flag, struct FTW *ftw)
{
	(void)sb; (void)flag; (void)ftw;
	remove(path);
	return 0;
}

/* A one-input-per-process (AFL) run that crashes never reaches atexit(): remove the scratch dirs
 * of harness processes that no longer exist, so crashes do not pile up directories in $TMPDIR. */
static void fz_reap(const char *tmp)
{
	DIR *d = opendir(tmp);
	struct dirent *e;
	char p[4200];
	if (d == NULL) return;
	while ((e = readdir(d)) != NULL) {
		char *end;
		long pid;
		if (strncmp(e->d_name, "opendune-fuzz-", 14) != 0) continue;
		pid = strtol(e->d_name + 14, &end, 10);
		if (*end != '\0' || pid <= 0) continue;
		if (kill((pid_t)pid, 0) == 0 || errno != ESRCH) continue;
		snprintf(p, sizeof(p), "%s/%s", tmp, e->d_name);
		nftw(p, fz_rm_cb, 16, FTW_DEPTH | FTW_PHYS);
	}
	closedir(d);
}

static void fz_init(void)
{
	char ini[2200];
	const char *tmp = getenv("TMPDIR");
	FILE *f;

	if (tmp == NULL || *tmp == '\0') tmp = "/tmp";
	fz_reap(tmp);
	snprintf(fz_root, sizeof(fz_root), "%s/opendune-fuzz-%ld", tmp, (long)getpid());
	nftw(fz_root, fz_rm_cb, 16, FTW_DEPTH | FTW_PHYS);
	if (mkdir(fz_root, 0700) != 0) { perror(fz_root); abort(); }
	snprintf(fz_data, sizeof(fz_data), "%s/data", fz_root);
	mkdir(fz_data, 0700);
	snprintf(ini, sizeof(ini), "%s/.config", fz_root); mkdir(ini, 0700);
	snprintf(ini, sizeof(ini), "%s/.config/opendune", fz_root); mkdir(ini, 0700);
	setenv("HOME", fz_root, 1);
	snprintf(ini, sizeof(ini), "%s/.config/opendune/opendune.ini", fz_root);
	f = fopen(ini, "wb");
	if (f == NULL) { perror(ini); abort(); }
	fprintf(f, "[opendune]\ndatadir=%s\nsavedir=%s/.config/opendune\n", fz_data, fz_root);
	fclose(f);
	if (!Load_IniFile()) abort();
	atexit(fz_cleanup);
}

/* remove every file in the data dir */
static void fz_clear(void)
{
	DIR *d = opendir(fz_data);
	struct dirent *e;
	char p[4200];

	if (d == NULL) return;
	while ((e = readdir(d)) != NULL) {
		if (e->d_name[0] == '.') continue;
		snprintf(p, sizeof(p), "%s/%s", fz_data, e->d_name);
		unlink(p);
	}
	closedir(d);
}

/* write <data>/<name> */
static void fz_write(const char *name, const uint8_t *data, size_t size)
{
	char p[4200];
	FILE *f;

	snprintf(p, sizeof(p), "%s/%s", fz_data, name);
	f = fopen(p, "wb");
	if (f == NULL) { perror(p); abort(); }
	if (size != 0 && fwrite(data, 1, size, f) != size) { perror(p); abort(); }
	fclose(f);
}

/* remove the whole scratch tree (one-shot targets, so AFL execs leave nothing behind) */
static void fz_cleanup(void)
{
	char p[4200];
	fz_clear();
	rmdir(fz_data);
	snprintf(p, sizeof(p), "%s/.config/opendune/opendune.ini", fz_root); unlink(p);
	snprintf(p, sizeof(p), "%s/.config/opendune", fz_root); rmdir(p);
	snprintf(p, sizeof(p), "%s/.config", fz_root); rmdir(p);
	rmdir(fz_root);
}

#endif /* FUZZ_COMMON_H */
