#ifndef __PPFC_SPEAKER_H__
#define __PPFC_SPEAKER_H__

#include "common.h"

class PPFC;

class Speaker {
public:
    PPFC& bus;
    SDL_AudioSpec spec;
    SDL_AudioDeviceID device;
    Speaker(PPFC& bus);
    void init(void);
    void fill(void *userdata, uint8_t *stream, int len);
    void sample(void);
};

#endif
