#include "ppfc.h"
#include "cpu.h"

CPU::CPU(PPFC& bus) :bus(bus), memory(65536) {
    this->a = 0;
    this->x = 0;
    this->y = 0;
    this->sp = 0;
    this->pc = 0;
    this->p.status = MASK_RF;
    this->pageCrossed = false;
    this->count = 0;
}

void CPU::init(void) {
    this->memoryInit();
    this->reset();
}

void CPU::reset(void) {
    // http://wiki.nesdev.com/w/index.php/CPU_power_up_state
    this->pc = this->memory.readw(VERCTOR_RESET);
    this->cycle = cpuOptCycles[this->memory.read(this->pc)];
    this->p.i = 1;
    this->nmi = 0;
    this->irq = 0;
    this->preNmi = 0;
    this->sp -= 0x03;
}

void CPU::memoryInit(void) {
    this->memory.map(0, 0x2000, this->ram, 2048); // Internal-RAM
    this->memory.map(0x2000, 0x4000, READBIND(this->bus.ppu.regRead), WRITEBIND(this->bus.ppu.regWrite), 8); // PPU-Registers
    this->memory.map(0x4000, 0x4014, READBIND(this->bus.apu.regRead), WRITEBIND(this->bus.apu.regWrite), 20); // APU-Registers of $4000-$4013
    this->memory.map(0x4014, 0x4015, nullptr, WRITEBIND(this->bus.ppu.dma), 1, false); // PPU-OAM-DMA
    this->memory.map(0x4015, 0x4016, READBIND(this->bus.apu.regRead), WRITEBIND(this->bus.apu.regWrite), 1); // APU-Registers of $4015
    this->memory.map(0x4016, 0x4017, READBIND(this->bus.keyboard.port16Read), WRITEBIND(this->bus.keyboard.port16Write), 1); // I/O-Registers1
    this->memory.map(0x4017, 0x4018, READBIND(this->bus.keyboard.port17Read), WRITEBIND(this->bus.apu.regWrite), 1); // I/O-Registers2 and APU-Registers of $4017
    if (this->bus.cartridge.saveram) { // Save-RAM
        this->memory.map(0x6000, 0x8000, this->bus.cartridge.saveram, 8192);
    }
}

void CPU::cpulog(const char* op, const char* addrmode, uint32_t addr, uint8_t no, uint16_t pc) {
    this->count++;
    printf("%04d ", this->count);
    // printf("$%04X %s %s $%02X A:%02X X:%02X Y:%02X P:%02X SP:%02X ctrl:%02X mask:%02X status:%02X vaddr:%04X cycle:%03d line:%03d",
    // pc, op, addrmode, no, this->a, this->x, this->y, this->p.status, this->sp, this->bus.ppu.ctrl.ctrl, this->bus.ppu.mask.mask, this->bus.ppu.status.status, this->bus.ppu.vaddr, this->bus.ppu.cycle, this->bus.ppu.scanline);
    printf("$%04X $%02X A:%02X X:%02X Y:%02X P:%02X SP:%02X ctrl:%02X mask:%02X status:%02X vaddr:%04X cycle:%03d line:%03d",
        pc, no, this->a, this->x, this->y, this->p.status, this->sp, this->bus.ppu.ctrl.ctrl, this->bus.ppu.mask.mask, this->bus.ppu.status.status, this->bus.ppu.vaddr, this->bus.ppu.cycle, this->bus.ppu.scanline);
    addr == ACCIMPID ? printf(" -001") : printf(" %04X", addr);
    putchar('\n');
}

#define OP(op, address, cyc, no) \
case 0x##no: { \
    uint32_t addr = this->addr_##address(); \
    CPULOG(#op, #address, addr, 0x##no, this->pc - 1); \
    this->opt_##op(addr); \
    break; \
}

#define CHECK_NMI() this->nmi = this->bus.ppu.status.vblank && this->bus.ppu.ctrl.nmiGen

void CPU::run(void) {
    CHECK_NMI();
    if (--this->cycle == 0) {
        uint8_t optcode = this->memory.read(this->pc);
        this->pc++;
        switch (optcode) {
            #include "instruction.h"
        }
        this->pageCrossed = false;
        this->handleNmi();
        this->handleIrq();
        this->cycle += cpuOptCycles[this->memory.read(this->pc)];
    }
}

