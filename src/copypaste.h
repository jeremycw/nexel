#ifndef COPYPASTE_H
#define COPYPASTE_H

#include "image.h"

void init_copy_paste(view_t* v);
int handle_copy_paste_events(SDL_Event* e, undo_t* undo_head, bitmap_t* image);
void render_copy_paste(int x, int y, SDL_Renderer* ren);

#endif
