#ifndef ARRAY_H
#define ARRAY_H

#define array_declare(type) \
  typedef struct { \
    type* buf; \
    int n; \
  } array_##type##_t;

#define asplat(a) \
  a.buf, a.n

#define asplatp(a) \
  a->buf, a->n

#define array(type) array_##type##_t

#endif
