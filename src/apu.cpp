#include "ppfc.h"
#include "apu.h"

APU::APU(PPFC& bus) : bus(bus) {}

void APU::init(void) {
    this->reset();
}

void APU::reset(void) {
    memset(&this->pulseChannel1, 0, sizeof(this->pulseChannel1));
    memset(&this->pulseChannel2, 0, sizeof(this->pulseChannel2));
    memset(&this->triangleChannel, 0, sizeof(this->triangleChannel));
    memset(&this->noiseChannel, 0, sizeof(this->noiseChannel));
    memset(&this->dmcChannel, 0, sizeof(this->dmcChannel));
    this->writeableStatus.status = 0;
    this->readableStatus.status = 0;
    this->frameCounter.value = 0;
    this->cycle = 0;
    this->dividerCount = 0;
}

void APU::run(void) {
    this->cycle++;
    // https://www.nesdev.org/wiki/APU_Frame_Counter
    switch (this->cycle) {
        case 3728:
        case 7456:
        case 11185:
        case 14914:
            this->clockFrameCounter();
            this->clockDivider();
    }
    if (this->cycle == 14914) this->cycle = 0;
}

void APU::clockFrameCounter() {
    if (this->frameCounter.mode == 0) {
        // 4-step sequence
        // 0 1 2 3
        // - - - i (i = trigger frame interrupt if interruptInhibit = 0)
        // - l - l (l = length counter and sweep unit) half frame
        // e e e e (e = envelope and triangle's linear counter) quarter frame
        switch (this->frameCounter.counter) {
            case 1:
            case 3:
                this->clockLengthCounter();
                this->clockSweepUnit();
                if (this->frameCounter.interruptInhibit == 0) {
                    this->readableStatus.frameCounterInterrupt = 1;
                    this->bus.cpu.handleIrq();
                }
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
            case 0:
                this->clockLinearCounter();
                this->clockEnvelope();
                break;
            case 1:
                this->clockLengthCounter();
                this->clockSweepUnit();
                this->clockLinearCounter();
                this->clockEnvelope();
                break;
            case 2:
                this->clockLinearCounter();
                this->clockEnvelope();
                break;
            case 4:
                this->clockLengthCounter();
                this->clockSweepUnit();
                this->clockLinearCounter();
                this->clockEnvelope();
        }
        this->frameCounter.counter = (this->frameCounter.counter + 1) % 5;
    }
}

void APU::clockDivider() {

}

void APU::clockLengthCounter() {
    if (this->pulseChannel1.reg0.lengthCounterHalt == 0 && this->pulseChannel1.lengthCounter > 0) {
        this->pulseChannel1.lengthCounter--;
    }
    if (this->pulseChannel2.reg0.lengthCounterHalt == 0 && this->pulseChannel2.lengthCounter > 0) {
        this->pulseChannel2.lengthCounter--;
    }
    if (this->triangleChannel.reg0.linearCounterControlFlag == 0 && this->triangleChannel.lengthCounter > 0) {
        this->triangleChannel.lengthCounter--;
    }
    if (this->noiseChannel.reg0.lengthCounterHalt == 0 && this->noiseChannel.lengthCounter > 0) {
        this->noiseChannel.lengthCounter--;
    }
}

void APU::clockSweepUnit() {
    for (uint8_t i = 0; i < 2; i++) {
        PULSEREG* pulse = i == 0 ? &this->pulseChannel1 : &this->pulseChannel2;
        uint8_t diff = i == 0 ? 1 : 0;
        // calculating the target period
        uint16_t timerPeriod = pulse->reg2.timerLow | (pulse->reg3.timerHigh << 8);
        uint16_t changeAmount = timerPeriod >> pulse->reg1.sweepShiftCount;
        if (pulse->reg1.sweepNegate == 0) {
            pulse->realTimer = timerPeriod + changeAmount;
        } else {
            changeAmount += diff;
            pulse->realTimer = timerPeriod - changeAmount;
        }
        // If the divider's counter is zero, the sweep is enabled, and the sweep unit is not muting the channel: The pulse's period is set to the target period.
        if (pulse->realTimer >= 8 && pulse->realTimer <= 0x7ff) {
            pulse->reg2.timerLow = pulse->realTimer & 0xff;
            pulse->reg3.timerHigh = (pulse->realTimer >> 8) & 0x7;
        }
    }
}

void APU::clockEnvelope() {
    // TODO get the p value from register
    uint8_t p = 5;
    if (this->dividerCount == 0) {
        this->dividerCount = p;

    } else {
        this->dividerCount--;
    }
}

void APU::clockLinearCounter() {

}



uint8_t APU::regRead(uint16_t addr) {
    // apu 只有 $4015 状态寄存器可读
    if (addr == 0x4015) {
        // TODO
        if (this->dmcChannel.sampleLength > 0) {
            this->readableStatus.dmcActive = 1;
        }
        return this->readableStatus.status;
    }
    return 0;
}

void APU::regWrite(uint16_t addr, uint8_t data) {
    // pulse 1 channel $4000-$4003
    if (addr == 0x4000) {
        *(uint8_t*)(&this->pulseChannel1.reg0) = data;
    } else if (addr == 0x4001) {
        *(uint8_t*)(&this->pulseChannel1.reg1) = data;
    } else if (addr == 0x4002) {
        *(uint8_t*)(&this->pulseChannel1.reg2) = data;
    } else if (addr == 0x4003) {
        *(uint8_t*)(&this->pulseChannel1.reg3) = data;
    }
    // pulse 2 channel $4004-$4007
    else if (addr == 0x4004) {
        *(uint8_t*)(&this->pulseChannel2.reg0) = data;
    } else if (addr == 0x4005) {
        *(uint8_t*)(&this->pulseChannel2.reg1) = data;
    } else if (addr == 0x4006) {
        *(uint8_t*)(&this->pulseChannel2.reg2) = data;
    } else if (addr == 0x4007) {
        *(uint8_t*)(&this->pulseChannel2.reg3) = data;
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
    // status register $4015
    else if (addr == 0x4015) {
        this->writeableStatus.status = data;
        // clear dmc interrupt flag
        this->dmcChannel.reg0.irqEnable = 0;
        if (this->writeableStatus.dmcEnable) {

        } else {
            this->dmcChannel.sampleLength = 0;
            this->dmcChannel.sampleAddress = 0;
        }
    } else if (addr == 0x4017) {
        this->frameCounter.value = data;
    }
}