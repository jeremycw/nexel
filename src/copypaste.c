#include <SDL2/SDL.h>
#include "bitmap.h"
#include "copypaste.h"
#include "image.h"
#include "varray.h"

typedef struct {
  SDL_Rect rect;
  SDL_Rect dest;
  SDL_Point start;
  SDL_Point end;
  int copying;
  int pasting;
  bitmap_t bitmap;
} copy_t;

static copy_t copy;

static view_t* view;

void end_paste() {
  safe_free_bitmap(&copy.bitmap);
  copy.pasting = 0;
}

void rotate_paste() {
  unsigned int* rotated = malloc(copy.rect.w * copy.rect.h * 4);
  rotate_clockwise(copy.bitmap.data, copy.rect.w, copy.rect.h, rotated);
  free(copy.bitmap.data);
  copy.bitmap.data = rotated;
  int tmp = copy.rect.w;
  copy.rect.w = copy.rect.h;
  copy.rect.h = tmp;
  rebuild_bitmap(&copy.bitmap);
}

void flip_horizontal() {
  mirror_horizontal(copy.bitmap.data, copy.rect.w, copy.rect.h);
  rebuild_bitmap(&copy.bitmap);
}

void flip_vertical() {
  mirror_vertical(copy.bitmap.data, copy.rect.w, copy.rect.h);
  rebuild_bitmap(&copy.bitmap);
}

void copy_paste_init(view_t* v) {
  view = v;
  copy.bitmap.data = NULL;
  copy.copying = 0;
  copy.pasting = 0;
}

void snap_rect_to_block(SDL_Rect* rect) {
  int offsetx = view->translation.x % (view->scale * BLOCK_SIZE);
  int offsety = view->translation.y % (view->scale * BLOCK_SIZE);
  rect->x -= (rect->x - offsetx) % (view->scale * BLOCK_SIZE);
  rect->y -= (rect->y - offsety) % (view->scale * BLOCK_SIZE);
  rect->w -= rect->w % view->scale * BLOCK_SIZE;
  rect->h -= rect->h % view->scale * BLOCK_SIZE;
}

void start_copy(int x, int y) {
  copy.copying = 1;
  copy.start.x = x;
  copy.start.y = y;
  copy.end.x = x;
  copy.end.y = y;
}

void snap_rect_to_pixel(SDL_Rect* rect) {
  int offsetx = view->translation.x % view->scale;
  int offsety = view->translation.y % view->scale;
  rect->x -= (rect->x - offsetx) % view->scale;
  rect->y -= (rect->y - offsety) % view->scale;
  rect->w -= rect->w % view->scale;
  rect->h -= rect->h % view->scale;
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

void end_copy(bitmap_t* image) {
  coords_to_rect(copy.start, copy.end, &copy.rect);
  snap_rect_to_pixel(&copy.rect);
  translate_coord(copy.rect.x, copy.rect.y, &copy.rect.x, &copy.rect.y, view);
  copy.rect.w /= view->scale;
  copy.rect.h /= view->scale;
  unsigned int* new_data = malloc(4 * copy.rect.w * copy.rect.h);
  pixel_loop(copy.rect.x, copy.rect.y, image->width, 0, 0, copy.rect.w, copy.rect.w, copy.rect.h) {
    new_data[di] = image->data[si];
  }
  build_bitmap_from_pixels(&copy.bitmap, new_data, copy.rect.w, copy.rect.h, &rgba32);
  copy.copying = 0;
  if (copy.rect.h > 1 || copy.rect.w > 1) {
    copy.pasting = 1;
  }
}

void paste(int x, int y, undo_t* undo_head, bitmap_t* image) {
  translate_coord(x, y, &x, &y, view);
  x -= x % BLOCK_SIZE;
  y -= y % BLOCK_SIZE;
  pixel_loop(0, 0, copy.rect.w, x, y, image->width, copy.rect.w, copy.rect.h) {
    pixel_t pixel = { .index = di, .color = image->data[di] };
    image->data[di] = copy.bitmap.data[si];
    varray_push(pixel_t, &undo_head->pixels, pixel);
  }
  rebuild_bitmap(image);
}

int copy_paste_handle_events(SDL_Event* e, undo_t* undo_head, bitmap_t* image) {
  if (!(copy.pasting || copy.copying)) return 0;

  int x, y, state;
  switch (e->type) {
    case SDL_KEYDOWN:
      if (copy.pasting) {
        if (e->key.keysym.sym == SDLK_r) rotate_paste();
        if (e->key.keysym.sym == SDLK_ESCAPE) end_paste();
        if (e->key.keysym.sym == SDLK_h) flip_horizontal();
        if (e->key.keysym.sym == SDLK_v) flip_vertical();
      }
    case SDL_MOUSEBUTTONDOWN:
      if (e->button.button == SDL_BUTTON_LEFT) {
        if (copy.pasting) paste(e->button.x, e->button.y, undo_head, image);
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
      if (copy.copying) {
        end_copy(image);
      }
      break;
  }
  return copy.pasting;
}

void copy_paste_render(int x, int y, SDL_Renderer* ren) {
  if (copy.copying) {
    SDL_SetRenderDrawColor(ren, 0, 0, 255, 100);
    coords_to_rect(copy.start, copy.end, &copy.dest);
    snap_rect_to_pixel(&copy.dest);
    SDL_RenderFillRect(ren, &copy.dest);
  }
  if (copy.pasting) {
    SDL_GetMouseState(&x, &y);
    SDL_Rect dst = {
      .x = x,
      .y = y,
      .w = copy.rect.w * view->scale,
      .h = copy.rect.h * view->scale
    };
    snap_rect_to_block(&dst);
    SDL_RenderCopy(ren, copy.bitmap.tex, NULL, &dst);
  }
}
