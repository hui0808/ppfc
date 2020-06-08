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
	SDL_Surface *screen;
	SDL_Texture *texture;
	SDL_Event event;
	uint32_t frame;

	Screen(PPFC& bus, const char *title, uint16_t width, uint16_t height);
	void init(void);
	void updata(uint32_t *buffer);
	void clear(void);
	void render(void);
	void quit(void);
};

#endif // __PPFC_SCREEN_H__
