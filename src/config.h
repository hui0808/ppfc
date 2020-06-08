#ifndef __PPFC_CLOCK_H__
#define __PPFC_CLOCK_H__

// https://wiki.nesdev.com/w/index.php/Cycle_reference_chart

#define PRGROMSIZE 16384 // 16k PRG-ROM
#define CHRROMSIZE 8192 // 8k CHR-ROM

#define NTSC_CYCLES           341
#define NTSC_SCANLINE         262
#define NTSC_POSTRENDER_LINE  240
#define NTSC_VBLANK           241
#define NTSC_PRERENDER_LINE   261
#define NTSC_FRAME_RATE       60
#endif // __PPFC_CLOCK_H__
