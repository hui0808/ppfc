#include "ppfc.h"
#include "apu.h"

uint8_t clamp(float value, uint8_t low, uint8_t high) {
    if (value < low) return low;
    else if (value > high) return high;
    else return uint8_t(value);
};

APU::APU(PPFC& bus):
    bus(bus),
    pulseChannel1(*this, 0),
    pulseChannel2(*this, 1),
    triangleChannel(*this),
    noiseChannel(*this) {}

void APU::init(void) {
    this->pulseChannel1.init();
    this->pulseChannel2.init();
    this->triangleChannel.init();
    this->noiseChannel.init();
    this->reset();
}

void APU::reset(void) {
    memset(&this->dmcChannel, 0, sizeof(this->dmcChannel));
    memset(&this->frameCounter, 0, sizeof(this->frameCounter));
    memset(this->buffer, 0, sizeof(this->buffer));
    this->bufferPos = 0;
    this->writeableStatus.status = 0;
    this->readableStatus.status = 0;
    this->cycle = 0;
    this->output = 0;
    this->samplePos = 0;
}

void APU::run(void) {
    this->cycle++;
    // https://www.nesdev.org/wiki/APU_Frame_Counter
    // 每隔大约1/4帧时间就会触发一次
    switch (this->cycle) {
        case 3728:
        case 7456:
        case 11185:
        case 14914:
            this->clockFrameCounter();
    }
    this->pulseChannel1.run();
    this->pulseChannel2.run();
//    this->triangleChannel.run();
//    this->triangleChannel.run();
    this->noiseChannel.run();
    this->noiseChannel.run();
//    float latestSamplePos = this->cycle * 44100 / (CPU_CYCLES_PER_SEC / 2);
//    if (latestSamplePos - this->samplePos > 0.99f) {
//        // 触发一次采样
//        if (this->bufferPos == 0) {
//            SDL_LockAudio();
//        }
//        this->samplePos = latestSamplePos;
//        this->buffer[this->bufferPos] = this->sample(44100, uint32_t(latestSamplePos));
////        if (this->bufferPos == sizeof(this->buffer) - 1) {
////            SDL_QueueAudio(1, this->buffer, sizeof(this->buffer));
////        }
//        if (this->bufferPos >= 2048) {
//            SDL_UnlockAudio();
//        }
//        this->bufferPos = (this->bufferPos + 1) % sizeof(this->buffer);
//    }
    if (this->cycle == 14914) {
//        this->samplePos = latestSamplePos - this->samplePos;
        this->cycle = 0;
    }
}
void APU::clockFrameCounter() {
    if (this->frameCounter.mode == 0) {
        // 4-step sequence
        // 0 1 2 3
        // - - - i (i = trigger frame interrupt if interruptInhibit = 0)
        // - l - l (l = length counter and sweep unit) half frame
        // e e e e (e = envelope and triangle's linear counter) quarter frame
        switch (this->frameCounter.counter) {
            case 3:
                if (this->frameCounter.interruptInhibit == 0) {
                    this->readableStatus.frameCounterInterrupt = 1;
                    this->bus.cpu.handleIrq();
                }
            case 1:
                this->clockSweepUnit();
                this->clockLengthCounter();
        }
        this->clockLinearCounter();
        this->clockEnvelope();
        this->frameCounter.counter = (this->frameCounter.counter + 1) % 4;
    } else {
        // 5-step sequence
        // 0 1 2 3 4
        // - l - - l (l = length counter and sweep unit) half frame
        // e e e - e (e = envelope and triangle's linear counter) quarter frame
        switch (this->frameCounter.counter) {
            case 1:
            case 4:
                this->clockSweepUnit();
                this->clockLengthCounter();
            case 0:
            case 2:
                this->clockLinearCounter();
                this->clockEnvelope();
        }
        this->frameCounter.counter = (this->frameCounter.counter + 1) % 5;
    }
}

void APU::clockLengthCounter() {
    this->pulseChannel1.clockLengthCounter();
    this->pulseChannel2.clockLengthCounter();
    this->triangleChannel.clockLengthCounter();
    this->noiseChannel.clockLengthCounter();
}

