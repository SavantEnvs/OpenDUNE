/* SHP sprite files: Sprites_Init() -> Sprites_Load() (src/sprites.c), the code changed by upstream
 * 9bd0046b89 ("Do not crash when failing to load sprite file"). The input is MOUSE.SHP; the other
 * sprite files are absent (Sprites_Load tolerates that). g_unpackSHPonLoad is set, as the engine
 * does by default, so sprites are Format80-unpacked at load time.
 *
 * Built as an AFL target (one input per process, see mayhem/build.sh): Sprites_Uninit() does not
 * reset the static s_spritesCount, so a second Sprites_Init() in one process would free stale
 * entries of g_sprites, a harness artefact rather than a bug. */
#include "fuzz_common.h"
#include "opendune.h"
#include "sprites.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	uint16 i;
	unsigned sum = 0;

	fz_init();
	fz_write("MOUSE.SHP", data, size);
	if (!File_Init()) { Free_IniFile(); fz_cleanup(); return 0; }
	g_unpackSHPonLoad = true;
	Sprites_Init();
	for (i = 0; i < 7 && g_sprites != NULL; i++) {
		if (g_sprites[i] == NULL) continue;
		sum += Sprite_GetWidth(g_sprites[i]) + Sprite_GetHeight(g_sprites[i]);
	}
	(void)sum;
	Sprites_Uninit();
	File_Uninit();
	Free_IniFile();
	fz_cleanup();
	return 0;
}
