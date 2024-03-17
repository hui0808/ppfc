#include "ppfc.h"

PPFC::PPFC(const char* path) :
        cartridge(path),
        cpu(*this),
        ppu(*this),
        mapper(*this),
        screen(*this, "PPFC", 256, 240),
        keyboard(*this) {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        error(format("Screen: could not initialize SDL2: %s", SDL_GetError()));
    }
    this->status = PPFC_STOP;
}

void PPFC::init(void) {
    this->mapper.init();
    this->cpu.init();
    this->ppu.init();
    this->screen.init();
    this->keyboard.init();

    registerFunc(SDL_QUIT, EVENTBIND(this->quit));
}

void PPFC::run(void) {
    this->init();
    this->beforeRun();
    this->status = PPFC_RUN;
    int counter = 0;
    while (this->status != PPFC_STOP) {
        switch (this->status) {
        case(PPFC_RUN):
            do {
                // each cpu run, ppu run 3 times
                if (counter == 0) this->cpu.run();
                this->ppu.run();
                counter = (counter + 1) % PPU_CPU_CLOCK_RATIO;
            } while (this->ppu.frameClock != NTSC_CYCLES * 241 + 1);
            break;
        case(PPFC_RESET):
            this->ppu.reset();
            this->cpu.reset();
            this->status = PPFC_RUN;
            break;
        }
        this->screen.updata(this->ppu.buf);
        this->screen.render();
        this->handleEvent();
    }
    this->quit();
}


void PPFC::beforeRun(void) {
    printf(
        "ROM: PRG-ROM: %d x 16kb   CHR-ROM %d x 8kb   Mapper: %03d\n",
        this->cartridge.header.prgromCount,
        this->cartridge.header.chrromCount,
        this->cartridge.mapper
    );
    uint16_t nmi = this->cpu.memory.readw(VERCTOR_NMI);
    uint16_t reset = this->cpu.memory.readw(VERCTOR_RESET);
    uint16_t irq = this->cpu.memory.readw(VERCTOR_IRQBRK);
    printf("ROM: NMI: $%04X  RESET: $%04X  IRQ/BRK: $%04X\n", nmi, reset, irq);
}

void PPFC::handleEvent(void) {
    static SDL_Event event;
    while (SDL_PollEvent(&event)) {
        for (auto i : this->events) {
            if (event.type == i.first) {
                i.second(event);
            }
        }
    }
}

void PPFC::registerFunc(uint16_t eventType, EventCallBack callback) {
    this->events.push_back(EventPair(eventType, callback));
}

void PPFC::quit(void) {
    printf("PPFC: quit\n");
    this->screen.quit();
    SDL_Quit();
}

void PPFC::quit(SDL_Event& event) {
    this->status = PPFC_STOP;
}