void APU::clockSweepUnit() {
    this->pulseChannel1.clockSweep();
    this->pulseChannel2.clockSweep();
}

void APU::clockEnvelope() {
    this->pulseChannel1.clockEnvelope();
    this->pulseChannel2.clockEnvelope();
    this->noiseChannel.clockEnvelope();
}

void APU::clockLinearCounter() {
    this->triangleChannel.clockLinearCounter();
}

uint8_t APU::channelRegRead(uint16_t addr) {
    return 0;
}

void APU::channelRegWrite(uint16_t addr, uint8_t data) {
    // pulse 1 channel $4000-$4003
    // pulse 2 channel $4004-$4007
    addr |= 0x4000;
    if (0x4000 <= addr && addr <= 0x4003) {
        this->pulseChannel1.regWrite(addr, data);
    } else if (0x4004 <= addr && addr <= 0x4007) {
        this->pulseChannel2.regWrite(addr, data);
    }
    // triangle channel $4008-$400B
    else if (0x4008 <= addr && addr <= 0x400B) {
        this->triangleChannel.regWrite(addr, data);
    }
    // noise channel $400C-$400F, $400D unused
    else if (0x400C <= addr && addr <= 0x400F) {
        this->noiseChannel.regWrite(addr, data);
    }
    // dmc channel $4010-$4013
    else if (addr == 0x4010) {
        *(uint8_t*)(&this->dmcChannel.reg0) = data;
    } else if (addr == 0x4011) {
        *(uint8_t*)(&this->dmcChannel.reg1) = data;
    } else if (addr == 0x4012) {
        this->dmcChannel.sampleAddress = data;
    } else if (addr == 0x4013) {
        this->dmcChannel.sampleLength = data;
    }
}

uint8_t APU::statusRegRead(uint16_t addr) {
    // apu 只有 $4015 状态寄存器可读
    // TODO
    if (this->pulseChannel1.enable != 0 && this->pulseChannel1.lengthCounter > 0) {
        this->readableStatus.pulse1LengthCounterZero = 1;
    }
    if (this->pulseChannel2.enable != 0 && this->pulseChannel2.lengthCounter > 0) {
        this->readableStatus.pulse2LengthCounterZero = 1;
    }
    if (this->triangleChannel.enable != 0 && this->triangleChannel.lengthCounter > 0) {
        this->readableStatus.triangleLengthCounterZero = 1;
    }
    if (this->noiseChannel.enable != 0 && this->noiseChannel.lengthCounter > 0) {
        this->readableStatus.noiseLengthCounterZero = 1;
    }
    if (this->dmcChannel.sampleLength > 0) {
        this->readableStatus.dmcActive = 1;
    }
    uint8_t ret = this->readableStatus.status;
    // clear frame counter interrupt after
    this->readableStatus.frameCounterInterrupt = 0;
    return ret;
}

void APU::statusRegWrite(uint16_t addr, uint8_t data) {
    // status register $4015
    // d4-d0为DNT21通道使能位，写入0都会使其静音以及清除其长度计数器
    this->writeableStatus.status = data;
    if (this->writeableStatus.pulse1Enable) {
        this->pulseChannel1.enable = 1;
        this->pulseChannel1.lengthCounter = lengthTable[this->pulseChannel1.lengthCounterLoad];
    } else {
        this->pulseChannel1.enable = 0;
        this->pulseChannel1.lengthCounter = 0;
    }
    if (this->writeableStatus.pulse2Enable) {
        this->pulseChannel2.enable = 1;
        this->pulseChannel2.lengthCounter = lengthTable[this->pulseChannel2.lengthCounterLoad];
    } else {
        this->pulseChannel2.enable = 0;
        this->pulseChannel2.lengthCounter = 0;
    }
    if (this->writeableStatus.triangleEnable) {
        this->triangleChannel.enable = 1;
        this->triangleChannel.lengthCounter = lengthTable[this->triangleChannel.lengthCounterLoad];
    } else {
        this->triangleChannel.enable = 0;
        this->triangleChannel.lengthCounter = 0;
    }
    if (this->writeableStatus.noiseEnable) {
        this->noiseChannel.enable = 1;
        this->noiseChannel.lengthCounter = lengthTable[this->noiseChannel.lengthCounterLoad];
    } else {
        this->noiseChannel.enable = 0;
        this->noiseChannel.lengthCounter = 0;
    }
    // clear dmc interrupt flag
    this->dmcChannel.reg0.irqEnable = 0;
    if (this->writeableStatus.dmcEnable) {

    } else {
        this->dmcChannel.sampleLength = 0;
        this->dmcChannel.sampleAddress = 0;
    }
}

