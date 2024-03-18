#include "ppfc.h"
#include "speaker.h"

alignas(8) static uint8_t *audioBuffer;
static uint16_t audioBufferSamplePos;
static uint16_t audioBufferFillPos;
static uint16_t audioBufferSize;
//alignas(8) static uint8_t *recordBuffer;
//uint32_t recordSize = 512 * 1024;
void fillAudioDataCallback(void *userdata, uint8_t *stream, int length) {
    SDL_memset(stream, 0, length);
    if (audioBufferSize == 0) return;
    if (length > audioBufferSize) length = audioBufferSize;
    if (audioBufferFillPos + length > 4096) {
        uint16_t len = 4096 - audioBufferFillPos;
        SDL_memcpy(stream, audioBuffer + audioBufferFillPos, len);
        stream += len;
        length -= len;
        audioBufferFillPos = 0;
        audioBufferSize -= len;
    }
    SDL_memcpy(stream, audioBuffer + audioBufferFillPos, length);
    audioBufferFillPos += length;
    audioBufferSize -= length;
}

Speaker::Speaker(PPFC &bus) : bus(bus) {}

void Speaker::init(void) {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        error(format("Speaker: could not initialize SDL2: %s", SDL_GetError()));
    }
    SDL_memset(&this->spec, 0, sizeof(this->spec));
//    recordBuffer = new uint8_t[recordSize]{0};
    audioBuffer = new uint8_t[4096]{0};
    audioBufferSamplePos = 0;
    audioBufferFillPos = 0;
    audioBufferSize = 0;
    this->spec.freq = 48000;
    this->spec.format = AUDIO_U8;
    this->spec.channels = 1;
    this->spec.silence = 0;
    this->spec.samples = 1024;
    this->spec.callback = fillAudioDataCallback;
    if (SDL_OpenAudio(&this->spec, nullptr) < 0){
        printf("can't open audio.\n");
    }
    this->last = SDL_GetPerformanceCounter();
//    printf("first counter: %lu, f: %lu\n", this->last, SDL_GetPerformanceFrequency());
    SDL_PauseAudio(0);
}

void Speaker::run(void) {
    this->task = std::thread([this]() {
        while (true) {
            if (this->bus.status == PPFC_STOP) break;
            uint64_t now = SDL_GetPerformanceCounter();
            auto interval = static_cast<uint16_t>(std::round(SDL_GetPerformanceFrequency() / 48000.0f));
            if (now -this->last < interval) {
                SDL_Delay(1);
            } else {
                this->last = now;
                audioBuffer[audioBufferSamplePos] = this->bus.apu.output;
                audioBufferSamplePos = (audioBufferSamplePos + 1) % 4096;
                audioBufferSize++;
            }
        }
    });
}

void Speaker::sample(void) {
//    uint64_t now = SDL_GetPerformanceCounter();
//    auto interval = static_cast<uint16_t>(std::round(SDL_GetPerformanceFrequency() / 48000.0f));
////    printf("now: %lu, last: %lu, now-last: %lu\n", now, this->last, now - this->last);
//    if (now - this->last >= interval) {
//        audioBuffer[audioBufferSamplePos] = this->bus.apu.output;
//        audioBufferSamplePos = (audioBufferSamplePos + 1) % 4096;
//        audioBufferSize++;
//        if (recordBuffer != nullptr) {
//            recordBuffer[512 * 1024 - recordSize] = this->bus.apu.output;
//            recordSize--;
//            if (recordSize == 0) {
//                FILE* file = fopen("./record.pcm", "wb");
//                fseek(file, 0, SEEK_SET);
//                fwrite(recordBuffer, 512 * 1024, 1, file);
//                fclose(file);   // 关闭文件
//                recordBuffer = nullptr;
//                printf("done\n");
//                printf("end counter: %lu, f: %lu\n", now, SDL_GetPerformanceFrequency());
//            }
//        }
//
////        SDL_QueueAudio(1, &this->bus.apu.output, 1);
////        printf("\rAudio sample frequency: %.2f", float(1000/dtime));
//        this->last = now;
//    }
}
