#ifndef __PPFC_H__
#define __PPFC_H__

#include "common.h"
#include "cartridge.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "mapper.h"
#include "screen.h"
#include "keyboard.h"
#include "plugin_save_load.h"

using EventCallBack = std::function<void(SDL_Event&)>;
using EventPair = std::pair<uint16_t, EventCallBack>;
using EventList = std::vector<EventPair>;

enum PPFCStatus {
    PPFC_RUN,
    PPFC_RESET,
    PPFC_STOP,
};

class PPFC {
public:
    uint8_t status;
    Cartridge cartridge;
    CPU cpu;
    PPU ppu;
    APU apu;
    Mapper mapper;
    Screen screen;
    Keyboard keyboard;
    PluginSaveLoad pluginSaveLoad;

    EventList events;

    PPFC(const char* path);
    void init(void);
    void run(void);
    void beforeRun(void);
    void handleEvent(void);
    void registerFunc(uint16_t eventType, EventCallBack callback);
    void save(void);
    void load(void);
    void quit(void);
    void quit(SDL_Event& event);
};

#endif // __PPFC_H__
