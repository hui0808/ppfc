#ifndef __PPFC_SCREEN_H__
#define __PPFC_SCREEN_H__

#include "common.h"

class PPFC;

class Screen {
public:
    PPFC& bus;
    uint16_t width;
    uint16_t height;
    const char* title;

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Event event;
    SDL_mutex *mutex;

    Screen(PPFC& bus, const char *title, uint16_t width, uint16_t height);
    void init(void);
    void updata(uint32_t *buffer);
    void clear(void);
    void render(void);
    void quit(void);
    void resize(SDL_Event& event);
};

#endif // __PPFC_SCREEN_H__
