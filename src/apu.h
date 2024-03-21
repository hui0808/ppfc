#ifndef __PPFC_APU_H__
#define __PPFC_APU_H__
/**
 * 分频器（divider）周期性输出时钟，它包括一个周期重载值P，一个从P开始计数的计数器，当分频器被时钟驱动时
 * 如果计数器为0，则重新赋值为P，且同时输出时钟，否则计数器不断递减，计数器的变化周期则为P+1
 * 可以强制使计数器立即重载P值，但这个过程不会输出时钟，相似的，改变P值也不会使当前计数器的值重新加载
 * 一些分频器的计数器无法被强制重载，但是可以通过给P设置为0，使得计数器到时候重载为0
 * 分频器可能以递减的形式实现，也有可能以线性反馈移位寄存器（LFSR）形式实现
 * 分频器在方波和三角波通道的是递减的形式实现，为了节省逻辑门，在噪声、DMC以及APU Frame Counter上是LFSR形式实现
 * 5个通道上的计时器（timer）都是用来控制其输出的声音频率，它包含一个被CPU时钟驱动的分频器。
 * 三角波通道上的计时器同步被1个CPU时钟驱动，而方波，噪声以及DCM通道的计时器被2个CPU时钟驱动
 *
 * LFSR 看这里 https://www.zhihu.com/question/591327469/answer/2987694053
 */

/**
 * 方波通道包括
 * 包络发生器（envelope generator），用来生成原始的声音包络
 * 扫描单元（sweep unit），动态调整计时器的值，以控制方波的周期
 * 计时器（timer），控制方波的周期
 * 8步序列器（8-step sequencer），控制方波的占空比
 * 长度计数器（length counter），控制方波输出的时长
 *
 * 序列器
 * 序列器由11bit的计时器时钟驱动，而计时器又被APU时钟驱动，计数器以递减的形式实现
 * 序列器包含一个递减计数器，其在0-7之间不断被时钟驱动循环，然后通过不同的占空比以及当前计数，通过查表输出
 * 计时器重载值为t，当计时器值为0时，则重载为t，同时输出时钟驱动序列器
 * 计时器的周期则为t+1，而序列器为8步，则方波的周期为8*(t+1)个APU周期
 * 如果t < 8，则会使整个方波通道静音，所以方波最高频率为1.789773MHz / (2 * 8 * (8 + 1)) = 12.4kHz
 *
 * 扫描单元
 * 扫描单元包括一个分频器以及重载标志位
 * 当timer或者扫描单元设置发生改变时，扫描单元会重新计算着目标周期，
 * 目标周期 = 方波timer + 变化量
 * 变化量 = 方波timer >> shift count
 * 如果negate标志位为1，则变化量需要转为负数
 * 如果目标周期溢出了11bit（target >= t），需要限制（clamp）为0
 *
 * 扫描单元会使通道静音条件
 * timer < 8 或者 目标周期 > 0x07FF
 *
 * 何时将目标周期更新到timer？
 * 当frame counter发出半帧时钟信号时，会驱动扫描单元的分频器
 * 如果分频器的计数器为0，且扫描单元使能位为1，且shift count不为0时
 *   如果扫描单元不使通道静音，则将目标周期更新到timer
 * 如果重载标志位为1，则重新装载分频器的计数器为P，标志位清除
 * 写入$4001/$4005会使重载标志位为1
 *
 * 长度计数器
 * 如果$4015的使能标志位为1，计数器的值会以length counter load为入口，从 length table 查表得到
 * 如果使能位为0，计数器的值会被清掉
 * 长度计数器会被frame counter的半帧时钟驱动，计数器不断递减直至为0，除非length counter halt的值为1
 * 计数器为达到0后，会使通道静音
 * 长度计数器相关寄存器被写入后，方波的相位会重置，包络重新启动，三角波的线性计数器的reload标志位设为1
 *
 * 包络发生器（envelope generator）
 * 包括一个start标志符，分频器，衰减等级计数器（decay level counter）
 * 在合成器上，包络是量化声音特征随时间变化的方法
 * 包络发生器有两种控制音量输出的方法
 * 一种是输出设定好的固定音量（sustain）
 * 一种是输出不断衰弱的锯齿波（decay）
 * 如果向$4003、$4007、$400F写入，则设置start标志位为1
 * 工作原理
 * frame counter的四分之一帧时钟驱动
 *   如果start标志位为1，则清空标志位，decay level counter重新设置为15
 *   如果为0，则驱动分频器
 *     如果分频器计数器为0，重载V值，驱动decay level counter
 *       如果decay level counter不为0，递减
 *       为0且loop标志位为1，重新设置为15
 * 输出
 * 如果固定音量标志位为1，则输出固定音量V，否则输出decay level counter的值
 *
 * 方波通道输出
 * 如果sweep使通道静音，或者序列器输出0，或者长度计数器为0，则通道输出0
 * 否则输出包络音量
 *
 * 混音器(Mixer)
 * 混音器收集全部通道的输出，并将它们按照不同的比例组合在一起，最终转为模拟信号输出（范围0.0f~1.0f）
 */


