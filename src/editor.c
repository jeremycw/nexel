#include "editor.h"
#include "debugbreak.h"
#include "image.h"
#include "raw.h"

#define RMASK 0x000000ff
#define GMASK 0x0000ff00
#define BMASK 0x00ff0000
#define AMASK 0xff000000

enum editor_commands editor_select_command(SDL_Event* e, struct din_editor_select_command din) {
  switch (e->type) {
    case SDL_QUIT:
      return EDITOR_CMD_QUIT;
      break;
    case SDL_KEYDOWN:
      if (e->key.keysym.sym == SDLK_q) return EDITOR_CMD_QUIT;
      if (din.editor_state == EDITOR_STATE_PASTING) {
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
      if (din.editor_state == EDITOR_STATE_PASTING) {
        return EDITOR_CMD_APPLY_PASTE;
      }
      break;
    case SDL_MOUSEBUTTONUP:
      if (din.editor_state == EDITOR_STATE_SELECTING) {
        return EDITOR_CMD_COPY_SELECTION;
      }
      break;
    case SDL_MOUSEMOTION:
      if (din.mouse_state & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
        return EDITOR_CMD_RESIZE_SELECTION;
      } else if (din.mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        return EDITOR_CMD_CONTINUE_PAINT;
      }
      break;
    case SDL_MOUSEWHEEL:
      return EDITOR_CMD_PAN;
      break;
  }
  return EDITOR_CMD_NOP;
}

struct dout_editor_cancel_paste editor_cancel_paste(struct din_editor_cancel_paste din) {
  free(din.copies[0].pixels);
  din.dout.copies_meta = din.copies_meta;
  array_clear(din.dout.copies)
  return din.dout;
}

struct dout_editor_start_selection editor_start_selection(struct din_editor_start_selection din) {
  din.dout.state = EDITOR_STATE_SELECTING;
  din.dout.selection.x = din.mouse_x;
  din.dout.selection.y = din.mouse_y;
  din.dout.selection.w = 0;
  din.dout.selection.h = 0;
  return din.dout;
}

struct dout_editor_resize_selection editor_resize_selection(struct din_editor_resize_selection din) {
  din.dout.end_selection.x = din.mouse.x;
  din.dout.end_selection.y = din.mouse.y;
  return din.dout;
}

struct rect selection_to_rect(struct selection selection) {
  int dx = selection.end.x - selection.start.x;
  int dy = selection.end.y - selection.start.y;
  struct rect rect;
  if (dx < 0) {
    rect.x = selection.end.x;
    rect.w = -dx;
  } else {
    rect.x = selection.start.x;
    rect.w = dx;
  }
  if (dy < 0) {
    rect.y = selection.end.y;
    rect.h = -dy;
  } else {
    rect.y = selection.start.y;
    rect.h = dy;
  }
  return rect;
}

struct point screen_point_to_bitmap_point(struct point point, struct point image_translation, int image_scale) {
  struct point translated;
  translated.x = (point.x - image_translation.x) / image_scale; 
  translated.y = (point.y - image_translation.y) / image_scale;
  return translated;
}

struct rect screen_selection_to_bitmap_rect(struct selection selection, struct point image_translation, int image_scale) {
  struct rect rect = selection_to_rect(selection);

  // snap rect to pixel
  int offsetx = image_translation.x % image_scale;
  int offsety = image_translation.y % image_scale;
  rect.x = rect.x - (rect.x - offsetx) % image_scale;
  rect.y = rect.y - (rect.y - offsety) % image_scale;
  rect.w = rect.w - rect.w % image_scale;
  rect.h = rect.h - rect.h % image_scale;

  // translate
  struct point translated = screen_point_to_bitmap_point(
    (struct point) { .x = rect.x, .y = rect.y },
    image_translation,
    image_scale
  );

  rect.x = translated.x;
  rect.y = translated.y;

  // scale
  rect.w /= image_scale;
  rect.h /= image_scale;

