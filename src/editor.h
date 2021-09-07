#ifndef EDITOR_H
#define EDITOR_H

#include <SDL2/SDL.h>
#include "data.h"

#define EDITOR_BEGIN_UNDO_GROUP 1
#define EDITOR_CONTINUE_UNDO_GROUP 0
#define DEFAULT_IMAGE_SIZE 128

static uint8_t nes_palette_colours[] = {
  124,124,124,255,  0,0,252,255,      0,0,188,255,      68,40,188,255,
  148,0,132,255,    168,0,32,255,     168,16,0,255,     136,20,0,255,
  80,48,0,255,      0,120,0,255,      0,104,0,255,      0,88,0,255,
  0,64,88,255,      0,0,0,255,        0,0,0,255,        0,0,0,255,
  188,188,188,255,  0,120,248,255,    0,88,248,255,     104,68,252,255,
  216,0,204,255,    228,0,88,255,     248,56,0,255,     228,92,16,255,
  172,124,0,255,    0,184,0,255,      0,168,0,255,      0,168,68,255,
  0,136,136,255,    0,0,0,255,        0,0,0,255,        0,0,0,255,
  248,248,248,255,  60,188,252,255,   104,136,252,255,  152,120,248,255,
  248,120,248,255,  248,88,152,255,   248,120,88,255,   252,160,68,255,
  248,184,0,255,    184,248,24,255,   88,216,84,255,    88,248,152,255,
  0,232,216,255,    120,120,120,255,  0,0,0,255,        0,0,0,255,
  252,252,252,255,  164,228,252,255,  184,184,248,255,  216,184,248,255,
  248,184,248,255,  248,164,192,255,  240,208,176,255,  252,224,168,255,
  248,216,120,255,  216,248,120,255,  184,248,184,255,  184,248,216,255,
  0,252,252,255,    216,216,216,255,  0,0,0,255,        0,0,0,255
};

enum editor_commands {
  EDITOR_CMD_NOP,
  EDITOR_CMD_TOGGLE_GRID,
  EDITOR_CMD_START_PAINT,
  EDITOR_CMD_CONTINUE_PAINT,
  EDITOR_CMD_PALETTE_PICK_COLOUR,
  EDITOR_CMD_IMAGE_PICK_COLOUR,
  EDITOR_CMD_START_SELECTION,
  EDITOR_CMD_COPY_SELECTION,
  EDITOR_CMD_RESIZE_SELECTION,
  EDITOR_CMD_APPLY_PASTE,
  EDITOR_CMD_CANCEL_PASTE,
  EDITOR_CMD_ROTATE_SELECTION,
  EDITOR_CMD_HFLIP_SELECTION,
  EDITOR_CMD_VFLIP_SELECTION,
  EDITOR_CMD_SAVE,
  EDITOR_CMD_ZOOM_IN,
  EDITOR_CMD_ZOOM_OUT,
  EDITOR_CMD_UNDO,
  EDITOR_CMD_REDO,
  EDITOR_CMD_PAN,
  EDITOR_CMD_QUIT
};

typedef void (*indexed_pixel_transform_t)(indexed_pixel_t* in, int w, int h, int* wout, int* hout, indexed_pixel_t* out);
typedef struct transform (*zoom_fn_t)(struct dimensions window, struct transform transform);

enum editor_commands editor_select_command(SDL_Event* e, struct din_editor_select_command* din);
struct dout_editor_start_selection* editor_start_selection(struct din_editor_start_selection* din);
struct dout_editor_resize_selection* editor_resize_selection(struct din_editor_resize_selection* din);
struct dout_editor_copy_selection* editor_copy_selection(struct din_editor_copy_selection* din);
struct dout_editor_cancel_paste* editor_cancel_paste(struct din_editor_cancel_paste* din);
struct dout_editor_apply_paste* editor_apply_paste(struct din_editor_apply_paste* din);
struct dout_editor_toggle_grid* editor_toggle_grid(struct din_editor_toggle_grid* din);
struct dout_editor_transform_selection* editor_transform_selection(indexed_pixel_transform_t transform, struct din_editor_transform_selection* din);
struct dout_editor_paint* editor_paint(int begin_undoable_group, struct din_editor_paint* din);
struct dout_editor_zoom* editor_zoom(zoom_fn_t zoom_fn, struct din_editor_zoom* din);
struct dout_editor_pick_image_colour* editor_pick_image_colour(struct din_editor_pick_image_colour* din);
void editor_save_image(struct din_editor_save_image* din);
struct dout_editor_init* editor_init(struct din_editor_init* din);


#endif
