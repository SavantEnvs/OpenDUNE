/* Landscape tiles: Sprites_LoadTiles() (src/sprites.c) -> Tiles_LoadICNFile (ICON.ICN IFF chunks
 * SSET/RTBL/RPAL/SINF, Sprites_Decode, GFX_Init_TilesInfo), the ICON.MAP icon-group table read
 * with File_ReadWholeFileLE16, and UNIT.EMC via Script_LoadFromFile.
 * Input layout (harness-defined, since the loader reads two files): a little-endian uint32 N,
 * then N bytes of ICON.ICN, then the rest is ICON.MAP. */
#include "fuzz_common.h"
#include "gfx.h"
#include "sprites.h"
#include "script/script.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	static int inited = 0;
	uint32 n;
	if (!inited) { fz_init(); GFX_Init(); inited = 1; }
	if (size < 4) return -1;
	n = (uint32)data[0] | ((uint32)data[1] << 8) | ((uint32)data[2] << 16) | ((uint32)data[3] << 24);
	data += 4; size -= 4;
	if (n > size) return -1;
	fz_clear();
	fz_write("ICON.ICN", data, n);
	fz_write("ICON.MAP", data + n, size - n);
	if (File_Init()) {
		Sprites_LoadTiles();
		Sprites_UnloadTiles();
		Script_ClearInfo(g_scriptUnit);
		free(g_tilesPixels); g_tilesPixels = NULL;
		free(g_iconRTBL); g_iconRTBL = NULL;
		free(g_iconRPAL); g_iconRPAL = NULL;
		free(g_iconMap); g_iconMap = NULL;
	}
	File_Uninit();
	return 0;
}
