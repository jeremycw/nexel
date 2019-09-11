#include <string.h>
#include <stdlib.h>
#include "merge_sort.h"

void merge(void* a, int an, void* b, int bn, void* out, int stride, int* index, int (*cmp)(void*,void*)) {
  int ai = 0;
  int bi = 0;
  while (ai != an || bi != bn) {
    if (
      bi == bn //a2 is empty
      || (
        ai != an //a1 not empty
        && cmp(&a[ai], &b[bi])
      )
    ) {
      memcpy(&out[*index], &a[ai], stride);
      ai += stride;
    } else {
      memcpy(&out[*index], &b[bi], stride);
      bi += stride;
    }
    *index += stride;
  }

}

void merge_sort(void* in, int n, int stride, int (*cmp)(void*,void*)) {
  int size = 1;
  void* array = in;
  int total_bytes = n * stride;
  void* aux = malloc(total_bytes);
  while (size < n) {
    int index = 0;
    int partition_bytes = size * stride;
    for (int base = 0; base < total_bytes; base += partition_bytes * 2) {
      void* a1 = &array[base];
      void* a2 = &array[base + partition_bytes];
      int a2size = base + partition_bytes * 2 > total_bytes ?
        total_bytes % partition_bytes : partition_bytes;
      merge(a1, partition_bytes, a2, a2size, aux, stride, &index, cmp);
    }
    size *= 2;
    void* tmp = array;
    array = aux;
    aux = tmp;
  }
  if (array != in) {
    memcpy(in, array, total_bytes);
    free(array);
  } else {
    free(aux);
  }
}
