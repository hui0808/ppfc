#include "cartridge.h"
#include "utils.h"
#include <string>
#include <stdio.h>

Cartridge::Cartridge(const char* path) {
    this->path = path;
    this->file = fopen(path, "rb");
    assert(this->file != nullptr, "Cartridge: load nes file error!");
    this->loadHeader();
    if (this->flag1.trainer) { // 暂时跳过金手指
        fseek(this->file, 512, SEEK_CUR);
    }
    this->loadFlags();
    this->loadRom();
    assert(fgetc(this->file) == EOF, "Cartridge: failed to read end of file!");
    fclose(this->file);
}

Cartridge::~Cartridge() {
    delete[] this->prgrom;
    delete[] this->chrrom;
    if (this->saveram) delete[] this->saveram;
}

void Cartridge::loadHeader(void) {
    if (fread(&this->header, sizeof(CartridgeHeader), 1, this->file)) {
        const char* id = "NES\x1a";
        assert(this->header.id == *((uint32_t*)id), "Cartridge: error nes file header!");
    }
}

void Cartridge::loadFlags(void) {
    this->flag1.flag = this->header.flag1;
    this->flag2.flag = this->header.flag2;
    this->mapper = flag1.lmapper || (flag2.hmapper >> 4);
}

void Cartridge::loadRom(void) {
    this->prgrom = new uint8_t[PRGROMSIZE * this->header.prgromCount]();
    this->chrrom = new uint8_t[CHRROMSIZE * (this->header.chrromCount | 1)]();
    fread(this->prgrom, PRGROMSIZE * this->header.prgromCount, 1, this->file);
    fread(this->chrrom, CHRROMSIZE * this->header.chrromCount, 1, this->file);
    this->saveram = this->flag1.sram ? new uint8_t[8192]() : nullptr;
}