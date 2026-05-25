#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

void *instrumented_malloc(size_t size) {
  void *ptr = malloc(size);
  fprintf(stderr, "malloc %p %d\n", ptr, (int)size);

  return ptr;
}

// Optionally also wrap free, calloc, etc.
void instrumented_free(void *ptr) {
  free(ptr);
  fprintf(stderr, "free %p\n", ptr);
}