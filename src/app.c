#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "varray.h"
#include "image.h"
#include "roboto.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define RMASK 0x000000ff
#define GMASK 0x0000ff00
#define BMASK 0x00ff0000
#define AMASK 0xff000000

#define BLOCK_SIZE 8
#define MAJOR_BLOCK_SIZE 16

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

char* filename;
SDL_Rect dest;
int quit = 0;

int zoom = 1;
int color = 0xffffffff;

SDL_Window* win;
SDL_Renderer* ren;
char* status;

typedef struct {
  int bpp;
  int rmask;
  int gmask;
  int bmask;
  int amask;
} img_format_t;

img_format_t rgb32 = {
  .bpp = 32,
  .rmask = RMASK,
  .gmask = GMASK,
  .bmask = BMASK,
  .amask = AMASK,
};

typedef struct {
  SDL_Surface* surf;
  SDL_Texture* tex;
  unsigned int* data;
  int width;
  int height;
  int pitch;
  img_format_t* format;
} pixel_texture_t;

typedef struct {
  SDL_Texture* tex;
  SDL_Surface* surf;
  int* data;
  SDL_Rect dest;
  int on;
} grid_t;

pixel_texture_t image;
grid_t grid;

typedef struct {
  pixel_texture_t pixel_texture;
  SDL_Rect dest;
} palette_t;

palette_t palette;

typedef struct {
  int index;
  int color;
} pixel_t;

decl_varray(pixel_t);

typedef struct undo_s {
  struct undo_s* next;
  varray_t(pixel_t) pixels;
} undo_t;

typedef struct {
  SDL_Rect rect;
  SDL_Rect dest;
  SDL_Point start;
  SDL_Point end;
  int copying;
  int pasting;
  pixel_texture_t pixel_texture;
} copy_t;

typedef struct {
  stbtt_bakedchar cdata[96];
  unsigned char alpha[128*96];
  unsigned int bmp[128*96*4];
  SDL_Surface* surf;
  SDL_Texture* tex;
} font_t;

undo_t* undo_head = NULL;

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

copy_t copy;
font_t font;

void safe_free_texture(pixel_texture_t* pixel_texture) {
  if (pixel_texture->data) {
    free(pixel_texture->data);
    pixel_texture->data = NULL;
    SDL_FreeSurface(pixel_texture->surf);
    SDL_DestroyTexture(pixel_texture->tex);
  }
}

void build_texture(pixel_texture_t* pixel_texture) {
  pixel_texture->surf = SDL_CreateRGBSurfaceFrom(
    (void*)pixel_texture->data, pixel_texture->width, pixel_texture->height,
    pixel_texture->format->bpp, pixel_texture->pitch, pixel_texture->format->rmask,
    pixel_texture->format->gmask, pixel_texture->format->bmask, pixel_texture->format->amask
  );
  pixel_texture->tex = SDL_CreateTextureFromSurface(ren, pixel_texture->surf);
}

void rebuild_texture(pixel_texture_t* pixel_texture) {
  SDL_FreeSurface(pixel_texture->surf);
  SDL_DestroyTexture(pixel_texture->tex);
  build_texture(pixel_texture);
}

void build_texture_from_pixels(pixel_texture_t* pixel_texture, unsigned int* data, int w, int h, img_format_t* format) {
  safe_free_texture(pixel_texture);
  pixel_texture->width = w;
  pixel_texture->height = h;
  pixel_texture->pitch = w * (format->bpp / 8);
  pixel_texture->data = data;
  pixel_texture->format = format;
  build_texture(pixel_texture);
}

void translate_coord(int x, int y, int* tx, int* ty) {
  *tx = (x - dest.x) / zoom; 
  *ty = (y - dest.y) / zoom;
}

void translate_palette_coord(int x, int y, int* tx, int* ty) {
  *tx = (x - palette.dest.x) / 16; 
  *ty = (y - palette.dest.y) / 16;
}

int* create_grid_tile(int size) {
  int* data = calloc(size * size, sizeof(int));
  for (int i = 0, j = 0; i < size; i++, j += size) {
    data[i] = 0xffffffff;
    if (i % 2 == 0) data[i + size/2 * size] = 0xffdddddd;
    data[j] = 0xffffffff;
    if (i % 2 == 0) data[j + size/2] = 0xffdddddd;
  }
  return data;
}

void build_grid() {
  grid.data = create_grid_tile(zoom * MAJOR_BLOCK_SIZE);
  grid.surf = SDL_CreateRGBSurfaceFrom(
    (void*)grid.data, MAJOR_BLOCK_SIZE * zoom, MAJOR_BLOCK_SIZE * zoom,
    32,          4*MAJOR_BLOCK_SIZE * zoom, RMASK,
    GMASK,       BMASK, AMASK
  );
  grid.tex = SDL_CreateTextureFromSurface(ren, grid.surf);
  grid.dest.h = MAJOR_BLOCK_SIZE * zoom;
  grid.dest.w = MAJOR_BLOCK_SIZE * zoom;
}

