/* CPS images: Sprites_LoadImage() -> Sprites_LoadCPSFile() size/palette arithmetic and
 * Sprites_Decode()/Format80_Decode() into a screen buffer (src/sprites.c), the way the cutscenes
 * load BIGPLAN.CPS. The input is data/FUZZ.CPS. */
#include "fuzz_common.h"
#include "gfx.h"
#include "sprites.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	static int inited = 0;
	static uint8 palette[256 * 3];
	if (!inited) { fz_init(); GFX_Init(); inited = 1; }
	fz_clear();
	fz_write("FUZZ.CPS", data, size);
	if (File_Init()) Sprites_LoadImage("FUZZ.CPS", SCREEN_1, palette);
	File_Uninit();
	return 0;
}