void APU::frameCounterRegWrite(uint16_t addr, uint8_t data) {
    // $4017
    auto &frameCounterReg = (FrameCounterReg&)data;
    this->frameCounter.mode = frameCounterReg.mode;
    this->frameCounter.interruptInhibit = frameCounterReg.interruptInhibit;
    this->cycle = 0;
    if (this->frameCounter.interruptInhibit) {
        this->readableStatus.frameCounterInterrupt = 0;
    }
    // If the mode flag is set, then both "quarter frame" and "half frame" signals are also generated.
    if (this->frameCounter.mode) {
        this->clockSweepUnit();
        this->clockLengthCounter();
        this->clockLinearCounter();
        this->clockEnvelope();
    }
    /** TODO
     * 	After 4 CPU clock cycles*, the timer is reset.
     */
}

uint8_t APU::sample(uint32_t sampleFreq, uint32_t sampleIndex) {
    float pulse0 = this->pulseChannel1.sample(sampleFreq, sampleIndex);
//    float pulse0 = 0;
    float pulse1 = this->pulseChannel2.sample(sampleFreq, sampleIndex);
//    float pulse1 = 0;
    float triangle = this->triangleChannel.sample(sampleFreq, sampleIndex);
//    float triangle = 0;
//    float noise = this->noiseChannel.sample(sampleFreq, sampleIndex);
    float noise = 0;
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
    if (triangle >= 0.01f || noise >= 0.01f) {
        tnd = 159.79f / (1 / (triangle / 8227.0f + noise / 12241.0f) + 100.0f);
    }
    uint8_t output = clamp((pulse + tnd) * 127.0f, 0, 255);
    return output;
}

Pulse::Pulse(APU& bus, uint8_t channel) : channel(channel), bus(bus) {}

void Pulse::init(void) {
    this->reset();
}

void Pulse::reset(void) {
    this->envelope = {0};
    this->enable = 0;
    this->sweep = {0};
    this->duty = 0;
    this->sequencer = 0;
    this->timerLoad = 0;
    this->timer = 0;
    this->sequencerOutput = 0;
    this->lengthCounter = 0;
    this->lengthCounterLoad = 0;
    this->lengthCounterHalt = 0;
}

void Pulse::run(void) {
    // 扫描单元会持续不断地扫描
    // clock 8-step sequencer by timer
    // 方波周期 = 8 * (timer + 1) * APU cycles
    if (!this->enable) return;
    if (this->timer == 0) {
        this->timer = this->timerLoad;
        switch (this->duty) {
            case (0):
                if (this->sequencer < 7) this->sequencerOutput = 0;
                else this->sequencerOutput = 1;
                break;
            case (1):
                if (this->sequencer < 6) this->sequencerOutput = 0;
                else this->sequencerOutput = 1;
                break;
            case (2):
                if (this->sequencer < 4) this->sequencerOutput = 0;
                else this->sequencerOutput = 1;
                break;
            case (3):
                if (this->sequencer < 6) this->sequencerOutput = 1;
                else this->sequencerOutput = 0;
        }
        if (this->sequencer == 0) this->sequencer = 7;
        else this->sequencer--;
    } else {
        this->timer--;
    }
    // 方波输出
    if (this->enable && this->sweep.output != 0 && this->lengthCounter != 0) {
        this->mute = 0;
    } else {
        this->mute = 1;
    }
}

