#ifndef UI_H
#define UI_H

#include <SDL2/SDL.h>

#define UI_COLOUR_PICKER 0
#define UI_PALETTE_SELECTOR 1

void ui_init(SDL_Window* window, SDL_Renderer* renderer, int blksz);
void ui_toggle_grid();
int ui_in_bounds(int widget_id, int x, int y);
uint32_t ui_colour_picker_get_colour(int x, int y);
void ui_draw(int imgx, int imgy, int imgw, int imgh, int imgscale, int blksz);
void ui_set_status(char* s);
int ui_handle_events(SDL_Event* e);

#endif
