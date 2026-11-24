/* OpenDUNE functional oracle: hand-built game files go through the engine's real loaders
 * (PAK index, SHP sprites, FNT fonts, language strings, opendune.ini, Format80) and every
 * decoded value is asserted. Prints "PASS <name>"/"FAIL <name>" per check; mayhem/test.sh
 * turns that into a CTRF report. Built with the project's normal flags (no sanitizers). */
#include "../harnesses/fuzz_common.h"
#include "ini.h"
#include "opendune.h"
#include "sprites.h"
#include "string.h"
#include "config.h"
#include "gui/font.h"
#include "codec/format80.h"

static int failed = 0;
static void check(const char *name, int ok)
{
	printf("%s %s\n", ok ? "PASS" : "FAIL", name);
	if (!ok) failed++;
}

static size_t put16(uint8 *p, unsigned v) { p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; return 2; }
static size_t put32(uint8 *p, uint32 v) { put16(p, v & 0xFFFF); put16(p + 2, v >> 16); return 4; }

/* a PAK: [u32 offset][name\0]... [u32 0] then the file bodies */
static size_t make_pak(uint8 *b)
{
	size_t n = 0;
	uint32 hdr = 4 + 6 + 4 + 6 + 4;  /* "A.TXT\0" "B.BIN\0" */
	n += put32(b + n, hdr); memcpy(b + n, "A.TXT", 6); n += 6;
	n += put32(b + n, hdr + 5); memcpy(b + n, "B.BIN", 6); n += 6;
	n += put32(b + n, 0);
	memcpy(b + n, "hello", 5); n += 5;
	memcpy(b + n, "xyz1234", 7); n += 7;
	return n;
}

int main(int argc, char **argv)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	uint8 b[8192];
	size_t n;
	char *p;
	uint32 sz = 0;

	fz_init();

	/* opendune.ini (written by fz_init) is parsed */
	{
		char buf[2200];
		check("inifile_datadir", IniFile_GetString("datadir", NULL, buf, sizeof(buf)) != NULL && strcmp(buf, fz_data) == 0);
		check("inifile_default", IniFile_GetString("nosuchkey", "dflt", buf, sizeof(buf)) == NULL && strcmp(buf, "dflt") == 0);
	}
	{
		char src[] = "[video]\nwidth=320\n[audio]\nmusic=mt32 \nvolume=7\n";
		char buf[64];
		Ini_GetString("audio", "music", "none", buf, sizeof(buf), src);
		check("ini_string", strcmp(buf, "mt32") == 0);
		check("ini_integer", Ini_GetInteger("audio", "volume", -1, src) == 7);
		check("ini_category", Ini_GetInteger("video", "volume", -1, src) == -1);
	}

	/* PAK index + reads through it */
	n = make_pak(b);
	if (argc > 1) { FILE *f = fopen(argv[1], "wb"); fwrite(b, 1, n, f); fclose(f); }
	fz_write("TEST.PAK", b, n);

	/* SHP sprite, new format, one non-Format80 sprite of 6x4 */
	n = 0;
	n += put16(b + n, 1);
	n += put32(b + n, 8);           /* offset table: 4 + count*4 = 8 -> new format */
	n += put32(b + n, 0);
	n += put16(b + n, 0x0002);      /* flags: not Format80 encoded */
	b[n++] = 4;                     /* height */
	n += put16(b + n, 6);           /* width */
	b[n++] = 4;                     /* height */
	n += put16(b + n, 22);          /* "packed" size (header + 12 pixel bytes) */
	n += put16(b + n, 12);          /* decoded size */
	memset(b + n, 0x11, 12); n += 12;
	fz_write("MOUSE.SHP", b, n);

	/* FNT: 128 glyphs, 'A' 5 wide, 'B' 3 wide, no glyph data */
	memset(b, 0, sizeof(b));
	b[2] = 0x00; b[3] = 0x05;
	put16(b + 4, 16);               /* info block: height/maxWidth at +4/+5 */
	put16(b + 6, 32);               /* data offsets (all 0) */
	put16(b + 8, 300);              /* width list */
	put16(b + 10, 300 + 128);       /* end of width list -> count 128 */
	put16(b + 12, 500);             /* line list */
	b[16 + 4] = 8; b[16 + 5] = 5;
	b[300 + 'A'] = 5; b[300 + 'B'] = 3;
	fz_write("INTRO.FNT", b, 800); fz_write("new6p.fnt", b, 800); fz_write("new8p.fnt", b, 800);

	/* language file: two strings */
	n = 0;
	n += put16(b + n, 4); n += put16(b + n, 10);
	memcpy(b + n, "Hello\0World\0", 12); n += 12;
	{
		static const char *const lf[] = { "DUNE.ENG", "MESSAGE.ENG", "INTRO.ENG", "TEXTH.ENG", "TEXTA.ENG", "TEXTO.ENG", "PROTECT.ENG", NULL };
		int i;
		for (i = 0; lf[i] != NULL; i++) fz_write(lf[i], b, n);
	}

	check("file_init", File_Init());
	check("pak_exists", File_Exists_GetSize("A.TXT", &sz) && sz == 5);
	p = File_ReadWholeFile("B.BIN");
	check("pak_read", p != NULL && memcmp(p, "xyz1234", 7) == 0);
	free(p);
	check("pak_missing", !File_Exists("C.BIN"));

	g_unpackSHPonLoad = true;
	Sprites_Init();
	check("shp_loaded", g_sprites != NULL && g_sprites[0] != NULL);
	check("shp_width", g_sprites != NULL && Sprite_GetWidth(g_sprites[0]) == 6);
	check("shp_height", g_sprites != NULL && Sprite_GetHeight(g_sprites[0]) == 4);
	check("shp_absent_is_null", g_sprites != NULL && g_sprites[7] == NULL);
	Sprites_Uninit();

	check("font_init", Font_Init());
	if (g_fontNew8p != NULL) {
		Font_Select(g_fontNew8p);
		check("font_width", Font_GetStringWidth("AAB") == 13 + 3 * g_fontCharOffset);
		check("font_height", g_fontNew8p->height == 8 && g_fontNew8p->maxWidth == 5);
	}

	g_config.language = LANGUAGE_ENGLISH;
	String_Init();
	check("string_0", strcmp(String_Get_ByIndex(0), "Hello") == 0);
	check("string_1", strcmp(String_Get_ByIndex(1), "World") == 0);
	check("string_message", strcmp(String_Get_ByIndex(340), "Hello") == 0);
	String_Uninit();
	File_Uninit();

	{
		static const uint8 enc[] = { 0x83, 'a', 'b', 'c', 0x81, 'Z', 0x80 };
		uint8 out[16];
		uint16 len;
		memset(out, 0, sizeof(out));
		len = Format80_Decode(out, enc, sizeof(out));
		check("format80_literal", len == 4 && memcmp(out, "abcZ", 4) == 0);
	}

	fz_cleanup();
	return failed != 0;
}
