#ifndef SDL_RENDERER_H
#define SDL_RENDERER_H

#include "data.h"

#define COLOUR_PICKER_W 4
#define COLOUR_PICKER_H 16
#define COLOUR_SWATCH_SIZE_PX 16

struct dout_sdl_renderer_init* sdl_renderer_init(struct din_sdl_renderer_init* din);
void sdl_renderer_draw(struct din_sdl_renderer_draw* dro);
struct dout_sdl_get_state* sdl_get_state(struct din_sdl_get_state* din);

#endif
