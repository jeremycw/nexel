#ifndef MERGE_SORT_H
#define MERGE_SORT_H

void merge_sort(void* in, int n, int stride, int (*cmp)(void const*,void const*));
void merge(void* a, int an, void* b, int bn, void* out, int stride, int* index, int (*cmp)(void const*,void const*));

#endif
