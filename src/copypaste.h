#ifndef COPYPASTE_H
#define COPYPASTE_H

#include "image.h"

int pasting();
int copying();

void start_copy(int x, int y);
void drag_copy(int x, int y);
void end_copy(bitmap_t* image);
void paste(int x, int y, undo_t* undo_head, bitmap_t* image);

void init_copy_paste(view_t* v);
void handle_copy_paste_keys(SDL_Event* e);
void render_copy_paste(int x, int y, SDL_Renderer* ren);

#endif
