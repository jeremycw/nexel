#include <stdio.h>
#include <math.h>
#include "image.h"

void double_px(const int* in, int n, int pitch, int* out) {
  for (int i = 0, oi = 0; i < n; i += pitch, oi += pitch*4) {
    for (int j = 0, oj = 0; j < pitch; j++, oj += 2) {
      const int* src = in + i;
      int* dst1 = out + oi;
      int* dst2 = out + oi + pitch*2;
      dst1[oj] = src[j];
      dst1[oj + 1] = src[j];
      dst2[oj] = src[j];
      dst2[oj + 1] = src[j];
    }
  }
}

void half_px(const int* in, int n, int pitch, int* out) {
  for (int i = 0, oi = 0; i < n; i += pitch*2, oi += pitch/2) {
    for (int j = 0, oj = 0; j < pitch; j += 2, oj++) {
      const int* src = in + i;
      int* dst = out + oi;
      dst[oj] = src[j];
    }
  }
}

void alpha_channel_to_rgba(unsigned char* in, unsigned int* out, int n, unsigned int rgb) {
  for (int i = 0; i < n; i++) {
    out[i] = (in[i] << 24) | rgb;
  }
}

void rotate_clockwise(unsigned int* in, int w, int h, unsigned int* out) {
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int ry = (int)floor((x + .5f - w / 2.f) + h / 2.f);
      int rx = (int)floor(-(y + .5f - h / 2.f) + w / 2.f);
      int i = y * w + x;
      int ri = ry * h + rx;
      out[ri] = in[i];
    }
  }
}
