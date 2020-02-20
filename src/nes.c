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
  unsigned int* tiles = malloc(sizeof(unsigned int) * w * h);
  int ppb = BLOCK_SIZE * BLOCK_SIZE;
  #pragma omp parallel for
  for (int ty = 0; ty < h / BLOCK_SIZE; ty++) {
    for (int tx = 0; tx < w / BLOCK_SIZE; tx++) {
      int i = (ty * h / BLOCK_SIZE + tx) * ppb;
      pixel_loop(tx * BLOCK_SIZE, ty * BLOCK_SIZE, w, 0, 0, 0, BLOCK_SIZE, BLOCK_SIZE) {
        tiles[i] = data[si];
        i++;
      }
    }
  }

  int ntiles = w * h / ppb;
  #pragma omp parallel for
  for (int i = 0; i < ntiles; i++) {
    merge_sort(&tiles[i], ppb, sizeof(uint32_t), cmp_int);
    pal4_t pal = { .colours = { tiles[i], 0, 0, 0 } };
    int pi = 0;
    for (int j = i; j < i + ppb; j++) {
      if (tiles[j] != pal.colours[pi]) {
        pal.colours[++pi] = tiles[j];
      }
      if (pi == 3) break;
    }
    palettes[i / ppb] = pal;
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
