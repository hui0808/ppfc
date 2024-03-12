#ifndef __PPFC_PPU_H__
#define __PPFC_PPU_H__

#include "common.h"
#include "memory.h"

class PPFC;

union PPUCTRL {
    uint8_t ctrl;
    struct {
        uint8_t bNameAddr : 2; // Base nametable address (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
        bool varmInc : 1; // VRAM address increment per CPU read / write of PPUDATA (0: add 1, going across; 1: add 32, going down)
        bool sprTalAddr : 1; // Sprite pattern table address for 8x8 sprites (0: $0000; 1: $1000; ignored in 8x16 mode)
        bool bakTalAddr : 1; // Background pattern table address (0: $0000; 1: $1000)
        bool sprSize : 1; // Sprite size (0: 8x8 pixels; 1: 8x16 pixels)
        bool masOrSla : 1; // PPU master/slave select (0: read backdrop from EXT pins; 1: output color on EXT pins)
        bool nmiGen : 1; // Generate an NMI at the start of the vertical blanking interval(0: off; 1: on)
    };
};

union PPUMASK {
    uint8_t mask;
    struct {
        bool grey : 1; // Greyscale (0: normal color, 1: produce a greyscale display)
        bool back8 : 1; // 1: Show background in leftmost 8 pixels of screen, 0: Hide
        bool sprite8 : 1; // 1: Show sprites in leftmost 8 pixels of screen, 0: Hide
        bool background : 1; // Show background
        bool sprite : 1; // Show sprites
        bool emRed : 1; // Emphasize red
        bool emGreen : 1; // Emphasize green
        bool emBlue : 1; // Emphasize blue
    };
};

union PPUSTATUS {
    uint8_t status;
    struct {
        uint8_t reserved : 5;
        bool sprOver : 1; // Sprite overflow
        bool spr0Hit : 1; // Sprite 0 Hit
        bool vblank : 1; // Vertical blank has started (0: not in vblank; 1: in vblank)
    };
};

struct PPUOAM {
    uint8_t y : 8;
    uint8_t tileIndex : 8;
    uint8_t attr : 8;
    uint8_t x : 8;
};

union Palette {
    struct {
        uint8_t a : 8;
        uint8_t b : 8;
        uint8_t g : 8;
        uint8_t r : 8;
    };
    uint32_t rgba;
};

class PPU {
public:
    PPFC& bus;
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
    alignas(32) uint32_t *buffer; // 显示缓存
    alignas(32) uint32_t *buf;

    Memory memory;

    float fps;
    uint64_t current;
    uint64_t last;

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

    PPU(PPFC& bus);
    ~PPU();
    void reset(void);
    void init(void);
    void memoryInit(void);
    void run(void);
    void render(void);
    void renderBackground(void);
    void renderSprite(void);
    void spriteEvaluate(void);
    void fetchTile(void);
    void vaddrInc(void);
    void xInc(void);
    void yInc(void);
    void frameRateLimit(void);
    uint8_t regRead(uint32_t addr);
    void regWrite(uint32_t addr, uint8_t byte);
    void dma(uint32_t addr, uint8_t byte);
    uint16_t getDmaAddr(uint8_t data);
    void colorEmphasis(uint8_t flag);
};

extern const Palette gpalette[64];
extern const uint16_t colorEmphasisFactor[8][3];

#endif // __PPFC_PPU_H__

