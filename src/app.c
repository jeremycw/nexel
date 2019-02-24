#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "varry.h"
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

int prevx = -1, prevy = -1;
SDL_Rect dest;
int quit = 0;

int zoom = 1;
int color = 0xffffffff;

int width, height;
SDL_Window* win;
SDL_Surface* surf;
SDL_Texture* tex;
SDL_Renderer* ren;
unsigned char* data;

typedef struct {
  SDL_Texture* tex;
  SDL_Surface* surf;
  int* data;
  SDL_Rect dest;
  int on;
} grid_t;

grid_t grid;

typedef struct {
  SDL_Surface* surf;
  SDL_Texture* tex;
  unsigned char* data;
  SDL_Rect dest;
} palette_t;

palette_t palette;

typedef struct {
  int index;
  int color;
} pixel_t;

decl_varry(pixel_t);

typedef struct undo_s {
  struct undo_s* next;
  varry_t(pixel_t) pixels;
} undo_t;

typedef struct {
  SDL_Rect rect;
  SDL_Rect dest;
  SDL_Point start;
  SDL_Point end;
  int* data;
  int copying;
  int pasting;
} copy_t;

typedef struct {
  stbtt_bakedchar cdata[96];
  unsigned char bmp[128*128];
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

void build_image() {
  SDL_FreeSurface(surf);
  SDL_DestroyTexture(tex);
  surf = SDL_CreateRGBSurfaceFrom(
    (void*)data, width, height,
    32,          width*4, RMASK,
    GMASK,       BMASK, AMASK
  );
  tex = SDL_CreateTextureFromSurface(ren, surf);
}

void paint(int x, int y) {
  int tx, ty;
  translate_coord(x, y, &tx, &ty);
  int index = ty * width + tx;
  pixel_t pixel = {
    .index = index,
    .color = ((int*)data)[index]
  };
  if (undo_head) {
    varry_t(pixel_t) px = undo_head->pixels;
    int push = 1;
    for (int i = 0; i < px.size; i++) {
      if (px.buf[i].index == pixel.index) push = 0;
    }
    if (push) varry_push(pixel_t, &undo_head->pixels, pixel);
  }
  ((int*)data)[index] = color;
  build_image();
}

void pick_color(int x, int y) {
  int tx, ty;
  translate_coord(x, y, &tx, &ty);
  color = ((int*)data)[ty * width + tx];
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

void end_copy() {
  SDL_Point ts, te;
  translate_coord(copy.start.x, copy.start.y, &ts.x, &ts.y);
  translate_coord(copy.end.x, copy.end.y, &te.x, &te.y);
  coords_to_rect(ts, te, &copy.rect);
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
  prevx = x;
  prevy = y;
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
  stbi_write_png("out.png", surf->w, surf->h, 4, data, surf->w*4);
}

void undo() {
  if (undo_head == NULL) return;
  for (int i = 0; i < undo_head->pixels.size; i++) {
    pixel_t pixel = undo_head->pixels.buf[i];
    ((int*)data)[pixel.index] = pixel.color;
  }
  undo_t* tmp = undo_head;
  free(tmp->pixels.buf);
  undo_head = tmp->next;
  free(tmp);
  build_image();
}

int in_bounds(SDL_Rect* rect, int x, int y) {
  return x >= rect->x && x <= rect->x + rect->w &&
    y >= rect->y && y <= rect->y + rect->h;
}

void start_undo_record() {
  undo_t* undo = malloc(sizeof(undo_t));
  undo->next = undo_head;
  undo_head = undo;
  varry_init(pixel_t, &undo->pixels, 8);
}

void snap_rect_to_pixel(SDL_Rect* rect) {
  int offsetx = dest.x % zoom;
  int offsety = dest.y % zoom;
  rect->x -= (rect->x - offsetx) % zoom;
  rect->y -= (rect->y - offsety) % zoom;
  rect->w -= rect->w % zoom;
  rect->h -= rect->h % zoom;
}

void snap_rect_to_block(SDL_Rect* rect, int zoom) {
  int offsetx = dest.x % (zoom * BLOCK_SIZE);
  int offsety = dest.y % (zoom * BLOCK_SIZE);
  rect->x -= (rect->x - offsetx) % (zoom * BLOCK_SIZE);
  rect->y -= (rect->y - offsety) % (zoom * BLOCK_SIZE);
  rect->w -= rect->w % zoom * BLOCK_SIZE;
  rect->h -= rect->h % zoom * BLOCK_SIZE;
}

void copy_pixels(SDL_Rect* src, int x, int y, int* data) {
  for (int sy = src->y, dy = y; sy < src->y + src->h; sy++, dy++) {
    for (int sx = src->x, dx = x; sx < src->x + src->w; sx++, dx++) {
      int si = sy * surf->w + sx;
      int di = dy * surf->w + dx;
      pixel_t pixel = { .index = di, .color = data[di] };
      data[di] = data[si];
      varry_push(pixel_t, &undo_head->pixels, pixel);
    }
  }
}

void paste(int x, int y) {
  translate_coord(x, y, &x, &y);
  x -= x % BLOCK_SIZE;
  y -= y % BLOCK_SIZE;
  copy_pixels(&copy.rect, x, y, (int*)data);
  build_image();
}

void handle_event(SDL_Event* e) {
  int x, y, state;
  switch (e->type) {
    case SDL_KEYDOWN:
      if (e->key.keysym.sym == SDLK_ESCAPE) copy.pasting = 0;
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
          color = ((int*)palette.data)[ty * 4 + tx];
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
  for (int i = 0; i < surf->h / MAJOR_BLOCK_SIZE; i++) {
    for (int j = 0; j < surf->w / MAJOR_BLOCK_SIZE; j++) {
      SDL_RenderCopy(ren, grid.tex, NULL, &grid.dest);
      grid.dest.x += zoom * MAJOR_BLOCK_SIZE;
    }
    grid.dest.x = dest.x;
    grid.dest.y += zoom * MAJOR_BLOCK_SIZE;
  }
}

void build_palette() {
  palette.data = pdata;
  palette.surf = SDL_CreateRGBSurfaceFrom(
    (void*)palette.data, 4, 16,
    32,          4*4, RMASK,
    GMASK,       BMASK, AMASK
  );
	palette.tex = SDL_CreateTextureFromSurface(ren, palette.surf);
  palette.dest.y = 0;
  palette.dest.h = 16 * 16;
  palette.dest.w = 4 * 16;
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

void run_app(char* path) {
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
  int orig_format;
  data = stbi_load(path, &width, &height, &orig_format, req_format);
  if(data == NULL) {
    SDL_Log("Loading image failed: %s", stbi_failure_reason());
    exit(1);
  }

  // Set up the pixel format color masks for RGB(A) byte arrays.
  // Only STBI_rgb (3) and STBI_rgb_alpha (4) are supported here!
  int pitch = 4*width;
  surf = SDL_CreateRGBSurfaceFrom(
    (void*)data, width, height,
    32,          pitch, RMASK,
    GMASK,       BMASK, AMASK
  );

  build_grid();
  grid.on = 1;
  copy.copying = 0;
  copy.pasting = 0;
  build_palette();

  int x, y;
  SDL_GetWindowSize(win, &x, &y);
  dest.x = x / 2 - width / 2;
  dest.y = y / 2 - height / 2;
  dest.h = height;
  dest.w = width;

  if (surf == NULL) {
    SDL_Log("Creating surface failed: %s", SDL_GetError());
    stbi_image_free(data);
    exit(1);
  }

	tex = SDL_CreateTextureFromSurface(ren, surf);
	if (tex == NULL) {
		printf("SDL_CreateTextureFromSurface Error: %s\n", SDL_GetError());
    exit(1);
	}

  SDL_Event e;
  SDL_Rect clip = {
    .x = 576,
    .y = 0,
    .h = 480,
    .w = 64
  };

  stbtt_BakeFontBitmap(RobotoMono_Regular_ttf,0, 14.0, font.bmp, 128, 128, 32, 96, font.cdata);
  font.surf = SDL_CreateRGBSurfaceFrom(
    (void*)font.bmp, 128,   128,
    8,              128, 0xFF,
    0x0,           0x0, 0
  );
  char *text = "Hello, World!";
  SDL_Rect src, dst;
  x = 0, y = 0;
  while (*text) {
    stbtt_GetBakedRect(font.cdata, *text-32, &x, &y, &src, &dst);
    printf("(%d, %d) [(%d, %d), (%d, %d)] [(%d, %d), (%d, %d)]\n", x, y, dst.x, dst.y, dst.w, dst.h, src.x, src.y, src.w, src.h);
    text++;
  }
  font.tex = SDL_CreateTextureFromSurface(ren, font.surf);


  SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

  while (!quit) {
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, NULL, &dest);
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
      SDL_RenderCopy(ren, tex, &copy.rect, &dst);
    }
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    draw_grid();
    SDL_GetWindowSize(win, &x, &y);
    clip.x = x - 16 * 4;
    clip.h = y;
    SDL_RenderFillRect(ren, &clip);
    palette.dest.x = x - 16 * 4;
    SDL_RenderCopy(ren, palette.tex, NULL, &palette.dest);
    SDL_Rect dst = {
      .x = 0,
      .y = 0,
      .w = 128,
      .h = 128
    };
    SDL_RenderCopy(ren, font.tex, NULL, &dst);
    SDL_RenderPresent(ren);
    SDL_WaitEvent(&e);
    handle_event(&e);
    while (SDL_PollEvent(&e)) {
      handle_event(&e);
    }
  }

  SDL_FreeSurface(surf);
  stbi_image_free(data);

  SDL_DestroyTexture(tex);
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();

  exit(1);
}
