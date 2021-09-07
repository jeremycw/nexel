#include <string.h>
#include "data.h"
#include "debugbreak.h"

void data_init(struct data* data) {
  memset(data, 0, sizeof(struct data));
  array_init(data->editor.undos, sizeof(struct pixel_change_group), 16);
  array_init(data->editor.selections, sizeof(struct selection), 1);
  array_init(data->editor.copies, sizeof(struct indexed_bitmap), 1);
  array_init(data->editor.palette_selector.palettes, sizeof(struct palette), 1);
  array_init(data->sdl_renderer.copy_bitmaps, sizeof(struct sdl_bitmap), 1)
}

void free_copy_data(struct data* data) {
  if (array_len(data->editor.copies) > 0) {
    free(array_first(data->editor.copies).pixels);
  }
  array_clear(data->editor.copies);

  void* buffer = NULL;
  if (array_len(data->sdl_renderer.copy_bitmaps) > 0) {
    buffer = array_first(data->sdl_renderer.copy_bitmaps).surface->pixels;
  }
  array_itr(data->sdl_renderer.copy_bitmaps, i) {
    SDL_FreeSurface(data->sdl_renderer.copy_bitmaps[i].surface);
    SDL_DestroyTexture(data->sdl_renderer.copy_bitmaps[i].texture);
  }
  if (buffer) free(buffer);
  array_clear(data->sdl_renderer.copy_bitmaps);
}

void data_w_editor_cancel_paste(struct data* data, struct dout_editor_cancel_paste* dout) {
  free_copy_data(data);
  data->editor.state = dout->editor_state;
}

struct din_editor_cancel_paste* data_r_editor_cancel_paste(struct data* data) {
  return &data->_io_pool.editor_cancel_paste;
}

void data_w_editor_apply_paste(struct data* data, struct dout_editor_apply_paste* dout) {
  array_itr(dout->pixel_changes, i) {
    data->editor.image_bitmap.pixels[dout->pixel_changes[i].buffer_index] = dout->pixel_changes[i].to;
  }
  struct pixel_change_group pcg = {
    .pixel_changes = dout->pixel_changes,
    .pixel_changes_meta = dout->pixel_changes_meta
  };
  array_push(data->editor.undos, pcg);

  // XXX rebuild sdl_bitmap for image
}

struct din_editor_apply_paste* data_r_editor_apply_paste(struct data* data) {
  struct din_editor_apply_paste* din = &data->_io_pool.editor_apply_paste;
  din->mouse = data->sdl_state.mouse;
  din->image_width = data->editor.image_bitmap.width;
  din->image_pixels = data->editor.image_bitmap.pixels;
  din->editor_block_size = EDITOR_TILE_SIZE;
  din->selections_n = array_len(data->editor.selections);
  din->selections = data->editor.selections;
  din->copies = data->editor.copies;
  din->image_transform = data->editor.image_transform;
  return din;
}

void data_w_editor_toggle_grid(struct data* data, struct dout_editor_toggle_grid* dout) {
  data->editor.grid_enabled = dout->grid_enabled;
}

struct din_editor_toggle_grid* data_r_editor_toggle_grid(struct data* data) {
  return &data->_io_pool.editor_toggle_grid;
}

void data_w_editor_transform_selection(struct data* data, struct dout_editor_transform_selection* dout) {
  free_copy_data(data);

  array_free(data->editor.copies);
  array_free(data->sdl_renderer.copy_bitmaps);

  array_assign(data->editor.copies, dout->transformed_copies);
  array_assign(data->sdl_renderer.copy_bitmaps, dout->copy_bitmaps);
}

struct din_editor_transform_selection* data_r_editor_transform_selection(struct data* data) {
  struct din_editor_transform_selection* din = &data->_io_pool.editor_transform_selection;
  din->copies_n = array_len(data->editor.copies);
  din->copies = data->editor.copies;
  din->sdl_renderer = data->sdl_renderer.renderer;
  int index = data->editor.palette_selector.current_palette;
  din->palette_colours = data->editor.palette_selector.palettes[index].colours;
  return din;
}

