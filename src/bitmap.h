#ifndef BITMAP_H
#define BITMAP_H

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
  unsigned int* data;
  int width;
  int height;
  int pitch;
  img_format_t* format;
} bitmap_t;

void set_bitmap_renderer(SDL_Renderer* renderer);
void safe_free_bitmap(bitmap_t* bitmap);
void build_bitmap(bitmap_t* bitmap);
void rebuild_bitmap(bitmap_t* bitmap);
void build_bitmap_from_pixels(bitmap_t* bitmap, unsigned int* data, int w, int h, img_format_t* format);

extern img_format_t rgba32;

#endif