void CPU::handleNmi(void) {
    if (this->preNmi != this->nmi) {
        this->preNmi = this->nmi;
        if (this->nmi) {
            this->push(this->pc >> 8);
            this->push(uint8_t(this->pc));
            this->p.b = 0;
            this->push(this->p.status);
            this->p.i = 1;
            this->pc = this->memory.readw(VERCTOR_NMI);
            this->cycle += 7;
        }
    }
}

void CPU::handleIrq(void) {
    if (this->irq && !this->p.i) {
        this->push(this->pc >> 8);
        this->push(uint8_t(this->pc));
        this->p.b = 0;
        this->push(this->p.status);
        this->p.i = 1;
        this->pc = this->memory.readw(VERCTOR_IRQBRK);
        this->cycle += 7;
    }
}

void CPU::opt_NOP(uint32_t address) {
    this->cycle += this->pageCrossed;
}

void CPU::opt_KIL(uint32_t address) {
    printf("'KIL' stop\n");
    exit(0);
    //this->bus.status = PPFC_STOP;
}

void CPU::opt_SHY(uint32_t address) {
    this->memory.write(address, this->y & uint8_t((address >> 8) + 1));
    this->cycle += this->pageCrossed;
}

void CPU::opt_SHX(uint32_t address) {
    this->memory.write(address, this->x & uint8_t((address >> 8) + 1));
    this->cycle += this->pageCrossed;
}

void CPU::opt_TAS(uint32_t address) {
    this->sp = this->a & this->x;
    this->memory.write(address, this->sp & uint8_t((address >> 8) + 1));
    this->cycle += this->pageCrossed;
}

void CPU::opt_AHX(uint32_t address) {
    this->memory.write(address, this->a & this->x & uint8_t((address >> 8) + 1));
    this->cycle += this->pageCrossed;
}

void CPU::opt_XAA(uint32_t address) {
    // http://visual6502.org/wiki/index.php?title=6502_Opcode_8B_%28XAA,_ANE%29
    static const uint8_t magic = 0xEE;
    this->a = (this->a | magic) & this->x & this->memory.read(address);
    this->checkZSFlag(this->a);
}

void CPU::opt_LAS(uint32_t address) {
    // http://nesdev.com/undocumented_opcodes.txt
    uint8_t data = this->memory.read(address) & this->sp;
    this->a = this->x = this->sp = data;
    this->checkZSFlag(data);
    this->cycle += this->pageCrossed;
}

void CPU::opt_SRE(uint32_t address) {
    uint8_t data = this->memory.read(address);
    this->p.c = data & 0x01;
    data >>= 1;
    this->memory.write(address, data);
    this->a ^= data;
    this->checkZSFlag(this->a);
    this->cycle += this->pageCrossed;
}

void CPU::opt_SLO(uint32_t address) {
    uint8_t data = this->memory.read(address);
    this->p.c = data & 0x80;
    data <<= 1;
    this->memory.write(address, data);
    this->a |= data;
    this->checkZSFlag(this->a);
    this->cycle += this->pageCrossed;
}

void CPU::opt_RRA(uint32_t address) {
    uint16_t src = this->memory.read(address);
    src |= uint16_t(this->p.c) << 8;
    uint16_t cf = src & 0x01;
    src >>= 1;
    this->memory.write(address, uint8_t(src));
    this->cycle += this->pageCrossed;

    uint16_t res = this->a + src + cf;
    this->p.c = res & 0x100;
    this->p.v = (!((this->a ^ src) & 0x80) && ((this->a ^ uint8_t(res)) & 0x80));
    this->a = res;
    this->checkZSFlag(this->a);
}

void CPU::opt_RLA(uint32_t address) {
    uint16_t src = this->memory.read(address);
    src <<= 1;
    src |= this->p.c;
    this->p.c = src & 0x100;
    this->memory.write(address, src);
    this->a &= src;
    this->checkZSFlag(this->a);
    this->cycle += this->pageCrossed;
}

