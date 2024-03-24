#include "ppfc.h"
#include "speaker.h"

const static float dutyTable[] = {0.125f, 0.25f, 0.5f, 0.75f};



float approxSin(float t) {
    float j = t * 0.15915;
    j = j - (int)j;
    return 20.785 * j * (j - 0.5) * (j - 1.0);
}

uint8_t clamp(float value, uint8_t low, uint8_t high) {
    if (value < low) return low;
    else if (value > high) return high;
    else return uint8_t(value);
};

void fillAudioDataCallback(void *userdata, uint8_t *stream, int length) {
    auto self = (Speaker*)userdata;
    SDL_memset(stream, 0, length);
    for (int i = 0; i < length; i++) {
        stream[i] = self->sample(i);
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
    this->spec.userdata = this;
    if (SDL_OpenAudio(&this->spec, nullptr) < 0) {
        printf("can't open audio.\n");
    }
}

void Speaker::run(void) {
    SDL_PauseAudio(0);
}

uint8_t Speaker::sample(int seqIndex) {
    this->t = (float)this->clock / (float)this->sampleFreq;
    this->clock = (this->clock + 1) % (this->sampleFreq);
    float pulse0 = this->samplePulse(0);
    float pulse1 = this->samplePulse(1);
    // mixer
    // output = pulse_out + tnd_out
    //
    //                            95.88
    //pulse_out = ------------------------------------
    //             (8128 / (pulse1 + pulse2)) + 100
    //
    //                                       159.79
    //tnd_out = -------------------------------------------------------------
    //                                    1
    //           ----------------------------------------------------- + 100
    //            (triangle / 8227) + (noise / 12241) + (dmc / 22638)
    float pulse = 0;
    float tnd = 0;
    if (pulse0 >= 0.01f || pulse1 >= 0.01f) {
        pulse = 95.88f / (8128.0f / float(pulse0 + pulse1) + 100.0f);
    }
    uint8_t output = clamp((pulse + tnd) * 127.0f, 0, 255);
    return output;
}

float Speaker::samplePulse(uint8_t channel) {
    Pulse *pulse = channel == 0 ? &this->bus.apu.pulseChannel1 : &this->bus.apu.pulseChannel2;
    if (pulse->mute) return 0;
    uint8_t amplitude = pulse->envelope.output;
    float freq = 111860.8f / (float(this->bus.apu.pulseChannel1.timerLoad) + 1);
    float duty = dutyTable[pulse->duty];
    float p = 2 * 3.14159f * duty;
    float omega = 2 * 3.14159f * freq;
    float y1 = 0;
    float y2 = 0;
    for (uint32_t n = 1; n < this->harmonics; n++) {
        float phase = omega * float(n) * t;
        y1 += sin(phase) / float(n);
        y2 += sin(phase - p * float(n)) / float(n);
    }
    float output = float(amplitude) / 3.14159f * (y1 - y2) + 3 * duty * float(amplitude) / 3.14159f;
    return output;
}
