#ifndef __PPFC_CPU_H__
#define __PPFC_CPU_H__

#include "common.h"
#include "memory.h"

class PPFC;

#define ACCIMPID UINT32_MAX // 用于标记ACC和IMP寻址

union CPUStatus {
	uint8_t status;
	struct {
		bool c : 1;
		bool z : 1;
		bool i : 1;
		bool d : 1;
		bool b : 1;
		bool r : 1;
		bool v : 1;
		bool s : 1;
	};
};

enum CPUVector {
	VERCTOR_NMI    = 0xFFFA,   // 不可屏蔽中断
	VERCTOR_RESET  = 0xFFFC,   // 重置CP指针地址
	VERCTOR_IRQBRK = 0xFFFE,   // 中断重定向
};

// 状态寄存器掩码
enum CPUStatusMask {
	MASK_CF = 1 << 0,    // 进位标记(Carry flag)
	MASK_ZF = 1 << 1,    // 零标记 (Zero flag)
	MASK_IF = 1 << 2,    // 禁止中断(Irq disabled flag)
	MASK_DF = 1 << 3,    // 十进制模式(Decimal mode flag)
	MASK_BF = 1 << 4,    // 软件中断(BRK flag)
	MASK_RF = 1 << 5,    // 保留标记(Reserved), 一直为1
	MASK_VF = 1 << 6,    // 溢出标记(Overflow  flag)
	MASK_SF = 1 << 7,    // 符号标记(Sign flag)
};

class CPU {
public:
	PPFC& bus;
	uint8_t a;
	uint8_t x;
	uint8_t y;
	uint8_t sp;
	uint16_t pc;
	CPUStatus p;
	uint8_t ram[2048] = {0};
	Memory memory;
	bool pageCrossed;
	uint32_t cycle;
	uint32_t count;
	bool nmi;
	bool preNmi;
	bool irq;

	CPU(PPFC& bus);
	void init(void);
	void reset(void);
	void memoryInit(void);
	void run(void);
	void handleNmi(void);
	void handleIrq(void);
	void checkZSFlag(uint8_t value);
	void push(uint8_t value);
	uint8_t pop(void);
	void cpulog(const char* op, const char* addrmode, uint32_t addr, uint8_t no, uint16_t pc);

private:
    uint32_t addr_ACC(void); // 累加器寻址
    uint32_t addr_IMP(void); // 隐含寻址
    uint32_t addr_IMM(void); // 立即寻址
    uint32_t addr_ABS(void); // 绝对寻址
    uint32_t addr_ABX(void); // 绝对X变址寻址
    uint32_t addr_ABY(void); // 绝对Y变址寻址
    uint32_t addr_ZPG(void); // 绝对零页寻址
    uint32_t addr_ZPX(void); // 零页X变址寻址
    uint32_t addr_ZPY(void); // 零页Y变址寻址
    uint32_t addr_IND(void); // 间接寻址
    uint32_t addr_INX(void); // 间接X变址寻址(先变址X后间接寻址)
    uint32_t addr_INY(void); // 间接Y变址(后变址Y间接寻址)
    uint32_t addr_REL(void); // 相对寻址

	// 指令声明
	#define UOP(op, ...) void opt_##op(uint32_t address);
	#include "instruction.h"
};

inline void CPU::checkZSFlag(uint8_t value) {
	this->p.s = value & 0x80;
	this->p.z = value == 0;
}

inline void CPU::push(uint8_t value) {
	this->memory.write(0x0100 | this->sp, value);
	this->sp--;
}

inline uint8_t CPU::pop(void) {
	this->sp++;
	return this->memory.read(0x0100 | this->sp);
}

inline uint32_t CPU::addr_ACC(void) { // 累加器寻址
	return ACCIMPID;
}

inline uint32_t CPU::addr_IMP(void) { // 隐含寻址
	return ACCIMPID;
}

inline uint32_t CPU::addr_IMM(void) { // 立即寻址
	uint16_t addr = this->pc;
	this->pc++;
	return addr;
}

inline uint32_t CPU::addr_ABS(void) { // 绝对寻址
	uint16_t addr0 = this->memory.read(this->pc);
	uint16_t addr1 = this->memory.read(this->pc + 1);
	this->pc += 2;
	return addr0 | (addr1 << 8);
}

inline uint32_t CPU::addr_ABX(void) { // 绝对X变址寻址
	uint16_t base = this->addr_ABS();
	uint16_t addr = base + this->x;
	this->pageCrossed = ((base ^ addr) & 0x0100) == 0x0100;
	return addr;
}

inline uint32_t CPU::addr_ABY(void) { // 绝对Y变址寻址
	uint16_t base = this->addr_ABS();
	uint16_t addr = base + this->y;
	this->pageCrossed = ((base ^ addr) & 0x0100) == 0x0100;
	return addr;
}

inline uint32_t CPU::addr_ZPG(void) { // 绝对零页寻址
	uint8_t addr = this->memory.read(this->pc);
	this->pc++;
	return addr;
}

inline uint32_t CPU::addr_ZPX(void) { // 零页X变址寻址
	return uint8_t(this->addr_ZPG() + this->x);
}

inline uint32_t CPU::addr_ZPY(void) { // 零页Y变址寻址
	return uint8_t(this->addr_ZPG() + this->y);
}

inline uint32_t CPU::addr_IND(void) { // 间接寻址
	uint16_t base = this->addr_ABS();
	// 正常情况
	//uint16_t addr0 = this->memory.read(base);
	//uint16_t addr1 = this->memory.read(base + 1);
    // 现实是6502存在BUG，必须刻意实现
	uint16_t addr0 = this->memory.read(base);
	uint16_t addr1 = this->memory.read((base & 0xFF00) | ((base + 1) & 0x00FF));
	return addr0 | (addr1 << 8);
}

inline uint32_t CPU::addr_INX(void) { // 间接X变址寻址(先变址X后间接寻址)
	uint16_t base = this->memory.read(this->pc) + this->x;
	this->pc++;
	uint16_t addr0 = this->memory.read(uint8_t(base));
	uint16_t addr1 = this->memory.read(uint8_t(base + 1));
	return addr0 | (addr1 << 8);
}

inline uint32_t CPU::addr_INY(void) { // 间接Y变址(后变址Y间接寻址)
	uint16_t base = this->memory.read(this->pc);
	this->pc++;
	uint16_t addr0 = this->memory.read(uint8_t(base));
	uint16_t addr1 = this->memory.read(uint8_t(base + 1));
	uint16_t base1 = addr0 | (addr1 << 8);
	uint16_t addr = base1 + this->y;
	this->pageCrossed = ((base1 ^ addr) & 0x0100) == 0x0100;
	return addr;
}

inline uint32_t CPU::addr_REL(void) { // 相对寻址
	int8_t offset = this->memory.read(this->pc);
	this->pc++;
	uint16_t addr = this->pc + offset;
	return addr;
}

const uint8_t cpuOptCycles[256]{
	#define OP(opt, addrmode, cycles, no) cycles,
	#include "instruction.h"
};

#endif // __PPFC_CPU_H__
