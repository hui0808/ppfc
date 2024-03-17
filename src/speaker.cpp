#include "ppfc.h"
#include "speaker.h"

//static uint8_t *audioBuffer;
//static uint16_t audioBufferSamplePos;
//static uint16_t audioBufferFillPos;
//static uint16_t audioBufferSize;
//
//void fillAudioDataCallback(void *userdata, uint8_t *stream, int length) {
//    SDL_memset(stream, 0, length);
//    if (audioBufferSize == 0) return;
//    if (length > audioBufferSize) length = audioBufferSize;
//    if (audioBufferFillPos + length > 4096) {
//        uint16_t len = 4096 - audioBufferFillPos;
//        SDL_memcpy(stream, audioBuffer + audioBufferFillPos, len);
//        stream += len;
//        length -= len;
//        audioBufferFillPos = 0;
//        audioBufferSize -= len;
//    }
//    SDL_memcpy(stream, audioBuffer + audioBufferFillPos, length);
//    audioBufferFillPos += length;
//    audioBufferSize -= length;
//}

Speaker::Speaker(PPFC &bus) : bus(bus) {}

void Speaker::init(void) {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        error(format("Speaker: could not initialize SDL2: %s", SDL_GetError()));
    }
    SDL_memset(&this->spec, 0, sizeof(this->spec));
//    audioBuffer = new uint8_t[4096]{0};
//    audioBufferSamplePos = 0;
//    audioBufferFillPos = 0;
//    audioBufferSize = 0;
    this->spec.freq = 44100;
    this->spec.format = AUDIO_U8;
    this->spec.channels = 1;
    this->spec.silence = 0;
    this->spec.samples = 1024;
    this->spec.callback = nullptr;
    if (SDL_OpenAudio(&this->spec, nullptr) < 0){
        printf("can't open audio.\n");
    }
    this->last = SDL_GetPerformanceCounter();
    SDL_PauseAudio(0);
}

void Speaker::sample(void) {
//    static const float period = 1000.0f / 44100.0f;
    uint64_t now = SDL_GetPerformanceCounter();
    auto interval = static_cast<uint16_t>(std::round(SDL_GetPerformanceFrequency() / 44100.0f));
//    printf("now: %lu, last: %lu, now-last: %lu\n", now, this->last, now - this->last);
    if (now - this->last >= interval) {
//        audioBuffer[audioBufferSamplePos] = this->bus.apu.output;
//        audioBufferSamplePos = (audioBufferSamplePos + 1) % 4096;
//        audioBufferSize++;
        SDL_QueueAudio(1, &this->bus.apu.output, 1);
//        printf("\rAudio sample frequency: %.2f", float(1000/dtime));
        this->last = now;
    }
}