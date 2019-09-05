#include <stdlib.h>
#include <pthread/pthread.h>
#include <string.h>
#include <assert.h>

#include "pipe.h"

void pipe_new(pipe_t* pipe, ssize_t capacity) {
  pipe->current.generation = 0;
  pipe->current.index = 0;
  pipe->safe.generation = 0;
  pipe->safe.index = 0;
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

gen_index_t increment_index(
  gen_index_t gi,
  ssize_t capacity,
  ssize_t increment
) {
  ssize_t initial = gi.index;
  if (gi.index + increment > capacity) {
    gi.index = increment;
    gi.generation++;
  } else {
    gi.index = (gi.index + increment) % capacity;
    if (gi.index < initial || increment == capacity) gi.generation++;
  }
  return gi;
}

int cmp_index(gen_index_t a, gen_index_t b) {
  if (
    a.generation > b.generation
    || (a.index > b.index && a.generation == b.generation)
  ) {
    return 1;
  } else if (a.index == b.index && a.generation == b.generation) {
    return 0;
  } else {
    return -1;
  }
}

void write_queue_process(pipe_t* pipe) {
  write_queue_t* write_queue = &pipe->write_queue;
  while (
    write_queue->queue[write_queue->head].done
    && write_queue->head < write_queue->tail
  ) {
    ssize_t bytes = write_queue->queue[write_queue->head].bytes;
    pipe->safe = increment_index(pipe->safe, pipe->capacity, bytes);
    write_queue->head++;
  }
  if (write_queue->head == write_queue->tail) {
    write_queue->head = 0;
    write_queue->tail = 0;
  }
}

write_t* pipe_alloc_block_for_write(pipe_t* pipe, ssize_t bytes) {
  pthread_mutex_lock(&pipe->mutex);
  void* addr = &pipe->buffer[pipe->current.index];
  gen_index_t initial = pipe->current;
  pipe->current = increment_index(pipe->current, pipe->capacity, bytes);
  if (pipe->current.generation > initial.generation) {
    addr = pipe->buffer;
    pipe->end_padding = (pipe->capacity - initial.index) % bytes;
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

ssize_t pipe_read(
  pipe_t* pipe,
  pipe_reader_t* pipe_reader,
  void** dst,
  ssize_t max_bytes
) {
  pthread_mutex_lock(&pipe->mutex);
  if (cmp_index(*pipe_reader, pipe->safe) != -1) {
    pthread_cond_wait(&pipe->cond, &pipe->mutex);
  }
  ssize_t end_padding = pipe->end_padding;
  if (pipe->capacity - pipe_reader->index == end_padding) {
    pipe_reader->index = 0;
    pipe_reader->generation++;
  }
  gen_index_t safe = pipe->safe;
  gen_index_t current = pipe->current;
  pthread_mutex_unlock(&pipe->mutex);
  //reader too far behind writes, lost data
  if (
    pipe_reader->index < current.index
    && pipe_reader->generation < current.generation
  ) {
    return -1;
  }
  *dst = &pipe->buffer[pipe_reader->index];
  ssize_t remaining_read_bytes;
  if (pipe_reader->generation == safe.generation) {
    remaining_read_bytes = safe.index - pipe_reader->index;
  } else {
    remaining_read_bytes = pipe->capacity - end_padding - pipe_reader->index;
  }
  ssize_t actual_bytes = remaining_read_bytes > max_bytes && max_bytes != -1 ?
    max_bytes : remaining_read_bytes;
  *pipe_reader = increment_index(*pipe_reader, pipe->capacity, actual_bytes);
  return actual_bytes;
}

pipe_reader_t pipe_new_reader(pipe_t* pipe) {
  pthread_mutex_lock(&pipe->mutex);
  pipe_reader_t reader = pipe->safe;
  pthread_mutex_unlock(&pipe->mutex);
  return reader;
}
