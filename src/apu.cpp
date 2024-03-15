#include "apu.h"
#include "ppfc.h"

APU::APU(PPFC& bus):
    bus(bus),
    pulseChannel1(*this, 1),
    pulseChannel2(*this, 2) {}

void APU::init(void) {
    this->pulseChannel1.init();
    this->pulseChannel2.init();
    this->reset();
}

void APU::reset(void) {
    memset(&this->triangleChannel, 0, sizeof(this->triangleChannel));
    memset(&this->noiseChannel, 0, sizeof(this->noiseChannel));
    memset(&this->dmcChannel, 0, sizeof(this->dmcChannel));
    this->writeableStatus.status = 0;
    this->readableStatus.status = 0;
    this->frameCounter.value = 0;
    this->cycle = 0;
    this->output = 0;
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
    if (this->cycle == 14914) this->cycle = 0;
    if (this->writeableStatus.pulse1Enable) this->pulseChannel1.run();
    if (this->writeableStatus.pulse2Enable) this->pulseChannel2.run();
    this->mix();
}

void APU::mix(void) {
    // https://www.nesdev.org/wiki/APU_Mixer
    float pulseOut = 0;
    float fndOut = 0;
    if (this->pulseChannel1.output != 0 || this->pulseChannel2.output != 0) {
        pulseOut = 95.88f / (8128.0f / (this->pulseChannel1.output + this->pulseChannel2.output) + 100.0f);
    }
    uint8_t output = uint8_t((pulseOut + fndOut) * 255 + 0.5);
    this->output = output;
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
}

void APU::clockSweepUnit() {
    this->pulseChannel1.clockSweepDivider();
    this->pulseChannel2.clockSweepDivider();
}

void APU::clockEnvelope() {
   this->pulseChannel1.clockEnvelope();
    this->pulseChannel2.clockEnvelope();
}

