#include "keyboard.h"
#include "ppfc.h"

Keyboard::Keyboard(PPFC& bus) :bus(bus) {
    this->player1 = 0;
    this->player2 = 0;
    this->mask = 0;
}
void Keyboard::init(void) {
    this->bus.registerFunc(SDL_KEYDOWN, EVENTBIND(this->keydown));
    this->bus.registerFunc(SDL_KEYUP, EVENTBIND(this->keyup));
}

void Keyboard::keydown(SDL_Event& event) {
    uint32_t key = event.key.keysym.sym;
    uint16_t mod = event.key.keysym.mod;
    if ((mod & KMOD_CTRL) && (key == '3')) { // reset --- ctrl + 3
        this->bus.status = PPFC_RESET;
    } else {
        for (uint8_t i = 0; i < sizeof(this->map); i++) {
            if (key == this->map[i]) {
                this->status[i] = 1;
                break;
            }
        }
    }
    // printf("[key down] %c\n", key);
}

void Keyboard::keyup(SDL_Event& event) {
    uint32_t key = event.key.keysym.sym;
    for (uint8_t i = 0; i < sizeof(this->map); i++) {
        if (key == this->map[i]) {
            this->status[i] = 0;
            break;
        }
    }
    // printf("[key   up] %c\n", key);
}

uint8_t Keyboard::port16Read(uint32_t addr) {
    // 0x40是 open bus 值 https://wiki.nesdev.com/w/index.php/Standard_controller
    return this->status[this->player1++ & this->mask] | 0x40;
}

void Keyboard::port16Write(uint32_t addr, uint8_t byte) {
    // $4016写入1，mask为0b0000，此时读取永远只能读取到第一个键 A 的值
    // $4016写入0，mask为0b0111，此时正常读取8个按键
    this->mask = byte & 0x01 ? 0 : 0x07;
    // 清空偏移
    this->player1 = this->player2 = 0;
}

uint8_t Keyboard::port17Read(uint32_t addr) {
    return this->status[8 + (this->player2++ & this->mask)] | 0x40;
}