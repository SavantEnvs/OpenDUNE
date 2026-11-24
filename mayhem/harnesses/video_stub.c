/* Headless video driver for the fuzz harnesses: upstream ships no "none" video
 * backend (only SDL/SDL2/DOS/Atari/...), so this implements src/video/video.h
 * with an in-memory frame buffer. Nothing here parses input. */
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "video/video.h"

static uint8 s_framebuffer[64000 * 4];

bool Video_Init(int screen_magnification, VideoScaleFilter filter)
{
	(void)screen_magnification; (void)filter;
	return true;
}
void Video_Uninit(void) {}
void Video_Tick(void) {}
void Video_SetPalette(void *palette, int from, int length) { (void)palette; (void)from; (void)length; }
void Video_Mouse_SetPosition(uint16 x, uint16 y) { (void)x; (void)y; }
void Video_Mouse_SetRegion(uint16 minX, uint16 maxX, uint16 minY, uint16 maxY) { (void)minX; (void)maxX; (void)minY; (void)maxY; }
void Video_SetOffset(uint16 offset) { (void)offset; }
void *Video_GetFrameBuffer(uint16 size)
{
	(void)size;
	return s_framebuffer;
}
