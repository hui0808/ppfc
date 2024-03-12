#ifndef __PPFC_APU_H__
#define __PPFC_APU_H__

#include "common.h"
class PPFC;

struct PULSEREG {
    struct {
        uint8_t volume : 4;  // volume/envelope divider period
        uint8_t constantVolume : 1;
        uint8_t lengthCounterHalt : 1;
        uint8_t duty : 2;
    } reg0;
    uint8_t lengthCounter;
    // sweep unit
    struct {
        uint8_t sweepShiftCount : 3;
        uint8_t sweepNegate : 1;
        uint8_t sweepPeriod : 3;
        uint8_t sweepEnable : 1;
    } reg1;
    uint8_t sweepDividerCounter; // sweep unit divider counter
    uint8_t sweepReload; // sweep unit reload flag，force reload divider counter
    // sweep unit end
    uint16_t realTimer; // real timer, 11 bits, 0x000 - 0x7FF, inversely proportional to frequency
    struct {
        uint8_t timerLow : 8; // b0 - b7 of 11-bits timer
    } reg2;
    struct {
        uint8_t timerHigh : 3; // b8 - b10 of 11-bits timer
        uint8_t lengthCounterLoad : 5; // the index of length counter table
    } reg3;
};

struct TRIANGLEREG {
    struct {
        uint8_t linearCounterReloadValue : 7;
        uint8_t linearCounterControlFlag : 1; // also the length counter halt flag
    } reg0; // Linear counter setup
    struct {
        uint8_t timerLow : 8;
    } reg1;
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

union APUSTATUS {
    uint8_t status;
    struct {
        uint8_t pulse1Enable : 1;
        uint8_t pulse2Enable : 1;
        uint8_t triangleEnable : 1;
        uint8_t noiseEnable : 1;
        uint8_t dmcEnable : 1;
        uint8_t unused1 : 3;
    };
    struct {
        uint8_t pulse1LengthCounterZero : 1;
        uint8_t pulse2LengthCounterZero : 1;
        uint8_t triangleLengthCounterZero : 1;
        uint8_t noiseLengthCounterZero : 1;
        uint8_t dmcActive : 1;
        uint8_t unused2 : 1;
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
    PULSEREG pulseChannel1;
    PULSEREG pulseChannel2;
    TRIANGLEREG triangleChannel;
    NOISEREG noiseChannel;
    APUSTATUS status;
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

#endif // __PPFC_APU_H__
