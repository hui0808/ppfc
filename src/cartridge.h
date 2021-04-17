#ifndef __PPFC_CARTRIDGE_H__
#define __PPFC_CARTRIDGE_H__

#include "common.h"

// 文件头结构，共16个字节
struct CartridgeHeader {
    uint32_t id;            // 标识的字符串，必须为"NES<EOF>"
    uint8_t  prgromCount;   // PRG-ROM 数量，每个16kb
    uint8_t  chrromCount;   // CHR-ROM 数量，每个8kb
    uint8_t  flag1;         // CartridgeFlag1结构描述的标志符
    uint8_t  flag2;         // CartridgeFlag2结构描述的标志符
    uint8_t  reserved[8];   // 暂时没有用到的信息，保留
};

union CartridgeFlag1 {
    uint8_t flag;
    struct {
        bool mirroring : 1;  // 镜像标志位
        bool sram : 1;       // SRAM标志位
        bool trainer : 1;    // Trainer标志位
        bool fourScreen : 1; // 4屏标志位
        uint8_t lmapper : 4; // Mapper编号低4位
    };
};

union CartridgeFlag2 {
    uint8_t flag;
    struct {
        uint8_t consoleType : 2;
        uint8_t version : 2;
        uint8_t hmapper : 4;    // Mapper编号高4位
    };
};

class Cartridge {
public:
    Cartridge(const char* path);
    ~Cartridge();
    void loadHeader(void);
    void loadFlags(void);
    void loadTrainer(void);
    void loadRom(void);

    const char* path;
    FILE* file;
    CartridgeHeader header;
    CartridgeFlag1 flag1;
    CartridgeFlag2 flag2;
    uint8_t mapper;
    uint8_t *prgrom;
    uint8_t *chrrom;
    uint8_t *saveram;
};

#endif // __PPFC_CARTRIDGE_H__
