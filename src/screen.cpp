#include "screen.h"
#include "ppfc.h"

Screen::Screen(PPFC& bus, const char *title, uint16_t width, uint16_t height) :bus(bus) {
    this->title = title;
    this->width = width;
    this->height = height;
}

void Screen::init(void) {
    // 创建窗口 
    this->window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        width,
        height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );

    this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    this->texture = SDL_CreateTexture(
        this->renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );
    this->mutex = SDL_CreateMutex();
    this->bus.registerFunc(SDL_WINDOWEVENT, EVENTBIND(this->resize));
}

void Screen::updata(uint32_t *buffer) {
    uint32_t pitch;
    void *pixels = NULL;
    SDL_LockTexture(this->texture, NULL, (void**)&pixels, (int*)&pitch);
    memcpy(pixels, buffer, pitch * this->height);
    SDL_UnlockTexture(this->texture);
}

void Screen::clear(void) {
    //SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 255);
    SDL_RenderClear(this->renderer);
}

void Screen::render(void) {
    SDL_LockMutex(this->mutex);
    SDL_RenderCopy(this->renderer, this->texture, NULL, NULL);
    SDL_RenderPresent(this->renderer);
    SDL_UnlockMutex(this->mutex);
}

void Screen::quit(void) {
    SDL_DestroyMutex(this->mutex);
    SDL_DestroyTexture(this->texture);
    SDL_DestroyRenderer(this->renderer);
    SDL_DestroyWindow(this->window);
}

void Screen::resize(SDL_Event& event) {
    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
        SDL_LockMutex(this->mutex);
        SDL_SetWindowSize(this->window, event.window.data1, event.window.data2);
        SDL_DestroyTexture(this->texture);
        this->texture = SDL_CreateTexture(
            this->renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_STREAMING,
            this->width,
            this->height
        );
        SDL_SetRenderTarget(this->renderer, this->texture);
        SDL_UnlockMutex(this->mutex);
    }
}