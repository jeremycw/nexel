#include <string.h>
#include "data.h"

void data_init_palette_selector(struct palette_selector* palette_selector) {
  palette_selector->current_palette = 0;
  array_init(palette_selector->palettes, sizeof(struct palette), 1);
}

void data_init_sdl_renderer(struct sdl_renderer* sdl_renderer) {
  sdl_renderer->window = NULL;
  sdl_renderer->renderer = NULL;
}

void data_init_din_sdl_renderer_draw(struct din_sdl_renderer_draw* din_sdl_renderer_draw) {
  array_init(din_sdl_renderer_draw->textures, sizeof(SDL_Texture*), 2);
  array_init(din_sdl_renderer_draw->destinations, sizeof(struct rect), 2);
}

void data_init_read_object_pool(struct read_write_object_pool* read_write_object_pool) {
  data_init_din_sdl_renderer_draw(&read_write_object_pool->din_sdl_renderer_draw);
}

void data_init_editor(struct editor* editor) {
  editor->image_transform.scale = 1;
  editor->draw_color_index = 0;
  editor->display_name = "Untitled*";
  memset(&editor->image_transform.translation, 0, sizeof(struct point));
  array_init(editor->undos, sizeof(struct reverse_change), 16);
  array_init(editor->redos, sizeof(struct reverse_change), 16);
  array_init(editor->selections, sizeof(struct rect), 1);
  array_init(editor->copies, sizeof(struct indexed_bitmap), 1);
  data_init_palette_selector(&editor->palette_selector);
}

void data_init(struct data* data) {
  data_init_editor(&data->editor);
  data_init_sdl_renderer(&data->sdl_renderer);
}

void data_w_editor_cancel_paste(struct data* data, struct dout_editor_cancel_paste dwo) {
  // TODO
}

struct din_editor_cancel_paste data_r_editor_cancel_paste(struct data* data) {
  struct din_editor_cancel_paste dro;
  // TODO
  return dro;
}

void data_w_editor_apply_paste(struct data* data, struct dout_editor_apply_paste dwo) {
  // TODO
}

struct din_editor_apply_paste data_r_editor_apply_paste(struct data* data) {
  struct din_editor_apply_paste dro;
  // TODO
  return dro;
}

void data_w_editor_toggle_grid(struct data* data, struct dout_editor_toggle_grid dout) {
  // TODO
}

struct din_editor_toggle_grid data_r_editor_toggle_grid(struct data const* data) {
  struct din_editor_toggle_grid din;
  // TODO
  return din;
}

void data_w_editor_transform_selection(struct data* data, struct dout_editor_transform_selection dout) {
  // TODO
}

struct din_editor_transform_selection data_r_editor_transform_selection(struct data const* data) {
  struct din_editor_transform_selection din;
  // TODO
  return din;
}

void data_w_editor_paint(struct data* data, struct dout_editor_paint dout) {
  // TODO
}

struct din_editor_paint data_r_editor_paint(struct data const* data) {
  struct din_editor_paint din;
  // TODO
  return din;
}

void data_w_editor_zoom(struct data* data, struct dout_editor_zoom dout) {
  // TODO
}

struct din_editor_zoom data_r_editor_zoom(struct data const* data) {
  struct din_editor_zoom din;
  // TODO
  return din;
}

void data_w_sdl_get_state(struct data* data, struct dout_sdl_get_state dout) {
  // TODO
}

struct din_sdl_get_state data_r_sdl_get_state(struct data const* data) {
  struct din_sdl_get_state din;
  // TODO
  return din;
}

void data_w_sdl_renderer_init(struct data* data, struct dout_sdl_renderer_init dout) {
  // TODO
}

struct din_sdl_renderer_init data_r_sdl_renderer_init(struct data const* data) {
  struct din_sdl_renderer_init din;
  // TODO
  return din;
}

void data_w_editor_pick_image_colour(struct data* data, struct dout_editor_pick_image_colour dout) {
  // TODO
}

struct din_editor_pick_image_colour data_r_editor_pick_image_colour(struct data const* data) {
  struct din_editor_pick_image_colour din;
  // TODO
  return din;
}

