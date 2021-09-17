#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "editor.h"
#include "util.h"

enum editor_commands editor_select_command(SDL_Event* e, struct din_editor_select_command* din) {
  switch (e->type) {
    case SDL_QUIT:
      return EDITOR_CMD_QUIT;
      break;
    case SDL_KEYDOWN:
      if (e->key.keysym.sym == SDLK_q) return EDITOR_CMD_QUIT;
      if (din->editor_state == EDITOR_STATE_PASTING) {
        if (e->key.keysym.sym == SDLK_r) return EDITOR_CMD_ROTATE_SELECTION;
        if (e->key.keysym.sym == SDLK_ESCAPE) return EDITOR_CMD_CANCEL_PASTE;
        if (e->key.keysym.sym == SDLK_h) return EDITOR_CMD_HFLIP_SELECTION;
        if (e->key.keysym.sym == SDLK_v) return EDITOR_CMD_VFLIP_SELECTION;
      }
      if (e->key.keysym.sym == SDLK_s) return EDITOR_CMD_SAVE;
      if (e->key.keysym.sym == SDLK_EQUALS) return EDITOR_CMD_ZOOM_IN;
      if (e->key.keysym.sym == SDLK_MINUS) return EDITOR_CMD_ZOOM_OUT;
      if (e->key.keysym.sym == SDLK_u) return EDITOR_CMD_UNDO;
      if (e->key.keysym.sym == SDLK_g) return EDITOR_CMD_TOGGLE_GRID;
      break;
    case SDL_MOUSEBUTTONDOWN:
      if (e->button.button == SDL_BUTTON_LEFT) {
        // XXX
        //if (in_bounds_of(COLOUR_PICKER_ID, e->button.x, e->button.y)) {
        //  uint32_t colour = colour_picker_get_colour(e->button.x, e->button.y);
        //  image_set_paint_color(colour);
        //  return EDITOR_CMD_PALETTE_PICK_COLOUR;
        //}
        return EDITOR_CMD_START_PAINT;
      } else if (e->button.button == SDL_BUTTON_RIGHT) {
        return EDITOR_CMD_START_SELECTION;
      }
      if (din->editor_state == EDITOR_STATE_PASTING) {
        return EDITOR_CMD_APPLY_PASTE;
      }
      break;
    case SDL_MOUSEBUTTONUP:
      if (din->editor_state == EDITOR_STATE_SELECTING) {
        return EDITOR_CMD_COPY_SELECTION;
      }
      break;
    case SDL_MOUSEMOTION:
      if (din->mouse_state & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
        return EDITOR_CMD_RESIZE_SELECTION;
      } else if (din->mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        return EDITOR_CMD_CONTINUE_PAINT;
      }
      break;
    case SDL_MOUSEWHEEL:
      return EDITOR_CMD_PAN;
      break;
  }
  return EDITOR_CMD_NOP;
}

struct dout_editor_cancel_paste* editor_cancel_paste(struct din_editor_cancel_paste* din) {
  din->dout.editor_state = EDITOR_STATE_NONE;
  return &din->dout;
}

struct dout_editor_start_selection* editor_start_selection(struct din_editor_start_selection* din) {
  din->dout.state = EDITOR_STATE_SELECTING;
  din->dout.selection.start = din->mouse;
  din->dout.selection.end = din->mouse;
  return &din->dout;
}

struct dout_editor_resize_selection* editor_resize_selection(struct din_editor_resize_selection* din) {
  din->dout.end_selection = din->mouse;
  return &din->dout;
}

struct dout_editor_copy_selection* editor_copy_selection(struct din_editor_copy_selection* din) {
  struct rect* bitmap_rects = malloc(sizeof(struct rect) * din->selections_n);

  size_t pixel_count = 0;
  for (int i = 0; i < din->selections_n; i++) {
    // convert the start point end point selection from screen coords into pixel rect
    bitmap_rects[i] = screen_selection_to_bitmap_rect(din->selections[i], din->image_transform);
    pixel_count += bitmap_rects[i].w * bitmap_rects[i].h;
  }

  // NOTE allocate space for all copied selections. This strategy breaks if it
  // becomes possible to add more active copies at a later time. Could possibly
  // be solved by reallocing if that functionality is ever needed.
  indexed_pixel_t* buffer = malloc(sizeof(indexed_pixel_t) * pixel_count);

  array_init(din->dout.copies, sizeof(struct indexed_bitmap), din->selections_n);