#include "common.h"
class PPFC;
class APU;

const uint8_t lengthTable[0x20] = {
        10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

struct Envelope {
    uint8_t start; // start flag
    uint8_t dividerCounter;
    uint8_t volume; // constant volume value or dividerCounter reload value
    uint8_t decayLevelCounter;
    uint8_t loop; // decay level counter loop flag
    uint8_t constantVolume; // constant volume flag
    uint8_t output;
};

struct Sweep {
    uint8_t enable; // enabled flag
    uint8_t reload; // divider reload flag
    uint8_t dividerLoad; // divider reload value
    uint8_t dividerCounter;
    uint8_t negate; // negate flag; 0-add, 1-subtract
    uint8_t shiftCounter;
    uint16_t targetPeriod;
    uint8_t output; // 控制通道是否静音
};

class Pulse {
public:
    uint8_t channel; // 0 or 1
    uint8_t enable;
    APU& bus;
    Envelope envelope;
    Sweep sweep;
    uint8_t duty;
    uint8_t sequencer;
    uint16_t timerLoad : 11; // timerLoad, 11 bits, 0x000 - 0x7FF
    uint16_t timer: 11;
    uint8_t sequencerOutput; // 序列器输出，包含占空比信息
    uint8_t lengthCounter; // 时长计数器
    uint8_t lengthCounterLoad;
    uint8_t lengthCounterHalt;
    uint8_t output;

    Pulse(APU& bus, uint8_t channel);
    void reset(void);
    void init(void);
    void run(void);
    void regWrite(uint16_t addr, uint8_t data);
    void clockEnvelope(void);
    void clockSweep(void);
    void clockSweepDivider(void);
    void clockLengthCounter(void);

    struct PulseReg0 {
        uint8_t volume : 4;  // volume/envelope divider period
        uint8_t constantVolume : 1;
        uint8_t lengthCounterHalt : 1;
        uint8_t duty : 2;
    };
    struct PulseReg1 {
        uint8_t sweepShiftCount : 3;
        uint8_t sweepNegate : 1;
        uint8_t sweepPeriod : 3;
        uint8_t sweepEnable : 1;
    };
    struct PulseReg2 {
        uint8_t timerLow : 8; // b0 - b7 of 11-bits timerLoad
    };
    struct PulseReg3 {
        uint8_t timerHigh : 3; // b8 - b10 of 11-bits timerLoad
        uint8_t lengthCounterLoad : 5; // the index of length counter table
    };
};

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
        uint8_t interruptInhibit : 1; // 中断禁止标识符，如果为1，则清除frame interrupt 标识符
        uint8_t mode : 1; // Sequencer mode: 0 selects 4-step sequence, 1 selects 5-step sequence
    };
};

class APU {
public:
    PPFC& bus;
    Pulse pulseChannel1;
    Pulse pulseChannel2;
    TRIANGLEREG triangleChannel;
    NOISEREG noiseChannel;
    DMCREG dmcChannel;
    APU_WRITEABLE_STATUS writeableStatus;
    APU_READABLE_STATUS readableStatus;
    APUFRAMECOUNTER frameCounter;
    uint32_t cycle;
    uint8_t output;

    APU(PPFC& bus);
    void reset(void);
    void init(void);
    void run(void);

    void clockFrameCounter();
    void clockLengthCounter();
    void clockSweepUnit();
    void clockEnvelope();
    void clockLinearCounter();

    uint8_t channelRegRead(uint16_t addr);
    void channelRegWrite(uint16_t addr, uint8_t data);
    uint8_t statusRegRead(uint16_t addr);
    void statusRegWrite(uint16_t addr, uint8_t data);
    void frameCounterRegWrite(uint16_t addr, uint8_t data);
    void mix();
};

#endif // __PPFC_APU_H__
