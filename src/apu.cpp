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
    this->status.status = 0;
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
        case 18640:
            // 当模式1且14914 apu周期时， FrameCounter 不会触发
            if (this->frameCounter.mode != 1 || this->cycle != 14914) this->clockFrameCounter();
            this->clockDivider();
    }
    if (this->cycle == 18640) this->cycle = 0;
}

void APU::clockFrameCounter() {
    if (this->frameCounter.mode == 0) {
        // 4-step sequence
        // 3 2 1 0
        // - - - i (i = trigger frame interrupt if interruptInhibit = 0)
        // - l - l (l = length counter and sweep unit) half frame
        // e e e e (e = envelope and triangle's linear counter) quarter frame
        this->frameCounter.counter = (this->frameCounter.counter + 1) % 4;
        if (this->frameCounter.counter == 0 && this->frameCounter.interruptInhibit == 0) {
            this->status.frameCounterInterrupt = 1;
            this->bus.cpu.handleIrq();
        }
        if (this->frameCounter.counter == 0 || this->frameCounter.counter == 2) {
            this->clockLengthCounter();
            this->clockSweepUnit();
        }
        this->clockEnvelope();
        this->clockLinearCounter();
    } else {
        // 5-step sequence
        // 4 3 2 1 0
        // - l - l - (l = length counter and sweep unit) half frame
        // e e e e - (e = envelope and triangle's linear counter) quarter frame
        this->frameCounter.counter = (this->frameCounter.counter + 1) % 5;
        switch (this->frameCounter.counter) {
            case 1:
            case 3:
                this->clockLengthCounter();
                this->clockSweepUnit();
            case 2:
            case 4:
                this->clockEnvelope();
                this->clockLinearCounter();
        }

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
    return 0;
}

void APU::regWrite(uint16_t addr, uint8_t data) {

}