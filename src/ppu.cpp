#include "ppfc.h"
#include "ppu.h"

PPU::PPU(PPFC& bus) :bus(bus), memory(16384) {
    this->ctrl.ctrl = 0;
    this->mask.mask = 0;
    this->status.status = 0;
    this->oamaddr = 0;
    this->scroll = 0;
    this->vaddr = 0;
    this->frameClock = 0;
    this->scanline = 0;
    this->cycle = 0;
    this->buffer = new uint32_t[256 * 240]();
    this->buf = this->buffer;
    memcpy(this->palette, gpalette, sizeof(gpalette));
}

PPU::~PPU() {
    delete[] this->buf;
}

void PPU::reset(void) {
    this->ctrl.ctrl = 0;
    this->mask.mask = 0;
    this->scroll = 0;
    this->tmpvaddr = 0;
    this->tmpfinex = 0;
    this->vaddr = 0;
    this->finex = 0;
    this->toggle = 0;
    this->vramBuf = 0;
    this->odd = 0;
    this->cycle = 0;
    this->frameClock = NTSC_CYCLES * 242;
    this->scanline = 0;
    this->bgPatAddr = this->ctrl.bakTalAddr ? 0x1000 : 0;
    this->spPatAddr = this->ctrl.sprTalAddr ? 0x1000 : 0;
}

void PPU::init(void) {
    this->memoryInit();
    this->reset();
    this->last = SDL_GetPerformanceCounter();
}

void PPU::memoryInit() {
    if (this->bus.cartridge.flag1.fourScreen) { // 4屏
        this->memory.map(0x2000, 0x2800, this->ram, sizeof(this->ram));
        this->memory.map(0x2800, 0x3000, this->ramEx, sizeof(this->ramEx));
        this->memory.map(0x3000, 0x3800, this->ram, sizeof(this->ram));
        this->memory.map(0x3800, 0x3F00, this->ramEx, 0x0700);
    }
    else if (this->bus.cartridge.flag1.mirroring) {  // 横屏
        this->memory.map(0x2000, 0x3000, this->ram, sizeof(this->ram));
        this->memory.map(0x3000, 0x3F00, this->ram, 0x0F00);
    } else { // 竖版
        this->memory.map(0x2000, 0x2800, this->ram, 0x0400);
        this->memory.map(0x2800, 0x3000, this->ram + 0x0400, 0x0400);
        this->memory.map(0x3000, 0x3800, this->ram, 0x0400);
        this->memory.map(0x3800, 0x3C00, this->ram + 0x0400, 0x0400);
        this->memory.map(0x3C00, 0x3F00, this->ram + 0x0400, 0x0300);
    }
    this->memory.map(0x3F00, 0x4000, this->spindexes, sizeof(this->spindexes));
}

void PPU::run(void) {
    if (this->frameClock < NTSC_CYCLES * NTSC_POSTRENDER_LINE) { // visible scanlines 0~239
        if (this->cycle < 256) { // cycle 0~255 visible scanlines render
            this->render();
        } else if (this->cycle == 320) { // sprite evaluation
            if (this->mask.background || this->mask.sprite) {
                this->spriteEvaluate();
                this->vaddr &= ~0x041F;
                this->vaddr |= (this->tmpvaddr & 0x041F);
                this->finex = this->tmpfinex;
                this->yInc();
            }
        }
    } else if (this->frameClock == NTSC_CYCLES * NTSC_VBLANK + 0) { // set vblank flag 241
        this->preVblank = this->status.vblank;
        this->status.vblank = 1;
        this->buffer = this->buf; // reset to (0, 0)
    } else if (this->frameClock == NTSC_CYCLES * NTSC_PRERENDER_LINE + 0) { // pre-render line 261 cycle 1 clear vblank
        this->preVblank = this->status.vblank;
        this->status.vblank = 0;
        this->status.spr0Hit = 0;
        this->status.sprOver = 0;
        if (this->mask.background || this->mask.sprite) {
            this->vaddr = this->tmpvaddr;
            this->finex = this->tmpfinex;
        }
    } else if (this->frameClock == NTSC_CYCLES * NTSC_SCANLINE - 2) {
        if (this->odd && (this->mask.background || this->mask.sprite)) {
            this->frameClock = 0;
            this->cycle = 0;
            this->scanline = 0;
            this->odd = 0;
            return;
        }
    } else if (this->frameClock == NTSC_CYCLES * NTSC_SCANLINE - 1) {
        this->frameClock = 0;
        this->cycle = 0;
        this->scanline = 0;
        if (this->mask.background || this->mask.sprite) {
            this->odd = 1;
        }
        return;
    }
    this->frameRateLimit();
    this->frameClock++;
    this->cycle++;
    if (this->cycle == NTSC_CYCLES) {
        this->cycle = 0;
        this->scanline++;
    }
}

