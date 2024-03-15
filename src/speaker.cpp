#include "speaker.h"
#include "ppfc.h"

Speaker::Speaker(PPFC &bus) : bus(bus) {}

void Speaker::init(void) {
    SDL_memset(&this->spec, 0, sizeof(this->spec));
    this->spec.freq = 44100;
    this->spec.format = AUDIO_U8;
    this->spec.channels = 1;
    this->spec.samples = 1024;
//    this->spec.callback = [this](void *userdata, uint8_t *stream, int len) { this->fill(userdata, stream, len); };
    this->spec.callback = this->fill;
}

void Speaker::fill(void *userdata, uint8_t *stream, int len) {
    SDL_MixAudio(stream, NULL, len, SDL_MIX_MAXVOLUME);
}


void Speaker::sample(void) {

}