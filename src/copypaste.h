#ifndef COPYPASTE_H
#define COPYPASTE_H

#include "image.h"

enum { CP_NONE, CP_PASTING, CP_COPYING };

void copy_paste_init();
int copy_paste_handle_events(SDL_Event* e);
void copy_paste_render(int x, int y, SDL_Renderer* ren);

#endif
