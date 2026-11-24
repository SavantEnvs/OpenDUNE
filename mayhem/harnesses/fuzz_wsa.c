/* WSA animations: WSA_LoadFile() header/offset table + Format80/Format40 frame decoding
 * (src/wsa.c, src/codec/), then WSA_DisplayFrame() over every frame, the way house.c loads
 * STATIC.WSA into SCREEN_1. The input is data/FUZZ.WSA. */
#include "fuzz_common.h"
#include "gfx.h"
#include "wsa.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	static int inited = 0;
	void *wsa;
	if (!inited) { fz_init(); GFX_Init(); inited = 1; }
	fz_clear();
	fz_write("FUZZ.WSA", data, size);
	if (File_Init()) {
		wsa = WSA_LoadFile("FUZZ.WSA", GFX_Screen_Get_ByIndex(SCREEN_1), GFX_Screen_GetSize_ByIndex(SCREEN_1), true);
		if (wsa != NULL) {
			uint16 n = WSA_GetFrameCount(wsa), i;
			for (i = 0; i < n && i < 64; i++) WSA_DisplayFrame(wsa, i, 0, 0, SCREEN_0);
			WSA_Unload(wsa);
		}
	}
	File_Uninit();
	return 0;
}
