#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>

void renderer_draw();
void renderer_init(SDL_Window* win, char* filename, int width, int height);
void renderer_destroy();

#endif
