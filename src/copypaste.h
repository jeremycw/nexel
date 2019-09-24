#ifndef COPYPASTE_H
#define COPYPASTE_H

#include "image.h"

void copy_paste_init(view_t* v);
int copy_paste_handle_events(SDL_Event* e, undo_t* undo_head, bitmap_t* image);
void copy_paste_render(int x, int y, SDL_Renderer* ren);

#endif
