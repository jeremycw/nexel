#include <SDL2/SDL.h>
#include "data.h"
#include "util.h"

struct dout_sdl_renderer_init* sdl_renderer_init(struct din_sdl_renderer_init* din) {
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    printf("SDL_Init Error: %s\n", SDL_GetError());
    exit(1);
  }

  SDL_Window* window = SDL_CreateWindow("Nexel", 0, 0, 640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (window == NULL) {
    printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
    exit(1);
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer == NULL) {
    printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
    exit(1);
  }
  din->dout.sdl_renderer = (struct sdl_renderer) {
    .window = window,
    .renderer = renderer
  };
  return &din->dout;
}

struct dout_sdl_get_state* sdl_get_state(struct din_sdl_get_state* din) {
  struct sdl_state sdl_state;
  sdl_state.mouse_state = SDL_GetMouseState(&sdl_state.mouse.x, &sdl_state.mouse.y);
  SDL_GetWindowSize(din->sdl_window, &sdl_state.window.w, &sdl_state.window.h);
  return &din->dout;
}

void sdl_renderer_draw(struct din_sdl_renderer_draw* din) {
  colour_t colour = din->selection_colour;
  SDL_SetRenderDrawColor(din->sdl_renderer, colour.r, colour.g, colour.b, colour.a);
  SDL_RenderClear(din->sdl_renderer);

  int x = din->image_transform.translation.x;
  int y = din->image_transform.translation.y;
  int w = din->image_bitmap.surface->w * din->image_transform.scale * din->tile_size;
  int h = din->image_bitmap.surface->h * din->image_transform.scale * din->tile_size;

  // Render image to its transformed destination
  SDL_Rect image_destination = { .x = x, .y = y, .w = w, .h = h };
  SDL_RenderCopy(din->sdl_renderer, din->image_bitmap.texture, NULL, &image_destination);

  if (din->grid_enabled) {
    // The grid is drawn as a bunch of tiles over top of the image
    int tile_screen_size = din->tile_size * din->image_transform.scale;
    SDL_Rect grid_tile_destination = { .x = x, .y = y, .w = tile_screen_size, .h = tile_screen_size };
    for (int i = 0; i < h / din->tile_size; i++) {
      for (int j = 0; j < w / din->tile_size; j++) {
        SDL_RenderCopy(din->sdl_renderer, din->grid_texture, NULL, &grid_tile_destination);
        grid_tile_destination.x += din->image_transform.scale * din->tile_size;
      }
      grid_tile_destination.x = x;
      grid_tile_destination.y += din->image_transform.scale * din->tile_size;
    }
  }

  for (int i = 0; i < din->selections_n; i++) {
    colour = din->selection_colour;
    SDL_SetRenderDrawColor(din->sdl_renderer, colour.r, colour.g, colour.b, colour.a);
    struct rect screen_rect = selection_to_rect(din->selections[i]);
    screen_rect = snap_rect_to_pixel(screen_rect, din->image_transform);
    SDL_RenderFillRect(din->sdl_renderer, &(SDL_Rect) {
      .x = screen_rect.x,
      .y = screen_rect.y,
      .w = screen_rect.w,
      .h = screen_rect.h
    });
  }

  // Draw UI panes
}
