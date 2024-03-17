#ifndef PPFC_PLUGIN_SAVE_LOAD_H
#define PPFC_PLUGIN_SAVE_LOAD_H

#include "common.h"
#include "memory.h"
#include "cpu.h"
#include "ppu.h"
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>

class PPFC;

struct CpuData {
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t sp;
    uint16_t pc;
    CPUStatus p;
    uint8_t ram[2048] = {0};
    bool pageCrossed;
    uint32_t cycle;
    uint32_t count;
    bool nmi;
    bool preNmi;
    bool irq;
};

struct PpuData {
    uint8_t ram[2048] = {0};
    uint8_t ramEx[2048] = {0};
    PPUCTRL ctrl; // Controller ($2000) > write
    PPUMASK mask; // Mask ($2001) > write
    PPUSTATUS status; // Status ($2002) < read
    uint8_t oamaddr; // OAM address ($2003) > write
    uint8_t scroll; // Scroll ($2005) >> write x2
    uint16_t vaddr; // Address ($2006) >> write x2
    uint8_t spindexes[32] = {0}; // 调色板索引
    uint8_t oam[256] = {0}; // 精灵数据, 有64个4 byte 精灵
    Palette palette[64] = {0}; // 调色板
/*
    no need to save

    uint32_t *buffer; // 显示缓存
    uint32_t *buf;

    float fps;
    uint64_t current;
    uint64_t last;
*/
    uint16_t finex;
    uint16_t tmpfinex;
    uint16_t tmpvaddr;
    uint32_t frameClock;
    uint16_t scanline;
    uint16_t cycle;
    uint16_t bgPatAddr;
    uint16_t spPatAddr;
    uint8_t vramBuf;
    bool preVblank;
    bool toggle;
    bool odd;
    uint32_t titeDatal;
    uint32_t titeDatah;
    uint32_t attrData;
    uint32_t pixel;
    uint8_t secOam[64] = {0};
    uint8_t sprNum;
    uint8_t *sprZero;
};

struct SavData {
    CpuData cpuData;
    PpuData ppuData;
};

class PluginSaveLoad {
public:
    PPFC& bus;

    const char* sav_path;

    PluginSaveLoad(PPFC& bus);
    void init(void);
    void save(void);
    void load(void);
};


#endif //PPFC_PLUGIN_SAVE_LOAD_H

