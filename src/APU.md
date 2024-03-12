APU
APU 有5个声道
1. 两个方波声道
2. 一个三角波声道
3. 一个利用线性反馈移位寄存器（LFSR）的噪声声道
4. 一个用来播放DPCM的增量调制声道
每个声道都有一个波形生成器(waveform generator)，它是一个16位的线性计数器，每个时钟周期都会增加1，当计数器的值达到某个值时，会产生一个方波脉冲。这个值就是波形生成器的频率。波形生成器的频率可以通过写入APU的寄存器来改变。
每个声道都有调制器，由帧计数器（Frame Counter）降频驱动
The DMC plays samples while the other channels play waveforms.
每个通道都可以独立并行
修改一个通道的参数时，只会影响到该通道中的一个子单元，并且这个修改只会在该子单元的下一个内部周期开始后生效
每个通道都有一个长度计数器（length counter），它会在每个时钟周期减1，当计数器的值为0时，该通道会停止播放，用以控制通道的状态，长度计数器的值可以通过写入APU的寄存器来改变。
对于脉冲波通道，除了长度计数器，还有两个静音的条件：频率超过一定阈值或者扫描（sweep）的频率低到一定阈值。

APU 寄存器图表
Registers	应用的声道    Units
$4000–$4003	Pulse 1	     Timer, length counter, envelope, sweep
$4004–$4007	Pulse 2	     Timer, length counter, envelope, sweep
$4008–$400B	Triangle	 Timer, length counter, linear counter
$400C–$400F	Noise	     Timer, length counter, envelope, linear feedback shift register
$4010–$4013	DMC	         Timer, memory reader, sample buffer, output unit
$4015	    All	         Channel enable and length counter status
$4017	    All	         Frame counter

```ts
    class WaveformGenerator {
        // 时钟周期
        clock: number;
        // 寄存器
        registers: Uint8Array;
        // 时钟周期
        clock: number;
    }

    class Modulator {
        // 时钟周期
        clock: number;
        // 寄存器
        registers: Uint8Array;
        // 时钟周期
        clock: number;
    }

    class LengthCounter {
        // 时钟周期
        clock: number;
        // 寄存器
        registers: Uint8Array;
        // 时钟周期
        clock: number;
    }

    class Envelope {
        // 时钟周期
        clock: number;
        // 寄存器
        registers: Uint8Array;
        // 时钟周期
        clock: number;
    }

    class FrameCounter {
        // 时钟周期
        clock: number;
        // 帧计数器
        counter: number;
        // 模式
        mode: number;
        // 时钟周期
        clock: number;
    }

    class DMC {
        // 时钟周期
        clock: number;
        // 寄存器
        registers: Uint8Array;
        // 时钟周期
        clock: number;
    }

    class Noise {
        // 波形生成器
        waveformGenerator: WaveformGenerator;
        // 调制器
        modulator: Modulator;
        // 长度计数器
        lengthCounter: LengthCounter;
        // 音量调制器
        envelope: Envelope;
        // 寄存器
        registers: Uint8Array;
        // 时钟周期
        clock: number;
    }

    class Triangle {
        // 波形生成器
        waveformGenerator: WaveformGenerator;
        // 调制器
        modulator: Modulator;
        // 长度计数器
        lengthCounter: LengthCounter;
        // 线性计数器
        linearCounter: LinearCounter;
        // 寄存器
        registers: Uint8Array;
        // 时钟周期
        clock: number;
    }

    class Pulse {
        // 波形生成器
        waveformGenerator: WaveformGenerator;
        // 调制器
        modulator: Modulator;
        // 长度计数器
        lengthCounter: LengthCounter;
        // 音量调制器
        envelope: Envelope;
        // 扫描
        sweep: Sweep;
        // 寄存器
        registers: Uint8Array;
        // 时钟周期
        clock: number;
    }

    class APU {
        // 5个声道
        pulse1: Pulse;
        pulse2: Pulse;
        triangle: Triangle;
        noise: Noise;
        dmc: DMC;
        // 帧计数器
        frameCounter: FrameCounter;
        // 寄存器
        registers: Uint8Array;
        // 时钟周期
        clock: number;
    }
```
长度计数器用于控制声道的时长，每被frame counter驱动一次，长度计数器减1，当长度计数器为0时，声道停止播放
扫描单元可以定期调整方波声道的周期，以实现音调的变化
    每个扫描单元有一个分频器计数器以及一个reload标志

apu 运行频率为240hz，即1帧运行4次
在 apu 的一次周期中
触发一次帧计数器（frame counter）
    根据$4017寄存器的b7的值
        1 为5步模式，周期在 0 - 4 之间循环
            如果周期为 1 或 3，触发所有声道的长度计数器（length counter）以及两个方波的扫描（sweep）
            如果周期为 0124，触发所有声道的音量调制器（envelope unit）
        0 为4步模式，周期在 0 - 3 之间循环
            如果周期为0，触发frame IRQ
            如果周期为02，触发所有声道的长度计数器（length counter）以及两个方波的扫描（sweep）
            如果周期为 0123，触发所有声道的音量调制器（envelope unit）以及线性计数器（linear counter）
在触发所有声道的length counter的一次周期中
    所有声道的length counter halt标志位为0时且长度不为0时，length counter减1
在触发两个方波的扫描的一次周期中
    如果reload为1或者分频器计数器为0，将reload标志位清0，将分频器计数器设置为sweep period，否则分频器计数器减1



