#include "data.h"

/**
 * Snaps the given rect to pixel boundaries in screen space. This is useful in
 * cases where the image in zoomed in and you want to display a selection and
 * not have it visually subdivide a pixel.
 *
 * @param rect is the screen space rect to snap
 * @param image_transform is the current view transformation applied to display the image
 *
 * @return the snapped rect
 */
struct rect snap_rect_to_pixel(struct rect rect, struct transform image_transform) {
  int offsetx = image_transform.translation.x % image_transform.scale;
  int offsety = image_transform.translation.y % image_transform.scale;
  rect.x = rect.x - (rect.x - offsetx) % image_transform.scale;
  rect.y = rect.y - (rect.y - offsety) % image_transform.scale;
  rect.w = rect.w - rect.w % image_transform.scale;
  rect.h = rect.h - rect.h % image_transform.scale;
  return rect;
}

/**
 * Takes a selection, which is two points and converts it to a rect which is a
 * point and width and height.
 *
 * @param selection is the selection to be converted
 *
 * @return the rect form of the selection
 */
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

/**
 * Takes a point in screen coordinates and transforms it to a point that can be
 * used to index a pixel on the bitmap.
 *
 * @param point is the screen coord
 * @param image_transform is the view transform applied to display the image
 *
 * @return the point in bitmap coordinates
 */
struct point screen_point_to_bitmap_point(
  struct point point, struct transform image_transform
) {
  struct point translated;
  translated.x = (point.x - image_transform.translation.x) / image_transform.scale; 
  translated.y = (point.y - image_transform.translation.y) / image_transform.scale;
  return translated;
}

/**
 * Transforms a selection in screen coordinates to a rect in bitmap coordinates.
 *
 * @param selection is the selection to be transformed
 * @param image_transform is the view transform applied to display the image
 *
 * @return the bitmap coordinate rect
 */
struct rect screen_selection_to_bitmap_rect(
  struct selection selection, struct transform image_transform
) {
  struct rect rect = selection_to_rect(selection);

  rect = snap_rect_to_pixel(rect, image_transform);

  // translate
  struct point translated = screen_point_to_bitmap_point(
    (struct point) { .x = rect.x, .y = rect.y },
    image_transform
  );

  rect.x = translated.x;
  rect.y = translated.y;

  // scale
  rect.w /= image_transform.scale;
  rect.h /= image_transform.scale;

  return rect;
}

/**
 * Transforms an array of palette indexes into an array of rgba colours.
 *
 * @param indexed_pixels is the buffer holding the indexes
 * @param n is the count of the indexes in the buffer
 * @param palette_colours is an array of colours indexed by the indexed_pixels
 * @param out is the buffer to fill with the rgba colours
 */
void indexed_bitmap_to_rgba(
  indexed_pixel_t const* indexed_pixels,
  int n,
  colour_t const* palette_colours,
  colour_t* out
) {
  for (int i = 0; i < n; i++) {
    out[i] = palette_colours[indexed_pixels[i]];
  }
}

/**
 * Creates an sdl_bitmap from an rgba bitmap buffer
 *
 * @param renderer is the SDL_Renderer* we are using to render our screen
 * @param buffer is the rgba bitmap to convert to an sdl_bitmap
 * @param dimensions is the width and height of the input bitmap
 *
 * @return the new sdl_bitmap representation
 */
struct sdl_bitmap sdl_bitmap_from_raw_buffer(
  SDL_Renderer* renderer, void* buffer, struct dimensions dimensions
) {
  int pitch = dimensions.w * (BITS_PER_PIXEL / BITS_IN_BYTE);
  SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
    buffer,         // buffer
    dimensions.w,   // width
    dimensions.h,   // height
    BITS_PER_PIXEL, // bpp
    pitch,          // pitch
    RMASK,          // red mask
    GMASK,          // green mask
    BMASK,          // blue mask
    AMASK           // alpha mask
  );

  return (struct sdl_bitmap) {
    .surface = surface,
    .texture = SDL_CreateTextureFromSurface(renderer, surface)
  };
}

