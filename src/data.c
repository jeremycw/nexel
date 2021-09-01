#include <string.h>
#include "data.h"

void data_init_palette_selector(struct palette_selector* palette_selector) {
  palette_selector->current_palette = 0;
  array_init(palette_selector->palettes, sizeof(struct palette), 1);
}

void data_init_sdl_renderer(struct sdl_renderer* sdl_renderer) {
  memset(sdl_renderer, 0, sizeof(struct sdl_renderer));
  array_init(sdl_renderer->copy_bitmaps, sizeof(struct sdl_bitmap), 1)
}

void data_init_editor(struct editor* editor) {
  editor->image_transform.scale = 1;
  editor->paint_colour = 0;
  editor->display_name = "Untitled*";
  memset(&editor->image_transform.translation, 0, sizeof(struct point));
  array_init(editor->undos, sizeof(struct pixel_change_group), 16);
  array_init(editor->selections, sizeof(struct rect), 1);
  array_init(editor->copies, sizeof(struct indexed_bitmap), 1);
  data_init_palette_selector(&editor->palette_selector);
}

void data_init(struct data* data) {
  data_init_editor(&data->editor);
  data_init_sdl_renderer(&data->sdl_renderer);
}

void data_w_editor_cancel_paste(struct data* data, struct dout_editor_cancel_paste* dout) {
  // TODO
  (void)data;
  (void)dout;
}

struct din_editor_cancel_paste* data_r_editor_cancel_paste(struct data* data) {
  struct din_editor_cancel_paste* din = &data->_io_pool.editor_cancel_paste;
  // TODO
  return din;
}

void data_w_editor_apply_paste(struct data* data, struct dout_editor_apply_paste* dout) {
  // TODO
  (void)data;
  (void)dout;
}

struct din_editor_apply_paste* data_r_editor_apply_paste(struct data* data) {
  struct din_editor_apply_paste* din = &data->_io_pool.editor_apply_paste;
  // TODO
  return din;
}

void data_w_editor_toggle_grid(struct data* data, struct dout_editor_toggle_grid* dout) {
  // TODO
  (void)data;
  (void)dout;
}

struct din_editor_toggle_grid* data_r_editor_toggle_grid(struct data* data) {
  struct din_editor_toggle_grid* din = &data->_io_pool.editor_toggle_grid;
  // TODO
  return din;
}

void data_w_editor_transform_selection(struct data* data, struct dout_editor_transform_selection* dout) {
  // TODO
  (void)data;
  (void)dout;
}

struct din_editor_transform_selection* data_r_editor_transform_selection(struct data* data) {
  struct din_editor_transform_selection* din = &data->_io_pool.editor_transform_selection;
  // TODO
  return din;
}

void data_w_editor_paint(struct data* data, struct dout_editor_paint* dout) {
  // TODO
  (void)data;
  (void)dout;
}

struct din_editor_paint* data_r_editor_paint(struct data* data) {
  struct din_editor_paint* din = &data->_io_pool.editor_paint;
  // TODO
  return din;
}

void data_w_editor_zoom(struct data* data, struct dout_editor_zoom* dout) {
  // TODO
  (void)data;
  (void)dout;
}

struct din_editor_zoom* data_r_editor_zoom(struct data* data) {
  struct din_editor_zoom* din = &data->_io_pool.editor_zoom;
  // TODO
  return din;
}

void data_w_sdl_get_state(struct data* data, struct dout_sdl_get_state* dout) {
  // TODO
  (void)data;
  (void)dout;
}

struct din_sdl_get_state* data_r_sdl_get_state(struct data* data) {
  struct din_sdl_get_state* din = &data->_io_pool.sdl_get_state;
  // TODO
  return din;
}

void data_w_sdl_renderer_init(struct data* data, struct dout_sdl_renderer_init* dout) {
  // TODO
  (void)data;
  (void)dout;
}

struct din_sdl_renderer_init* data_r_sdl_renderer_init(struct data* data) {
  struct din_sdl_renderer_init* din = &data->_io_pool.sdl_renderer_init;
  // TODO
  return din;
}

void data_w_editor_pick_image_colour(struct data* data, struct dout_editor_pick_image_colour* dout) {
  // TODO
  (void)data;
  (void)dout;
}

struct din_editor_pick_image_colour* data_r_editor_pick_image_colour(struct data* data) {
  struct din_editor_pick_image_colour* din = &data->_io_pool.editor_pick_image_colour;
  // TODO
  return din;
}

void data_w_editor_copy_selection(struct data* data, struct dout_editor_copy_selection* dout) {
  // TODO
  (void)data;
  (void)dout;
}

struct din_editor_copy_selection* data_r_editor_copy_selection(struct data* data) {
  struct din_editor_copy_selection* din = &data->_io_pool.editor_copy_selection;
  // TODO
  return din;
}

void data_w_editor_resize_selection(struct data* data, struct dout_editor_resize_selection* dout) {
  // TODO
  (void)data;
  (void)dout;
}

struct din_editor_resize_selection* data_r_editor_resize_selection(struct data* data) {
  struct din_editor_resize_selection* din = &data->_io_pool.editor_resize_selection;
  // TODO
  return din;
}

void data_w_editor_start_selection(struct data* data, struct dout_editor_start_selection* dout) {
  // TODO
  (void)data;
  (void)dout;
}

struct din_editor_start_selection* data_r_editor_start_selection(struct data* data) {
  struct din_editor_start_selection* din = &data->_io_pool.editor_start_selection;
  // TODO
  return din;
}

struct din_sdl_renderer_draw* data_r_sdl_renderer_draw(struct data* data) {
  struct din_sdl_renderer_draw* din = &data->_io_pool.sdl_renderer_draw;
  // TODO
  return din;
}

struct din_editor_select_command* data_r_editor_select_command(struct data* data) {
  struct din_editor_select_command* din = &data->_io_pool.editor_select_command;
  // TODO
  return din;
}
