#include <SDL2/SDL.h>
#include "bitmap.h"

img_format_t rgba32 = {
  .bpp = 32,
  .rmask = RMASK,
  .gmask = GMASK,
  .bmask = BMASK,
  .amask = AMASK,
};

static SDL_Renderer* ren = NULL;

void bitmap_set_renderer(SDL_Renderer* renderer) {
  ren = renderer;
}

void bitmap_safe_free(bitmap_t* bitmap) {
  if (bitmap->data) {
    free(bitmap->data);
    bitmap->data = NULL;
    SDL_FreeSurface(bitmap->surf);
    SDL_DestroyTexture(bitmap->tex);
  }
}

void bitmap_build(bitmap_t* bitmap) {
  bitmap->surf = SDL_CreateRGBSurfaceFrom(
    (void*)bitmap->data, bitmap->width, bitmap->height,
    bitmap->format->bpp, bitmap->pitch, bitmap->format->rmask,
    bitmap->format->gmask, bitmap->format->bmask, bitmap->format->amask
  );
  bitmap->tex = SDL_CreateTextureFromSurface(ren, bitmap->surf);
}

void bitmap_rebuild(bitmap_t* bitmap) {
  SDL_FreeSurface(bitmap->surf);
  SDL_DestroyTexture(bitmap->tex);
  bitmap_build(bitmap);
}

void bitmap_build_from_pixels(bitmap_t* bitmap, unsigned int* data, int w, int h, img_format_t* format) {
  bitmap_safe_free(bitmap);
  bitmap->width = w;
  bitmap->height = h;
  bitmap->pitch = w * (format->bpp / 8);
  bitmap->data = data;
  bitmap->format = format;
  bitmap_build(bitmap);
}