  return rect;
}

void indexed_bitmap_to_rgba(indexed_pixel_t const* indexed_pixels, int n, uint32_t const* palette_colours, uint32_t* out) {
  for (int i = 0; i < n; i++) {
    out[i] = palette_colours[indexed_pixels[i]];
  }
}

void generate_sdl_bitmaps_for_indexed_bitmaps(
  indexed_pixel_t* pixels,
  int pixels_n,
  SDL_Renderer* sdl_renderer,
  uint32_t const* palette_colours,
  struct indexed_bitmap* indexed_bitmaps,
  int indexed_bitmaps_n
) {
  uint32_t* buffer = malloc(sizeof(uint32_t) * pixels_n);
  indexed_bitmap_to_rgba(pixels, pixels_n, palette_colours, buffer);
  uint32_t* bitmap = buffer;
  for (int i = 0; i < indexed_bitmaps_n; i++) {
    int w = indexed_bitmaps[i].width;
    int h = indexed_bitmaps[i].height;
    int bpp = 32;
    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
      bitmap,        // buffer
      w,             // width
      h,             // height
      bpp,           // bpp
      w * (bpp / 8), // pitch
      RMASK,         // red mask
      GMASK,         // green mask
      BMASK,         // blue mask
      AMASK          // alpha mask
    );
    indexed_bitmaps[i].sdl_bitmap.surface = surface;
    indexed_bitmaps[i].sdl_bitmap.texture =
       SDL_CreateTextureFromSurface(sdl_renderer, surface);
    bitmap += w * h;
  }
}

struct dout_editor_copy_selection editor_copy_selection(struct din_editor_copy_selection din) {
  struct rect* bitmap_rects = malloc(sizeof(struct rect) * din.selections_n);

  size_t pixel_count = 0;
  for (int i = 0; i < din.selections_n; i++) {
    // convert the start point end point selection from screen coords into pixel rect
    bitmap_rects[i] = screen_selection_to_bitmap_rect(din.selections[i], din.image_translation, din.image_scale);
    pixel_count += bitmap_rects[i].w * bitmap_rects[i].h;
  }

  // allocate space for all copied selections
  indexed_pixel_t* buffer = malloc(sizeof(indexed_pixel_t) * pixel_count);

  // XXX need to check for copying out of bounds
  indexed_pixel_t* copied_pixels = buffer;
  for (int i = 0; i < din.selections_n; i++) {
    pixel_loop(
      bitmap_rects[i].x, // source x
      bitmap_rects[i].y, // source y
      din.image_width,   // source pitch
      0,                 // destination x
      0,                 // destination y
      bitmap_rects[i].w, // destination pitch
      bitmap_rects[i].w, // destination width
      bitmap_rects[i].h  // destination height
    ) {
      copied_pixels[di] = din.image_pixels[si];
    }

    struct indexed_bitmap copy = {
      .width = bitmap_rects[i].w,
      .height = bitmap_rects[i].h,
      .pixels = copied_pixels
    };

    array_push(din.dout.copies, copy);

    copied_pixels += bitmap_rects[i].w * bitmap_rects[i].h;
  }

  generate_sdl_bitmaps_for_indexed_bitmaps(
    buffer,
    pixel_count,
    din.sdl_renderer,
    din.palette_colours,
    din.dout.copies,
    din.selections_n
  );
  
  din.dout.editor_state = EDITOR_STATE_NONE;
  if (din.selections_n > 1 || bitmap_rects[0].h > 1 || bitmap_rects[0].w > 1) {
    din.dout.editor_state = EDITOR_STATE_PASTING;
  }

  free(bitmap_rects);

  return din.dout;
}

struct dout_editor_apply_paste editor_apply_paste(struct din_editor_apply_paste din) {
  struct rect* paste_destinations = malloc(sizeof(struct rect) * din.selections_n);

  int n = 0;
  // Convert selections into paste destination bitmap rects
  for (int i = 0; i < din.selections_n; i++) {
    struct point full_translation = {
      .x = din.image_translation.x - din.mouse.x,
      .y = din.image_translation.y - din.mouse.y
    };
    struct rect bitmap_rect = screen_selection_to_bitmap_rect(din.selections[i], full_translation, din.image_scale);
    // get width and height from bitmap incase of rotation.
    bitmap_rect.w = din.copies[i].width;
    bitmap_rect.h = din.copies[i].height;

    // snap paste to block
    bitmap_rect.x -= bitmap_rect.x % din.editor_block_size;
    bitmap_rect.y -= bitmap_rect.y % din.editor_block_size;

    n += bitmap_rect.w * bitmap_rect.h;
    paste_destinations[i] = bitmap_rect;
  }

  array_init(din.dout.pixel_changes, sizeof(struct pixel_change), n)

  // XXX need to check for pasting out of bounds
  for (int i = 0; i < din.selections_n; i++) {
    pixel_loop(
      0,                       // source x
      0,                       // source y
      din.copies[i].width,     // source pitch
      paste_destinations[i].x, // destination x
      paste_destinations[i].y, // destination y
      din.image_width,         // destination pitch
      paste_destinations[i].w, // destination width
      paste_destinations[i].h  // destination height
    ) {
      if (din.image_pixels[di] == din.copies[i].pixels[si]) continue;

      struct pixel_change pixel_change = {
        .buffer_index = di,
        .from = din.image_pixels[di],
        .to = din.copies[i].pixels[si]
      };
      array_push(din.dout.pixel_changes, pixel_change)
    }
  }
  free(paste_destinations);

