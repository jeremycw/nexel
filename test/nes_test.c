#include <string.h>
#include <assert.h>
#include "../src/stb_image.h"
#include "../src/threads.h"
#include "../src/nes.h"

void test_nes() {
  global_thread_pool = thpool_init(GLOBAL_THREAD_POOL_SIZE);
  int width, height, orig_format, req_format = STBI_rgb_alpha;
  unsigned int* data = (unsigned int*)stbi_load("test/test.png", &width, &height, &orig_format, req_format);
  assert(data != NULL);

  pal4_t palettes[256];
  detect_palettes(data, width, height, palettes);
  varray_t(pal4_t) varray;
  varray_init(pal4_t, &varray, 4);
  unique_palettes(palettes, 256, &varray);
}