  indexed_pixel_t* copied_pixels = buffer;
  for (int i = 0; i < din->selections_n; i++) {
    pixel_loop(
      bitmap_rects[i].x, // source x
      bitmap_rects[i].y, // source y
      din->image_width,  // source pitch
      0,                 // destination x
      0,                 // destination y
      bitmap_rects[i].w, // destination pitch
      bitmap_rects[i].w, // destination width
      bitmap_rects[i].h  // destination height
    ) {
      // TODO It would be cleaner to properly calculate the bitmap rect instead
      // of just bounds checking.
      if (si >= (int)pixel_count || si < 0) break;

      copied_pixels[di] = din->image_pixels[si];
    }

    struct indexed_bitmap copy = {
      .width = bitmap_rects[i].w,
      .height = bitmap_rects[i].h,
      .pixels = copied_pixels
    };

    array_push(din->dout.copies, copy);

    copied_pixels += bitmap_rects[i].w * bitmap_rects[i].h;
  }

  array_init(din->dout.copy_bitmaps, sizeof(struct sdl_bitmap), din->selections_n);

  generate_sdl_bitmaps_for_indexed_bitmaps(
    buffer,
    pixel_count,
    din->sdl_renderer,
    din->palette_colours,
    din->dout.copies,
    din->selections_n,
    din->dout.copy_bitmaps
  );
  
  din->dout.editor_state = EDITOR_STATE_NONE;
  if (din->selections_n > 1 || bitmap_rects[0].h > 1 || bitmap_rects[0].w > 1) {
    din->dout.editor_state = EDITOR_STATE_PASTING;
  }

  free(bitmap_rects);

  return &din->dout;
}

struct dout_editor_apply_paste* editor_apply_paste(struct din_editor_apply_paste* din) {
  struct rect* paste_destinations = malloc(sizeof(struct rect) * din->selections_n);

  int n = 0;
  // Convert selections into paste destination bitmap rects
  for (int i = 0; i < din->selections_n; i++) {
    struct transform combined_transform = {
      .translation = (struct point) {
        .x = din->image_transform.translation.x - din->mouse.x,
        .y = din->image_transform.translation.y - din->mouse.y,
      },
      .scale = din->image_transform.scale
    };
    struct rect bitmap_rect = screen_selection_to_bitmap_rect(din->selections[i], combined_transform);
    // get width and height from bitmap incase of rotation.
    bitmap_rect.w = din->copies[i].width;
    bitmap_rect.h = din->copies[i].height;

    // snap paste to block
    bitmap_rect.x -= bitmap_rect.x % din->editor_block_size;
    bitmap_rect.y -= bitmap_rect.y % din->editor_block_size;

    n += bitmap_rect.w * bitmap_rect.h;
    paste_destinations[i] = bitmap_rect;
  }

  array_init(din->dout.pixel_changes, sizeof(struct pixel_change), n)

  for (int i = 0; i < din->selections_n; i++) {
    pixel_loop(
      0,                       // source x
      0,                       // source y
      din->copies[i].width,    // source pitch
      paste_destinations[i].x, // destination x
      paste_destinations[i].y, // destination y
      din->image_width,        // destination pitch
      paste_destinations[i].w, // destination width
      paste_destinations[i].h  // destination height
    ) {
      // TODO It would be cleaner to properly calculate the bitmap rect instead
      // of just bounds checking.
      if (si >= n || si < 0) break;

      if (din->image_pixels[di] == din->copies[i].pixels[si]) continue;

      struct pixel_change pixel_change = {
        .buffer_index = di,
        .from = din->image_pixels[di],
        .to = din->copies[i].pixels[si]
      };
      array_push(din->dout.pixel_changes, pixel_change)
    }
  }
  free(paste_destinations);

  return &din->dout;
}

struct dout_editor_toggle_grid* editor_toggle_grid(struct din_editor_toggle_grid* din) {
  din->dout.grid_enabled = !din->grid_enabled;
  return &din->dout;
}

struct dout_editor_transform_selection* editor_transform_selection(indexed_pixel_transform_t transform, struct din_editor_transform_selection* din) {
  int pixel_count = 0;
  for (int i = 0; i < din->copies_n; i++) {
    pixel_count += din->copies[i].width * din->copies[i].height;
  }
  array_init(din->dout.transformed_copies, sizeof(struct indexed_bitmap), din->copies_n)
  indexed_pixel_t* buffer = malloc(sizeof(indexed_pixel_t) * pixel_count);
  indexed_pixel_t* pixels = buffer;

