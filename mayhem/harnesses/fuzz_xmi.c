/* XMIDI music: MPU_SetData() IFF/XDIR/XMID/EVNT walk and the MPU_Interrupt() event interpreter
 * (src/audio/mt32mpu.c), driven by hand with the midi_none backend (no timer thread on Linux).
 * The input is the .XMI file content, in a heap buffer of exactly its size. Like the game's music
 * player, which picks a song by index inside one .XMI, every song index 0..3 is loaded and played. */
#include "fuzz_common.h"
#include "audio/mt32mpu.h"
#include "audio/midi.h"

/* A MIDI device that accepts and discards everything. Upstream's midi_none.c returns false from
 * midi_init(), which makes MPU_Init() give up before any XMIDI is parsed; these definitions take
 * the place of midi_none.o at link time (it is then not pulled from libopendune.a). */
bool midi_init(void) { return true; }
void midi_uninit(void) {}
void midi_send(uint32 data) { (void)data; }
uint16 midi_send_string(const uint8 *data, uint16 len) { (void)data; return len; }
void midi_reset(void) {}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	uint8 *buf;
	void *msdata;
	uint16 idx, song;
	int i;

	/* MPU_FindSoundStart() reads the 12-byte IFF chunk header (id, size, type) of any buffer it is
	 * given; an input shorter than one header is not an XMI file. */
	if (size < 12) return 0;
	buf = malloc(size);
	memcpy(buf, data, size);
	if (!MPU_Init()) { free(buf); return 0; }
	msdata = malloc(MPU_GetDataSize());
	for (song = 0; song < 4; song++) {
		memset(msdata, 0, MPU_GetDataSize());
		idx = MPU_SetData(buf, song, msdata);
		if (idx == 0xFFFF) break;
		MPU_Play(idx);
		for (i = 0; i < 2000 && MPU_IsPlaying(idx) == 1; i++) MPU_Interrupt();
		MPU_Stop(idx);
		MPU_ClearData(idx);
	}
	MPU_Uninit();
	free(msdata);
	free(buf);
	return 0;
}
