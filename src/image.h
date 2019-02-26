void double_px(const int* in, int n, int pitch, int* out);
void half_px(const int* in, int n, int pitch, int* out);
void alpha_channel_to_rgba(unsigned char* in, unsigned int* out, int n, unsigned int rgb);
void rotate_clockwise(unsigned int* in, int w, int h, unsigned int* out);
void mirror_horizontal(unsigned int* in, int w, int h);
void mirror_vertical(unsigned int* in, int w, int h);
