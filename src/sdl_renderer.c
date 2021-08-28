#include <SDL2/SDL.h>
#include "data.h"

struct dout_sdl_renderer_init sdl_renderer_init(struct din_sdl_renderer_init din) {
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
  din.dout.sdl_renderer = (struct sdl_renderer) {
    .window = window,
    .renderer = renderer
  };
  return din.dout;
}

struct dout_sdl_get_state sdl_get_state(struct din_sdl_get_state din) {
  struct sdl_state sdl_state;
  sdl_state.mouse_state = SDL_GetMouseState(&sdl_state.mouse.x, &sdl_state.mouse.y);
  SDL_GetWindowSize(din.sdl_window, &sdl_state.window.w, &sdl_state.window.h);
  return din.dout;
}

// void sdl_renderer_draw(struct din_sdl_renderer_draw* dro) {
//   SDL_SetRenderDrawColor(dro->sdl_renderer.renderer, 0, 0x2b, 0x36, 255);
//   SDL_RenderClear(dro->sdl_renderer.renderer);
// 
//   for (int i = 0; i < dro->textures_meta.size; i++) {
//     SDL_Rect translation = {
//       .x = dro->translations[i].x,
//       .y = dro->translations[i].y,
//       .w = dro->translations[i].w,
//       .h = dro->translations[i].h
//     };
//     SDL_RenderCopy(dro->sdl_renderer.renderer, dro->textures[i], NULL, &translation);
//   }
// 
//   if (dro->grid_enabled) {
//     // build_grid(scale, blksize);
//     //grid.dest.x = x;
//     //grid.dest.y = y;
//     //for (int i = 0; i < h / blksize; i++) {
//     //  for (int j = 0; j < w / blksize; j++) {
//     //    SDL_RenderCopy(ren, grid.bitmap.tex, NULL, &grid.dest);
//     //    grid.dest.x += scale * blksize;
//     //  }
//     //  grid.dest.x = x;
//     //  grid.dest.y += scale * blksize;
//     //}
//   }
// 
//   for (int i = 0; i < dro->selections_n; i++) {
//     SDL_SetRenderDrawColor(dro->sdl_renderer.renderer, 0, 0, 255, 100);
//     //coords_to_rect(copy.start, copy.end, &copy.dest);
//     //image_snap_rect_to_pixel(&copy.dest);
//     SDL_Rect draw_dest;
//     SDL_RenderFillRect(dro->sdl_renderer.renderer, &draw_dest);
//   }
// 
//   // Draw UI panes
// }
