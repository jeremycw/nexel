#ifndef NES_H
#define NES_H

#define BLOCK_SIZE 8
#define MAJOR_BLOCK_SIZE 16

#include "varray.h"

typedef struct {
  unsigned int colours[4];
} pal4_t;

varray_decl(pal4_t);

void detect_palettes(unsigned int* data, int w, int h, pal4_t* palettes);
void unique_palettes(pal4_t* palettes, int n, varray_t(pal4_t)* varray);

#endif
