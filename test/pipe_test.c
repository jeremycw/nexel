#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <pthread/pthread.h>
#include "../src/pipe.h"

void* producer(void* arg) {
  pipe_t* pipe = arg;
  int data[1024];
  for (int i = 0; i < 1024; i++) {
    data[i] = i;
  }
  for (int i = 0; i < 1000; i++) {
    write_t* write = pipe_alloc_block_for_write(pipe, sizeof(data));
    memcpy(write->buf, data, sizeof(data));
    pipe_commit_write(pipe, write);
    usleep(100);
  }
  return NULL;
}

void* consumer(void* arg) {
  pipe_t* pipe = arg;
  int* data;
  int reference[1024];
  for (int i = 0; i < 1024; i++) {
    reference[i] = i;
  }
  for (int i = 0; i < 1000; i++) {
    pipe_reader_t reader;
    pipe_read(pipe, &reader, (void**)&data, sizeof(reference));
    assert(memcmp(data, reference, sizeof(reference)) == 0);
  }
  return NULL;
}

typedef struct {
  pipe_t* pipe;
  char* str;
} str_producer_arg_t;

char* str1 = "Starting on version 2, the development of MATHC uses calendar versioning, with a tag YYYY.MM.DD.MICRO for each stable release. If a release breaks backward compatibility, then it is mentioned in the release notes.";
char* str2 = "By default, vectors, quaternions and matrices can be declared as arrays of mint_t, arrays of mfloat_t, or structures.";

void* str_producer(void* arg) {
  str_producer_arg_t* a = arg;
  pipe_t* pipe = a->pipe;
  char* str = a->str;
  for (int i = 0; i < 2000; i++) {
    int bytes = strlen(str) + 1;
    write_t* write = pipe_alloc_block_for_write(pipe, bytes);
    memcpy(write->buf, str, bytes);
    pipe_commit_write(pipe, write);
    usleep(100);
  }
  return NULL;
}

void* str_consumer(void* arg) {
  pipe_t* pipe = arg;
  int count = 0;
  char* ptr;
  pipe_reader_t reader = pipe_new_reader(pipe);
  ssize_t bytes;
  while (count < 4000) {
    bytes = pipe_read(pipe, &reader, (void**)&ptr, -1);
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

void test_pipe() {
  {
    pipe_t pipe2;
    pipe_new(&pipe2, sizeof(int) * 4);
    pipe_reader_t reader = pipe_new_reader(&pipe2);
    int array[6] = {0, 1, 2, 3, 4, 5};
    write_t* write = pipe_alloc_block_for_write(&pipe2, 4 * sizeof(int));
    memcpy(write->buf, array, sizeof(int) * 4);
    pipe_commit_write(&pipe2, write);
    int* read;
    size_t bytes = pipe_read(&pipe2, &reader, (void**)&read, -1);
    assert(bytes == sizeof(int) * 4);
    for (int i = 0; i < 4; i++) {
      assert(read[i] == i);
    }
    write = pipe_alloc_block_for_write(&pipe2, 2 * sizeof(int));
    memcpy(write->buf, &array[4], sizeof(int) * 2);
    pipe_commit_write(&pipe2, write);
    bytes = pipe_read(&pipe2, &reader, (void**)&read, -1);
    assert(read[0] == 4);
    assert(read[1] == 5);
    write = pipe_alloc_block_for_write(&pipe2, 3 * sizeof(int));
    memcpy(write->buf, array, sizeof(int) * 3);
    pipe_commit_write(&pipe2, write);
    bytes = pipe_read(&pipe2, &reader, (void**)&read, -1);
    assert(bytes == sizeof(int) * 3);
    assert(read[0] == 0);
    assert(read[1] == 1);
    assert(read[2] == 2);
  }

  {
    pipe_t pipe;
    pipe_new(&pipe, 8 * 1024 * 1024);
    pthread_t p, c;
    pthread_create(&p, NULL, producer, &pipe);
    pthread_create(&c, NULL, consumer, &pipe);
    pthread_join(p, NULL);
    pthread_join(c, NULL);
  }

  {
    pipe_t pipe;
    pipe_new(&pipe, 4 * 1024);
    pthread_t p1, p2, c;
    str_producer_arg_t parg1 = { &pipe, str1 };
    str_producer_arg_t parg2 = { &pipe, str2 };
    pthread_create(&c, NULL, str_consumer, &pipe);
    pthread_create(&p1, NULL, str_producer, &parg1);
    pthread_create(&p2, NULL, str_producer, &parg2);
    pthread_join(p1, NULL);
    pthread_join(p2, NULL);
    pthread_join(c, NULL);
  }
}
