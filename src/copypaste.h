#ifndef COPYPASTE_H
#define COPYPASTE_H

enum { CP_NONE, CP_PASTING, CP_COPYING };

int copy_paste_handle_events(SDL_Event* e);
void copy_paste_render(SDL_Renderer* ren);

#endif
