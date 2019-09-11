#include <stdlib.h>
#include <pthread/pthread.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "pipe.h"

void pipe_new(pipe_t* pipe, ssize_t capacity) {
  pipe->current = 0;
  pipe->safe = 0;
  pipe->writers = 0;
  pipe->capacity = capacity;
  pipe->end_padding = 0;
  pipe->buffer = malloc(pipe->capacity);
  pipe->write_queue.head = 0;
  pipe->write_queue.tail = 0;
  pipe->write_queue.size = 0;
  pipe->write_queue.capacity = 32;
  pthread_mutex_init(&pipe->mutex, NULL);
  pthread_cond_init(&pipe->cond, NULL);
}

write_t* write_queue_enqueue(write_queue_t* write_queue, ssize_t bytes) {
  assert(write_queue->tail < write_queue->capacity);
  write_queue->queue[write_queue->tail].done = 0;
  write_queue->queue[write_queue->tail].bytes = bytes;
  write_t* w = &write_queue->queue[write_queue->tail];
  write_queue->tail = (write_queue->tail + 1) % write_queue->capacity;
  return w;
}

int increment_index(ssize_t* index, ssize_t capacity, ssize_t inc) {
  ssize_t old = *index;
  ssize_t new = *index + inc;
  ssize_t new_i = new % capacity;
  ssize_t old_i = old % capacity;
  int wrapped = new_i < old_i;
  if (wrapped) {
    *index = capacity * (new / capacity) + inc;
  } else {
    *index = new;
  }
  return wrapped || inc == capacity;
}

int cmp_index(ssize_t a, ssize_t b) {
  return a - b;
}

void write_queue_process(pipe_t* pipe) {
  write_queue_t* write_queue = &pipe->write_queue;
  while (
    write_queue->queue[write_queue->head].done
    && write_queue->head < write_queue->tail
  ) {
    ssize_t bytes = write_queue->queue[write_queue->head].bytes;
    increment_index(&pipe->safe, pipe->capacity, bytes);
    write_queue->head++;
  }
  if (write_queue->head == write_queue->tail) {
    write_queue->head = 0;
    write_queue->tail = 0;
  }
}

write_t* pipe_alloc_block_for_write(pipe_t* pipe, ssize_t bytes) {
  pthread_mutex_lock(&pipe->mutex);
  char* addr = &pipe->buffer[pipe->current % pipe->capacity];
  ssize_t old = pipe->current;
  ssize_t old_i = old % pipe->capacity;
  int wrapped = increment_index(&pipe->current, pipe->capacity, bytes);
  if (wrapped) {
    addr = pipe->buffer;
    pipe->end_padding = (pipe->capacity - old_i) % bytes;
  }
  pipe->writers++;
  write_t* write = write_queue_enqueue(&pipe->write_queue, bytes);
  pthread_mutex_unlock(&pipe->mutex);
  write->buf = addr;
  return write;
}

void pipe_commit_write(pipe_t* pipe, write_t* write) {
  pthread_mutex_lock(&pipe->mutex);
  pipe->writers--;
  write->done = 1;
  write_queue_process(pipe);
  if (pipe->writers == 0) pthread_cond_signal(&pipe->cond);
  pthread_mutex_unlock(&pipe->mutex);
}

ssize_t pipe_read(pipe_t* pipe, ssize_t* read_index, void** dst) {
  pthread_mutex_lock(&pipe->mutex);
  if (cmp_index(*read_index, pipe->safe) >= 0) {
    pthread_cond_wait(&pipe->cond, &pipe->mutex);
  }
  ssize_t r_i = *read_index % pipe->capacity;
  ssize_t end_padding = pipe->end_padding;
  if (pipe->capacity - r_i == end_padding) {
    *read_index += end_padding;
    r_i = 0;
  }
  pthread_mutex_unlock(&pipe->mutex);

  //reader too far behind writes, lost data
  if (pipe->current - *read_index > pipe->capacity) return -1;

  *dst = &pipe->buffer[r_i];
  ssize_t w_i = pipe->safe % pipe->capacity;
  ssize_t remaining_read_bytes;
  if (r_i < w_i) {
    remaining_read_bytes = w_i - r_i; 
  } else {
    remaining_read_bytes = pipe->capacity - end_padding - r_i;
  }
  increment_index(read_index, pipe->capacity, remaining_read_bytes);
  return remaining_read_bytes;
}
