#include <stdlib.h>
#include <pthread/pthread.h>
#include <string.h>
#include <assert.h>

#include "pool.h"

void pool_new(pool_t* pool, ssize_t capacity) {
  pool->current.generation = 0;
  pool->current.index = 0;
  pool->safe.generation = 0;
  pool->safe.index = 0;
  pool->writers = 0;
  pool->capacity = capacity;
  pool->end_padding = 0;
  pool->buffer = malloc(pool->capacity);
  pool->write_queue.head = 0;
  pool->write_queue.tail = 0;
  pool->write_queue.size = 0;
  pool->write_queue.capacity = 32;
  pthread_mutex_init(&pool->mutex, NULL);
  pthread_cond_init(&pool->cond, NULL);
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

void write_queue_process(pool_t* pool) {
  write_queue_t* write_queue = &pool->write_queue;
  while (
    write_queue->queue[write_queue->head].done
    && write_queue->head < write_queue->tail
  ) {
    ssize_t bytes = write_queue->queue[write_queue->head].bytes;
    pool->safe = increment_index(pool->safe, pool->capacity, bytes);
    write_queue->head++;
  }
  if (write_queue->head == write_queue->tail) {
    write_queue->head = 0;
    write_queue->tail = 0;
  }
}

write_t* pool_alloc_block_for_write(pool_t* pool, ssize_t bytes) {
  pthread_mutex_lock(&pool->mutex);
  void* addr = &pool->buffer[pool->current.index];
  gen_index_t initial = pool->current;
  pool->current = increment_index(pool->current, pool->capacity, bytes);
  if (pool->current.generation > initial.generation) {
    addr = pool->buffer;
    pool->end_padding = (pool->capacity - initial.index) % bytes;
  }
  pool->writers++;
  write_t* write = write_queue_enqueue(&pool->write_queue, bytes);
  pthread_mutex_unlock(&pool->mutex);
  write->buf = addr;
  return write;
}

void pool_commit_write(pool_t* pool, write_t* write) {
  pthread_mutex_lock(&pool->mutex);
  pool->writers--;
  write->done = 1;
  write_queue_process(pool);
  if (pool->writers == 0) pthread_cond_signal(&pool->cond);
  pthread_mutex_unlock(&pool->mutex);
}

ssize_t pool_read(
  pool_t* pool,
  pool_reader_t* pool_reader,
  void** dst,
  ssize_t max_bytes
) {
  pthread_mutex_lock(&pool->mutex);
  if (cmp_index(*pool_reader, pool->safe) != -1) {
    pthread_cond_wait(&pool->cond, &pool->mutex);
  }
  ssize_t end_padding = pool->end_padding;
  if (pool->capacity - pool_reader->index == end_padding) {
    pool_reader->index = 0;
    pool_reader->generation++;
  }
  gen_index_t safe = pool->safe;
  gen_index_t current = pool->current;
  pthread_mutex_unlock(&pool->mutex);
  //reader too far behind writes, lost data
  if (
    pool_reader->index < current.index
    && pool_reader->generation < current.generation
  ) {
    return -1;
  }
  *dst = &pool->buffer[pool_reader->index];
  ssize_t remaining_read_bytes;
  if (pool_reader->generation == safe.generation) {
    remaining_read_bytes = safe.index - pool_reader->index;
  } else {
    remaining_read_bytes = pool->capacity - end_padding - pool_reader->index;
  }
  ssize_t actual_bytes = remaining_read_bytes > max_bytes && max_bytes != -1 ?
    max_bytes : remaining_read_bytes;
  *pool_reader = increment_index(*pool_reader, pool->capacity, actual_bytes);
  return actual_bytes;
}

pool_reader_t pool_new_reader(pool_t* pool) {
  pthread_mutex_lock(&pool->mutex);
  pool_reader_t reader = pool->safe;
  pthread_mutex_unlock(&pool->mutex);
  return reader;
}