  for (int i = 0; i < din->copies_n; i++) {
    struct indexed_bitmap transformed_copy;
    int w = din->copies[i].width, h = din->copies[i].height;
    transform(din->copies[i].pixels, w, h, &transformed_copy.width, &transformed_copy.height, pixels);
    transformed_copy.pixels = pixels;
    array_push(din->dout.transformed_copies, transformed_copy)
    pixels += w * h;
  }

  array_init(din->dout.copy_bitmaps, sizeof(struct sdl_bitmap), din->copies_n);

  // generate sdl_bitmaps
  generate_sdl_bitmaps_for_indexed_bitmaps(
    buffer,
    pixel_count,
    din->sdl_renderer,
    din->palette_colours,
    din->dout.transformed_copies,
    din->copies_n,
    din->dout.copy_bitmaps
  );
  
  return &din->dout;
}

struct dout_editor_paint* editor_paint(int begin_undoable_group, struct din_editor_paint* din) {
  struct point translated_mouse = screen_point_to_bitmap_point(
    din->mouse,
    din->image_transform
  );
  int index = translated_mouse.y * din->image_width + translated_mouse.x;
  indexed_pixel_t from = index < din->image_pixels_n && index >= 0 ? din->image_pixels[index] : 0;
  din->dout.pixel_change = (struct pixel_change) {
    .buffer_index = index,
    .from = from,
    .to = din->paint_colour
  };
  din->dout.begin_undoable_group = begin_undoable_group;
  return &din->dout;
}

struct dout_editor_zoom* editor_zoom(zoom_fn_t zoom_fn, struct din_editor_zoom* din) {
  din->dout.image_transform = zoom_fn(din->window_size, din->image_transform);

  // resize grid
  din->dout.grid_bitmap = create_grid_sdl_bitmap(din->sdl_renderer, din->tile_size, din->dout.image_transform.scale);
  
  return &din->dout;
}

struct dout_editor_pick_image_colour* editor_pick_image_colour(struct din_editor_pick_image_colour* din) {
  struct point translated_mouse = screen_point_to_bitmap_point(
    din->mouse,
    din->image_transform
  );
  int index = translated_mouse.y * din->image_width + translated_mouse.x;
  if (index >= din->image_pixels_n || index < 0) {
    din->dout.paint_colour = din->paint_colour;
  } else {
    din->dout.paint_colour = din->image_pixels[index];
  }
  return &din->dout;
}

void editor_save_image(struct din_editor_save_image* din) {
  int w = din->image.width;
  int h = din->image.height;
  colour_t* bitmap_buffer = malloc(sizeof(colour_t) * w * h);
  int pitch = w * (BITS_PER_PIXEL / BITS_IN_BYTE);
  stbi_write_png(din->image_filename, w, h, 4, (uint8_t*)bitmap_buffer, pitch);
  free(bitmap_buffer);
}

struct dout_editor_init* editor_init(struct din_editor_init* din) {
  struct dout_editor_init* dout = &din->dout;
  dout->default_palette.colours = (colour_t*)nes_palette_colours;
  dout->default_palette.n = 64;
  dout->default_palette.name = "NES";
  dout->display_name = "Untitled*";
  dout->grid_enabled = 1;
  int buffer_bytes = sizeof(indexed_pixel_t) * DEFAULT_IMAGE_SIZE * DEFAULT_IMAGE_SIZE;
  dout->image.pixels = calloc(buffer_bytes, 1);
  dout->image_scale = 4;
  dout->image.width = DEFAULT_IMAGE_SIZE;
  dout->image.height = DEFAULT_IMAGE_SIZE;
  return dout;
}

// struct dout_editor_resize_window {
// };
// 
// struct din_editor_resize_window {
//   struct dout_editor_resize_window dout;
//   
// };
// 
// void data_w_editor_resize_window(struct data* data, struct dout_editor_resize_window* dout);
// struct din_editor_resize_window* data_r_editor_resize_window(struct data* data);
// 
// void data_w_editor_resize_window(struct data* data, struct dout_editor_resize_window* dout) {
//   // TODO
// }
// 
// struct din_editor_resize_window* data_r_editor_resize_window(struct data* data) {
//   struct din_editor_resize_window* din = &data->_io_pool.editor_resize_window;
//   // TODO
//   return din;
// }
// 
// struct dout_editor_resize_window* editor_resize_window(struct din_editor_resize_window* din);
// 
// struct dout_editor_resize_window* editor_resize_window(struct din_editor_resize_window* din) {
//   struct dout_editor_resize_window* dout = &din->dout;
//   // TODO
//   return dout;
// }
