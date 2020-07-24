#ifndef BITMAP_H
#define BITMAP_H

#include <SDL2/SDL.h>

#define RMASK 0x000000ff
#define GMASK 0x0000ff00
#define BMASK 0x00ff0000
#define AMASK 0xff000000

typedef struct {
  int bpp;
  int rmask;
  int gmask;
  int bmask;
  int amask;
} img_format_t;

typedef struct {
  SDL_Surface* surf;
  SDL_Texture* tex;
  uint32_t* data;
  int width;
  int height;
  int pitch;
  img_format_t* format;
} bitmap_t;

void bitmap_set_renderer(SDL_Renderer* renderer);
void bitmap_safe_free(bitmap_t* bitmap);
void bitmap_build(bitmap_t* bitmap);
void bitmap_rebuild(bitmap_t* bitmap);
void bitmap_build_from_pixels(bitmap_t* bitmap, uint32_t* data, int w, int h, img_format_t* format);

extern img_format_t rgba32;

#endif
