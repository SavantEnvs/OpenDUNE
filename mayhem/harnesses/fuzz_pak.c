/* PAK archives: File_Init() parses every *.PAK in the data dir (_File_Init_ProcessPak, src/file.c)
 * and indexes its entries; File_Exists/File_ReadWholeFile/ChunkFile_* then read entries through
 * the PAK offsets. The input is data/FUZZ.PAK; the harness reads back the entry names the
 * original game PAKs use and an IFF chunk walk over each. */
#include "fuzz_common.h"
#include "multichar.h"
#include "os/endian.h"

static const char *const names[] = {
	"MOUSE.SHP", "ICON.ICN", "ICON.MAP", "UNIT.EMC", "DUNE.ENG", "new8p.fnt", "INTRO.WSA",
	"SCENA001.INI", "A", "B", "FUZZ", "X.CPS", NULL
};

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	static int inited = 0;
	int i;
	if (!inited) { fz_init(); inited = 1; }
	fz_clear();
	fz_write("FUZZ.PAK", data, size);
	if (File_Init()) {
		for (i = 0; names[i] != NULL; i++) {
			uint8 buf[64];
			uint8 idx;
			if (!File_Exists(names[i])) continue;
			free(File_ReadWholeFile(names[i]));
			idx = ChunkFile_Open(names[i]);
			if (idx != FILE_INVALID) {
				ChunkFile_Read(idx, HTOBE32(CC_INFO), buf, sizeof(buf));
				ChunkFile_Read(idx, HTOBE32(CC_DATA), buf, sizeof(buf));
				ChunkFile_Close(idx);
			}
		}
	}
	File_Uninit();
	return 0;
}