void paint(int x, int y) {
  int tx, ty;
  translate_coord(x, y, &tx, &ty);
  int index = ty * image.width + tx;
  pixel_t pixel = {
    .index = index,
    .color = image.data[index]
  };
  if (undo_head) {
    varray_t(pixel_t) px = undo_head->pixels;
    int push = 1;
    for (int i = 0; i < px.size; i++) {
      if (px.buf[i].index == pixel.index) push = 0;
    }
    if (push) varray_push(pixel_t, &undo_head->pixels, pixel);
  }
  image.data[index] = color;
  rebuild_texture(&image);
}

void pick_color(int x, int y) {
  int tx, ty;
  translate_coord(x, y, &tx, &ty);
  color = image.data[ty * image.width + tx];
}

void start_copy(int x, int y) {
  copy.copying = 1;
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

void print_rect(char* name, SDL_Rect* rect) {
  printf("%s: [(%d, %d), (%d, %d)]\n", name, rect->x, rect->y, rect->w, rect->h);
}

void snap_rect_to_pixel(SDL_Rect* rect) {
  int offsetx = dest.x % zoom;
  int offsety = dest.y % zoom;
  rect->x -= (rect->x - offsetx) % zoom;
  rect->y -= (rect->y - offsety) % zoom;
  rect->w -= rect->w % zoom;
  rect->h -= rect->h % zoom;
}

void end_copy() {
  coords_to_rect(copy.start, copy.end, &copy.rect);
  snap_rect_to_pixel(&copy.rect);
  translate_coord(copy.rect.x, copy.rect.y, &copy.rect.x, &copy.rect.y);
  copy.rect.w /= zoom;
  copy.rect.h /= zoom;
  unsigned int* new_data = malloc(4 * copy.rect.w * copy.rect.h);
  pixel_loop(copy.rect.x, copy.rect.y, image.width, 0, 0, copy.rect.w, copy.rect.w, copy.rect.h) {
    new_data[di] = image.data[si];
  }
  build_texture_from_pixels(&copy.pixel_texture, new_data, copy.rect.w, copy.rect.h, &rgb32);
  copy.copying = 0;
  if (copy.rect.h > 1 || copy.rect.w > 1) {
    copy.pasting = 1;
  }
}

void mouse_move(int state, int x, int y) {
  if (state & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
    copy.end.x = x, copy.end.y = y;
  } else if (state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
    if (copy.pasting) {
    } else {
      paint(x, y);
    }
  }
}

void zoom_in() {
  int x, y;
  SDL_GetWindowSize(win, &x, &y);
  dest.y -= (y/2 - dest.y);
  dest.x -= (x/2 - dest.x);
  dest.h *= 2;
  dest.w *= 2;
  zoom *= 2;
  free(grid.data);
  build_grid();
}

void zoom_out() {
  if (zoom == 1) return;
  int x, y;
  SDL_GetWindowSize(win, &x, &y);
  dest.y += (y/2 - dest.y)/2;
  dest.x += (x/2 - dest.x)/2;
  dest.h /= 2;
  dest.w /= 2;
  zoom /= 2;
  free(grid.data);
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
  rebuild_texture(&image);
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

void snap_rect_to_block(SDL_Rect* rect, int zoom) {
  int offsetx = dest.x % (zoom * BLOCK_SIZE);
  int offsety = dest.y % (zoom * BLOCK_SIZE);
  rect->x -= (rect->x - offsetx) % (zoom * BLOCK_SIZE);
  rect->y -= (rect->y - offsety) % (zoom * BLOCK_SIZE);
  rect->w -= rect->w % zoom * BLOCK_SIZE;
  rect->h -= rect->h % zoom * BLOCK_SIZE;
}

void paste(int x, int y) {
  translate_coord(x, y, &x, &y);
  x -= x % BLOCK_SIZE;
  y -= y % BLOCK_SIZE;
  pixel_loop(0, 0, copy.rect.w, x, y, image.width, copy.rect.w, copy.rect.h) {
    pixel_t pixel = { .index = di, .color = image.data[di] };
    image.data[di] = copy.pixel_texture.data[si];
    varray_push(pixel_t, &undo_head->pixels, pixel);
  }
  rebuild_texture(&image);
}

void end_paste() {
  safe_free_texture(&copy.pixel_texture);
  copy.pasting = 0;
}

void rotate_paste() {
  unsigned int* rotated = malloc(copy.rect.w * copy.rect.h * 4);
  rotate_clockwise(copy.pixel_texture.data, copy.rect.w, copy.rect.h, rotated);
  free(copy.pixel_texture.data);
  copy.pixel_texture.data = rotated;
  int tmp = copy.rect.w;
  copy.rect.w = copy.rect.h;
  copy.rect.h = tmp;
  rebuild_texture(&copy.pixel_texture);
}

void flip_horizontal() {
  mirror_horizontal(copy.pixel_texture.data, copy.rect.w, copy.rect.h);
  rebuild_texture(&copy.pixel_texture);
}

void flip_vertical() {
  mirror_vertical(copy.pixel_texture.data, copy.rect.w, copy.rect.h);
  rebuild_texture(&copy.pixel_texture);
}

void handle_event(SDL_Event* e) {
  int x, y, state;
  switch (e->type) {
    case SDL_KEYDOWN:
      if (copy.pasting) {
        if (e->key.keysym.sym == SDLK_r) rotate_paste();
        if (e->key.keysym.sym == SDLK_ESCAPE) end_paste();
        if (e->key.keysym.sym == SDLK_h) flip_horizontal();
        if (e->key.keysym.sym == SDLK_v) flip_vertical();
      }
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
      dest.x -= e->wheel.x*10;
      dest.y += e->wheel.y*10;
      break;
    case SDL_MOUSEBUTTONDOWN:
      if (e->button.button == SDL_BUTTON_LEFT) {
        if (in_bounds(&palette.dest, e->button.x, e->button.y)) {
          int tx, ty;
          translate_palette_coord(e->button.x, e->button.y, &tx, &ty);
          color = palette.pixel_texture.data[ty * 4 + tx];
          break;
        }
        start_undo_record();
        if (copy.pasting) {
          paste(e->button.x, e->button.y);
        } else {
          paint(e->button.x, e->button.y);
        }
      } else if (e->button.button == SDL_BUTTON_RIGHT) {
        start_copy(e->button.x, e->button.y);
        pick_color(e->button.x, e->button.y);
      }
      break;
    case SDL_MOUSEBUTTONUP:
      if (copy.copying) {
        end_copy();
      }
      break;
  }
}

void draw_grid() {
  if (!grid.on) return;
  grid.dest.x = dest.x;
  grid.dest.y = dest.y;
  for (int i = 0; i < image.height / MAJOR_BLOCK_SIZE; i++) {
    for (int j = 0; j < image.width / MAJOR_BLOCK_SIZE; j++) {
      SDL_RenderCopy(ren, grid.tex, NULL, &grid.dest);
      grid.dest.x += zoom * MAJOR_BLOCK_SIZE;
    }
    grid.dest.x = dest.x;
    grid.dest.y += zoom * MAJOR_BLOCK_SIZE;
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
    SDL_RenderCopy(ren, font.tex, &src, &dst);
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

void run_app(char* path) {
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

  // the color format you request stb_image to output,
  // use STBI_rgb if you don't want/need the alpha channel
  int req_format = STBI_rgb_alpha;
  int orig_format, width, height;
  unsigned int* data = (unsigned int*)stbi_load(path, &width, &height, &orig_format, req_format);
  if(data == NULL) {
    SDL_Log("Loading image failed: %s", stbi_failure_reason());
    exit(1);
  }

  image.data = NULL;
  build_texture_from_pixels(&image, data, width, height, &rgb32);

  build_grid();
  grid.on = 1;
  copy.pixel_texture.data = NULL;
  copy.copying = 0;
  copy.pasting = 0;

  palette.pixel_texture.data = NULL;
  build_texture_from_pixels(&palette.pixel_texture, (unsigned int*)pdata, 4, 16, &rgb32);
  palette.dest.y = 0;
  palette.dest.h = 16 * 16;
  palette.dest.w = 4 * 16;

  int x, y;
  SDL_GetWindowSize(win, &x, &y);
  dest.x = x / 2 - width / 2;
  dest.y = y / 2 - height / 2;
  dest.h = height;
  dest.w = width;

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
  alpha_channel_to_rgba(font.alpha, font.bmp, 128*96, 0xa1a193);
  font.surf = SDL_CreateRGBSurfaceFrom(
    (void*)font.bmp, 128,   96,
    32,              4*128, RMASK,
    GMASK,           BMASK, AMASK
  );
  font.tex = SDL_CreateTextureFromSurface(ren, font.surf);


  SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

  while (!quit) {
    SDL_SetRenderDrawColor(ren, 0, 0x2b, 0x36, 255);
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, image.tex, NULL, &dest);
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
        .w = copy.rect.w * zoom,
        .h = copy.rect.h * zoom
      };
      snap_rect_to_block(&dst, zoom);
      SDL_RenderCopy(ren, copy.pixel_texture.tex, NULL, &dst);
    }
    draw_grid();
    SDL_GetWindowSize(win, &x, &y);
    clip.x = x - 16 * 4;
    clip.h = y;
    SDL_SetRenderDrawColor(ren, 0x07, 0x36, 0x42, 255);
    SDL_RenderFillRect(ren, &clip);
    palette.dest.x = x - 16 * 4;
    SDL_RenderCopy(ren, palette.pixel_texture.tex, NULL, &palette.dest);
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

  exit(1);
}
