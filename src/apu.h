#ifndef __PPFC_APU_H__
#define __PPFC_APU_H__

#include "common.h"
class PPFC;
class PULSE;

struct TRIANGLEREG {
    // $4008
    struct {
        uint8_t linearCounterReloadValue : 7;
        uint8_t linearCounterControlFlag : 1; // also the length counter halt flag
    } reg0; // Linear counter setup
    // $4009 unused
    // $400A
    struct {
        uint8_t timerLow : 8;
    } reg1;
    // $400B
    struct {
        uint8_t timerHigh : 3;
        uint8_t lengthCounterLoad : 5;
    } reg2;
    uint8_t lengthCounter;
};

struct NOISEREG {
    struct {
        uint8_t volume : 4;  // volume/envelope divider period
        uint8_t constantVolume : 1;  // constant volume/envelope flag
        uint8_t lengthCounterHalt : 1; // length counter halt flag
        uint8_t unused : 2;
    } reg0;
    struct {
        uint8_t noisePeriod : 4;
        uint8_t unused : 3;
        uint8_t noiseEnable : 1;
    } reg1;
    struct {
        uint8_t unused : 3;
        uint8_t lengthCounterLoad : 5;
    } reg2;
    uint8_t lengthCounter;
};

// TODO: DMC
struct DMCREG {
    struct {
        uint8_t frequency : 4;
        uint8_t unused : 2;
        uint8_t loop : 1;
        uint8_t irqEnable : 1;
    } reg0;
    struct {
        uint8_t loadCounter : 7;
        uint8_t unused : 1;
    } reg1;
    uint8_t sampleAddress; // reg2
    uint8_t sampleLength; // reg3
};

// $4015 status register for write
union APU_WRITEABLE_STATUS {
    uint8_t status;
    struct {
        uint8_t pulse1Enable : 1;
        uint8_t pulse2Enable : 1;
        uint8_t triangleEnable : 1;
        uint8_t noiseEnable : 1;
        uint8_t dmcEnable : 1;
        uint8_t unused : 3;
    };
};

// $4015 status register for read
union APU_READABLE_STATUS {
    uint8_t status;
    struct {
        uint8_t pulse1LengthCounterZero : 1;
        uint8_t pulse2LengthCounterZero : 1;
        uint8_t triangleLengthCounterZero : 1;
        uint8_t noiseLengthCounterZero : 1;
        uint8_t dmcActive : 1;
        uint8_t unused : 1;
        uint8_t frameCounterInterrupt : 1;
        uint8_t dmcInterrupt : 1;
    };
};

union APUFRAMECOUNTER {
    uint8_t value;
    struct {
        uint8_t counter : 6;
        uint8_t interruptInhibit : 1; // Interrupt inhibit flag. If set, the frame interrupt flag is cleared, otherwise it is unaffected.
        uint8_t mode : 1; // Sequencer mode: 0 selects 4-step sequence, 1 selects 5-step sequence
    };
};

class APU {
public:
    PPFC& bus;
    PULSE pulse1;
    PULSE pulse2;
    PULSEREG pulseChannel1;
    PULSEREG pulseChannel2;
    TRIANGLEREG triangleChannel;
    NOISEREG noiseChannel;
    DMCREG dmcChannel;
    APU_WRITEABLE_STATUS writeableStatus;
    APU_READABLE_STATUS readableStatus;
    APUFRAMECOUNTER frameCounter;
    uint32_t cycle;
    uint8_t dividerCount;

    APU(PPFC& bus);
    void reset(void);
    void init(void);
    void run(void);

    void clockFrameCounter();
    void clockDivider();
    void clockLengthCounter();
    void clockSweepUnit();
    void clockEnvelope();
    void clockLinearCounter();

    uint8_t regRead(uint16_t addr);
    void regWrite(uint16_t addr, uint8_t data);
};

class PULSE {
    APU& bus;
    uint8_t sequencer;
    uint16_t timer : 11; // real timer, 11 bits, 0x000 - 0x7FF, inversely proportional to frequency
    uint8_t lengthCounter; // 时长计数器
    // envelope

    uint8_t sweepDividerCounter; // sweep unit divider counter
    uint8_t sweepReload; // sweep unit reload flag，force reload divider counter
    uint8_t sweepEnable;
    uint8_t sweepShiftCount;
    uint8_t sweepNegate;
    uint8_t sweepPeriod;

    uint8_t volume;
    uint8_t constantVolume;
    uint8_t lengthCounterHalt;
    uint8_t duty;

    uint8_t output;
    PULSE(APU& bus);
    void reset(void);
    void init(void);
    void run(void);

    struct PULSE_REG0 {
        uint8_t volume : 4;  // volume/envelope divider period
        uint8_t constantVolume : 1;
        uint8_t lengthCounterHalt : 1;
        uint8_t duty : 2;
    };

    struct PULSE_REG1 {
        uint8_t sweepShiftCount : 3;
        uint8_t sweepNegate : 1;
        uint8_t sweepPeriod : 3;
        uint8_t sweepEnable : 1;
    };

    struct PULSE_REG2 {
        uint8_t timerLow : 8; // b0 - b7 of 11-bits timer
    };

    struct PULSE_REG3 {
        uint8_t timerHigh : 3; // b8 - b10 of 11-bits timer
        uint8_t lengthCounterLoad : 5; // the index of length counter table
    };
};

#endif // __PPFC_APU_H__
