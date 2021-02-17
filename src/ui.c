#include <SDL2/SDL.h>
#include <stdint.h>
#include "bitmap.h"
#include "roboto.h"
#include "raw.h"
#include "ui.h"
#include "image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

typedef struct {
  stbtt_bakedchar cdata[96];
  unsigned char alpha[128*96];
  bitmap_t bitmap;
} font_t;

typedef struct {
  bitmap_t bitmap;
  SDL_Rect dest;
  int on;
} grid_t;

typedef struct {
  bitmap_t bitmap;
  SDL_Rect dest;
} ui_widget_t;

uint8_t pdata[] = {
  124,124,124,255,
  0,0,252,255,
  0,0,188,255,
  68,40,188,255,
  148,0,132,255,
  168,0,32,255,
  168,16,0,255,
  136,20,0,255,
  80,48,0,255,
  0,120,0,255,
  0,104,0,255,
  0,88,0,255,
  0,64,88,255,
  0,0,0,255,
  0,0,0,255,
  0,0,0,255,
  188,188,188,255,
  0,120,248,255,
  0,88,248,255,
  104,68,252,255,
  216,0,204,255,
  228,0,88,255,
  248,56,0,255,
  228,92,16,255,
  172,124,0,255,
  0,184,0,255,
  0,168,0,255,
  0,168,68,255,
  0,136,136,255,
  0,0,0,255,
  0,0,0,255,
  0,0,0,255,
  248,248,248,255,
  60,188,252,255,
  104,136,252,255,
  152,120,248,255,
  248,120,248,255,
  248,88,152,255,
  248,120,88,255,
  252,160,68,255,
  248,184,0,255,
  184,248,24,255,
  88,216,84,255,
  88,248,152,255,
  0,232,216,255,
  120,120,120,255,
  0,0,0,255,
  0,0,0,255,
  252,252,252,255,
  164,228,252,255,
  184,184,248,255,
  216,184,248,255,
  248,184,248,255,
  248,164,192,255,
  240,208,176,255,
  252,224,168,255,
  248,216,120,255,
  216,248,120,255,
  184,248,184,255,
  184,248,216,255,
  0,252,252,255,
  216,216,216,255,
  0,0,0,255,
  0,0,0,255,
};

#define BLACK 0x1d

static grid_t grid;
static ui_widget_t picker;

static font_t font;
static char* status;

static SDL_Rect clip;

static SDL_Renderer* ren;
static SDL_Window* win;

#define COLOUR_PICKER_W 4
#define COLOUR_PICKER_H 16
#define COLOUR_SWATCH_SIZE_PX 16

#define PALETTE_SIZE 3
#define PALETTE_COUNT 4

void translate_picker_coord(int x, int y, int* tx, int* ty) {
  *tx = (x - picker.dest.x) / COLOUR_SWATCH_SIZE_PX; 
  *ty = (y - picker.dest.y) / COLOUR_SWATCH_SIZE_PX;
}

uint32_t* create_grid_tile(int size) {
  uint32_t* data = calloc(size * size, sizeof(uint32_t));
  for (int i = 0, j = 0; i < size; i++, j += size) {
    data[i] = 0xffffffff;
    if (i % 2 == 0) data[i + size/2 * size] = 0xffdddddd;
    data[j] = 0xffffffff;
    if (i % 2 == 0) data[j + size/2] = 0xffdddddd;
  }
  return data;
}

void build_grid(int scale, int blksz) {
  uint32_t* data = create_grid_tile(scale * blksz);
  int size = blksz * scale;
  bitmap_build_from_pixels(&grid.bitmap, data, size, size, &rgba32);
  grid.dest.h = blksz * scale;
  grid.dest.w = blksz * scale;
}

void draw_grid(int x, int y, int w, int h, int scale, int blksize) {
  if (!grid.on) return;
  build_grid(scale, blksize);
  grid.dest.x = x;
  grid.dest.y = y;
  for (int i = 0; i < h / blksize; i++) {
    for (int j = 0; j < w / blksize; j++) {
      SDL_RenderCopy(ren, grid.bitmap.tex, NULL, &grid.dest);
      grid.dest.x += scale * blksize;
    }
    grid.dest.x = x;
    grid.dest.y += scale * blksize;
  }
}

void stbtt_GetBakedRect(
  const stbtt_bakedchar *chardata,
  int char_index,
  int *xpos,
  int *ypos,
  SDL_Rect* src,
  SDL_Rect* dst
) {
  const stbtt_bakedchar *b = chardata + char_index;
  int round_x = *xpos + b->xoff;
  int round_y = *ypos + b->yoff;

  dst->x = round_x;
  dst->y = round_y;
  dst->w = b->x1 - b->x0;
  dst->h = b->y1 - b->y0;

  src->x = b->x0;
  src->y = b->y0;
  src->w = b->x1 - b->x0;
  src->h = b->y1 - b->y0;

  *xpos += b->xadvance;
}

