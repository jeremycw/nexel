#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <pthread/pthread.h>
#include "../src/pool.h"

void* producer(void* arg) {
  pool_t* pool = arg;
  int data[1024];
  for (int i = 0; i < 1024; i++) {
    data[i] = i;
  }
  for (int i = 0; i < 1000; i++) {
    write_t* write = pool_alloc_block_for_write(pool, sizeof(data));
    memcpy(write->buf, data, sizeof(data));
    pool_commit_write(pool, write);
    usleep(100);
  }
  return NULL;
}

void* consumer(void* arg) {
  pool_t* pool = arg;
  int* data;
  int reference[1024];
  for (int i = 0; i < 1024; i++) {
    reference[i] = i;
  }
  for (int i = 0; i < 1000; i++) {
    pool_reader_t reader;
    pool_read(pool, &reader, (void**)&data, sizeof(reference));
    assert(memcmp(data, reference, sizeof(reference)) == 0);
  }
  return NULL;
}

typedef struct {
  pool_t* pool;
  char* str;
} str_producer_arg_t;

char* str1 = "Starting on version 2, the development of MATHC uses calendar versioning, with a tag YYYY.MM.DD.MICRO for each stable release. If a release breaks backward compatibility, then it is mentioned in the release notes.";
char* str2 = "By default, vectors, quaternions and matrices can be declared as arrays of mint_t, arrays of mfloat_t, or structures.";

void* str_producer(void* arg) {
  str_producer_arg_t* a = arg;
  pool_t* pool = a->pool;
  char* str = a->str;
  for (int i = 0; i < 2000; i++) {
    int bytes = strlen(str) + 1;
    write_t* write = pool_alloc_block_for_write(pool, bytes);
    memcpy(write->buf, str, bytes);
    pool_commit_write(pool, write);
    usleep(100);
  }
  return NULL;
}

void* str_consumer(void* arg) {
  pool_t* pool = arg;
  int count = 0;
  char* ptr;
  pool_reader_t reader = pool_new_reader(pool);
  ssize_t bytes;
  while (count < 4000) {
    bytes = pool_read(pool, &reader, (void**)&ptr, -1);
    assert(bytes > 0);
    while (bytes > 0) {
      assert(
        strcmp(ptr, str1) == 0
        || strcmp(ptr, str2) == 0
      );
      int len = strlen(ptr);
      bytes -= len + 1;
      ptr += len+1;
      count++;
    }
  }
  return NULL;
}

void test_pool() {
  {
    pool_t pool2;
    pool_new(&pool2, sizeof(int) * 4);
    pool_reader_t reader = pool_new_reader(&pool2);
    int array[6] = {0, 1, 2, 3, 4, 5};
    write_t* write = pool_alloc_block_for_write(&pool2, 4 * sizeof(int));
    memcpy(write->buf, array, sizeof(int) * 4);
    pool_commit_write(&pool2, write);
    int* read;
    size_t bytes = pool_read(&pool2, &reader, (void**)&read, -1);
    assert(bytes == sizeof(int) * 4);
    for (int i = 0; i < 4; i++) {
      assert(read[i] == i);
    }
    write = pool_alloc_block_for_write(&pool2, 2 * sizeof(int));
    memcpy(write->buf, &array[4], sizeof(int) * 2);
    pool_commit_write(&pool2, write);
    bytes = pool_read(&pool2, &reader, (void**)&read, -1);
    assert(read[0] == 4);
    assert(read[1] == 5);
    write = pool_alloc_block_for_write(&pool2, 3 * sizeof(int));
    memcpy(write->buf, array, sizeof(int) * 3);
    pool_commit_write(&pool2, write);
    bytes = pool_read(&pool2, &reader, (void**)&read, -1);
    assert(bytes == sizeof(int) * 3);
    assert(read[0] == 0);
    assert(read[1] == 1);
    assert(read[2] == 2);
  }

  {
    pool_t pool;
    pool_new(&pool, 8 * 1024 * 1024);
    pthread_t p, c;
    pthread_create(&p, NULL, producer, &pool);
    pthread_create(&c, NULL, consumer, &pool);
    pthread_join(p, NULL);
    pthread_join(c, NULL);
  }

  {
    pool_t pool;
    pool_new(&pool, 4 * 1024);
    pthread_t p1, p2, c;
    str_producer_arg_t parg1 = { &pool, str1 };
    str_producer_arg_t parg2 = { &pool, str2 };
    pthread_create(&c, NULL, str_consumer, &pool);
    pthread_create(&p1, NULL, str_producer, &parg1);
    pthread_create(&p2, NULL, str_producer, &parg2);
    pthread_join(p1, NULL);
    pthread_join(p2, NULL);
    pthread_join(c, NULL);
  }
}