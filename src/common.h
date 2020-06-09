#ifndef __PPFU_COMMON_H__
#define __PPFU_COMMON_H__

#define READBIND(func) [this](uint32_t addr) { return func(addr); }
#define WRITEBIND(func) [this](uint32_t addr, uint8_t byte) { func(addr, byte); }
#define EVENTBIND(func) [this](SDL_Event& e) { func(e); }

#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <thread>
#include <utility>
#include <vector>
#include <string>
#include <functional>
#include "SDL.h"
#include "config.h"
#include "utils.h"

#endif // __PPFU_COMMON_H__