/**
 * Transforms an array of indexed bitmaps into sdl bitmaps
 *
 * @param pixels is an array of palette indexes. It should contain the pixels
 *   for all bitmaps we want to generate stored contiguously.
 * @param pixels_n is the length of the pixels array
 * @param sdl_renderer is the SDL_Renderer* used to draw the screen
 * @param palette_colours is an array of rgba colours indexed by the pixels
 * @param indexed_bitmaps is an array of indexed bitmaps backed by the pixels buffer
 * @param indexed_bitmaps_n is the length of indexed_bitmaps
 * @param copy_bitmaps is the output array for the sdl_bitmaps. It should have
 *   capacity for indexed_bitmaps_n sdl_bitmaps
 */
void generate_sdl_bitmaps_for_indexed_bitmaps(
  indexed_pixel_t const* pixels,
  int pixels_n,
  SDL_Renderer* sdl_renderer,
  colour_t const* palette_colours,
  struct indexed_bitmap const* indexed_bitmaps,
  int indexed_bitmaps_n,
  struct sdl_bitmap* copy_bitmaps
) {
  colour_t* buffer = malloc(sizeof(uint32_t) * pixels_n);
  indexed_bitmap_to_rgba(pixels, pixels_n, palette_colours, buffer);
  colour_t* bitmap = buffer;
  for (int i = 0; i < indexed_bitmaps_n; i++) {
    int w = indexed_bitmaps[i].width;
    int h = indexed_bitmaps[i].height;
    copy_bitmaps[i] = sdl_bitmap_from_raw_buffer(
      sdl_renderer,
      bitmap,
      (struct dimensions) { .w = w, .h = h }
    );
    bitmap += w * h;
  }
}

void rotate_clockwise_transform(
  indexed_pixel_t* in, int w, int h, int* wout, int* hout, indexed_pixel_t* out
) {
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

void flip_horizontal_transform(
  indexed_pixel_t* in, int w, int h, int* wout, int* hout, indexed_pixel_t* out
) {
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

void flip_vertical_transform(
  indexed_pixel_t* in, int w, int h, int* wout, int* hout, indexed_pixel_t* out
) {
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

struct transform zoom_out(struct dimensions window, struct transform transform) {
  if (transform.scale == 1) return transform;

  transform.translation.y += (window.h/2 - transform.translation.y)/2;
  transform.translation.x += (window.w/2 - transform.translation.x)/2;
  transform.scale /= 2;
  return transform;
}

struct transform zoom_in(struct dimensions window, struct transform transform) {
  if (transform.scale == 64) return transform;

  transform.translation.y -= (window.h/2 - transform.translation.y)/2;
  transform.translation.x -= (window.w/2 - transform.translation.x)/2;
  transform.scale *= 2;
  return transform;
}

/**
 * Creates an rgba bitmap of a single tile of the grid overlay. This is necessary
 * because we can't just scale this up at different zoom levels since we want the
 * line with to remain the same at different zoom levels.
 *
 * @param size is the width and height of the grid tile to create
 *
 * @return the newly allocated bitmap of colours
 */
colour_t* create_grid_tile_bitmap(int size) {
  colour_t* grid_pixels = calloc(size * size, sizeof(colour_t));
  for (int i = 0, j = 0; i < size; i++, j += size) {
    grid_pixels[i].colour = 0xffffffff;
    if (i % 2 == 0) grid_pixels[i + size/2 * size].colour = 0xffdddddd;
    grid_pixels[j].colour = 0xffffffff;
    if (i % 2 == 0) grid_pixels[j + size/2].colour = 0xffdddddd;
  }
  return grid_pixels;
}

/**
 * Creates an sdl bitmap of the grid tile at the current zoom scale.
 *
 * @param renderer is the SDL_Renderer* used to draw the screen
 * @param tile_size the width or height in bitmap pixels of one grid square
 * @param scale is the scale of the image being overlaid with the grid
 *
 * @return the sdl bitmap of the grid tile
 */
struct sdl_bitmap create_grid_sdl_bitmap(SDL_Renderer* renderer, int tile_size, int scale) {
  int width = tile_size * scale;
  colour_t* grid_buffer = create_grid_tile_bitmap(width);
  return sdl_bitmap_from_raw_buffer(
    renderer,
    grid_buffer,
    (struct dimensions) { .w = width, .h = width }
  );
}
