#ifndef __PPFC_SPEAKER_H__
#define __PPFC_SPEAKER_H__

#include "common.h"

class PPFC;
#pragma pack(push, 1)
class Speaker {
public:
    PPFC& bus;
    uint32_t clock = 0;
    uint32_t sampleFreq = 44100;
    uint32_t sampleSize = 64;
    uint32_t harmonics = 20;
    float t = 0;
    SDL_AudioSpec spec;

    Speaker(PPFC& bus);
    void init(void);
    void run(void);
    uint8_t sample(int length);
    float samplePulse(uint8_t channel);
};
#pragma pack(pop)
#endif
