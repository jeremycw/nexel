#ifndef UTIL_H
#define UTIL_H

#include "data.h"

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

void rotate_clockwise_transform(indexed_pixel_t* in, int w, int h, int* wout, int* hout, indexed_pixel_t* out);
void flip_horizontal_transform(indexed_pixel_t* in, int w, int h, int* wout, int* hout, indexed_pixel_t* out);
void flip_vertical_transform(indexed_pixel_t* in, int w, int h, int* wout, int* hout, indexed_pixel_t* out);

struct transform zoom_out(struct dimensions window, struct transform transform);
struct transform zoom_in(struct dimensions window, struct transform transform);

struct rect snap_rect_to_pixel(struct rect rect, struct transform image_transform);
struct rect selection_to_rect(struct selection selection);
struct point screen_point_to_bitmap_point(struct point point, struct transform image_transform);
struct rect screen_selection_to_bitmap_rect(struct selection selection, struct transform image_transform);
void indexed_bitmap_to_rgba(indexed_pixel_t const* indexed_pixels, int n, colour_t const* palette_colours, colour_t* out);
void generate_sdl_bitmaps_for_indexed_bitmaps(
  indexed_pixel_t const* pixels,
  int pixels_n,
  SDL_Renderer* sdl_renderer,
  colour_t const* palette_colours,
  struct indexed_bitmap const* indexed_bitmaps,
  int indexed_bitmaps_n,
  struct sdl_bitmap* copy_bitmaps
);
colour_t* create_grid_tile_bitmap(int size);
struct sdl_bitmap create_grid_sdl_bitmap(SDL_Renderer* renderer, int tile_size, int scale);
struct sdl_bitmap sdl_bitmap_from_raw_buffer(SDL_Renderer* renderer, void* buffer, struct dimensions dimensions);

#endif
