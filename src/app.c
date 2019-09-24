#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "varray.h"
#include "image.h"
#include "roboto.h"
#include "nes.h"
#include "threads.h"
#include "bitmap.h"
#include "copypaste.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
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

view_t view;

undo_t* undo_head = NULL;

int quit = 0;
int color = 0xffffffff;

char* filename;
SDL_Window* win;
SDL_Renderer* ren;

bitmap_t image;

grid_t grid;
palette_t palette;

font_t font;
char* status;

threadpool global_thread_pool = NULL;

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

void build_grid() {
  unsigned int* data = create_grid_tile(view.scale * MAJOR_BLOCK_SIZE);
  int size = MAJOR_BLOCK_SIZE * view.scale;
  build_bitmap_from_pixels(&grid.bitmap, data, size, size, &rgba32);
  grid.dest.h = MAJOR_BLOCK_SIZE * view.scale;
  grid.dest.w = MAJOR_BLOCK_SIZE * view.scale;
}

void paint(int x, int y) {
  int tx, ty;
  translate_coord(x, y, &tx, &ty, &view);
  int index = ty * image.width + tx;
  pixel_t pixel = {
    .index = index,
    .color = image.data[index]
  };
  varray_t(pixel_t) px = undo_head->pixels;
  int push = 1;
  for (int i = 0; i < px.size; i++) {
    if (px.buf[i].index == pixel.index) push = 0;
  }
  if (push) varray_push(pixel_t, &undo_head->pixels, pixel);
  image.data[index] = color;
  rebuild_bitmap(&image);
}

void pick_color(int x, int y) {
  int tx, ty;
  translate_coord(x, y, &tx, &ty, &view);
  color = image.data[ty * image.width + tx];
}

void print_rect(char* name, SDL_Rect* rect) {
  printf("%s: [(%d, %d), (%d, %d)]\n", name, rect->x, rect->y, rect->w, rect->h);
}

void mouse_move(int state, int x, int y) {
  if (state & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
    drag_copy(x, y);
  } else if (state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
    if (pasting()) {
    } else {
      paint(x, y);
    }
  }
}

void zoom_in() {
  int x, y;
  SDL_GetWindowSize(win, &x, &y);
  view.translation.y -= (y/2 - view.translation.y);
  view.translation.x -= (x/2 - view.translation.x);
  view.translation.h *= 2;
  view.translation.w *= 2;
  view.scale *= 2;
  build_grid();
}

void zoom_out() {
  if (view.scale == 1) return;
  int x, y;
  SDL_GetWindowSize(win, &x, &y);
  view.translation.y += (y/2 - view.translation.y)/2;
  view.translation.x += (x/2 - view.translation.x)/2;
  view.translation.h /= 2;
  view.translation.w /= 2;
  view.scale /= 2;
  build_grid();
}

void save() {
  stbi_write_png(filename, image.width, image.height, 4, (unsigned char*)image.data, image.pitch);
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
  rebuild_bitmap(&image);
}

int in_bounds(SDL_Rect* rect, int x, int y) {
  return x >= rect->x && x <= rect->x + rect->w &&
    y >= rect->y && y <= rect->y + rect->h;
}

void start_undo_record() {
  undo_t* undo = malloc(sizeof(undo_t));
  undo->next = undo_head;
  undo_head = undo;
  varray_init(pixel_t, &undo->pixels, 8);
}

