#ifndef PIPE_H
#define PIPE_H

#include <pthread/pthread.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
  char* buf;
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
  char* buffer;
  ssize_t capacity;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  ssize_t current;
  ssize_t safe;
  write_queue_t write_queue;
  int end_padding;
  int writers;
} pipe_t;

void pipe_new(pipe_t* pipe, ssize_t capacity);
ssize_t pipe_read(pipe_t* pipe, ssize_t* read_index, void** dst);
write_t* pipe_alloc_block_for_write(pipe_t* pipe, ssize_t bytes);
void pipe_commit_write(pipe_t* pipe, write_t* write);

#endif
