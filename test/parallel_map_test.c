#include <string.h>
#include <assert.h>
#include "../src/thpool.h"
#include "../src/parallel_map.h"

parallel_map_declare(int)
parallel_map_define(int)

void mapfn(int* data, int start, int n, void* arg) {
  (void)arg;
  data = &data[start];
  for (int i = 0; i < n; i++) {
    data[i] = start;
  }
}

void test_parallel_map() {
  int data[100];
  memset(data, 2, sizeof(data));
  threadpool thpool = thpool_init(4);
  parallel_map(int, thpool, 4, data, 100, NULL, mapfn);
  thpool_wait(thpool);
  assert(data[0] == 0);
  assert(data[25] == 25);
  assert(data[50] == 50);
  assert(data[75] == 75);
}