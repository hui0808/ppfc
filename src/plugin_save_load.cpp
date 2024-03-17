#include "ppfc.h"
#include "plugin_save_load.h"

PluginSaveLoad::PluginSaveLoad(PPFC& bus) :bus(bus){
    std::string savDir = "./sav";
    if (!opendir(savDir.c_str())) {
        mkdir(savDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    this->sav_path = "./sav/fc.sav";
}

void PluginSaveLoad::init(void) {
    printf("save load init");
}

void PluginSaveLoad::save(void) {

    CpuData cpuData;
    memcpy(cpuData.ram, this->bus.cpu.ram, sizeof(this->bus.cpu.ram));
    cpuData.a = this->bus.cpu.a;
    cpuData.x = this->bus.cpu.x;
    cpuData.y = this->bus.cpu.y;
    cpuData.sp = this->bus.cpu.sp;
    cpuData.pc = this->bus.cpu.pc;
    cpuData.p = this->bus.cpu.p;

    cpuData.pageCrossed = this->bus.cpu.pageCrossed;
    cpuData.cycle = this->bus.cpu.cycle;
    cpuData.count = this->bus.cpu.count;
    cpuData.nmi = this->bus.cpu.nmi;
    cpuData.preNmi = this->bus.cpu.preNmi;
    cpuData.irq = this->bus.cpu.irq;

    PpuData ppuData;
    memcpy(ppuData.ram, this->bus.ppu.ram, sizeof(this->bus.ppu.ram)),
    memcpy(ppuData.ramEx, this->bus.ppu.ramEx, sizeof(this->bus.ppu.ramEx));
    ppuData.ctrl = this->bus.ppu.ctrl;
    ppuData.mask = this->bus.ppu.mask;
    ppuData.status = this->bus.ppu.status;
    ppuData.oamaddr = this->bus.ppu.oamaddr;
    ppuData.scroll = this->bus.ppu.scroll;
    ppuData.vaddr = this->bus.ppu.vaddr;
    memcpy(ppuData.spindexes, this->bus.ppu.spindexes, sizeof(this->bus.ppu.spindexes)),
    memcpy(ppuData.oam, this->bus.ppu.oam, sizeof(this->bus.ppu.oam));
    memcpy(ppuData.palette, this->bus.ppu.palette, sizeof(this->bus.ppu.palette));
    ppuData.buffer = this->bus.ppu.buffer;
    ppuData.buf = this->bus.ppu.buf;
    ppuData.fps = this->bus.ppu.fps;
    ppuData.current = this->bus.ppu.current;
    ppuData.last = this->bus.ppu.last;
    ppuData.finex = this->bus.ppu.finex;
    ppuData.tmpfinex = this->bus.ppu.tmpfinex;
    ppuData.tmpvaddr = this->bus.ppu.tmpvaddr;
    ppuData.frameClock = this->bus.ppu.frameClock;
    ppuData.scanline = this->bus.ppu.scanline;
    ppuData.cycle = this->bus.ppu.cycle;
    ppuData.bgPatAddr = this->bus.ppu.bgPatAddr;
    ppuData.spPatAddr = this->bus.ppu.spPatAddr;
    ppuData.vramBuf = this->bus.ppu.vramBuf;
    ppuData.preVblank = this->bus.ppu.preVblank;
    ppuData.toggle = this->bus.ppu.toggle;
    ppuData.odd = this->bus.ppu.odd;
    ppuData.titeDatal = this->bus.ppu.titeDatal;
    ppuData.titeDatah = this->bus.ppu.titeDatah;
    ppuData.attrData = this->bus.ppu.attrData;
    ppuData.pixel = this->bus.ppu.pixel;
    memcpy(ppuData.secOam, this->bus.ppu.secOam, sizeof(this->bus.ppu.secOam));
    ppuData.sprZero = this->bus.ppu.sprZero;
    ppuData.sprNum = this->bus.ppu.sprNum;

    SavData savData;
    savData.cpuData = cpuData;
    savData.ppuData = ppuData;
    printf("save pc %d \n", cpuData.pc);

    FILE* file = fopen(this->sav_path, "wb");  // 以二进制写入模式创建文件
    if (file == NULL) {
        error("无法创建文件\n");
    }
    fseek(file, 0, SEEK_SET);  // 将文件指针移动到文件开头
    fwrite(&savData, sizeof(savData), 1, file);  // 写入 cpuData
    fclose(file);   // 关闭文件
}

void PluginSaveLoad::load(void) {
    FILE* file = fopen(this->sav_path, "rb");  // 以二进制读取模式打开文件
    if (file == NULL) {
        error("无法打开文件\n");
    }
    fseek(file, 0, SEEK_SET);  // 将文件指针移动到文件开头
    SavData savData;
    fread(&savData, sizeof(SavData), 1, file);  // 从文件中读取数据到 savData
    fclose(file);  // 关闭文件

    // cpu
    memcpy(this->bus.cpu.ram, savData.cpuData.ram, sizeof(this->bus.cpu.ram));
    this->bus.cpu.a = savData.cpuData.a;
    this->bus.cpu.x = savData.cpuData.x;
    this->bus.cpu.y = savData.cpuData.y;
    this->bus.cpu.sp = savData.cpuData.sp;
    this->bus.cpu.pc = savData.cpuData.pc;
    this->bus.cpu.p = savData.cpuData.p;

    this->bus.cpu.pageCrossed = savData.cpuData.pageCrossed;
    this->bus.cpu.cycle = savData.cpuData.cycle;
    this->bus.cpu.count = savData.cpuData.count;
    this->bus.cpu.nmi = savData.cpuData.nmi;
    this->bus.cpu.preNmi = savData.cpuData.preNmi;
    this->bus.cpu.irq = savData.cpuData.irq;

    // ppu
    memcpy(this->bus.ppu.ram, savData.ppuData.ram, sizeof(this->bus.ppu.ram));
    memcpy(this->bus.ppu.ramEx, savData.ppuData.ramEx, sizeof(this->bus.ppu.ramEx));
    this->bus.ppu.ctrl = savData.ppuData.ctrl;
    this->bus.ppu.mask = savData.ppuData.mask;
    this->bus.ppu.status = savData.ppuData.status;
    this->bus.ppu.oamaddr = savData.ppuData.oamaddr;
    this->bus.ppu.scroll = savData.ppuData.scroll;
    this->bus.ppu.vaddr = savData.ppuData.vaddr;
    memcpy(this->bus.ppu.spindexes, savData.ppuData.spindexes, sizeof(this->bus.ppu.spindexes));
    memcpy(this->bus.ppu.oam, savData.ppuData.oam, sizeof(this->bus.ppu.oam));
    memcpy(this->bus.ppu.palette, savData.ppuData.palette, sizeof(this->bus.ppu.palette));
    this->bus.ppu.buffer = savData.ppuData.buffer;
    this->bus.ppu.buf = savData.ppuData.buf;
    this->bus.ppu.fps = savData.ppuData.fps;
    this->bus.ppu.current = savData.ppuData.current;
    this->bus.ppu.last = savData.ppuData.last;
    this->bus.ppu.finex = savData.ppuData.finex;
    this->bus.ppu.tmpfinex = savData.ppuData.tmpfinex;
    this->bus.ppu.tmpvaddr = savData.ppuData.tmpvaddr;
    this->bus.ppu.frameClock = savData.ppuData.frameClock;
    this->bus.ppu.scanline = savData.ppuData.scanline;
    this->bus.ppu.cycle = savData.ppuData.cycle;
    this->bus.ppu.bgPatAddr = savData.ppuData.bgPatAddr;
    this->bus.ppu.spPatAddr = savData.ppuData.spPatAddr;
    this->bus.ppu.vramBuf = savData.ppuData.vramBuf;
    this->bus.ppu.preVblank = savData.ppuData.preVblank;
    this->bus.ppu.toggle = savData.ppuData.toggle;
    this->bus.ppu.odd = savData.ppuData.odd;
    this->bus.ppu.titeDatal = savData.ppuData.titeDatal;
    this->bus.ppu.titeDatah = savData.ppuData.titeDatah;
    this->bus.ppu.attrData = savData.ppuData.attrData;
    this->bus.ppu.pixel = savData.ppuData.pixel;
    memcpy( this->bus.ppu.secOam, savData.ppuData.secOam, sizeof(this->bus.ppu.secOam));
    this->bus.ppu.sprZero = savData.ppuData.sprZero;
    this->bus.ppu.sprNum = savData.ppuData.sprNum;

    printf("load pc %d \n", this->bus.cpu.pc);
}

