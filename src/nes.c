#include <stdlib.h>
#include <string.h>
#include "varray.h"
#include "raw.h"
#include "nes.h"
#include "merge_sort.h"

varray_defn(pal4_t);

int cmp_int(const void* a, const void* b) {
  uint32_t c = *(uint32_t*)a;
  uint32_t d = *(uint32_t*)b;
  return c == d ? 0 : c < d ? -1 : 1;
}

void nes_detect_palettes(unsigned int const* data, int w, int h, pal4_t* palettes) {
  int ppb = BLOCK_SIZE * BLOCK_SIZE;
  #pragma omp parallel for
  for (int ty = 0; ty < h / BLOCK_SIZE; ty++) {
    for (int tx = 0; tx < w / BLOCK_SIZE; tx++) {
      unsigned int tile[64];
      int i = 0;
      pixel_loop(tx * BLOCK_SIZE, ty * BLOCK_SIZE, w, 0, 0, 0, BLOCK_SIZE, BLOCK_SIZE) {
        tile[i] = data[si];
        i++;
      }
      merge_sort(tile, ppb, sizeof(uint32_t), cmp_int);
      pal4_t pal = { .colours = { tile[0], 0, 0, 0 } };
      int pi = 0;
      for (int j = 0; j < ppb; j++) {
        if (tile[j] != pal.colours[pi]) {
          pal.colours[++pi] = tile[j];
        }
        if (pi == 3) break;
      }
      palettes[ty * w / BLOCK_SIZE + tx] = pal;
    }
  }
}

void nes_unique_palettes(pal4_t* palettes, int n, varray_t(pal4_t) *varray) {
  varray_push(pal4_t, varray, palettes[0]);
  for (int i = 1; i < n; i++) {
    int match = 0;
    for (int j = 0; j < varray->size; j++) {
      match = match || !memcmp(&varray->buf[j], &palettes[i], sizeof(pal4_t));
    }
    if (!match) {
      varray_push(pal4_t, varray, palettes[i]);
    }
  }
}
