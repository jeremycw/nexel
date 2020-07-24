#include <SDL2/SDL.h>
#include "image.h"
#include "bitmap.h"
#include "copypaste.h"
#include "ui.h"

static SDL_Renderer* ren;

void renderer_init(SDL_Window* win, char* filename, int width, int height) {
  ui_set_status(filename);
  ren = SDL_CreateRenderer(win, -1,SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (ren == NULL) {
    printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
    exit(1);
  }
  bitmap_set_renderer(ren);
  image_init(filename, win, width, height);
  ui_init(win, ren, MAJOR_BLOCK_SIZE);
  SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
}

void renderer_draw() {
  image_draw(ren);
  copy_paste_render(ren);
  image_info_t info;
  image_info(&info);
  ui_draw(info.x, info.y, info.width, info.height, info.scale, MAJOR_BLOCK_SIZE);
  SDL_RenderPresent(ren);
}

void renderer_destroy() {
  image_destroy();
  SDL_DestroyRenderer(ren);
}
