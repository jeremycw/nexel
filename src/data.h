#include <stdint.h>
#include <SDL2/SDL.h>

#ifndef DATA_H
#define DATA_H

#define array_push(buf, a) \
  if (buf##_meta.size == buf##_meta.capacity) { \
    buf##_meta.capacity *= 2; \
    buf = realloc(buf, buf##_meta.capacity * buf##_meta.type_size); \
  } \
  buf[buf##_meta.size] = a; \
  buf##_meta.size++;

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
  uint32_t* colours;
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
  struct sdl_bitmap sdl_bitmap;
};

struct indexed_bitmap_file {
  char const* filename;
  struct indexed_bitmap bitmap;
};

struct reverse_change {
  array_decl(struct pixel, pixels)
};

struct palette_selector {
  int current_palette;
  array_decl(struct palette, palettes)
};

struct transform {
  struct point translation;
  int scale;
};

struct editor {
  int draw_color_index;
  struct transform image_transform;
  char const* display_name;
  struct indexed_bitmap image_bitmap;
  array_decl(struct pixel_change, undos)
  array_decl(struct selection, selections)
  array_decl(struct indexed_bitmap, copies)
  enum editor_states state;
  struct palette_selector palette_selector;
};

struct sdl_state {
  struct point mouse;
  int mouse_state;
  struct dimensions window;
};

struct sdl_renderer {
  SDL_Window* window;
  SDL_Renderer* renderer;
};

// write objects

struct dout_editor_start_selection {
  struct rect selection;
  enum editor_states state;
};

struct dout_editor_copy_selection {
  enum editor_states editor_state;
  array_decl(struct indexed_bitmap, copies)
};

struct dout_editor_resize_selection {
  struct point end_selection;
};

struct dout_editor_cancel_paste {
  struct array_meta copies_meta;
};

// read objects

struct din_sdl_renderer_draw {
  uint32_t background_colour;
  int grid_enabled;
  int scale;

  struct sdl_renderer sdl_renderer;

  SDL_Texture const* grid_texture;

  array_decl(SDL_Texture const* const, textures)
  array_decl(struct rect const, destinations)

  uint32_t selection_colour;

  int selections_n;
  struct selection const* selections;
};

struct din_editor_copy_selection {
  struct dout_editor_copy_selection dout;
  SDL_Renderer* sdl_renderer;
  uint32_t const* palette_colours;
  int image_scale;
  struct point image_translation;
  int selections_n;
  struct selection const* selections;
  int image_pixels_n;
  int image_width;
  indexed_pixel_t const* image_pixels;
};

struct din_editor_start_selection {
  struct dout_editor_start_selection dout;
  int mouse_x;
  int mouse_y;
};

struct din_editor_select_command {
  enum editor_states editor_state;
  int mouse_state;
};

struct din_editor_resize_selection {
  struct dout_editor_resize_selection dout;
  struct point mouse;
};

struct din_editor_cancel_paste {
  struct dout_editor_cancel_paste dout;
  array_decl(struct indexed_bitmap const, copies)
};

struct read_write_object_pool {
  // read
  struct din_sdl_renderer_draw din_sdl_renderer_draw;
  struct din_editor_resize_selection din_editor_resize_selection;
};

struct data {
  struct editor editor;
  struct sdl_renderer sdl_renderer;
  struct sdl_state sdl_state;
  struct read_write_object_pool _pool;
};

// editor_apply_paste
struct dout_editor_apply_paste {
  array_decl(struct pixel_change, pixel_changes)
};

struct din_editor_apply_paste {
  struct dout_editor_apply_paste dout;
  struct point mouse;
  int image_scale;
  int image_width;
  indexed_pixel_t const* image_pixels;
  int editor_block_size;
  int selections_n;
  struct selection const* selections;
  struct indexed_bitmap const* copies;
  struct point image_translation;
};

void data_w_editor_apply_paste(struct data* data, struct dout_editor_apply_paste dout);
struct din_editor_apply_paste data_r_editor_apply_paste(struct data* data);


// editor_toggle_grid
struct dout_editor_toggle_grid {
  int grid_enabled;
};

struct din_editor_toggle_grid {
  struct dout_editor_toggle_grid dout;
  int grid_enabled;
};

void data_w_editor_toggle_grid(struct data* data, struct dout_editor_toggle_grid dout);
struct din_editor_toggle_grid data_r_editor_toggle_grid(struct data const* data);


// editor_transform_selection
struct dout_editor_transform_selection {
  array_decl(struct indexed_bitmap, transformed_copies)
};

struct din_editor_transform_selection {
  struct dout_editor_transform_selection dout;
  int copies_n;
  SDL_Renderer* sdl_renderer;
  uint32_t const* palette_colours;
  struct indexed_bitmap const* copies;
};

void data_w_editor_transform_selection(struct data* data, struct dout_editor_transform_selection dout);
struct din_editor_transform_selection data_r_editor_transform_selection(struct data const* data);

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

void data_w_editor_paint(struct data* data, struct dout_editor_paint dout);
struct din_editor_paint data_r_editor_paint(struct data const* data);

// editor_zoom
struct dout_editor_zoom {
  struct transform image_transform;
};

struct din_editor_zoom {
  struct dout_editor_zoom dout;
  struct dimensions window_size;
  struct transform image_transform;
};

void data_w_editor_zoom(struct data* data, struct dout_editor_zoom dout);
struct din_editor_zoom data_r_editor_zoom(struct data const* data);

// sdl_get_state
struct dout_sdl_get_state {
  struct sdl_state sdl_state;
};

struct din_sdl_get_state {
  struct dout_sdl_get_state dout;
  SDL_Window* sdl_window;
};

void data_w_sdl_get_state(struct data* data, struct dout_sdl_get_state dout);
struct din_sdl_get_state data_r_sdl_get_state(struct data const* data);

// sdl_renderer_init
struct dout_sdl_renderer_init {
  struct sdl_renderer sdl_renderer;
};

struct din_sdl_renderer_init {
  struct dout_sdl_renderer_init dout;
};

void data_w_sdl_renderer_init(struct data* data, struct dout_sdl_renderer_init dout);
struct din_sdl_renderer_init data_r_sdl_renderer_init(struct data const* data);

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

void data_w_editor_pick_image_colour(struct data* data, struct dout_editor_pick_image_colour dout);
struct din_editor_pick_image_colour data_r_editor_pick_image_colour(struct data const* data);

void data_init(struct data* data);

void data_w_sdl_renderer(struct data* data, struct sdl_renderer sdl_renderer);
void data_w_editor_start_selection(struct data* data, struct dout_editor_start_selection dout);
void data_w_editor_resize_selection(struct data* data, struct dout_editor_resize_selection dout);
void data_w_editor_copy_selection(struct data* data, struct dout_editor_copy_selection dout);
void data_w_editor_cancel_paste(struct data* data, struct dout_editor_cancel_paste dout);

struct din_editor_select_command data_r_editor_select_command(struct data* data);
struct din_editor_copy_selection data_r_editor_copy_selection(struct data* data);
struct din_sdl_renderer_draw* data_r_sdl_renderer_draw(struct data* data);
struct din_editor_resize_selection data_r_editor_resize_selection(struct data* data);
struct din_editor_start_selection data_r_editor_start_selection(struct data* data);
struct din_editor_cancel_paste data_r_editor_cancel_paste(struct data* data);

#endif
