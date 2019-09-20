#include <stdlib.h>
#include <string.h>
#include "varray.h"
#include "image.h"
#include "parallel_map.h"
#include "threads.h"
#include "nes.h"

parallel_map_declare(uint32_t);
parallel_map_define(uint32_t);

varray_defn(pal4_t);

int cmp_int(const void* a, const void* b) {
  uint32_t c = *(uint32_t*)a;
  uint32_t d = *(uint32_t*)b;
  return c == d ? 0 : c < d ? -1 : 1;
}

void process_tiles(uint32_t* data, int index, int n, void* arg) {
  int ppb = BLOCK_SIZE * BLOCK_SIZE;
  pal4_t* palettes = (pal4_t*)arg;
  for (int i = index * ppb; i < n + index * ppb; i += ppb) {
    mergesort(&data[i], ppb, sizeof(uint32_t), cmp_int);
    pal4_t pal = { .colours = { data[i], 0, 0, 0 } };
    int pi = 0;
    for (int j = i; j < i + ppb; j++) {
      if (data[j] != pal.colours[pi]) {
        pal.colours[++pi] = data[j];
      }
      if (pi == 3) break;
    }
    palettes[i / ppb] = pal;
  }
}

void detect_palettes(unsigned int* data, int w, int h, pal4_t* palettes) {
  unsigned int* tiles = malloc(sizeof(unsigned int) * w * h);
  int i = 0;
  for (int ty = 0; ty < h / BLOCK_SIZE; ty++) {
    for (int tx = 0; tx < w / BLOCK_SIZE; tx++) {
      pixel_loop(tx * BLOCK_SIZE, ty * BLOCK_SIZE, w, 0, 0, 0, BLOCK_SIZE, BLOCK_SIZE) {
        tiles[i] = data[si];
        i++;
      }
    }
  }
  int ppb = BLOCK_SIZE * BLOCK_SIZE;
  parallel_map(uint32_t, global_thread_pool, GLOBAL_THREAD_POOL_SIZE, tiles, w * h / ppb, palettes, process_tiles);
  thpool_wait(global_thread_pool);
}

void unique_palettes(pal4_t* palettes, int n, varray_t(pal4_t) *varray) {
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
