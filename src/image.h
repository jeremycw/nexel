#ifndef IMAGE_H
#define IMAGE_H

#include <SDL2/SDL.h>
#include "varray.h"

#define pixel_loop(src_x, src_y, src_pitch, dst_x, dst_y, dst_pitch, w, h) \
  for ( \
    int y_ = 0, \
        x_ = 0, \
        i_ = 0, \
        si = src_y * src_pitch + src_x, \
        di = dst_y * dst_pitch + dst_x; \
\
    i_ < w * h; \
\
    ++i_, \
    x_ = i_ % w, \
    y_ = i_ / w, \
    si = (y_ + src_y) * src_pitch + (x_ + src_x), \
    di = (y_ + dst_y) * dst_pitch + (x_ + dst_x) \
  )

#define BLOCK_SIZE 8
#define MAJOR_BLOCK_SIZE 16

typedef struct {
  int index;
  int color;
} pixel_t;

typedef struct {
  int scale;
  SDL_Rect translation;
} view_t;

varray_decl(pixel_t)

typedef struct undo_s {
  struct undo_s* next;
  varray_t(pixel_t) pixels;
} undo_t;

void alpha_channel_to_rgba(unsigned char* in, unsigned int* out, int n, unsigned int rgb);
void rotate_clockwise(unsigned int* in, int w, int h, unsigned int* out);
void mirror_horizontal(unsigned int* in, int w, int h);
void mirror_vertical(unsigned int* in, int w, int h);
void translate_coord(int x, int y, int* tx, int* ty, view_t* view);

#endif