void Pulse::regWrite(uint16_t addr, uint8_t data) {
    if (addr == 0x4000 || addr == 0x4004) {
        auto &reg = (PulseReg0&)data;
        this->duty = reg.duty;
        this->lengthCounterHalt = reg.lengthCounterHalt;
        // Halt length counter is also the envelope's loop flag
        this->envelope.loop = reg.lengthCounterHalt;
        this->envelope.constantVolume = reg.constantVolume;
        this->envelope.volume = reg.volume;
    } else if (addr == 0x4001 || addr == 0x4005) {
        auto &reg = (PulseReg1&)data;
        this->sweep.enable = reg.sweepEnable;
        this->sweep.dividerLoad = reg.sweepPeriod;
        this->sweep.negate = reg.sweepNegate;
        this->sweep.shiftCounter = reg.sweepShiftCount;
        // side effect
        this->sweep.reload = 1;
    } else if (addr == 0x4002 || addr == 0x4006) {
        auto &reg = (PulseReg2&)data;
        // 更新定时器d0-d7
        this->timerLoad = (this->timerLoad & 0x0700) | reg.timerLow;
    } else if (addr == 0x4003 || addr == 0x4007) {
        auto &reg = (PulseReg3&)data;
        // 更新定时器d8-d10
        this->timerLoad = (this->timerLoad & 0x00FF) | (uint16_t)(reg.timerHigh << 8);
        this->lengthCounterLoad = reg.lengthCounterLoad;
        // https://forums.nesdev.org/viewtopic.php?p=186129#p186129
        // 清空 sequencers，重置相位
        this->sequencer = 0;
        this->timer = 0;
        // side effect, set the envelope start flag
        this->envelope.start = 1;
        if ((this->channel == 0 && this->bus.writeableStatus.pulse1Enable) || (this->channel == 1 && this->bus.writeableStatus.pulse2Enable)) {
            this->enable = 1;
        }
    }
}

void Pulse::clockEnvelope(void) {
    if (this->envelope.start == 1) {
        this->envelope.start = 0;
        this->envelope.decayLevelCounter = 15;
        this->envelope.dividerCounter = this->envelope.volume;
    } else {
        // clock the envelope divider
        if (this->envelope.dividerCounter == 0) {
            this->envelope.dividerCounter = this->envelope.volume; // reload
            // clock the decay level counter
            // loop 为1，则15到0循环，否则直到0为止
            if (this->envelope.decayLevelCounter != 0) this->envelope.decayLevelCounter--;
            else if (this->envelope.loop) {
                this->envelope.decayLevelCounter = 15;
            }
        } else {
            this->envelope.dividerCounter--;
        }
    }
    // update envelope sequencerOutput
    if (this->envelope.constantVolume) {
        this->envelope.output = this->envelope.volume;
    } else {
        this->envelope.output = this->envelope.dividerCounter;
    }
}


void Pulse::trackSweep(void) {
    // https://www.nesdev.org/wiki/APU_Sweep
    uint16_t changeAmount = this->timerLoad >> this->sweep.shiftCounter;
    // timer是11bit
    if (this->sweep.enable && this->sweep.shiftCounter > 0) {
        if (this->sweep.negate) {
            // 减少周期
            this->sweep.targetPeriod = this->timerLoad - changeAmount;
            if (this->channel == 0) this->sweep.targetPeriod--;
        } else {
            // 增加周期
            this->sweep.targetPeriod = this->timerLoad + changeAmount;
        }
    }
    // targetPeriod 溢出了或者当前周期小于8，需要钳制为0，且使通道静音
    // sweep 禁用不影响静音逻辑
    if ((this->sweep.targetPeriod > (this->channel == 0 ? 0x07FE : 0x07FF)) || this->timerLoad < 8) {
        this->sweep.targetPeriod = 0;
        this->sweep.output = 0;
    } else {
        this->sweep.output = 1;
    }
}

