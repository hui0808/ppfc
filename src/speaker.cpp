#include "ppfc.h"
#include "speaker.h"

//uint32_t bufferSize = 0;
//static uint8_t sampleStart = 0;

void fillAudioDataCallback(void *userdata, uint8_t *stream, int length) {
    auto self = (Speaker*)userdata;
    SDL_memset(stream, 0, length);
//    SDL_memcpy(stream, self->bus.apu.buffer + bufferSize, length);
//    bufferSize += length;
//    if (bufferSize >= 4096) {
//        bufferSize = 0;
//    }
    for (int i = 0; i < length; i++) {
        stream[i] = self->sample(i) * 0.5;
    }
}

Speaker::Speaker(PPFC &bus) : bus(bus) {}

void Speaker::init(void) {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        error(format("Speaker: could not initialize SDL2: %s", SDL_GetError()));
    }
    SDL_memset(&this->spec, 0, sizeof(this->spec));
    this->spec.freq = this->sampleFreq;
    this->spec.format = AUDIO_U8;
    this->spec.channels = 1;
    this->spec.silence = 0;
    this->spec.samples = this->sampleSize;
    this->spec.callback = fillAudioDataCallback;
//    this->spec.callback = nullptr;
    this->spec.userdata = this;
    if (SDL_OpenAudio(&this->spec, nullptr) < 0) {
        printf("can't open audio.\n");
    }
}

void Speaker::run(void) {
    SDL_PauseAudio(0);
}

uint8_t Speaker::sample(int streamIndex) {
    uint8_t output = this->bus.apu.sample(this->sampleFreq, this->sampleIndex);
    this->sampleIndex = (this->sampleIndex + 1) % (this->sampleFreq);
    return output;
}
