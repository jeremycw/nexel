#include <SDL2/SDL.h>
#include "data.h"
#include "debugbreak.h"
#include "util.h"

struct dout_sdl_renderer_init* sdl_renderer_init(struct din_sdl_renderer_init* din) {
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    printf("SDL_Init Error: %s\n", SDL_GetError());
    exit(1);
  }

  int options = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
  SDL_Window* window = SDL_CreateWindow("Nexel", 0, 0, 640, 480, options);
  if (window == NULL) {
    printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
    exit(1);
  }

  options = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, options);
  if (renderer == NULL) {
    printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
    exit(1);
  }

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

  din->dout.renderer = renderer;
  din->dout.window = window;

  din->dout.grid = create_grid_sdl_bitmap(renderer, din->tile_size, din->image_scale);

  int pixel_count = din->image_indexed_bitmap.width * din->image_indexed_bitmap.height;
  generate_sdl_bitmaps_for_indexed_bitmaps(
    din->image_indexed_bitmap.pixels,
    pixel_count,
    renderer,
    din->palette_colours,
    &din->image_indexed_bitmap,
    1,
    &din->dout.image
  );

  din->dout.palette = sdl_bitmap_from_raw_buffer(
    renderer,
    (void*)din->palette_colours,
    (struct dimensions) { .w = 4, .h = 16 }
  );

  return &din->dout;
}

struct dout_sdl_get_state* sdl_get_state(struct din_sdl_get_state* din) {
  struct sdl_state sdl_state;
  sdl_state.mouse_state = SDL_GetMouseState(&sdl_state.mouse.x, &sdl_state.mouse.y);
  SDL_GetWindowSize(din->sdl_window, &sdl_state.window.w, &sdl_state.window.h);
  din->dout.sdl_state = sdl_state;
  return &din->dout;
}

void sdl_renderer_draw(struct din_sdl_renderer_draw* din) {
  colour_t colour = din->background_colour;
  SDL_SetRenderDrawColor(din->sdl_renderer, colour.r, colour.g, colour.b, colour.a);
  SDL_RenderClear(din->sdl_renderer);

  int x = din->image_transform.translation.x;
  int y = din->image_transform.translation.y;
  int w = din->image_sdl_bitmap.surface->w * din->image_transform.scale;
  int h = din->image_sdl_bitmap.surface->h * din->image_transform.scale;

  // Render image to its transformed destination
  SDL_Rect image_destination = { .x = x, .y = y, .w = w, .h = h };
  SDL_RenderCopy(din->sdl_renderer, din->image_sdl_bitmap.texture, NULL, &image_destination);

  if (din->grid_enabled) {
    // The grid is drawn as a bunch of tiles over top of the image
    int tile_screen_size = din->tile_size * din->image_transform.scale;
    SDL_Rect grid_tile_destination = {
      .x = x,
      .y = y,
      .w = tile_screen_size,
      .h = tile_screen_size
    };
    for (int i = 0; i < h / tile_screen_size; i++) {
      for (int j = 0; j < w / tile_screen_size; j++) {
        SDL_RenderCopy(din->sdl_renderer, din->grid_texture, NULL, &grid_tile_destination);
        grid_tile_destination.x += tile_screen_size;
      }
      grid_tile_destination.x = x;
      grid_tile_destination.y += tile_screen_size;
    }
  }

  colour = din->selection_colour;
  SDL_SetRenderDrawColor(din->sdl_renderer, colour.r, colour.g, colour.b, colour.a);
  for (int i = 0; i < din->selections_n; i++) {
    colour = din->selection_colour;
    struct rect screen_rect = selection_to_rect(din->selections[i]);
    screen_rect = snap_rect_to_pixel(screen_rect, din->image_transform);
    SDL_RenderFillRect(din->sdl_renderer, &(SDL_Rect) {
      .x = screen_rect.x,
      .y = screen_rect.y,
      .w = screen_rect.w,
      .h = screen_rect.h
    });
  }

  SDL_RenderPresent(din->sdl_renderer);
  // Draw UI panes
}
