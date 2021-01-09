#include <unistd.h>
#include <sys/user.h>

#include "htable.h"


void htable_init(htable_t *htable) {
  /* add space for iter and count */
  htable->tble = malloc((TABLE_SIZE + 1) * 2*sizeof(uint64_t));
  /* iter */
  htable->tble[HTABLE_IDX_ITER] = 0;
  /* count */
  htable->tble[HTABLE_IDX_COUNT] = 0;
}

void htable_add_iter(htable_t *htable, uint64_t iter) {
  htable->tble[HTABLE_IDX_ITER] = iter;
  htable->tble[HTABLE_IDX_COUNT] = 0;
}

void htable_add(htable_t *htable, uintptr_t vaddr, uintptr_t paddr) {
  uintptr_t page_vaddr = (vaddr & PAGE_MASK);
  uintptr_t cl_vaddr = (vaddr & CL_MASK);

  uint64_t tail = htable->tble[HTABLE_IDX_COUNT] + 1;
  int N = (vaddr - page_vaddr) / CL_SIZE;
  if ((tail > 1) && (htable->tble[2*tail - 2] == page_vaddr)) {
    /* same page */
    htable->tble[2*tail-1] |= (1ULL << N);
  } else {
    /* add new page */
    if (tail >= (TABLE_SIZE + 1)) {
      printf("Too many entriesi %lu!\n", tail);
      return;
    }

    htable->tble[2*tail] = page_vaddr;
    htable->tble[2*tail + 1] = (1ULL << N);

    htable->tble[HTABLE_IDX_COUNT]++;
  }
}

void htable_print(htable_t *htable, int fd, uint64_t iter) {
#ifdef PB_WRITE_DCL
  uint64_t count = htable->tble[HTABLE_IDX_COUNT];
  size_t size = 2*sizeof(uint64_t)*(count+1);
  if (count > 0) {
    ssize_t num_bytes = write(fd, (htable->tble), 2 * sizeof(uint64_t) * (count + 1));
    if (num_bytes <= 0) {
      perror("Writing to dcl file failed");
    }
    ASSERT(num_bytes == size);
  }
#endif
}
