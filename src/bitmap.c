#include <SDL2/SDL.h>
#include "bitmap.h"

img_format_t rgba32 = {
  .bpp = 32,
  .rmask = RMASK,
  .gmask = GMASK,
  .bmask = BMASK,
  .amask = AMASK,
};

SDL_Renderer* ren = NULL;

void set_bitmap_renderer(SDL_Renderer* renderer) {
  ren = renderer;
}

void safe_free_bitmap(bitmap_t* bitmap) {
  if (bitmap->data) {
    free(bitmap->data);
    bitmap->data = NULL;
    SDL_FreeSurface(bitmap->surf);
    SDL_DestroyTexture(bitmap->tex);
  }
}

void build_bitmap(bitmap_t* bitmap) {
  bitmap->surf = SDL_CreateRGBSurfaceFrom(
    (void*)bitmap->data, bitmap->width, bitmap->height,
    bitmap->format->bpp, bitmap->pitch, bitmap->format->rmask,
    bitmap->format->gmask, bitmap->format->bmask, bitmap->format->amask
  );
  bitmap->tex = SDL_CreateTextureFromSurface(ren, bitmap->surf);
}

void rebuild_bitmap(bitmap_t* bitmap) {
  SDL_FreeSurface(bitmap->surf);
  SDL_DestroyTexture(bitmap->tex);
  build_bitmap(bitmap);
}

void build_bitmap_from_pixels(bitmap_t* bitmap, unsigned int* data, int w, int h, img_format_t* format) {
  safe_free_bitmap(bitmap);
  bitmap->width = w;
  bitmap->height = h;
  bitmap->pitch = w * (format->bpp / 8);
  bitmap->data = data;
  bitmap->format = format;
  build_bitmap(bitmap);
}

