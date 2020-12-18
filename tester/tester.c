#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

//#define GBSIZE (1024L*1024L*1024L)
//#define GBSIZE (1024L*4*1024)
#define GBSIZE (1024L*4)  
#define MAXCL  64
#define PAGESIZE 4096
#define DEFAULT_GBYTES 16

#define USEC    10*1000*1000
#define NTIMES  1024

int main(int argc, char **argv) {
  unsigned int microsecs = USEC;

  if (argc < 3) {
    printf("Usage: %s <num-dirty-cl-per-page> <num-GB>\n", argv[0]);
    return -1;
  }

  int ncl = atoi(argv[1]) % (MAXCL + 1);
  unsigned long long bytes = 0;
  unsigned long long gbytes = atoi(argv[2]);
  if (gbytes < 0) gbytes = DEFAULT_GBYTES;

  bytes = gbytes * GBSIZE;
  printf("Dirtying %d cache lines per page in %llu GB (%llu bytes)...\n", ncl, gbytes, bytes);
  uint8_t *mem = (uint8_t*)aligned_alloc(PAGESIZE, bytes * sizeof(uint8_t));
  memset(mem, 0, bytes*sizeof(uint8_t));

  char c;
  for (size_t k = 1; k < NTIMES; ++k) {
    for (size_t i = 0; i < bytes; i += PAGESIZE) {
      //printf("%p\n", &mem[i]);
      for (size_t j = 0; j < MAXCL * ncl; j += MAXCL) {
        mem[i + j] = k;
        printf("%p\n", &mem[i+j]);
      }
    }
    //usleep(microsecs);
    printf("Done round %lu - presss any key to continue...\n", k);
    getchar();
  }

  printf("Done\n");
  while(1);
  free(mem);
}