void handle_event(SDL_Event* e) {
  int x, y, state;
  switch (e->type) {
    case SDL_KEYDOWN:
      handle_copy_paste_keys(e);
      if (e->key.keysym.sym == SDLK_EQUALS) zoom_in();
      if (e->key.keysym.sym == SDLK_MINUS) zoom_out();
      if (e->key.keysym.sym == SDLK_s) save();
      if (e->key.keysym.sym == SDLK_u) undo();
      if (e->key.keysym.sym == SDLK_g) grid.on = !grid.on;
      if (e->key.keysym.sym == SDLK_q) quit = 1;
      break;
    case SDL_QUIT:
      quit = 1;
      break;
    case SDL_MOUSEMOTION:
      state = SDL_GetMouseState(&x, &y);
      mouse_move(state, x, y);
      break;
    case SDL_MOUSEWHEEL:
      view.translation.x -= e->wheel.x*10;
      view.translation.y += e->wheel.y*10;
      break;
    case SDL_MOUSEBUTTONDOWN:
      if (e->button.button == SDL_BUTTON_LEFT) {
        if (in_bounds(&palette.dest, e->button.x, e->button.y)) {
          int tx, ty;
          translate_palette_coord(e->button.x, e->button.y, &tx, &ty);
          color = palette.bitmap.data[ty * 4 + tx];
          break;
        }
        start_undo_record();
        if (pasting()) {
          paste(e->button.x, e->button.y, undo_head, &image);
        } else {
          paint(e->button.x, e->button.y);
        }
      } else if (e->button.button == SDL_BUTTON_RIGHT) {
        start_copy(e->button.x, e->button.y);
        pick_color(e->button.x, e->button.y);
      }
      break;
    case SDL_MOUSEBUTTONUP:
      if (copying()) {
        end_copy(&image);
      }
      break;
  }
}

void draw_grid() {
  if (!grid.on) return;
  grid.dest.x = view.translation.x;
  grid.dest.y = view.translation.y;
  for (int i = 0; i < image.height / MAJOR_BLOCK_SIZE; i++) {
    for (int j = 0; j < image.width / MAJOR_BLOCK_SIZE; j++) {
      SDL_RenderCopy(ren, grid.bitmap.tex, NULL, &grid.dest);
      grid.dest.x += view.scale * MAJOR_BLOCK_SIZE;
    }
    grid.dest.x = view.translation.x;
    grid.dest.y += view.scale * MAJOR_BLOCK_SIZE;
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

  global_thread_pool = thpool_init(GLOBAL_THREAD_POOL_SIZE);

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
  set_bitmap_renderer(ren);
  init_copy_paste(&view);

  int req_format = STBI_rgb_alpha;
  int orig_format;
  unsigned int* data;
  if (access(path, F_OK) != -1) {
    // file exists
    data = (unsigned int*)stbi_load(path, &width, &height, &orig_format, req_format);
    if(data == NULL) {
      SDL_Log("Loading image failed: %s", stbi_failure_reason());
      exit(1);
    }
  } else {
    int pixels = width * height;
    size_t bytes = sizeof(unsigned int) * pixels;
    data = malloc(bytes);
    for (int i = 0; i < pixels; i++) {
      data[i] = 0xFF000000;
    }
  }

  image.data = NULL;
  palette.bitmap.data = NULL;
  grid.bitmap.data = NULL;
  font.bitmap.data = NULL;

  build_bitmap_from_pixels(&image, data, width, height, &rgba32);

  build_grid();
  grid.on = 1;

  build_bitmap_from_pixels(&palette.bitmap, (unsigned int*)pdata, 4, 16, &rgba32);
  palette.dest.y = 0;
  palette.dest.h = 16 * 16;
  palette.dest.w = 4 * 16;

  int x, y;
  SDL_GetWindowSize(win, &x, &y);
  view.translation.x = x / 2 - width / 2;
  view.translation.y = y / 2 - height / 2;
  view.translation.h = height;
  view.translation.w = width;

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
  alpha_channel_to_rgba(font.alpha, bmp, 128*96, 0xa1a193);
  build_bitmap_from_pixels(&font.bitmap, bmp, 128, 96, &rgba32);

  SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

  while (!quit) {
    SDL_SetRenderDrawColor(ren, 0, 0x2b, 0x36, 255);
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, image.tex, NULL, &view.translation);
    render_copy_paste(x, y, ren);
    draw_grid();
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

  SDL_FreeSurface(image.surf);
  stbi_image_free(image.data);

  SDL_DestroyTexture(image.tex);
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
}