void CPU::opt_ISB(uint32_t address) {
    uint8_t data = this->memory.read(address) + 1;
    this->memory.write(address, data);
    uint8_t res = this->a - data - (this->p.c == 0);
    this->p.c = res <= this->a;
    this->p.v = (((this->a ^ data) & 0x80) && ((this->a ^ res) & 0x80));
    this->a = res;
    this->checkZSFlag(this->a);
    this->cycle += this->pageCrossed;
}

void CPU::opt_DCP(uint32_t address) {
    uint8_t data = this->memory.read(address) - 1;
    this->memory.write(address, data);
    uint8_t res = this->a - data;
    this->p.c = res <= this->a;
    this->checkZSFlag(res);
    this->cycle += this->pageCrossed;
}

void CPU::opt_SAX(uint32_t address) {
    this->memory.write(address, this->a & this->x);
}

void CPU::opt_LAX(uint32_t address) {
    this->a = this->x = this->memory.read(address);
    this->checkZSFlag(this->x);
    this->cycle += this->pageCrossed;
}

void CPU::opt_AXS(uint32_t address) {
    uint16_t tmp = (this->a & this->x) - this->memory.read(address);
    this->x = tmp;
    this->checkZSFlag(this->x);
    this->p.c = (tmp & 0x8000) == 0;
}

void CPU::opt_ARR(uint32_t address) {
    this->a &= this->memory.read(address);
    this->a = (this->a >> 1) | (this->p.c << 7);
    this->checkZSFlag(this->a);
    this->p.c = (this->a >> 6) & 0x01;
    this->p.v = (((this->a >> 5) ^ (this->a >> 6)) & 0x01);
}

void CPU::opt_ANC(uint32_t address) {
    this->a &= this->memory.read(address);
    this->checkZSFlag(this->a);
    this->p.c = this->p.s;
}

void CPU::opt_ASR(uint32_t address) {
    this->a &= this->memory.read(address);
    this->p.c = this->a & 0x01;
    this->a >>= 1;
    this->checkZSFlag(this->a);
}

void CPU::opt_RTI(uint32_t address) {
    this->p.status = this->pop();
    this->p.b = 0;
    this->p.r = 1;
    uint16_t pcl = this->pop();
    uint16_t pch = this->pop();
    this->pc = pcl | (pch << 8);
}

void CPU::opt_BRK(uint32_t address) {
    uint16_t pc = this->pc + 1;
    this->push(pc >> 8);
    this->push(uint8_t(pc));
    this->p.b = 1;
    this->push(this->p.status);
    this->p.i = 1;
    this->pc = this->memory.readw(VERCTOR_IRQBRK);
}

void CPU::opt_RTS(uint32_t address) {
    uint16_t pcl = this->pop();
    uint16_t pch = this->pop();
    this->pc = (pcl | pch << 8) + 1;
}

void CPU::opt_JSR(uint32_t address) {
    uint16_t pc = this->pc - 1;
    this->push(pc >> 8);
    this->push(uint8_t(pc));
    this->pc = address;
}

void CPU::opt_BVC(uint32_t address) {
    if (!this->p.v){
        this->cycle += ((address & 0xFF00) != (this->pc & 0xFF00)) + 1;
        //this->cycle += (((address ^ this->pc) & 0x0100) == 0x0100) + 1;
        this->pc = address;
    }
}

void CPU::opt_BVS(uint32_t address) {
    if (this->p.v) {
        this->cycle += ((address & 0xFF00) != (this->pc & 0xFF00)) + 1;
        //this->cycle += (((address ^ this->pc) & 0x0100) == 0x0100) + 1;
        this->pc = address;
    }
}

void CPU::opt_BPL(uint32_t address) {
    if (!this->p.s) {
        this->cycle += ((address & 0xFF00) != (this->pc & 0xFF00)) + 1;
        //this->cycle += (((address ^ this->pc) & 0x0100) == 0x0100) + 1;
        this->pc = address;
    }
}

void CPU::opt_BMI(uint32_t address) {
    if (this->p.s) {
        this->cycle += ((address & 0xFF00) != (this->pc & 0xFF00)) + 1;
        //this->cycle += (((address ^ this->pc) & 0x0100) == 0x0100) + 1;
        this->pc = address;
    }
}

void CPU::opt_BCC(uint32_t address) {
    if (!this->p.c) {
        this->cycle += ((address & 0xFF00) != (this->pc & 0xFF00)) + 1;
        //this->cycle += (((address ^ this->pc) & 0x0100) == 0x0100) + 1;
        this->pc = address;
    }
}

