#include <unistd.h>
#include "bitmap.h"
#include "varray.h"
#include "image.h"
#include "stb_image_write.h"
#include "stb_image.h"

varray_decl(pixel_t)
varray_defn(pixel_t);

typedef struct {
  int scale;
  SDL_Rect translation;
} view_t;

typedef struct undo_s {
  struct undo_s* next;
  varray_t(pixel_t) pixels;
} undo_t;

int scale;
SDL_Rect translation;
static undo_t* undo_head = NULL;
static int color = 0xffffffff;
static bitmap_t image;
static char* filename;

void image_translate_coord(int x, int y, int* tx, int* ty) {
  *tx = (x - translation.x) / scale; 
  *ty = (y - translation.y) / scale;
}

void paint(int x, int y) {
  int tx, ty;
  image_translate_coord(x, y, &tx, &ty);
  int index = ty * image.width + tx;
  image_undoable_write(index, color);
  bitmap_rebuild(&image);
}

void pick_color(int x, int y) {
  int tx, ty;
  image_translate_coord(x, y, &tx, &ty);
  color = image.data[ty * image.width + tx];
}

void zoom_in(SDL_Window* win) {
  if (scale == 32) return;
  int x, y;
  SDL_GetWindowSize(win, &x, &y);
  translation.y -= (y/2 - translation.y);
  translation.x -= (x/2 - translation.x);
  translation.h *= 2;
  translation.w *= 2;
  scale *= 2;
}

void zoom_out(SDL_Window* win) {
  if (scale == 1) return;
  int x, y;
  SDL_GetWindowSize(win, &x, &y);
  translation.y += (y/2 - translation.y)/2;
  translation.x += (x/2 - translation.x)/2;
  translation.h /= 2;
  translation.w /= 2;
  scale /= 2;
}

void undo() {
  if (undo_head == NULL) return;
  for (int i = 0; i < undo_head->pixels.size; i++) {
    pixel_t pixel = undo_head->pixels.buf[i];
    image.data[pixel.index] = pixel.color;
  }
  undo_t* tmp = undo_head;
  free(tmp->pixels.buf);
  undo_head = tmp->next;
  free(tmp);
  bitmap_rebuild(&image);
}

void image_begin_undo_recording() {
  undo_t* undo = malloc(sizeof(undo_t));
  undo->next = undo_head;
  undo_head = undo;
  varray_init(pixel_t, &undo->pixels, 8);
}

void save() {
  stbi_write_png(filename, image.width, image.height, 4, (uint8_t*)image.data, image.pitch);
}

void image_set_paint_color(int c) {
  color = c;
}

void image_info(image_info_t* info) {
  info->width = image.width;
  info->height = image.height;
  info->scale = scale;
  info->x = translation.x;
  info->y = translation.y;
}

void image_init(char* path, SDL_Window* win, int width, int height) {
  filename = path;
  image.data = NULL;
  scale = 1;

  int req_format = STBI_rgb_alpha;
  int orig_format;
  uint32_t* data;
  if (access(path, F_OK) != -1) {
    // file exists
    data = (uint32_t*)stbi_load(path, &width, &height, &orig_format, req_format);
    if(data == NULL) {
      SDL_Log("Loading image failed: %s", stbi_failure_reason());
      exit(1);
    }
  } else {
    int pixels = width * height;
    size_t bytes = sizeof(uint32_t) * pixels;
    data = malloc(bytes);
    for (int i = 0; i < pixels; i++) {
      data[i] = 0xFF000000;
    }
  }

  bitmap_build_from_pixels(&image, data, width, height, &rgba32);

  int x, y;
  SDL_GetWindowSize(win, &x, &y);
  translation.x = x / 2 - width / 2;
  translation.y = y / 2 - height / 2;
  translation.h = height;
  translation.w = width;
}

void image_draw(SDL_Renderer* ren) {
  SDL_SetRenderDrawColor(ren, 0, 0x2b, 0x36, 255);
  SDL_RenderClear(ren);
  SDL_RenderCopy(ren, image.tex, NULL, &translation);
}

void image_destroy() {
  SDL_FreeSurface(image.surf);
  stbi_image_free(image.data);
  SDL_DestroyTexture(image.tex);
}

void image_refresh() { bitmap_rebuild(&image); }
uint32_t const * image_raw() { return image.data; }
int image_pitch() { return image.width; }

void image_undoable_write(int index, uint32_t color) {
  if (image.data[index] == color) return;

  pixel_t pixel = { .index = index, .color = image.data[index] };
  varray_push(pixel_t, &undo_head->pixels, pixel);
  image.data[index] = color;
}

int image_handle_events(SDL_Event* e, SDL_Window* win) {
  int x, y, state;
  switch (e->type) {
    case SDL_KEYDOWN:
      if (e->key.keysym.sym == SDLK_s) save();
      if (e->key.keysym.sym == SDLK_EQUALS) zoom_in(win);
      if (e->key.keysym.sym == SDLK_MINUS) zoom_out(win);
      if (e->key.keysym.sym == SDLK_u) undo();
      break;
    case SDL_MOUSEMOTION:
      state = SDL_GetMouseState(&x, &y);
      if (state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        // FIXME Sometimes a mouse motion event with mouse button down state
        // will arrive before the click event. This will prevent segfaults but
        // isn't the cleanest way to handle it.
        if (undo_head == NULL) image_begin_undo_recording();
        paint(x, y);
      }
      break;
    case SDL_MOUSEWHEEL:
      translation.x -= e->wheel.x*10;
      translation.y += e->wheel.y*10;
      break;
    case SDL_MOUSEBUTTONDOWN:
      if (e->button.button == SDL_BUTTON_LEFT) {
        image_begin_undo_recording();
        paint(e->button.x, e->button.y);
      } else if (e->button.button == SDL_BUTTON_RIGHT) {
        pick_color(e->button.x, e->button.y);
      }
      break;
  }
  return 0;
}

void image_snap_rect_to_block(SDL_Rect* rect) {
  int offsetx = translation.x % (scale * BLOCK_SIZE);
  int offsety = translation.y % (scale * BLOCK_SIZE);
  rect->x -= (rect->x - offsetx) % (scale * BLOCK_SIZE);
  rect->y -= (rect->y - offsety) % (scale * BLOCK_SIZE);
  rect->w -= rect->w % scale * BLOCK_SIZE;
  rect->h -= rect->h % scale * BLOCK_SIZE;
}

void image_snap_rect_to_pixel(SDL_Rect* rect) {
  int offsetx = translation.x % scale;
  int offsety = translation.y % scale;
  rect->x -= (rect->x - offsetx) % scale;
  rect->y -= (rect->y - offsety) % scale;
  rect->w -= rect->w % scale;
  rect->h -= rect->h % scale;
}

void image_descale_rect(SDL_Rect* rect) {
  rect->w /= scale;
  rect->h /= scale;
}

void image_scale_rect(SDL_Rect* rect) {
  rect->w *= scale;
  rect->h *= scale;
}
