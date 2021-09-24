#include <stdint.h>
#include <SDL2/SDL.h>

#ifndef DATA_H
#define DATA_H

#define RMASK 0x000000ff
#define GMASK 0x0000ff00
#define BMASK 0x00ff0000
#define AMASK 0xff000000

#define BITS_IN_BYTE 8
#define BITS_PER_PIXEL 32

#define EDITOR_TILE_SIZE 8

#define BACKGROUND_COLOUR 0x002b36ff
#define SELECTION_COLOUR 0x0000ff64

#define array_push(buf, a) \
  if (buf##_meta.size == buf##_meta.capacity) { \
    buf##_meta.capacity *= 2; \
    buf = realloc(buf, buf##_meta.capacity * buf##_meta.type_size); \
  } \
  buf[buf##_meta.size] = a; \
  buf##_meta.size++;

#define array_accommodate(buf, cap) \
  if (buf##_meta.capacity < cap) { \
    buf##_meta.capacity = cap; \
    buf = realloc(buf, cap * buf##_meta.type_size); \
  }

#define array_init(buf, tsize, cap) \
  buf = malloc(tsize * cap); \
  buf##_meta.size = 0; \
  buf##_meta.capacity = cap; \
  buf##_meta.type_size = tsize;

#define array_remove(buf, i) \
  if (i < buf##_meta.size && buf##_meta.size > 0) { \
    buf[i] = buf[buf##_meta.size-1]; \
    buf##_meta.size--; \
  }

#define array_clear(buf) \
  buf##_meta.size = 0;

#define array_decl(type, name) \
  struct array_meta name##_meta; \
  type* name;

#define array_itr(buf, i) \
  for (int i = 0; i < buf##_meta.size; i++)

#define array_free(buf) \
  free(buf);

#define array_len(buf) \
  buf##_meta.size

#define array_first(buf) \
  buf[0]

#define array_last(buf) \
  buf[buf##_meta.size-1]

#define array_assign(buf, other) \
  buf = other; \
  buf##_meta = other##_meta;

struct data;

enum editor_states {
  EDITOR_STATE_NONE,
  EDITOR_STATE_SELECTING,
  EDITOR_STATE_PASTING
};

typedef int16_t indexed_pixel_t;

struct point {
  int x;
  int y;
};

struct dimensions {
  int w;
  int h;
};

typedef union {
  uint32_t colour;
  uint8_t bytes[4];
  struct {
    uint8_t a;
    uint8_t b;
    uint8_t g;
    uint8_t r;
  };
} colour_t;

struct selection {
  struct point start;
  struct point end;
};

struct pixel_change {
  int buffer_index;
  indexed_pixel_t from;
  indexed_pixel_t to;
};

struct array_meta {
  int size;
  int capacity;
  int type_size;
};

struct palette {
  char const* name;
  colour_t* colours;
  int n;
};

struct rect {
  union {
    struct {
      int x;
      int y;
    };
    struct point point;
  };
  union {
    struct {
      int w;
      int h;
    };
    struct dimensions dimensions;
  };
};

struct bitmap {
  union {
    struct {
      int width;
      int height;
    };
    struct dimensions dimensions;
  };
  int bpp;
  uint8_t buffer[0];
};

struct sdl_bitmap {
  SDL_Surface* surface;
  SDL_Texture* texture;
};

struct indexed_bitmap {
  union {
    struct {
      int width;
      int height;
    };
    struct dimensions dimensions;
  };
  indexed_pixel_t* pixels;
};

struct indexed_bitmap_file {
  char const* filename;
  struct indexed_bitmap bitmap;
};

struct palette_selector {
  int current_palette;
  array_decl(struct palette, palettes)
};

struct transform {
  struct point translation;
  int scale;
};

struct pixel_change_group {
  array_decl(struct pixel_change, pixel_changes)
};

struct editor {
  indexed_pixel_t paint_colour;
  struct transform image_transform;
  char const* display_name;
  struct indexed_bitmap image_bitmap;
  array_decl(struct pixel_change_group, undos)
  array_decl(struct selection, selections)
  array_decl(struct indexed_bitmap, copies)
  enum editor_states state;
  struct palette_selector palette_selector;
  int grid_enabled;
};

struct sdl_state {
  struct point mouse;
  int mouse_state;
  struct dimensions window;
};

struct sdl_renderer {
  struct sdl_bitmap image;
  struct sdl_bitmap palette;
  struct sdl_bitmap grid;
  array_decl(struct sdl_bitmap, copy_bitmaps);

  SDL_Window* window;
  SDL_Renderer* renderer;
};

// sdl_renderer_draw
struct din_sdl_renderer_draw* data_r_sdl_renderer_draw(struct data* data);

struct din_sdl_renderer_draw {
  SDL_Renderer* sdl_renderer;
  colour_t background_colour;
  int grid_enabled;
  int tile_size;
  struct transform image_transform;
  struct sdl_bitmap image_sdl_bitmap;
  SDL_Texture* grid_texture;
  colour_t selection_colour;
  int selections_n;
  struct selection const* selections;
  int copy_bitmaps_n;
  struct sdl_bitmap const* copy_bitmaps;
  struct dimensions window_size;
};

// editor_copy_selection
struct dout_editor_copy_selection {
  enum editor_states editor_state;
  array_decl(struct indexed_bitmap, copies)
  array_decl(struct sdl_bitmap, copy_bitmaps)
};

struct din_editor_copy_selection {
  struct dout_editor_copy_selection dout;
  SDL_Renderer* sdl_renderer;
  colour_t const* palette_colours;
  struct transform image_transform;
  int selections_n;
  struct selection const* selections;
  int image_pixels_n;
  int image_width;
  indexed_pixel_t const* image_pixels;
};

void data_w_editor_copy_selection(struct data* data, struct dout_editor_copy_selection* dout);
struct din_editor_copy_selection* data_r_editor_copy_selection(struct data* data);

// editor_resize_selection
struct dout_editor_resize_selection {
  struct point end_selection;
};

struct din_editor_resize_selection {
  struct dout_editor_resize_selection dout;
  struct point mouse;
};

void data_w_editor_resize_selection(struct data* data, struct dout_editor_resize_selection* dout);
struct din_editor_resize_selection* data_r_editor_resize_selection(struct data* data);

// editor_start_selection
struct dout_editor_start_selection {
  struct selection selection;
  enum editor_states state;
};

struct din_editor_start_selection {
  struct dout_editor_start_selection dout;
  struct point mouse;
};

void data_w_editor_start_selection(struct data* data, struct dout_editor_start_selection* dout);
struct din_editor_start_selection* data_r_editor_start_selection(struct data* data);

// editor_cancel_paste
struct dout_editor_cancel_paste {
  enum editor_states editor_state;
};

struct din_editor_cancel_paste {
  struct dout_editor_cancel_paste dout;
};

void data_w_editor_cancel_paste(struct data* data, struct dout_editor_cancel_paste* dout);
struct din_editor_cancel_paste* data_r_editor_cancel_paste(struct data* data);

// editor_apply_paste
struct dout_editor_apply_paste {
  array_decl(struct pixel_change, pixel_changes)
};

struct din_editor_apply_paste {
  struct dout_editor_apply_paste dout;
  struct point mouse;
  int image_width;
  indexed_pixel_t const* image_pixels;
  int editor_block_size;
  int selections_n;
  struct selection const* selections;
  struct indexed_bitmap const* copies;
  struct transform image_transform;
};

void data_w_editor_apply_paste(struct data* data, struct dout_editor_apply_paste* dout);
struct din_editor_apply_paste* data_r_editor_apply_paste(struct data* data);


// editor_toggle_grid
struct dout_editor_toggle_grid {
  int grid_enabled;
};

struct din_editor_toggle_grid {
  struct dout_editor_toggle_grid dout;
  int grid_enabled;
};

void data_w_editor_toggle_grid(struct data* data, struct dout_editor_toggle_grid* dout);
struct din_editor_toggle_grid* data_r_editor_toggle_grid(struct data* data);


// editor_transform_selection
struct dout_editor_transform_selection {
  array_decl(struct indexed_bitmap, transformed_copies)
  array_decl(struct sdl_bitmap, copy_bitmaps)
};

struct din_editor_transform_selection {
  struct dout_editor_transform_selection dout;
  int copies_n;
  SDL_Renderer* sdl_renderer;
  colour_t const* palette_colours;
  struct indexed_bitmap const* copies;
};

void data_w_editor_transform_selection(struct data* data, struct dout_editor_transform_selection* dout);
struct din_editor_transform_selection* data_r_editor_transform_selection(struct data* data);

// editor_paint
struct dout_editor_paint {
  int begin_undoable_group;
  struct pixel_change pixel_change;
};

struct din_editor_paint {
  struct dout_editor_paint dout;
  struct transform image_transform;
  struct point mouse;
  int image_width;
  indexed_pixel_t const* image_pixels;
  int image_pixels_n;
  indexed_pixel_t paint_colour;
};

void data_w_editor_paint(struct data* data, struct dout_editor_paint* dout);
struct din_editor_paint* data_r_editor_paint(struct data* data);

// editor_zoom
struct dout_editor_zoom {
  struct transform image_transform;
  struct sdl_bitmap grid_bitmap;
};

struct din_editor_zoom {
  struct dout_editor_zoom dout;
  struct dimensions window_size;
  struct transform image_transform;
  int tile_size;
  SDL_Renderer* sdl_renderer;
};

void data_w_editor_zoom(struct data* data, struct dout_editor_zoom* dout);
struct din_editor_zoom* data_r_editor_zoom(struct data* data);

// sdl_get_state
struct dout_sdl_get_state {
  struct sdl_state sdl_state;
};

struct din_sdl_get_state {
  struct dout_sdl_get_state dout;
  SDL_Window* sdl_window;
};

void data_w_sdl_get_state(struct data* data, struct dout_sdl_get_state* dout);
struct din_sdl_get_state* data_r_sdl_get_state(struct data* data);

// sdl_renderer_init
struct dout_sdl_renderer_init {
  struct SDL_Renderer* renderer;
  struct SDL_Window* window;
  struct sdl_bitmap image;
  struct sdl_bitmap grid;
  struct sdl_bitmap palette;
};

struct din_sdl_renderer_init {
  struct dout_sdl_renderer_init dout;
  int image_scale;
  struct indexed_bitmap image_indexed_bitmap;
  int tile_size;
  colour_t const* palette_colours;
};

void data_w_sdl_renderer_init(struct data* data, struct dout_sdl_renderer_init* dout);
struct din_sdl_renderer_init* data_r_sdl_renderer_init(struct data* data);

// editor_pick_image_colour
struct dout_editor_pick_image_colour {
  indexed_pixel_t paint_colour;
};

struct din_editor_pick_image_colour {
  struct dout_editor_pick_image_colour dout;
  struct transform image_transform;
  indexed_pixel_t const* image_pixels;
  int image_pixels_n;
  int image_width;
  struct point mouse;
  indexed_pixel_t paint_colour;
};

void data_w_editor_pick_image_colour(struct data* data, struct dout_editor_pick_image_colour* dout);
struct din_editor_pick_image_colour* data_r_editor_pick_image_colour(struct data* data);

// editor_select_command
struct din_editor_select_command {
  enum editor_states editor_state;
  int mouse_state;
};

struct din_editor_select_command* data_r_editor_select_command(struct data* data);

// editor_save_image
struct din_editor_save_image {
  char const* image_filename;
  struct indexed_bitmap image;
};

struct din_editor_save_image* data_r_editor_save_image(struct data* data);

// editor_init
struct dout_editor_init {
  int grid_enabled;
  struct palette default_palette;
  struct indexed_bitmap image;
  char const* display_name;
  int image_scale;
};

struct din_editor_init {
  struct dout_editor_init dout;
};

void data_w_editor_init(struct data* data, struct dout_editor_init* dout);
struct din_editor_init* data_r_editor_init(struct data* data);

// editor_pan_image
struct dout_editor_pan_image {
  struct point image_translation;
};

struct din_editor_pan_image {
  struct dout_editor_pan_image dout;
  struct point image_translation;
};

void data_w_editor_pan_image(struct data* data, struct dout_editor_pan_image* dout);
struct din_editor_pan_image* data_r_editor_pan_image(struct data* data);

// 
struct data_io_pool {
  union {
    struct din_sdl_renderer_draw sdl_renderer_draw;
    struct din_sdl_renderer_init sdl_renderer_init;
    struct din_sdl_get_state sdl_get_state;

    struct din_editor_resize_selection editor_resize_selection;
    struct din_editor_cancel_paste editor_cancel_paste;
    struct din_editor_apply_paste editor_apply_paste;
    struct din_editor_pick_image_colour editor_pick_image_colour;
    struct din_editor_zoom editor_zoom;
    struct din_editor_transform_selection editor_transform_selection;
    struct din_editor_start_selection editor_start_selection;
    struct din_editor_copy_selection editor_copy_selection;
    struct din_editor_toggle_grid editor_toggle_grid;
    struct din_editor_paint editor_paint;
    struct din_editor_select_command editor_select_command;
    struct din_editor_save_image editor_save_image;
    struct din_editor_init editor_init;
    struct din_editor_pan_image editor_pan_image;
  };
};

struct data {
  struct editor editor;
  struct sdl_renderer sdl_renderer;
  struct sdl_state sdl_state;
  struct data_io_pool _io_pool;
};

void data_init(struct data* data);

#endif
