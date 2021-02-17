#ifndef UI_H
#define UI_H

#include <SDL2/SDL.h>

void ui_init(SDL_Window* window, SDL_Renderer* renderer, int blksz);
void ui_draw(int imgx, int imgy, int imgw, int imgh, int imgscale, int blksz);
void ui_set_status(char* s);
int ui_handle_events(SDL_Event* e);

#endif
