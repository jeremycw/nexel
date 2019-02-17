#ifndef VARRY_H
#define VARRY_H

#define decl_varry(type) \
  typedef struct { \
    type* buf; \
    int capacity; \
    int size; \
  } varry_##type##_t; \
  void varry_##type##_push(varry_##type##_t* varry, type a) { \
    if (varry->size == varry->capacity) { \
      varry->capacity *= 2; \
      varry->buf = realloc(varry->buf, varry->capacity * sizeof(type)); \
    } \
    varry->buf[varry->size] = a; \
    varry->size++; \
  } \
  void varry_##type##_init(varry_##type##_t* varry, int capacity) { \
    varry->buf = malloc(sizeof(type) * capacity); \
    varry->size = 0; \
    varry->capacity = capacity; \
  }

#define varry_t(type) \
  varry_##type##_t

#define varry_push(type, varry, a) \
  varry_##type##_push(varry, a); 

#define varry_init(type, varry, capacity) \
  varry_##type##_init(varry, capacity);

#endif