//void PPU::run(void) {
//    if (this->scanline < NTSC_POSTRENDER_LINE) { // visible scanlines 0~239
//        if (this->cycle < 256) { // cycle 0~255 visible scanlines render
//            this->render();
//        } else if (this->cycle == 320) { // sprite evaluation
//            if (this->mask.background || this->mask.sprite) {
//                this->spriteEvaluate();
//                this->vaddr &= ~0x041F;
//                this->vaddr |= (this->tmpvaddr & 0x041F);
//                this->finex = this->tmpfinex;
//                this->yInc();
//            }
//        }
//    } else if (this->scanline == NTSC_VBLANK) { // set vblank flag 241
//        if (this->cycle == 0) {
//            this->preVblank = this->status.vblank;
//            this->status.vblank = 1;
//            this->buffer = this->buf; // reset to (0, 0)
//        }
//    } else if (this->scanline == NTSC_PRERENDER_LINE) { // pre-render line 261
//        if (this->cycle == 0) { // cycle 1 clear vblank
//            this->preVblank = this->status.vblank;
//            this->status.vblank = 0;
//            this->status.spr0Hit = 0;
//            this->status.sprOver = 0;
//            if (this->mask.background || this->mask.sprite) {
//                this->vaddr = this->tmpvaddr;
//                this->finex = this->tmpfinex;
//            }
//        } else if (this->cycle == NTSC_CYCLES - 2) {
//            if (this->odd && (this->mask.background || this->mask.sprite)) {
//                this->frameClock = 0;
//                this->cycle = 0;
//                this->scanline = 0;
//                this->odd = 0;
//                return;
//            }
//        } else if (this->cycle == NTSC_CYCLES - 1) {
//            this->frameClock = 0;
//            this->cycle = 0;
//            this->scanline = 0;
//            if (this->mask.background || this->mask.sprite) {
//                this->odd = 1;
//            }
//            return;
//        }
//    }
//    this->frameRateLimit();
//    this->frameClock++;
//    this->cycle++;
//    if (this->cycle == NTSC_CYCLES) {
//        this->cycle = 0;
//        this->scanline++;
//    }
//}

void PPU::render(void) {
    // Visible scanlines render
    this->pixel = 0;
    if (this->mask.background || this->mask.sprite) {
        if (this->cycle == 0) {
            this->titeDatal = 0;
            this->titeDatah = 0;

            this->fetchTile();
            this->vaddrInc();
            this->titeDatal <<= 8;
            this->titeDatah <<= 8;

            this->fetchTile();
            this->vaddrInc();
            this->titeDatal <<= 8;
            this->titeDatah <<= 8;

            this->fetchTile();
            this->vaddrInc();
            this->titeDatal <<= this->finex;
            this->titeDatah <<= this->finex;
        }
        if (this->mask.background) { // render background
            this->renderBackground();
        }
        if (this->mask.sprite) { // render sprite
            this->renderSprite();
        }
        this->xInc(); // x increment
    } else if ((this->vaddr & 0x3F00) == 0x3F00) {
        this->pixel = this->vaddr & 0x1F;
    }
    uint32_t color = this->palette[this->spindexes[this->pixel]].rgba;
    *this->buffer++ = color;
}

void PPU::renderBackground(void) {
    if (this->mask.back8 || this->cycle >= 8) {
        this->pixel = (this->titeDatah << 8 >> 31 << 1) | (this->titeDatal << 8 >> 31);
        if (this->pixel)
            this->pixel |= (this->attrData & 0x0F);
    }
    this->titeDatal <<= 1;
    this->titeDatah <<= 1;
}

