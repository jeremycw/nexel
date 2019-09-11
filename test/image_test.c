#include <assert.h>
#include "../src/image.h"

void test_image() {
  int in[] = {
    1, 2,
    3, 4
  };
  int expect[] = {
    1, 1, 2, 2,
    1, 1, 2, 2,
    3, 3, 4, 4,
    3, 3, 4, 4
  };
  int out[16];
  double_px(in, 4, 2, out);
  for (int i = 0; i < 16; i++) {
    assert(out[i] == expect[i]);
  }

  int expect2[] = {
    1, 2,
    3, 4
  };
  int in2[] = {
    1, 1, 2, 2,
    1, 1, 2, 2,
    3, 3, 4, 4,
    3, 3, 4, 4
  };
  int out2[4];
  half_px(in2, 16, 4, out2);
  for (int i = 0; i < 4; i++) {
    assert(out2[i] == expect2[i]);
  }
}
