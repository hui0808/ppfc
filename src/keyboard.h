#ifndef __PPFC_KEYBOARD_H__
#define __PPFC_KEYBOARD_H__

#include "common.h"

class PPFC;

class Keyboard {
public:
    PPFC& bus;
    uint8_t player1;
    uint8_t player2;
    uint8_t mask;
    bool status[16] = { false };
    // A, B, Select, Start, Up, Down, Left, Right
    uint32_t map[16] = {
        'j', 'k', 'u', 'i', 'w', 's', 'a', 'd', // player1
        SDLK_KP_1, SDLK_KP_2, SDLK_KP_4, SDLK_KP_5, SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT, // player2
    };

    Keyboard(PPFC& bus);
    void init(void);

    void keydown(SDL_Event& event);
    void keyup(SDL_Event& event);
    uint8_t port16Read(uint32_t addr);
    void port16Write(uint32_t addr, uint8_t byte);
    uint8_t port17Read(uint32_t addr);
};

#endif // __PPFC_KEYBOARD_H__
