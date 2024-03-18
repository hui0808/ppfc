#ifndef __PPFC_SPEAKER_H__
#define __PPFC_SPEAKER_H__

#include "common.h"

class PPFC;

class Speaker {
public:
    PPFC& bus;
    SDL_AudioSpec spec;
    uint64_t last;
    std::thread task;
    Speaker(PPFC& bus);
    void init(void);
    void run(void);
    void sample(void);
};

#endif
