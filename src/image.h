#ifndef IMAGE_H
#define IMAGE_H

#include <SDL2/SDL.h>

#define BLOCK_SIZE 8
#define MAJOR_BLOCK_SIZE 16

typedef struct {
  int index;
  int color;
} pixel_t;

typedef struct {
  int width;
  int height;
  int scale;
  int x;
  int y;
} image_info_t;

void image_translate_coord(int x, int y, int* tx, int* ty);
void image_set_paint_color(int c);
void image_info(image_info_t* info);
void image_init(char* path, SDL_Window* win, int width, int height);
void image_destroy();
int image_handle_events(SDL_Event* e, SDL_Window* win);
void image_draw(SDL_Renderer* ren);
void image_snap_rect_to_pixel(SDL_Rect* rect);
void image_snap_rect_to_block(SDL_Rect* rect);
void image_descale_rect(SDL_Rect* rect);
void image_scale_rect(SDL_Rect* rect);
void image_refresh();
void image_undoable_write(int index, unsigned int color);
int image_pitch();
unsigned int const * image_raw();
void image_begin_undo_recording();

#endif
