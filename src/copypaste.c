#include <SDL2/SDL.h>
#include "bitmap.h"
#include "image.h"
#include "varray.h"
#include "raw.h"
#include "copypaste.h"

enum { CP_NONE, CP_PASTING, CP_COPYING };

typedef struct {
  SDL_Rect rect;
  SDL_Rect dest;
  SDL_Point start;
  SDL_Point end;
  int state;
  bitmap_t bitmap;
} copy_t;

static copy_t copy = {};

void end_paste() {
  bitmap_safe_free(&copy.bitmap);
  copy.state = CP_NONE;
}

void rotate_paste() {
  unsigned int* rotated = malloc(copy.rect.w * copy.rect.h * 4);
  raw_rotate_clockwise(copy.bitmap.data, copy.rect.w, copy.rect.h, rotated);
  free(copy.bitmap.data);
  copy.bitmap.data = rotated;
  int tmp = copy.rect.w;
  copy.rect.w = copy.rect.h;
  copy.rect.h = tmp;
  bitmap_rebuild(&copy.bitmap);
}

void flip_horizontal() {
  raw_mirror_horizontal(copy.bitmap.data, copy.rect.w, copy.rect.h);
  bitmap_rebuild(&copy.bitmap);
}

void flip_vertical() {
  raw_mirror_vertical(copy.bitmap.data, copy.rect.w, copy.rect.h);
  bitmap_rebuild(&copy.bitmap);
}

void start_copy(int x, int y) {
  copy.state = CP_COPYING;
  copy.start.x = x;
  copy.start.y = y;
  copy.end.x = x;
  copy.end.y = y;
}

void coords_to_rect(SDL_Point start, SDL_Point end, SDL_Rect* rect) {
  int dx = end.x - start.x;
  int dy = end.y - start.y;
  if (dx < 0) {
    rect->x = end.x;
    rect->w = dx * -1;
  } else {
    rect->x = start.x;
    rect->w = dx;
  }
  if (dy < 0) {
    rect->y = end.y;
    rect->h = dy * -1;
  } else {
    rect->y = start.y;
    rect->h = dy;
  }
}

void end_copy() {
  coords_to_rect(copy.start, copy.end, &copy.rect);
  image_snap_rect_to_pixel(&copy.rect);
  image_translate_coord(copy.rect.x, copy.rect.y, &copy.rect.x, &copy.rect.y);
  image_descale_rect(&copy.rect);
  unsigned int* new_data = malloc(4 * copy.rect.w * copy.rect.h);
  unsigned int const * data = image_raw();
  pixel_loop(copy.rect.x, copy.rect.y, image_pitch(), 0, 0, copy.rect.w, copy.rect.w, copy.rect.h) {
    new_data[di] = data[si];
  }
  bitmap_build_from_pixels(&copy.bitmap, new_data, copy.rect.w, copy.rect.h, &rgba32);
  copy.state = CP_NONE;
  if (copy.rect.h > 1 || copy.rect.w > 1) {
    copy.state = CP_PASTING;
  }
}

void paste(int x, int y) {
  image_begin_undo_recording();
  image_translate_coord(x, y, &x, &y);
  x -= x % BLOCK_SIZE;
  y -= y % BLOCK_SIZE;
  pixel_loop(0, 0, copy.rect.w, x, y, image_pitch(), copy.rect.w, copy.rect.h) {
    image_undoable_write(di, copy.bitmap.data[si]);
  }
  image_refresh();
}

int copy_paste_handle_events(SDL_Event* e) {
  int x, y, state;
  switch (e->type) {
    case SDL_KEYDOWN:
      if (copy.state == CP_PASTING) {
        if (e->key.keysym.sym == SDLK_r) rotate_paste();
        if (e->key.keysym.sym == SDLK_ESCAPE) end_paste();
        if (e->key.keysym.sym == SDLK_h) flip_horizontal();
        if (e->key.keysym.sym == SDLK_v) flip_vertical();
      }
    case SDL_MOUSEBUTTONDOWN:
      if (e->button.button == SDL_BUTTON_LEFT) {
        if (copy.state == CP_PASTING) {
          paste(e->button.x, e->button.y);
          return 1;
        }
      } else if (e->button.button == SDL_BUTTON_RIGHT) {
        start_copy(e->button.x, e->button.y);
      }
      break;
    case SDL_MOUSEMOTION:
      state = SDL_GetMouseState(&x, &y);
      if (state & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
        copy.end.x = x, copy.end.y = y;
      }
      break;
    case SDL_MOUSEBUTTONUP:
      if (copy.state == CP_COPYING) {
        end_copy();
      }
      break;
  }
  return 0;
}

void copy_paste_render(SDL_Renderer* ren) {
  if (copy.state == CP_COPYING) {
    SDL_SetRenderDrawColor(ren, 0, 0, 255, 100);
    coords_to_rect(copy.start, copy.end, &copy.dest);
    image_snap_rect_to_pixel(&copy.dest);
    SDL_RenderFillRect(ren, &copy.dest);
  }
  if (copy.state == CP_PASTING) {
    int x, y;
    SDL_GetMouseState(&x, &y);
    SDL_Rect dst = {
      .x = x,
      .y = y,
      .w = copy.rect.w,
      .h = copy.rect.h
    };
    image_scale_rect(&dst);
    image_snap_rect_to_block(&dst);
    SDL_RenderCopy(ren, copy.bitmap.tex, NULL, &dst);
  }
}