void CPU::opt_BCS(uint32_t address) {
    if (this->p.c) {
        this->cycle += ((address & 0xFF00) != (this->pc & 0xFF00)) + 1;
        //this->cycle += (((address ^ this->pc) & 0x0100) == 0x0100) + 1;
        this->pc = address;
    }
}

void CPU::opt_BNE(uint32_t address) {
    if (!this->p.z) {
        this->cycle += ((address & 0xFF00) != (this->pc & 0xFF00)) + 1;
        //this->cycle += (((address ^ this->pc) & 0x0100) == 0x0100) + 1;
        this->pc = address;
    }
}

void CPU::opt_BEQ(uint32_t address) {
    if (this->p.z) {
        this->cycle += ((address & 0xFF00) != (this->pc & 0xFF00)) + 1;
        //this->cycle += (((address ^ this->pc) & 0x0100) == 0x0100) + 1;
        this->pc = address;
    }
}

void CPU::opt_JMP(uint32_t address) {
    this->pc = address;
}

void CPU::opt_PLP(uint32_t address) {
    this->p.status = this->pop();
    this->p.b = 0;
    this->p.r = 1;
}

void CPU::opt_PHP(uint32_t address) {
    this->push(this->p.status | MASK_RF | MASK_BF);
}

void CPU::opt_PLA(uint32_t address) {
    this->a = this->pop();
    this->checkZSFlag(this->a);
}

void CPU::opt_PHA(uint32_t address) {
    this->push(this->a);
}

void CPU::opt_ROR(uint32_t address) {
    uint16_t res;
    if (address == ACCIMPID) {
        res = this->a;
    } else {
        res = this->memory.read(address);
    }
    res |= uint16_t(this->p.c) << 8;
    this->p.c = res & 0x01;
    res >>= 1;
    if (address == ACCIMPID) {
        this->a = res;
    } else {
        this->memory.write(address, res);
    }
    this->checkZSFlag(res);
}

void CPU::opt_ROL(uint32_t address) {
    uint16_t res;
    if (address == ACCIMPID) {
        res = this->a;
    } else {
        res = this->memory.read(address);
    }
    res <<= 1;
    res |= this->p.c;
    this->p.c = res & 0x100;
    if (address == ACCIMPID) {
        this->a = res;
    } else {
        this->memory.write(address, res);
    }
    this->checkZSFlag(res);
}

void CPU::opt_LSR(uint32_t address) {
    uint16_t data;
    if (address == ACCIMPID) {
        data = this->a;
    } else {
        data = this->memory.read(address);
    }
    this->p.c = data & 0x01;
    data >>= 1;
    if (address == ACCIMPID) {
        this->a = data;
    } else {
        this->memory.write(address, data);
    }
    this->checkZSFlag(data);
}

void CPU::opt_ASL(uint32_t address) {
    uint16_t data;
    if (address == ACCIMPID) {
        data = this->a;
    } else {
        data = this->memory.read(address);
    }
    this->p.c = data & 0x80;
    data <<= 1;
    if (address == ACCIMPID) {
        this->a = data;
    } else {
        this->memory.write(address, data);
    }
    this->checkZSFlag(data);
}

void CPU::opt_BIT(uint32_t address) {
    uint8_t value = this->memory.read(address);
    this->p.v = (value >> 6) & 0x01;
    this->p.s = (value >> 7) & 0x01;
    this->p.z = !(this->a & value);
}

void CPU::opt_CPY(uint32_t address) {
    uint16_t m = this->memory.read(address);
    this->p.c = this->y >= m;
    this->checkZSFlag(this->y - m);
}

void CPU::opt_CPX(uint32_t address) {
    uint16_t m = this->memory.read(address);
    this->p.c = this->x >= m;
    this->checkZSFlag(this->x - m);
}

void CPU::opt_CMP(uint32_t address) {
    uint16_t m = this->memory.read(address);
    this->p.c = this->a >= m;
    this->checkZSFlag(this->a - m);
    this->cycle += this->pageCrossed;
}

void CPU::opt_SEI(uint32_t address) {
    this->p.i = 1;
}