  return din.dout;
}

struct dout_editor_toggle_grid editor_toggle_grid(struct din_editor_toggle_grid din) {
  din.dout.grid_enabled = !din.grid_enabled;
  return din.dout;
}

void rotate_clockwise_transform(indexed_pixel_t* in, int w, int h, int* wout, int* hout, indexed_pixel_t* out) {
  for (int j = 0; j < h; j++) {
    for (int k = 0; k < w; k++) {
      int source_index = j * w + k;
      int dest_index = k * w + (h-1 - j);
      out[dest_index] = in[source_index];
    }
  }
  *wout = h;
  *hout = w;
}

void flip_horizontal_transform(indexed_pixel_t* in, int w, int h, int* wout, int* hout, indexed_pixel_t* out) {
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w / 2; x++) {
      int i = y * w + x;
      int j = y * w + (w - x - 1);
      out[i] = in[j];
      out[j] = in[i];
    }
  }
  *wout = w;
  *hout = h;
}

void flip_vertical_transform(indexed_pixel_t* in, int w, int h, int* wout, int* hout, indexed_pixel_t* out) {
  for (int y = 0; y < h / 2; y++) {
    for (int x = 0; x < w; x++) {
      int i = y * w + x;
      int j = (h - y - 1) * w + x;
      out[i] = in[j];
      out[j] = in[i];
    }
  }
  *wout = w;
  *hout = h;
}

struct dout_editor_transform_selection editor_transform_selection(indexed_pixel_transform_t transform, struct din_editor_transform_selection din) {
  int n = 0;
  for (int i = 0; i < din.copies_n; i++) {
    n += din.copies[i].width * din.copies[i].height;
  }
  array_init(din.dout.transformed_copies, sizeof(struct indexed_bitmap), din.copies_n)
  indexed_pixel_t* pixels = malloc(sizeof(indexed_pixel_t) * n);

  for (int i = 0; i < din.copies_n; i++) {
    struct indexed_bitmap transformed_copy;
    int w = din.copies[i].width, h = din.copies[i].height;
    transform(din.copies[i].pixels, w, h, &transformed_copy.width, &transformed_copy.height, pixels);
    transformed_copy.pixels = pixels;
    array_push(din.dout.transformed_copies, transformed_copy)
    pixels += w * h;
  }

  // generate sdl_bitmaps
  generate_sdl_bitmaps_for_indexed_bitmaps(
    pixels,
    n,
    din.sdl_renderer,
    din.palette_colours,
    din.dout.transformed_copies,
    din.copies_n
  );
  
  return din.dout;
}

struct dout_editor_paint editor_paint(int begin_undoable_group, struct din_editor_paint din) {
  struct point translated_mouse = screen_point_to_bitmap_point(
    din.mouse,
    din.image_transform.translation,
    din.image_transform.scale
  );
  int index = translated_mouse.y * din.image_width + translated_mouse.x;
  indexed_pixel_t from = index < din.image_pixels_n && index >= 0 ? din.image_pixels[index] : 0;
  din.dout.pixel_change = (struct pixel_change) {
    .buffer_index = index,
    .from = from,
    .to = din.paint_colour
  };
  din.dout.begin_undoable_group = begin_undoable_group;
  return din.dout;
}

struct transform zoom_out(struct dimensions window, struct transform transform) {
  transform.translation.y += (window.h/2 - transform.translation.y)/2;
  transform.translation.x += (window.w/2 - transform.translation.x)/2;
  transform.scale /= 2;
  return transform;
}

struct transform zoom_in(struct dimensions window, struct transform transform) {
  transform.translation.y -= (window.h/2 - transform.translation.y)/2;
  transform.translation.x -= (window.w/2 - transform.translation.x)/2;
  transform.scale *= 2;
  return transform;
}

struct dout_editor_zoom editor_zoom(zoom_fn_t zoom_fn, struct din_editor_zoom din) {
  din.dout.image_transform = zoom_fn(din.window_size, din.image_transform);
  return din.dout;
}

struct dout_editor_pick_image_colour editor_pick_image_colour(struct din_editor_pick_image_colour din) {
  struct point translated_mouse = screen_point_to_bitmap_point(
    din.mouse,
    din.image_transform.translation,
    din.image_transform.scale
  );
  int index = translated_mouse.y * din.image_width + translated_mouse.x;
  if (index >= din.image_pixels_n || index < 0) {
    din.dout.paint_colour = din.paint_colour;
  } else {
    din.dout.paint_colour = din.image_pixels[index];
  }
  return din.dout;
}
