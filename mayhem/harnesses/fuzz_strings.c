/* Language string files: String_Init() -> String_Load() (src/string.c) offset tables, plain and
 * compressed (String_DecompressAndTranslate) strings, and the V1.0/EU fix-ups. The input is
 * written as every *.ENG file String_Init reads (g_config.language is ENGLISH). */
#include "fuzz_common.h"
#include "config.h"
#include "string.h"

static const char *const files[] = {
	"DUNE.ENG", "MESSAGE.ENG", "INTRO.ENG", "TEXTH.ENG", "TEXTA.ENG", "TEXTO.ENG", "PROTECT.ENG", NULL
};

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	static int inited = 0;
	int i;
	if (!inited) { fz_init(); inited = 1; }
	fz_clear();
	for (i = 0; files[i] != NULL; i++) fz_write(files[i], data, size);
	g_config.language = LANGUAGE_ENGLISH;
	if (File_Init()) {
		unsigned sum = 0;
		uint16 s;
		String_Init();
		for (s = 0; s < 526; s++) sum += (unsigned)strlen(String_Get_ByIndex(s));
		(void)sum;
		String_Uninit();
	}
	File_Uninit();
	return 0;
}
