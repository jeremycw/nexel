#include <SDL2/SDL.h>
#include "data.h"
#include "sdl_renderer.h"
#include "editor.h"
#include "util.h"

static int quit = 0;

int main() {
  struct data data;
  data_init(&data);

  data_w_editor_init(&data,
    editor_init(
      data_r_editor_init(&data)));

  data_w_sdl_renderer_init(&data,
    sdl_renderer_init(
      data_r_sdl_renderer_init(&data)));

  data_w_sdl_get_state(&data,
    sdl_get_state(
      data_r_sdl_get_state(&data)));

  sdl_renderer_draw(
    data_r_sdl_renderer_draw(&data));

  SDL_Event e;

  while (!quit) {
    SDL_WaitEvent(&e);

    data_w_sdl_get_state(&data,
      sdl_get_state(
        data_r_sdl_get_state(&data)));

    do {
      enum editor_commands command = editor_select_command(&e, data_r_editor_select_command(&data));
      switch (command) {
        case EDITOR_CMD_QUIT:
          quit = 1;
          break;
        case EDITOR_CMD_START_SELECTION:
          data_w_editor_start_selection(&data,
            editor_start_selection(
              data_r_editor_start_selection(&data)));
          break;
        case EDITOR_CMD_COPY_SELECTION:
          data_w_editor_copy_selection(&data,
            editor_copy_selection(
              data_r_editor_copy_selection(&data)));
          break;
        case EDITOR_CMD_RESIZE_SELECTION:
          data_w_editor_resize_selection(&data,
            editor_resize_selection(
              data_r_editor_resize_selection(&data)));
          break;
        case EDITOR_CMD_CANCEL_PASTE:
          data_w_editor_cancel_paste(&data,
            editor_cancel_paste(
              data_r_editor_cancel_paste(&data)));
          break;
        case EDITOR_CMD_APPLY_PASTE:
          data_w_editor_apply_paste(&data,
            editor_apply_paste(
              data_r_editor_apply_paste(&data)));
          break;
        case EDITOR_CMD_TOGGLE_GRID:
          data_w_editor_toggle_grid(&data,
            editor_toggle_grid(
              data_r_editor_toggle_grid(&data)));
          break;
        case EDITOR_CMD_ROTATE_SELECTION:
          data_w_editor_transform_selection(&data,
            editor_transform_selection(rotate_clockwise_transform,
              data_r_editor_transform_selection(&data)));
          break;
        case EDITOR_CMD_HFLIP_SELECTION:
          data_w_editor_transform_selection(&data,
            editor_transform_selection(flip_horizontal_transform,
              data_r_editor_transform_selection(&data)));
          break;
        case EDITOR_CMD_VFLIP_SELECTION:
          data_w_editor_transform_selection(&data,
            editor_transform_selection(flip_vertical_transform,
              data_r_editor_transform_selection(&data)));
          break;
        case EDITOR_CMD_PALETTE_PICK_COLOUR:
          break;
        case EDITOR_CMD_SAVE:
          editor_save_image(
            data_r_editor_save_image(&data));
          break;
        case EDITOR_CMD_ZOOM_IN:
          data_w_editor_zoom(&data,
            editor_zoom(zoom_in,
              data_r_editor_zoom(&data)));
          break;
        case EDITOR_CMD_ZOOM_OUT:
          data_w_editor_zoom(&data,
            editor_zoom(zoom_out,
              data_r_editor_zoom(&data)));
          break;
        case EDITOR_CMD_UNDO:
          break;
        case EDITOR_CMD_REDO:
          break;
        case EDITOR_CMD_START_PAINT:
          data_w_editor_paint(&data,
            editor_paint(EDITOR_BEGIN_UNDO_GROUP,
              data_r_editor_paint(&data)));
          break;
        case EDITOR_CMD_CONTINUE_PAINT:
          data_w_editor_paint(&data,
            editor_paint(EDITOR_CONTINUE_UNDO_GROUP,
              data_r_editor_paint(&data)));
          break;
        case EDITOR_CMD_PAN:
          data_w_editor_pan_image(&data,
            editor_pan_image(
              data_r_editor_pan_image(&data), e.wheel.x, e.wheel.y));
          break;
        case EDITOR_CMD_IMAGE_PICK_COLOUR:
          data_w_editor_pick_image_colour(&data,
            editor_pick_image_colour(
              data_r_editor_pick_image_colour(&data)));
          break;
         case EDITOR_CMD_NOP:
           break;
         default:
           break;
      }
    } while (SDL_PollEvent(&e));

    sdl_renderer_draw(
      data_r_sdl_renderer_draw(&data));
  }

  // SDL_DestroyWindow(win);
  SDL_Quit();
}
