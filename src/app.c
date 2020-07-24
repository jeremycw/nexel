#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "renderer.h"
#include "ui.h"
#include "image.h"
#include "copypaste.h"

static int quit = 0;

static SDL_Window* win;

void print_rect(char* name, SDL_Rect* rect) {
  printf("%s: [(%d, %d), (%d, %d)]\n", name, rect->x, rect->y, rect->w, rect->h);
}

int win_handle_events(SDL_Event* e) {
  if ((e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_q) || e->type == SDL_QUIT) {
    quit = 1;
    return 1;
  }
  return 0;
}

void handle_event(SDL_Event* e) {
  if (win_handle_events(e)) return;
  if (ui_handle_events(e)) return;
  if (copy_paste_handle_events(e)) return;
  if (image_handle_events(e, win)) return;
}

void run_app(char* path, int width, int height) {
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    printf("SDL_Init Error: %s\n", SDL_GetError());
    exit(1);
  }

  win = SDL_CreateWindow("Nexel", 0, 0, 640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (win == NULL) {
    printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
    exit(1);
  }

  renderer_init(win, path, width, height);

  SDL_Event e;

  while (!quit) {
    renderer_draw();

    SDL_WaitEvent(&e);
    handle_event(&e);
    while (SDL_PollEvent(&e)) {
      handle_event(&e);
    }
  }

  renderer_destroy();

  SDL_DestroyWindow(win);
  SDL_Quit();
}
