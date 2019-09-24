#ifndef BITMAP_H
#define BITMAP_H

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

#endif