void data_w_editor_paint(struct data* data, struct dout_editor_paint* dout) {
  if (dout->begin_undoable_group) {
    struct pixel_change_group pcg;
    array_init(pcg.pixel_changes, sizeof(struct pixel_change), 4);
    array_push(pcg.pixel_changes, dout->pixel_change);
    array_push(data->editor.undos, pcg);
  } else {
    array_push(array_last(data->editor.undos).pixel_changes, dout->pixel_change);
  }
}

struct din_editor_paint* data_r_editor_paint(struct data* data) {
  struct din_editor_paint* din = &data->_io_pool.editor_paint;
  din->paint_colour = data->editor.paint_colour;
  din->image_pixels = data->editor.image_bitmap.pixels;
  din->image_pixels_n = data->editor.image_bitmap.width * data->editor.image_bitmap.height;
  din->image_transform = data->editor.image_transform;
  din->image_width = data->editor.image_bitmap.width;
  din->mouse = data->sdl_state.mouse;
  return din;
}

void data_w_editor_zoom(struct data* data, struct dout_editor_zoom* dout) {
  struct sdl_bitmap bitmap = data->sdl_renderer.grid;
  void* buffer = bitmap.surface->pixels;
  SDL_FreeSurface(bitmap.surface);
  SDL_DestroyTexture(bitmap.texture);
  free(buffer);
  data->sdl_renderer.grid = dout->grid_bitmap;
  data->editor.image_transform = dout->image_transform;
}

struct din_editor_zoom* data_r_editor_zoom(struct data* data) {
  struct din_editor_zoom* din = &data->_io_pool.editor_zoom;
  din->image_transform = data->editor.image_transform;
  din->sdl_renderer = data->sdl_renderer.renderer;
  din->tile_size = EDITOR_TILE_SIZE;
  din->window_size = data->sdl_state.window;
  return din;
}

void data_w_sdl_get_state(struct data* data, struct dout_sdl_get_state* dout) {
  data->sdl_state = dout->sdl_state;
}

struct din_sdl_get_state* data_r_sdl_get_state(struct data* data) {
  struct din_sdl_get_state* din = &data->_io_pool.sdl_get_state;
  din->sdl_window = data->sdl_renderer.window;
  return din;
}

void data_w_sdl_renderer_init(struct data* data, struct dout_sdl_renderer_init* dout) {
  data->sdl_renderer.renderer = dout->renderer;
  data->sdl_renderer.window = dout->window;
  data->sdl_renderer.grid = dout->grid;
  data->sdl_renderer.image = dout->image;
  data->sdl_renderer.palette = dout->palette;
}

struct din_sdl_renderer_init* data_r_sdl_renderer_init(struct data* data) {
  struct din_sdl_renderer_init* din = &data->_io_pool.sdl_renderer_init;
  din->image_indexed_bitmap = data->editor.image_bitmap;
  din->image_scale = data->editor.image_transform.scale;
  int index = data->editor.palette_selector.current_palette;
  din->palette_colours = data->editor.palette_selector.palettes[index].colours;
  din->tile_size = EDITOR_TILE_SIZE;
  return din;
}

void data_w_editor_pick_image_colour(struct data* data, struct dout_editor_pick_image_colour* dout) {
  data->editor.paint_colour = dout->paint_colour;
}

struct din_editor_pick_image_colour* data_r_editor_pick_image_colour(struct data* data) {
  struct din_editor_pick_image_colour* din = &data->_io_pool.editor_pick_image_colour;
  int n = data->editor.image_bitmap.width * data->editor.image_bitmap.height;
  din->image_pixels = data->editor.image_bitmap.pixels;
  din->image_pixels_n = n;
  din->image_transform = data->editor.image_transform;
  din->image_width = data->editor.image_bitmap.width;
  din->paint_colour = data->editor.paint_colour;
  din->mouse = data->sdl_state.mouse;
  return din;
}

