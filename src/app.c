#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "image.h"
#include "raw.h"
#include "roboto.h"
#include "nes.h"
#include "bitmap.h"
#include "copypaste.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

typedef struct {
  bitmap_t bitmap;
  SDL_Rect dest;
  int on;
} grid_t;

typedef struct {
  bitmap_t bitmap;
  SDL_Rect dest;
} palette_t;

typedef struct {
  stbtt_bakedchar cdata[96];
  unsigned char alpha[128*96];
  bitmap_t bitmap;
} font_t;

unsigned char pdata[] = {
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

static int quit = 0;

static char* filename;
static SDL_Window* win;
static SDL_Renderer* ren;

static grid_t grid;
static palette_t palette;

static font_t font;
static char* status;

void translate_palette_coord(int x, int y, int* tx, int* ty) {
  *tx = (x - palette.dest.x) / 16; 
  *ty = (y - palette.dest.y) / 16;
}

unsigned int* create_grid_tile(int size) {
  unsigned int* data = calloc(size * size, sizeof(unsigned int));
  for (int i = 0, j = 0; i < size; i++, j += size) {
    data[i] = 0xffffffff;
    if (i % 2 == 0) data[i + size/2 * size] = 0xffdddddd;
    data[j] = 0xffffffff;
    if (i % 2 == 0) data[j + size/2] = 0xffdddddd;
  }
  return data;
}

void build_grid(int scale) {
  unsigned int* data = create_grid_tile(scale * MAJOR_BLOCK_SIZE);
  int size = MAJOR_BLOCK_SIZE * scale;
  bitmap_build_from_pixels(&grid.bitmap, data, size, size, &rgba32);
  grid.dest.h = MAJOR_BLOCK_SIZE * scale;
  grid.dest.w = MAJOR_BLOCK_SIZE * scale;
}

void print_rect(char* name, SDL_Rect* rect) {
  printf("%s: [(%d, %d), (%d, %d)]\n", name, rect->x, rect->y, rect->w, rect->h);
}

int in_bounds(SDL_Rect* rect, int x, int y) {
  return x >= rect->x && x <= rect->x + rect->w &&
    y >= rect->y && y <= rect->y + rect->h;
}

void handle_event(SDL_Event* e) {
  if (copy_paste_handle_events(e)) return;
  if (image_handle_events(e, win)) return;
  switch (e->type) {
    case SDL_KEYDOWN:
      if (e->key.keysym.sym == SDLK_g) grid.on = !grid.on;
      if (e->key.keysym.sym == SDLK_q) quit = 1;
      break;
    case SDL_QUIT:
      quit = 1;
      break;
    case SDL_MOUSEBUTTONDOWN:
      if (e->button.button == SDL_BUTTON_LEFT) {
        if (in_bounds(&palette.dest, e->button.x, e->button.y)) {
          int tx, ty;
          translate_palette_coord(e->button.x, e->button.y, &tx, &ty);
          image_set_paint_color(palette.bitmap.data[ty * 4 + tx]);
          break;
        }
      }
      break;
  }
}

void draw_grid(int x, int y, int w, int h, int scale, int blksize) {
  if (!grid.on) return;
  build_grid(scale);
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

void run_app(char* path, int width, int height) {
  status = path;
  filename = path;

  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    printf("SDL_Init Error: %s\n", SDL_GetError());
    exit(1);
  }

  win = SDL_CreateWindow("Nexel", 0, 0, 640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (win == NULL) {
    printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
    exit(1);
  }

  ren = SDL_CreateRenderer(win, -1,SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (ren == NULL) {
    printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
    exit(1);
  }
  bitmap_set_renderer(ren);
  image_init(path, win, width, height);

  palette.bitmap.data = NULL;
  grid.bitmap.data = NULL;
  font.bitmap.data = NULL;

  build_grid(1);
  grid.on = 1;

  bitmap_build_from_pixels(&palette.bitmap, (unsigned int*)pdata, 4, 16, &rgba32);
  palette.dest.y = 0;
  palette.dest.h = 16 * 16;
  palette.dest.w = 4 * 16;

  SDL_Event e;
  SDL_Rect clip = {
    .x = 576,
    .y = 0,
    .h = 480,
    .w = 64
  };

  stbtt_BakeFontBitmap(
    RobotoMono_Regular_ttf, 0, 14.0,
    font.alpha, 128, 96,
    32, 96, font.cdata
  );

  unsigned int* bmp = malloc(128 * 96 * sizeof(unsigned int));
  raw_alpha_channel_to_rgba(font.alpha, bmp, 128*96, 0xa1a193);
  bitmap_build_from_pixels(&font.bitmap, bmp, 128, 96, &rgba32);

  SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
  int ntiles = width / BLOCK_SIZE * height / BLOCK_SIZE;
  pal4_t* palettes = malloc(sizeof(pal4_t) * ntiles);

  int x, y;
  while (!quit) {
    image_draw(ren);
    copy_paste_render(ren);
    image_info_t info;
    image_info(&info);
    draw_grid(info.x, info.y, info.width, info.height, info.scale, MAJOR_BLOCK_SIZE);

    nes_detect_palettes(image_raw(), info.width, info.height, palettes);
    //nes_unique_palettes(palettes, ntiles, 
    SDL_GetWindowSize(win, &x, &y);
    clip.x = x - 16 * 4;
    clip.h = y;
    SDL_SetRenderDrawColor(ren, 0x07, 0x36, 0x42, 255);
    SDL_RenderFillRect(ren, &clip);
    palette.dest.x = x - 16 * 4;
    SDL_RenderCopy(ren, palette.bitmap.tex, NULL, &palette.dest);
    draw_status_line();
    SDL_RenderPresent(ren);
    SDL_WaitEvent(&e);
    handle_event(&e);
    while (SDL_PollEvent(&e)) {
      handle_event(&e);
    }
  }

  image_destroy();
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
}
