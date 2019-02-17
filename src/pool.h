#ifndef POOL_H
#define POOL_H

#include <pthread/pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define POOL_NO_FLAGS 0x0
#define POOL_WAIT_FOR_DATA 0x1

typedef struct {
  ssize_t index;
  int generation;
} gen_index_t;

typedef gen_index_t pool_reader_t;

typedef struct {
  void* buf;
  ssize_t bytes;
  int done;
} write_t;

typedef struct {
  write_t queue[32];
  int tail;
  int head;
  int size;
  int capacity;
} write_queue_t;

typedef struct {
  void* buffer;
  ssize_t capacity;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  gen_index_t current;
  gen_index_t safe;
  write_queue_t write_queue;
  int end_padding;
  int writers;
} pool_t;

void pool_new(pool_t* pool, ssize_t capacity);
ssize_t pool_read(pool_t* pool, pool_reader_t* pool_reader, void** dst, ssize_t bytes);
pool_reader_t pool_new_reader(pool_t* pool);
write_t* pool_alloc_block_for_write(pool_t* pool, ssize_t bytes);
void pool_commit_write(pool_t* pool, write_t* write);

#endif