void data_w_editor_copy_selection(struct data* data, struct dout_editor_copy_selection* dout) {
  free_copy_data(data);

  array_free(data->editor.copies);
  array_free(data->sdl_renderer.copy_bitmaps);

  array_assign(data->editor.copies, dout->copies);
  array_assign(data->sdl_renderer.copy_bitmaps, dout->copy_bitmaps);

  array_clear(data->editor.selections);

  data->editor.state = dout->editor_state;
}

struct din_editor_copy_selection* data_r_editor_copy_selection(struct data* data) {
  struct din_editor_copy_selection* din = &data->_io_pool.editor_copy_selection;
  din->selections = data->editor.selections;
  din->selections_n = array_len(data->editor.selections);
  din->image_pixels = data->editor.image_bitmap.pixels;
  int n = data->editor.image_bitmap.width * data->editor.image_bitmap.height;
  din->image_pixels_n = n;
  din->image_transform = data->editor.image_transform;
  din->image_width = data->editor.image_bitmap.width;
  int index = data->editor.palette_selector.current_palette;
  din->palette_colours = data->editor.palette_selector.palettes[index].colours;
  din->sdl_renderer = data->sdl_renderer.renderer;
  return din;
}

void data_w_editor_resize_selection(struct data* data, struct dout_editor_resize_selection* dout) {
  array_last(data->editor.selections).end = dout->end_selection;
}

struct din_editor_resize_selection* data_r_editor_resize_selection(struct data* data) {
  struct din_editor_resize_selection* din = &data->_io_pool.editor_resize_selection;
  din->mouse = data->sdl_state.mouse;
  return din;
}

void data_w_editor_start_selection(struct data* data, struct dout_editor_start_selection* dout) {
  array_push(data->editor.selections, dout->selection);
  data->editor.state = dout->state;
}

struct din_editor_start_selection* data_r_editor_start_selection(struct data* data) {
  struct din_editor_start_selection* din = &data->_io_pool.editor_start_selection;
  din->mouse = data->sdl_state.mouse;
  return din;
}

struct din_sdl_renderer_draw* data_r_sdl_renderer_draw(struct data* data) {
  struct din_sdl_renderer_draw* din = &data->_io_pool.sdl_renderer_draw;
  din->sdl_renderer = data->sdl_renderer.renderer;
  din->background_colour.colour = BACKGROUND_COLOUR;
  din->selection_colour.colour = SELECTION_COLOUR;
  din->selections = data->editor.selections;
  din->selections_n = array_len(data->editor.selections);
  din->grid_enabled = data->editor.grid_enabled;
  din->image_sdl_bitmap = data->sdl_renderer.image;
  din->image_transform = data->editor.image_transform;
  din->grid_texture = data->sdl_renderer.grid.texture;
  din->tile_size = EDITOR_TILE_SIZE;
  return din;
}

struct din_editor_select_command* data_r_editor_select_command(struct data* data) {
  struct din_editor_select_command* din = &data->_io_pool.editor_select_command;
  din->editor_state = data->editor.state;
  din->mouse_state = data->sdl_state.mouse_state;
  return din;
}

struct din_editor_save_image* data_r_editor_save_image(struct data* data) {
  struct din_editor_save_image* din = &data->_io_pool.editor_save_image;
  // TODO
  return din;
}

void data_w_editor_init(struct data* data, struct dout_editor_init* dout) {
  array_push(data->editor.palette_selector.palettes, dout->default_palette);
  data->editor.display_name = dout->display_name;
  data->editor.grid_enabled = dout->grid_enabled;
  data->editor.image_bitmap = dout->image;
  data->editor.image_transform.scale = dout->image_scale;
}

struct din_editor_init* data_r_editor_init(struct data* data) {
  return &data->_io_pool.editor_init;
}

