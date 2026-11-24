/* opendune.ini: Load_IniFile() + IniFile_GetString/GetInteger (src/inifile.c, src/ini.c) for the
 * keys the engine reads (strings as strings, integers as integers), and SetLanguage_From_IniFile(). It also enumerates the [opendune] keys
 * with Ini_GetString(key == NULL), the way scenario.c walks a category, and looks each found key
 * up again as a string. The enumeration buffer is sized so it can hold every key of the input (each key is at
 * most the input's length plus a NUL, plus the list's two-NUL end). The input is
 * ~/.config/opendune/opendune.ini (HOME is the scratch dir). */
#include "fuzz_common.h"
#include "config.h"
#include "ini.h"

/* each key read the way the engine reads it (src/file.c, opendune.c, audio/, video/) */
static const char *const str_keys[] = {
	"language", "datadir", "savedir", "scalefilter", "mt32rompath", "fs_soundfont", "fs_audiodriver",
	NULL
};
static const char *const int_keys[] = {
	"mt32midi", "dune2_enhanced", "debug_game", "debug_scenario", "debug_skip_dialogs",
	"debug_log_game", "startport_unit_cap", "scalefactor", "framerate", "midideviceid", "fullscreen",
	"renderer_software_fallback", NULL
};

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	static int inited = 0;
	static char path[4200];
	char buf[1024];
	DuneCfg cfg;
	FILE *f;
	int i;
	char *list, *k, *ini;
	if (!inited) {
		fz_init(); Free_IniFile();
		snprintf(path, sizeof(path), "%s/.config/opendune/opendune.ini", fz_root);
		inited = 1;
	}
	f = fopen(path, "wb");
	if (f == NULL) abort();
	if (size != 0) fwrite(data, 1, size, f);
	fclose(f);
	if (Load_IniFile()) {
		for (i = 0; str_keys[i] != NULL; i++) (void)IniFile_GetString(str_keys[i], NULL, buf, sizeof(buf));
		for (i = 0; int_keys[i] != NULL; i++) (void)IniFile_GetInteger(int_keys[i], 0);
		/* g_openduneini is static to inifile.c: enumerate over a copy of the same bytes, NUL-terminated
		 * the way Load_IniFile() terminates its buffer. */
		ini = malloc(size + 1);
		memcpy(ini, data, size);
		ini[size] = '\0';
		list = malloc(2 * size + 16);
		if (Ini_GetString("opendune", NULL, NULL, list, (uint16)(2 * size + 16 > 0xFFFF ? 0xFFFF : 2 * size + 16), ini) != NULL) {
			for (k = list; *k != '\0'; k += strlen(k) + 1) {
				(void)IniFile_GetString(k, NULL, buf, sizeof(buf));
			}
		}
		free(list);
		free(ini);
		memset(&cfg, 0, sizeof(cfg));
		(void)SetLanguage_From_IniFile(&cfg);
		Free_IniFile();
	}
	return 0;
}
