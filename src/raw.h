#ifndef RAW_H
#define RAW_H

#define pixel_loop(src_x, src_y, src_pitch, dst_x, dst_y, dst_pitch, w, h) \
  for ( \
    int y_ = 0, \
        x_ = 0, \
        i_ = 0, \
        si = src_y * src_pitch + src_x, \
        di = dst_y * dst_pitch + dst_x; \
\
    i_ < w * h; \
\
    ++i_, \
    x_ = i_ % w, \
    y_ = i_ / w, \
    si = (y_ + src_y) * src_pitch + (x_ + src_x), \
    di = (y_ + dst_y) * dst_pitch + (x_ + dst_x) \
  )

void raw_alpha_channel_to_rgba(uint8_t* in, uint32_t* out, int n, uint32_t rgb);
void raw_rotate_clockwise(uint32_t* in, int w, int h, uint32_t* out);
void raw_mirror_horizontal(uint32_t* in, int w, int h);
void raw_mirror_vertical(uint32_t* in, int w, int h);

#endif