void Pulse::clockSweep(void) {
    this->trackSweep();
    // 当shiftCounter为0时，相当于enable为0的效果
    if (this->sweep.dividerCounter == 0 && this->sweep.enable && this->sweep.shiftCounter != 0 && this->sweep.output) {
        // 更新方波通道的周期
        this->timerLoad = this->sweep.targetPeriod;
    }
    if (this->sweep.dividerCounter == 0 || this->sweep.reload) {
        this->sweep.reload = 0;
        this->sweep.dividerCounter = this->sweep.dividerLoad;
    } else if (this->sweep.enable) {
        this->sweep.dividerCounter--;
    }
}

void Pulse::clockLengthCounter(void) {
    if (!this->enable) {
        this->lengthCounter = 0;
    } else if (this->lengthCounter > 0 && this->lengthCounterHalt == 0) {
        this->lengthCounter--;
    }
}

float Pulse::sample(uint32_t sampleFreq, uint32_t sampleIndex) {
    if (this->mute) return 0;
    uint8_t amplitude = this->envelope.output;
    float freq = 111860.8f / (float(this->timerLoad) + 1);
    float duty = dutyTable[this->duty];
    float p = 2 * 3.14159f * duty;
    float omega = 2 * 3.14159f * freq;
    float y1 = 0;
    float y2 = 0;
    float t = float(sampleIndex) / float(sampleFreq);
    for (uint32_t n = 1; n < 20; n++) {
        float phase = omega * float(n) * t;
        y1 += sin(phase) / float(n);
        y2 += sin(phase - p * float(n)) / float(n);
    }
    float output = float(amplitude) / 3.141579f * (y1 - y2) + 3 * duty * float(amplitude) / 3.14159f;
    return output;
}


Triangle::Triangle(APU &bus) : bus(bus) {}

void Triangle::init(void) {
    this->reset();
}

void Triangle::reset(void) {
    this->enable = 0;
    this->linearCounter = 0;
    this->linearCounterReLoad = 0;
    this->linearCounterHalt = 0;
    this->linearCounterLoad = 0;
    this->lengthCounter = 0;
    this->lengthCounterLoad = 0;
    this->timerLoad = 0;
    this->timer = 0;
    this->sequencer = 0;
}

void Triangle::regWrite(uint16_t addr, uint8_t data) {
    if (addr == 0x4008) {
        auto &reg = (TriangleReg0&)data;
        this->linearCounterHalt = reg.linearCounterHalt;
        this->linearCounterLoad = reg.linearCounterLoad;
    } else if (addr == 0x400A) {
        auto &reg = (TriangleReg2&)data;
        // 更新定时器d0-d7
        this->timerLoad = (this->timerLoad & 0x0700) | reg.timerLow;
    } else if (addr == 0x400B) {
        auto &reg = (TriangleReg3&)data;
        // 更新定时器d8-d10
        this->timerLoad = (this->timerLoad & 0x00FF) | (uint16_t)(reg.timerHigh << 8);
        this->lengthCounterLoad = reg.lengthCounterLoad;
        // side effect
        this->linearCounterReLoad = 1;
    }
}

void Triangle::run(void) {}

void Triangle::clockLinearCounter(void) {
    if (!this->enable) {
        this->linearCounter = 0;
    } else if (this->linearCounterReLoad) {
        this->linearCounter = this->linearCounterLoad;
    } else if (this->linearCounter != 0) {
        this->linearCounter--;
    }
    // 暂停线性计数器也会暂停清除reload标志位
    if (this->linearCounterHalt == 0) {
        this->linearCounterReLoad = 0;
    }
}

void Triangle::clockLengthCounter(void) {
    if (!this->enable) {
        this->lengthCounter = 0;
    } else if (this->lengthCounter > 0 && this->linearCounterHalt == 0) {
        this->lengthCounter--;
    }
}

