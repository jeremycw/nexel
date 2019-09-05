#ifndef PIPE_H
#define PIPE_H

#include <pthread/pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define PIPE_NO_FLAGS 0x0
#define PIPE_WAIT_FOR_DATA 0x1

typedef struct {
  ssize_t index;
  int generation;
} gen_index_t;

typedef gen_index_t pipe_reader_t;

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
} pipe_t;

void pipe_new(pipe_t* pipe, ssize_t capacity);
ssize_t pipe_read(pipe_t* pipe, pipe_reader_t* pipe_reader, void** dst, ssize_t bytes);
pipe_reader_t pipe_new_reader(pipe_t* pipe);
write_t* pipe_alloc_block_for_write(pipe_t* pipe, ssize_t bytes);
void pipe_commit_write(pipe_t* pipe, write_t* write);

#endif
