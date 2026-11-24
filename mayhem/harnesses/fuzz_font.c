/* FNT fonts: Font_Init() -> Font_LoadFile() (src/gui/font.c) header/offset/width tables, then
 * Font_GetStringWidth over a fixed string. The input is written as all three font files the
 * engine loads (INTRO.FNT, new6p.fnt, new8p.fnt). */
#include "fuzz_common.h"
#include "gui/font.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	static int inited = 0;
	if (!inited) { fz_init(); inited = 1; }
	fz_clear();
	fz_write("INTRO.FNT", data, size);
	fz_write("new6p.fnt", data, size);
	fz_write("new8p.fnt", data, size);
	if (File_Init()) {
		if (Font_Init()) {
			Font_Select(g_fontNew8p);
			(void)Font_GetStringWidth("OpenDUNE fuzz 0123456789 !?");
			Font_Select(g_fontIntro);
			(void)Font_GetStringWidth("Dune II");
		}
		if (g_fontIntro != NULL && g_fontNew6p != NULL && g_fontNew8p != NULL) Font_Uninit();
	}
	File_Uninit();
	return 0;
}