void PPU::renderSprite(void) {
    uint8_t p;
    uint8_t res = this->pixel;
    uint8_t *sprite = this->secOam + this->sprNum * 4;

    while (sprite > this->secOam) {
        sprite -= 4;
        if (sprite[3] == this->cycle && !(sprite[2] & (1 << 3))) {
            if (sprite[2] & (1 << 7)) { // Flip sprite vertically
                p = (sprite[0] & 0x01) | ((sprite[1] & 0x01) << 1);
                sprite[0] >>= 1;
                sprite[1] >>= 1;
            } else {
                p = (sprite[0] >> 7) | (sprite[1] >> 7 << 1);
                sprite[0] <<= 1;
                sprite[1] <<= 1;
            }
            if (!(this->mask.sprite8) && this->cycle < 8) {
                p = 0;
            }
            if (p != 0) {
                if (sprite[2] & (1 << 6) && (this->pixel & 0x03)) {
                    res = this->pixel;
                } else {
                    res = (p | ((sprite[2] >> 2) & 0x0C)) + 16;
                }

                // update sprite 0 hit flag
#if 0
                if (this->sprZero == sprite && (this->pixel & 0x03))
#else
                if (this->sprZero == sprite && this->cycle != 255 && (this->pixel & 0x03))
#endif
                {
                    this->status.spr0Hit = 1;
                }
            }
            sprite[2]++;
            sprite[3]++;
        }
    }
    this->pixel = res;
}

void PPU::spriteEvaluate(void) {
    uint8_t tile;
    uint8_t high = this->ctrl.sprSize ? 16 : 8;
    uint8_t y;
    uint16_t patAddr = 0;
    uint8_t onum = 0;
    uint8_t oy = 0;
    PPUOAM *oam = (PPUOAM*)this->oam;
    PPUOAM *end = (PPUOAM*)(this->oam + 256);
    uint8_t *secOam = this->secOam;

    if (this->oam[0] == 255)
        this->status.spr0Hit = 0;

    this->sprNum = 0;
    this->sprZero = nullptr;
    if (this->scanline >= oam->y && this->scanline < oam->y + high)
        this->sprZero = secOam;

    do {
        if (this->scanline >= oam->y && this->scanline < oam->y + high) {
            tile = oam->tileIndex;
            y = this->scanline - oam->y; // y
            if (oam->attr & (1 << 7))
                y = high - y - 1; // vflip
            switch (high) {
                case 8:
                    patAddr = this->spPatAddr;
                    break;
                case 16:
                    patAddr = (oam->tileIndex & 0x01) ? 0x1000 : 0;
                    if (y < 8) {
                        tile &= ~(1 << 0);
                    } else {
                        tile |= (1 << 0);
                        y -= 8;
                    }
                    break;
            }
            *secOam++ = this->memory.read(patAddr + tile * 16 + y);
            *secOam++ = this->memory.read(patAddr + tile * 16 + 8 + y);
            *secOam++ = ((oam->attr << 1) & 0xC0) | ((oam->attr << 4) & 0x30);
            *secOam++ = oam->x;
            this->sprNum++; // sprNum++
        }
        // for sprite overflow obscure
        if (this->scanline >= ((uint8_t*)oam)[oy] && this->scanline < ((uint8_t*)oam)[oy] + high) onum++;
        if (onum >= 9) { oy++; oy &= 3; }
        if (onum == 8) { onum++; }
        if (onum == 10) { onum++; this->status.sprOver = 1; }
        oam++;
    } while ((oam < end) && this->sprNum < 16);
}

void PPU::fetchTile(void) {
    uint8_t finey = this->vaddr >> 12;
    uint16_t ntalAddr = (this->vaddr & 0x0FFF) + 0x2000;
    uint16_t attrAddr = (this->vaddr & 0x0C00) + 0x23C0 + ((this->vaddr >> 4) & 0x38) + ((this->vaddr >> 2) & 0x07);

    uint8_t ntalData = this->memory.read(ntalAddr);
    uint8_t attrData = this->memory.read(attrAddr);

    this->titeDatal |= this->memory.read(this->bgPatAddr + ntalData * 16 + finey);
    this->titeDatah |= this->memory.read(this->bgPatAddr + ntalData * 16 + finey + 8);

    uint8_t ashift = ((this->vaddr & (1 << 6)) >> 4) | ((this->vaddr & (1 << 1)));
    this->attrData >>= 4;
    this->attrData |= ((attrData >> ashift) & 0x03) << 10;
}

void PPU::vaddrInc(void) {
    if ((this->vaddr & 0x1F) == 0x1F) {
        this->vaddr &= ~0x1F;
        this->vaddr ^= (1 << 10);
    } else {
        this->vaddr++;
    }
}

void PPU::xInc(void) {
    if (this->finex == 0x07) {
        this->finex = 0;
        this->fetchTile();
        this->vaddrInc();
    } else {
        this->finex++;
    }
}

void PPU::yInc(void) {
    if ((this->vaddr & 0x7000) == 0x7000) {
        this->vaddr &= ~0x7000;              // fine Y = 0
        switch (this->vaddr & 0x03E0) {
            case(29 << 5):
                this->vaddr &= ~0x03E0;      // coarse Y = 0
                this->vaddr ^= (0x01 << 11); // switch vertical nametable
                break;
            case(31 << 5):
                this->vaddr &= ~0x03E0;      // coarse Y = 0, nametable not switched
                break;
            default:
                this->vaddr += (0x01 << 5);  // increment coarse Y
        }
    } else {
        this->vaddr += 0x1000;               // increment fine Y
    }
}

