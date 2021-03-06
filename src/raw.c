#include <math.h>
#include <stdint.h>
#include "raw.h"

void raw_alpha_channel_to_rgba(uint8_t* in, uint32_t* out, int n, uint32_t rgb) {
  for (int i = 0; i < n; i++) {
    out[i] = (in[i] << 24) | rgb;
  }
}

void raw_rotate_clockwise(uint32_t* in, int w, int h, uint32_t* out) {
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int ry = (int)floor((x + .5f - w / 2.f) + w / 2.f);
      int rx = (int)floor(-(y + .5f - h / 2.f) + h / 2.f);
      int i = y * w + x;
      int ri = ry * h + rx;
      out[ri] = in[i];
    }
  }
}

void raw_mirror_horizontal(uint32_t* in, int w, int h) {
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w / 2; x++) {
      int i = y * w + x;
      int j = y * w + (w - x - 1);
      int tmp = in[i];
      in[i] = in[j];
      in[j] = tmp;
    }
  }
}

void raw_mirror_vertical(uint32_t* in, int w, int h) {
  for (int y = 0; y < h / 2; y++) {
    for (int x = 0; x < w; x++) {
      int i = y * w + x;
      int j = (h - y - 1) * w + x;
      int tmp = in[i];
      in[i] = in[j];
      in[j] = tmp;
    }
  }
}
