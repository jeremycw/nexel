#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "data.h"
#include "sdl_renderer.h"
#include "editor.h"
#include "util.h"

static int quit = 0;

void print_rect(char* name, SDL_Rect* rect) {
  printf("%s: [(%d, %d), (%d, %d)]\n", name, rect->x, rect->y, rect->w, rect->h);
}

void run_app() {
}
