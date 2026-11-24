/* EMC scripts: Script_LoadFromFile() (src/script/script.c) IFF reader for the TEXT/ORDR/DATA
 * chunks of a compiled team/unit/structure script, then Script_Reset + Script_Load for each
 * entry of the ORDR table. The input is data/TEAM.EMC. */
#include "fuzz_common.h"
#include "script/script.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	static int inited = 0;
	static ScriptEngine engine;
	if (!inited) { fz_init(); inited = 1; }
	fz_clear();
	fz_write("TEAM.EMC", data, size);
	if (File_Init()) {
		uint16 t;
		(void)Script_LoadFromFile("TEAM.EMC", g_scriptTeam, g_scriptFunctionsTeam, NULL);
		for (t = 0; g_scriptTeam->offsets != NULL && t < g_scriptTeam->offsetsCount && t < 256; t++) {
			memset(&engine, 0, sizeof(engine));
			Script_Reset(&engine, g_scriptTeam);
			Script_Load(&engine, (uint8)t);
		}
		Script_ClearInfo(g_scriptTeam);
	}
	File_Uninit();
	return 0;
}