void APU::clockLinearCounter() {

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

    // triangle channel $4008-$400B, $4009 unused
    else if (addr == 0x4008) {
        *(uint8_t*)(&this->triangleChannel.reg0) = data;
    } else if (addr == 0x400A) {
        *(uint8_t*)(&this->triangleChannel.reg1) = data;
    } else if (addr == 0x400B) {
        *(uint8_t*)(&this->triangleChannel.reg2) = data;
    }
    // noise channel $400C-$400F, $400D unused
    else if (addr == 0x400C) {
        *(uint8_t*)(&this->noiseChannel.reg0) = data;
    } else if (addr == 0x400E) {
        *(uint8_t*)(&this->noiseChannel.reg1) = data;
    } else if (addr == 0x400F) {
        *(uint8_t*)(&this->noiseChannel.reg2) = data;
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
    if (this->pulseChannel1.lengthCounter > 0 && this->pulseChannel1.lengthCounterHalt == 0) {
        this->readableStatus.pulse1LengthCounterZero = 1;
    }
    if (this->pulseChannel2.lengthCounter > 0 && this->pulseChannel2.lengthCounterHalt == 0) {
        this->readableStatus.pulse2LengthCounterZero = 1;
    }
    if (this->dmcChannel.sampleLength > 0) {
        this->readableStatus.dmcActive = 1;
    }
    return this->readableStatus.status;
}

void APU::statusRegWrite(uint16_t addr, uint8_t data) {
    // status register $4015
    this->writeableStatus.status = data;
    if (this->writeableStatus.pulse1Enable) {
        this->pulseChannel1.lengthCounter = lengthTable[this->pulseChannel1.lengthCounterLoad];
    } else {
        this->pulseChannel1.lengthCounter = 0;
    }
    if (this->writeableStatus.pulse2Enable) {
        this->pulseChannel2.lengthCounter = lengthTable[this->pulseChannel2.lengthCounterLoad];
    } else {
        this->pulseChannel2.lengthCounter = 0;
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
    this->frameCounter.value = data;
//    this->cycle = 0;
    /** TODO
     * 	After 4 CPU clock cycles*, the timer is reset.
     * 	If the mode flag is set, then both "quarter frame" and "half frame" signals are also generated.
     */
}

Pulse::Pulse(APU& bus, uint8_t channel) : channel(channel), bus(bus) {}

void Pulse::init(void) {
    this->reset();
}

void Pulse::reset(void) {
    this->envelope = {0};
    this->sweep = {0};
    this->duty = 0;
    this->sequencer = 0;
    this->timerLoad = 0;
    this->timer = 0;
    this->sequencerOutput = 0;
    this->lengthCounter = 0;
    this->lengthCounterLoad = 0;
    this->lengthCounterHalt = 0;
    this->output = 0;
}

void Pulse::run(void) {
    // 扫描单元会持续不断地扫描
    this->clockSweep();
    // clock 8-step sequencer by timer
    // 方波周期 = 8 * (timer + 1) * APU cycles
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
    if (this->sweep.output != 0 && this->sequencerOutput != 0 && this->lengthCounter != 0) {
        this->output = this->envelope.output;
    } else {
        this->output = 0;
    }
//    printf("sweep output: %d, sequencer output: %d, length counter: %d, output: %d\n", this->sweep.output, this->sequencerOutput, this->lengthCounter, this->output);
//    if (this->channel == 2) {
//        printf("pulse output: %d\n", this->output);
//    }
}

void Pulse::regWrite(uint16_t addr, uint8_t data) {
    if (addr == 0x4000 || addr == 0x4004) {
        auto *reg = (PulseReg0*)(&data);
        this->duty = reg->duty;
        this->lengthCounterHalt = reg->lengthCounterHalt;
        // Halt length counter is also the envelope's loop flag
        this->envelope.loop = reg->lengthCounterHalt;
        this->envelope.constantVolume = reg->constantVolume;
        this->envelope.volume = reg->volume;
    } else if (addr == 0x4001 || addr == 0x4005) {
        auto *reg = (PulseReg1*)(&data);
        this->sweep.enable = reg->sweepEnable;
        this->sweep.dividerLoad = reg->sweepPeriod;
        this->sweep.negate = reg->sweepNegate;
        this->sweep.shiftCounter = reg->sweepShiftCount;
        // side effect
        this->sweep.reload = 1;
    } else if (addr == 0x4002 || addr == 0x4006) {
        auto *reg = (PulseReg2*)(&data);
        // 更新定时器d0-d7
        this->timerLoad = (this->timerLoad & 0x0700) | reg->timerLow;
    } else if (addr == 0x4003 || addr == 0x4007) {
        auto *reg = (PulseReg3*)(&data);
        // 更新定时器d8-d10
        this->timerLoad = (this->timerLoad & 0x00FF) | (uint16_t)(reg->timerHigh << 8);
        this->lengthCounterLoad = reg->lengthCounterLoad;
        // https://forums.nesdev.org/viewtopic.php?p=186129#p186129
        // 清空 sequencers，重置相位
        this->sequencer = 0;
        // side effect, set the envelope start flag
        this->envelope.start = 1;
    }
}

void Pulse::clockEnvelope(void) {
    if (this->envelope.start == 1) {
        this->envelope.start = 0;
        this->envelope.decayLevelCounter = 0;
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
    // TODO，更新时机
    // update envelope sequencerOutput
    if (this->envelope.constantVolume) {
        this->envelope.output = this->envelope.volume;
    } else {
        this->envelope.output = this->envelope.dividerCounter;
    }
}


void Pulse::clockSweep(void) {
    // https://www.nesdev.org/wiki/APU_Sweep
    uint16_t changeAmount = this->timerLoad >> this->sweep.shiftCounter;
    // timer是11bit
    if (this->sweep.negate) {
        this->sweep.targetPeriod = this->timerLoad - changeAmount;
        if (this->channel == 1) this->sweep.targetPeriod--;
    } else {
        this->sweep.targetPeriod = this->timerLoad + changeAmount;
    }
    // targetPeriod 溢出了或者当前周期小于8，需要钳制为0，且使通道静音
    if ((this->sweep.targetPeriod > (this->channel == 1 ? 0x07FE : 0x07FF)) || this->timerLoad < 8) {
        this->sweep.targetPeriod = 0;
        this->sweep.output = 0;
    } else {
        this->sweep.output = 1;
    }
}

void Pulse::clockSweepDivider(void) {
    // 当shiftCounter为0时，相当于enable为0的效果
    if (this->sweep.dividerCounter == 0 && this->sweep.enable && this->sweep.shiftCounter != 0 && this->sweep.output) {
        // 更新方波通道的周期
        this->timerLoad = this->sweep.targetPeriod;
    }
    if (this->sweep.dividerCounter == 0 || this->sweep.reload) {
        this->sweep.reload = 0;
        this->sweep.dividerCounter = this->sweep.dividerLoad;
    } else {
        this->sweep.dividerCounter--;
    }
}

void Pulse::clockLengthCounter(void) {
    if (this->lengthCounter > 0 && this->lengthCounterHalt == 0) {
        this->lengthCounter--;
    }
}


