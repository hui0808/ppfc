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
#include <exception>
#include "config.h"
#include "utils.h"

#if defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
#define WIN
#include "SDL.h" // Windows
#elif defined(__linux__) || defined(__MACOSX) || defined(__MACOS_CLASSIC__) || defined(__APPLE__) || defined(__apple__)
#define UNIX
#include <SDL2/SDL.h> // Linux
#endif


#endif // __PPFU_COMMON_H__
