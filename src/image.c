#include <stdio.h>
#include <math.h>
#include "image.h"

varray_defn(pixel_t);

void translate_coord(int x, int y, int* tx, int* ty, view_t* view) {
  *tx = (x - view->translation.x) / view->scale; 
  *ty = (y - view->translation.y) / view->scale;
}

void alpha_channel_to_rgba(unsigned char* in, unsigned int* out, int n, unsigned int rgb) {
  for (int i = 0; i < n; i++) {
    out[i] = (in[i] << 24) | rgb;
  }
}

void rotate_clockwise(unsigned int* in, int w, int h, unsigned int* out) {
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

void mirror_horizontal(unsigned int* in, int w, int h) {
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

void mirror_vertical(unsigned int* in, int w, int h) {
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
