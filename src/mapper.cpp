#include "mapper.h"
#include "ppfc.h"

#define MAPPER(n) case(n): this->mapper##n(); return

Mapper::Mapper(PPFC& bus) :bus(bus) {
    this->mapper = bus.cartridge.mapper;
}

void Mapper::init(void) {
    switch (this->mapper) {
        MAPPER(0);
    }
    error(format("Mapper: does not support Mapper%d", this->mapper));
}

void Mapper::mapper0(void) {
    uint8_t prgromCount = this->bus.cartridge.header.prgromCount;
    assert(0 < prgromCount && prgromCount <= 2,
        format("Mapper: mapper0 bad PRG-ROM count(%d)!", prgromCount)
    );
    this->bus.ppu.memory.map(0, 0x2000, this->bus.cartridge.chrrom, CHRROMSIZE * 1, true, false);
    this->bus.cpu.memory.map(0x8000, 0x10000, this->bus.cartridge.prgrom, PRGROMSIZE * prgromCount, true, false);
}