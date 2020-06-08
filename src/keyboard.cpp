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
	//printf("[key down] %c\n", key);
}

void Keyboard::keyup(SDL_Event& event) {
	uint32_t key = event.key.keysym.sym;
	for (uint8_t i = 0; i < sizeof(this->map); i++) {
		if (key == this->map[i]) {
			this->status[i] = 0;
            break;
		}
	}
	//printf("[key   up] %c\n", key);
}

uint8_t Keyboard::port16Read(uint32_t addr) {
	return this->status[this->player1++ & this->mask];
}

void Keyboard::port16Write(uint32_t addr, uint8_t byte) {
	this->mask = byte & 0x01 ? 0 : 0x07;
	if (byte & 0x01) this->player1 = this->player2 = 0;
}

uint8_t Keyboard::port17Read(uint32_t addr) {
	return this->status[8 + (this->player2++ & this->mask)];
}