void PPU::frameRateLimit(void) {
    static const float perFrameTime = 1000.0f / NTSC_FRAME_RATE;
    if (this->frameClock == 0) {
        float dtime = (SDL_GetPerformanceCounter() - this->last) / float(SDL_GetPerformanceFrequency()) * 1000.0f;
        int32_t d = int32_t(perFrameTime - dtime);
        if (d > 0) {
            SDL_Delay(d);
        }
        this->current = SDL_GetPerformanceCounter();
        this->fps = float(SDL_GetPerformanceFrequency()) / (this->current - this->last);
        this->last = this->current;
        printf("\rFPS: %.2f", this->fps);
    }
}

uint8_t PPU::regRead(uint32_t addr) {
    // TODO: ppu io open bus
    uint16_t vaddr = this->vaddr & 0x3FFF;

    switch (addr) {
        case(0): // $2000 PPU控制寄存器
            error("PPU: controller register is write-only!");
            return 0;
        case(1): // $2001 PPU掩码寄存器
            error("PPU: mask register is write-only!");
        case(2): // $2002 PPU状态寄存器
        {
            uint8_t data = this->status.status;
            this->status.vblank = 0;
            if (this->frameClock == NTSC_CYCLES * NTSC_VBLANK + 2
                || this->frameClock == NTSC_CYCLES * NTSC_PRERENDER_LINE + 2) {
                data &= ~(1 << 7);
                data |= uint8_t(this->preVblank) << 7;
            }
            this->toggle = 0;
            return data;
        }
        case(3): // $2003 OAM 地址端口 精灵RAM的8位指针
            error("PPU: oam address register is write-only!");
            return 0;
        case(4): // $2004 OAM 数据端口 精灵RAM数据
        {
            uint8_t data = this->oam[this->oamaddr];
            if ((this->oamaddr & 0x03) == 0x02)
                data &= ~(0x07 << 2);
            return data;
        }
            //return this->oam[this->oamaddr++];
        case(5): // $2005 PPU 滚动位置寄存器
            error("PPU: scrolling position register is write-only!");
        case(6): // $2006 PPU 地址寄存器
            error("PPU: address register is write-only!");
        case(7): // $2007 PPU 数据端口
        {
            uint8_t data = 0;
            if (vaddr < 0x3F00) {
                data = this->vramBuf;
                this->vramBuf = this->memory.read(vaddr);
            } else {
                //data = this->memory.read(vaddr);
                data = this->spindexes[vaddr & 0x1F] & 0x3F;
                this->vramBuf = this->memory.read(vaddr & 0x2FFF);
            }
            this->vaddr += this->ctrl.varmInc ? 32 : 1;
            return data;
        }
    }
    error(format("PPU: %0#6X out of register index!", addr));
}

void PPU::regWrite(uint32_t addr, uint8_t byte) {
    uint16_t vaddr = this->vaddr & 0x3FFF;
    switch (addr) {
        case(0): // $2000 PPU控制寄存器
            this->ctrl.ctrl = byte;
            this->bgPatAddr = this->ctrl.bakTalAddr ? 0x1000 : 0;
            this->spPatAddr = this->ctrl.sprTalAddr ? 0x1000 : 0;
            this->tmpvaddr &= ~(0x03 << 10); // TODO
            this->tmpvaddr |= (byte & 0x03) << 10;
            break;
        case(1): // $2001 PPU掩码寄存器
            if ((this->mask.mask & 0xE1) != (byte & 0xE1)) {
                this->colorEmphasis(byte & 0xE1);
            }
            this->mask.mask = byte; 
            break;
        case(2): // $2002 PPU状态寄存器
            error("PPU: status register is read-only!");
        case(3): // $2003 OAM 数据端口 精灵RAM的8位指针
            this->oamaddr = byte; break;
        case(4): // $2004 OAM 数据端口 精灵RAM数据
            this->oam[this->oamaddr++] = byte; 
            break;
        case(5): // $2005 PPU 滚动位置寄存器
            this->toggle = !this->toggle;
            if (this->toggle) { // x
                this->tmpvaddr &= ~0x1F;
                this->tmpvaddr |= (byte >> 3);
                this->tmpfinex = (byte & 0x07);
            } else { // y
                this->tmpvaddr &= ~(0x1F << 5); // bit5~bit10
                this->tmpvaddr |= (byte >> 3) << 5;
                this->tmpvaddr &= ~(0x07 << 12); // bit12~bit15
                this->tmpvaddr |= (byte & 0x07) << 12;
            }
            this->scroll = byte;
            break;
        case(6): // $2006 PPU 地址寄存器
            this->toggle = !this->toggle;
            if (this->toggle) {
                this->tmpvaddr = ((this->tmpvaddr & 0x00FF) | (byte << 8)) & 0x3FFF;
            } else {
                this->tmpvaddr = ((this->tmpvaddr & 0xFF00) | byte);
                this->vaddr = this->tmpvaddr;
            }
            break;
        case(7): // $2007 PPU 数据端口
            if (vaddr < 0x3F00) {
                this->memory.write(vaddr, byte);
            } else {
                vaddr &= 0x1F;
                byte &= 0x3F;
                if (vaddr & 0x03) {
                    this->spindexes[vaddr] = byte;
                } else {
                    this->spindexes[vaddr & ~0x10] = byte;
                    this->spindexes[vaddr | 0x10] = byte;
                }
            }
            this->vaddr += this->ctrl.varmInc ? 32 : 1;
            break;
        default:
            error(format("PPU: %0#6X out of register index!", addr));
    }
}

