#ifndef __HTABLE_H__
#define __HTABLE_H__

#include <stdint.h>

#include "util.h"

#define TABLE_SIZE 10000000


enum {
  HTABLE_IDX_ITER,
  HTABLE_IDX_COUNT,
  HTABLE_TOTAL_IDX
};

typedef struct {
#if 0
  uint64_t tail;
  uintptr_t *table;
  uintptr_t *bitmap;
#else 
  uint64_t *tble;
#endif
} htable_t;

void htable_init(htable_t *htable);
void htable_add_iter(htable_t *htable, uint64_t iter);
void htable_add(htable_t *htable, uintptr_t vaddr, uintptr_t paddr);
void htable_print(htable_t *htable, int fd, uint64_t num_iter);


#endif  // __HTABLE_H__