void render_text(char* text, int x, int y) {
  SDL_Rect src, dst;
  while (*text) {
    stbtt_GetBakedRect(font.cdata, *text-32, &x, &y, &src, &dst);
    SDL_RenderCopy(ren, font.bitmap.tex, &src, &dst);
    text++;
  }
}

void draw_status_line() {
  int x, y;
  SDL_GetWindowSize(win, &x, &y);
  SDL_Rect dst = {
    .w = x,
    .h = 16,
    .x = 0,
    .y = y - 16
  };
  SDL_RenderFillRect(ren, &dst);
  render_text(status, 5, y - 4);
}

void ui_init(SDL_Window* window, SDL_Renderer* renderer, int blksz) {
  win = window;
  ren = renderer;

  clip = (SDL_Rect){
    .x = 576,
    .y = 0,
    .h = 480,
    .w = COLOUR_SWATCH_SIZE_PX * COLOUR_PICKER_W
  };

  picker.bitmap.data = NULL;
  grid.bitmap.data = NULL;
  font.bitmap.data = NULL;

  build_grid(1, blksz);
  grid.on = 1;

  bitmap_build_from_pixels(
    &picker.bitmap,
    (uint32_t*)pdata,
    COLOUR_PICKER_W,
    COLOUR_PICKER_H,
    &rgba32
  );

  picker.dest.y = 0;
  picker.dest.h = COLOUR_PICKER_H * COLOUR_SWATCH_SIZE_PX;
  picker.dest.w = COLOUR_PICKER_W * COLOUR_SWATCH_SIZE_PX;

  stbtt_BakeFontBitmap(
    RobotoMono_Regular_ttf, 0, 14.0,
    font.alpha, 128, 96,
    32, 96, font.cdata
  );

  uint32_t* bmp = malloc(128 * 96 * sizeof(uint32_t));
  raw_alpha_channel_to_rgba(font.alpha, bmp, 128*96, 0xa1a193);
  bitmap_build_from_pixels(&font.bitmap, bmp, 128, 96, &rgba32);
}

int in_bounds(SDL_Rect* rect, int x, int y) {
  return x >= rect->x && x <= rect->x + rect->w &&
    y >= rect->y && y <= rect->y + rect->h;
}

void ui_toggle_grid() {
  grid.on = !grid.on;
}

int ui_in_bounds(int widget_id, int x, int y) {
  switch (widget_id) {
    case UI_COLOUR_PICKER:
      return in_bounds(&picker.dest, x, y);
  }
  return 0;
}

uint32_t ui_colour_picker_get_colour(int x, int y) {
  int tx, ty;
  translate_picker_coord(x, y, &tx, &ty);
  return picker.bitmap.data[ty * COLOUR_PICKER_W + tx];
}

void ui_set_status(char* s) {
  status = s;
}

int ui_handle_events(SDL_Event* e) {
  switch (e->type) {
    case SDL_KEYDOWN:
      if (e->key.keysym.sym == SDLK_g) {
        ui_toggle_grid();
        return 1;
      }
      break;
    case SDL_MOUSEBUTTONDOWN:
      if (e->button.button == SDL_BUTTON_LEFT) {
        if (ui_in_bounds(UI_COLOUR_PICKER, e->button.x, e->button.y)) {
          uint32_t colour = ui_colour_picker_get_colour(e->button.x, e->button.y);
          image_set_paint_color(colour);
          return 1;
        }
      }
      break;
  }
  return 0;
}

void ui_draw(int imgx, int imgy, int imgw, int imgh, int imgscale, int blksz) {
  int x, y;
  SDL_GetWindowSize(win, &x, &y);

  draw_grid(imgx, imgy, imgw, imgh, imgscale, blksz);

  // draw right pane
  clip.x = x - COLOUR_SWATCH_SIZE_PX * COLOUR_PICKER_W;
  clip.h = y;
  SDL_SetRenderDrawColor(ren, 0x07, 0x36, 0x42, 255);
  SDL_RenderFillRect(ren, &clip);
  int right_pane_x = x - COLOUR_SWATCH_SIZE_PX * COLOUR_PICKER_W;
  picker.dest.x = right_pane_x;
  SDL_RenderCopy(ren, picker.bitmap.tex, NULL, &picker.dest);

  draw_status_line();
}