void PPU::dma(uint32_t addr, uint8_t byte) {
    uint16_t src = byte * 256;
    for (uint16_t i = 0; i < 256; i++) {
        this->oam[this->oamaddr++] = this->bus.cpu.memory.read(src + i);
    }
    this->bus.cpu.cycle += 512;
    //this->bus.cpu.cycle += this->bus.cpu.cycle & 0x01;
}

uint16_t PPU::getDmaAddr(uint8_t data) {
    uint16_t offset = uint16_t(data & 0x07) << 8;
    uint16_t blank = data >> 5; // 分成8个8K空间[0: 0x2000: 0x10000)
    switch (blank) {
        case(0): // cpu rom [0, 0x2000)
            return offset;
        case(1): // PPU寄存器 [0x2000, 0x4000)
            error("PPU: DMA cannot upload PPU registers!");
        case(2): // 扩展区 [0x4000, 0x6000)
            error("PPU: DMA TODO!");
        case(3): // 存档 SRAM区 [0x6000, 0x8000)
            return 0x6000 + offset;
        case(4): // 程序PRG-ROM区 [0x8000, 0x10000) 
        case(5):
        case(6):
        case(7):
            return (blank << 13) + offset;
    }
    error(format("PPU: get dma address error %0#6X!", data));
}

void PPU::colorEmphasis(uint8_t flag) {
    const Palette *src = gpalette;
    Palette *dst = this->palette;
    uint8_t factor = flag >> 5;
    bool gray = flag & 0x01;
    uint8_t r, g, b, a;
    for (uint8_t i = 0; i < 64; i++) {
        if (gray) {
            r = g = b = (src[i].r * 77 + src[i].g * 150 + src[i].b * 29) >> 8;
        } else {
            r = src[i].r;
            g = src[i].g;
            b = src[i].b;
        }
        a = src[i].a;

        r = (r * colorEmphasisFactor[factor][0]) >> 8;
        g = (g * colorEmphasisFactor[factor][1]) >> 8;
        b = (b * colorEmphasisFactor[factor][2]) >> 8;
        r = (r < 255) ? r : 255;
        g = (g < 255) ? g : 255;
        b = (b < 255) ? b : 255;
        dst[i].r = r;
        dst[i].g = g;
        dst[i].b = b;
        dst[i].a = a;
    }
}

const Palette gpalette[64] = 
{
    #define COLOR(r, g, b, a) {a, b, g, r},
    #include "palette.h"
};

/*
001   R: 123.9%   G: 091.5%   B: 074.3%
010   R: 079.4%   G: 108.6%   B: 088.2%
011   R: 101.9%   G: 098.0%   B: 065.3%
100   R: 090.5%   G: 102.6%   B: 127.7%
101   R: 102.3%   G: 090.8%   B: 097.9%
110   R: 074.1%   G: 098.7%   B: 100.1%
111   R: 075.0%   G: 075.0%   B: 075.0%
*/
const uint16_t colorEmphasisFactor[8][3] = 
{
    { 256, 256, 256 },
    { 317, 234, 190 },
    { 203, 278, 225 },
    { 261, 251, 167 },
    { 232, 263, 327 },
    { 262, 232, 251 },
    { 190, 253, 256 },
    { 192, 192, 192 },
};