#ifndef PARALLEL_MAP_H
#define PARALLEL_MAP_H

#include <stdlib.h>
#include "thpool.h"

#define parallel_map_declare(type) \
  typedef struct { \
    type* data; \
    int n; \
    int start; \
    void* arg; \
    void (*mapfn)(type*,int,int,void*); \
  } parallel_map_##type##_arg_t; \
  \
  void parallel_map_##type(threadpool thpool, int threads, type* data, int n, void* arg, void (*mapfn)(type*,int,int,void*));

#define parallel_map_define(type) \
  void parallel_map_##type##_mapfn(void* arg) { \
    parallel_map_##type##_arg_t* argp = arg; \
    argp->mapfn(argp->data, argp->start, argp->n, argp->arg); \
  } \
  \
  void parallel_map_##type(threadpool thpool, int threads, type* data, int n, void* arg, void (*mapfn)(type*,int,int,void*)) { \
    int start = 0; \
    for (int i = 0; i < threads; i++) { \
      parallel_map_##type##_arg_t* argp = malloc(sizeof(parallel_map_##type##_arg_t)); \
      int m = n / threads; \
      if (n % threads > i) m++; \
      argp->n = m; \
      argp->start = start; \
      argp->data = data; \
      argp->arg = arg; \
      argp->mapfn = mapfn; \
      thpool_add_work(thpool, parallel_map_##type##_mapfn, argp); \
      start += m; \
    } \
  }

#define parallel_map(type, thpool, threads, data, n, arg, mapfn) \
  parallel_map_##type(thpool, threads, data, n, arg, mapfn);

#endif