void CPU::opt_CLI(uint32_t address) {
    this->p.i = 0;
}

void CPU::opt_CLV(uint32_t address) {
    this->p.v = 0;
}

void CPU::opt_SED(uint32_t address) {
    this->p.d = 1;
}

void CPU::opt_CLD(uint32_t address) {
    this->p.d = 0;
}

void CPU::opt_SEC(uint32_t address) {
    this->p.c = 1;
}

void CPU::opt_CLC(uint32_t address) {
    this->p.c = 0;
}

void CPU::opt_TXS(uint32_t address) {
    this->sp = this->x;
}

void CPU::opt_TSX(uint32_t address) {
    this->x = this->sp;
    this->checkZSFlag(this->x);
}

void CPU::opt_TYA(uint32_t address) {
    this->a = this->y;
    this->checkZSFlag(this->a);
}

void CPU::opt_TAY(uint32_t address) {
    this->y = this->a;
    this->checkZSFlag(this->y);
}

void CPU::opt_TXA(uint32_t address) {
    this->a = this->x;
    this->checkZSFlag(this->a);
}

void CPU::opt_TAX(uint32_t address) {
    this->x = this->a;
    this->checkZSFlag(this->x);
}

void CPU::opt_DEY(uint32_t address) {
    this->y--;
    this->checkZSFlag(this->y);
}

void CPU::opt_INY(uint32_t address) {
    this->y++;
    this->checkZSFlag(this->y);
}

void CPU::opt_DEX(uint32_t address) {
    this->x--;
    this->checkZSFlag(this->x);
}

void CPU::opt_INX(uint32_t address) {
    this->x++;
    this->checkZSFlag(this->x);
}

void CPU::opt_EOR(uint32_t address) {
    this->a ^= this->memory.read(address);
    this->checkZSFlag(this->a);
    this->cycle += this->pageCrossed;
}

void CPU::opt_ORA(uint32_t address) {
    this->a |= this->memory.read(address);
    this->checkZSFlag(this->a);
    this->cycle += this->pageCrossed;
}

void CPU::opt_AND(uint32_t address) {
    this->a &= this->memory.read(address);
    this->checkZSFlag(this->a);
    this->cycle += this->pageCrossed;
}

void CPU::opt_DEC(uint32_t address) {
    uint8_t data = this->memory.read(address) - 1;
    this->memory.write(address, data);
    this->checkZSFlag(data);
}

void CPU::opt_INC(uint32_t address) {
    uint8_t data = this->memory.read(address) + 1;
    this->memory.write(address, data);
    this->checkZSFlag(data);
}

void CPU::opt_SBC(uint32_t address) {
    uint8_t src = this->memory.read(address);
    uint16_t res = this->a - src - (this->p.c == 0);
    this->p.c = res < 0x100;
    this->p.v = (((this->a ^ src) & 0x80) && ((this->a ^ uint8_t(res)) & 0x80));
    this->a = res;
    this->checkZSFlag(this->a);
    this->cycle += this->pageCrossed;
}

void CPU::opt_ADC(uint32_t address) {
    uint8_t src = this->memory.read(address);
    uint16_t res = this->a + src + this->p.c;
    this->p.c = res & 0x100;
    this->p.v = (!((this->a ^ src) & 0x80) && ((this->a ^ uint8_t(res)) & 0x80));
    this->a = res;
    this->checkZSFlag(this->a);
    this->cycle += this->pageCrossed;
}

void CPU::opt_STY(uint32_t address) {
    this->memory.write(address, this->y);
}

void CPU::opt_STX(uint32_t address) {
    this->memory.write(address, this->x);
}

void CPU::opt_STA(uint32_t address) {
    this->memory.write(address, this->a);
}

void CPU::opt_LDY(uint32_t address) {
    this->y = this->memory.read(address);
    this->checkZSFlag(this->y);
    this->cycle += this->pageCrossed;
}

void CPU::opt_LDX(uint32_t address) {
    this->x = this->memory.read(address);
    this->checkZSFlag(this->x);
    this->cycle += this->pageCrossed;
}

void CPU::opt_LDA(uint32_t address) {
    this->a = this->memory.read(address);
    this->checkZSFlag(this->a);
    this->cycle += this->pageCrossed;
}