uint8_t Triangle::sample(uint32_t sampleFreq, uint32_t sampleIndex) {
    if (!this->enable || this->linearCounter == 0 || this->lengthCounter == 0) {
        return 0;
    }
    // 三角波频率 = Fcpu / (32 * (timer + 1))
    float freq = 55930.4f / (float(this->timerLoad) + 1.f);
    // 三角波当前执行了多少个周期
//    float period = float(this->bus.bus.cpu.cycle) / freq;
    float period = float(sampleIndex) / freq;
    // 取period小数部分
    float phase = period - int(period);
    return triangleSeqTable[int(phase * 32)];
}

Noise::Noise(APU &bus) : bus(bus) {}

void Noise::init(void) {
    this->reset();
}

void Noise::reset(void) {
    this->envelope = {0};
    this->loop = 0;
    this->timerLoad = 0;
    this->timer = 0;
    this->lengthCounter = 0;
    this->lengthCounterLoad = 0;
    this->lengthCounterHalt = 0;
    this->sequencer = 1; // lfsr 初始值
    this->sequencerOutput = 0;
}

void Noise::run(void) {
    if (!this->enable || this->lengthCounter == 0) return;
    if (this->timer == 0) {
        this->timer = this->timerLoad;
        // lfsr 逻辑
        // 1. 反馈值计算：loop为1，结果为b0与b6异或，否则结果为b0与b1
        // 2. 寄存器右移1位
        // 3. 寄存器b14设置位反馈值
        uint8_t op = this->loop ? ((this->sequencer >> 6) & 0x01) : ((this->sequencer >> 1) & 0x01);
        uint8_t fallback = (this->sequencer & 0x01) ^ op;
        this->sequencer = (this->sequencer >> 1) | (fallback << 14);
        this->sequencerOutput = this->sequencer & 0x01;
    } else {
        this->timer--;
    }
}

void Noise::regWrite(uint16_t addr, uint8_t data) {
    if (addr == 0x400C) {
        auto &reg = (NoiseReg0&)data;
        this->envelope.volume = reg.volume;
        this->envelope.constantVolume = reg.constantVolume;
        this->envelope.loop = reg.lengthCounterHalt;
        this->lengthCounterHalt = reg.lengthCounterHalt;
    } else if (addr == 0x400E) {
        auto &reg = (NoiseReg2&)data;
        this->timerLoad = noiseTimerTable[reg.timerLoad];
        this->loop = reg.loop;
    } else if (addr == 0x400F) {
        auto &reg = (NoiseReg3&)data;
        this->lengthCounterLoad = reg.lengthCounterLoad;
        // side effect
        this->envelope.start = 1;
    }
}

void Noise::clockEnvelope(void) {
    if (this->envelope.start == 1) {
        this->envelope.start = 0;
        this->envelope.decayLevelCounter = 15;
        this->envelope.dividerCounter = this->envelope.volume;
    } else {
        // clock the envelope divider
        if (this->envelope.dividerCounter == 0) {
            this->envelope.dividerCounter = this->envelope.volume; // reload
            // clock the decay level counter
            // loop 为1，则15到0循环，否则直到0为止
            if (this->envelope.decayLevelCounter != 0) this->envelope.decayLevelCounter--;
            else if (this->envelope.loop) {
                this->envelope.decayLevelCounter = 15;
            }
        } else {
            this->envelope.dividerCounter--;
        }
    }
    // update envelope sequencerOutput
    if (this->envelope.constantVolume) {
        this->envelope.output = this->envelope.volume;
    } else {
        this->envelope.output = this->envelope.dividerCounter;
    }
}

void Noise::clockLengthCounter(void) {
    if (!this->enable) {
        this->lengthCounter = 0;
    } else if (this->lengthCounter > 0 && this->lengthCounterHalt == 0) {
        this->lengthCounter--;
    }
}

float Noise::sample(uint32_t sampleFreq, uint32_t sampleIndex) {
//    for (uint8_t i = 0; i < 41; i++) {
//        this->run();
//    }
    if (this->enable == 0 || this->sequencerOutput == 0 || this->lengthCounter == 0) {
        return 0;
    } else {
        return float(this->envelope.output);
    }
}
