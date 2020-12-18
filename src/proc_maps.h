/// proc_maps.h
#ifndef __PROC_MAPS_H__
#define __PROC_MAPS_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

struct Mapping_ {
  /* info about tracee mapping */
  uint64_t vaddr_start;
  uint64_t vaddr_end;
  size_t size;
  int r_perm, w_perm, x_perm;
  /* private(1) or shared(0) */
  int private;
  char lib_name[256];
  /* local address copy of this mapping */
  uint64_t local_vaddr_start;
  uint64_t local_vaddr_end;
};
typedef struct Mapping_     Mapping;

struct ProcInfo_ {
  int fd;
  size_t size;
  uint32_t num_proc_mappings;
  Mapping *proc_mappings;
};
typedef struct ProcInfo_ ProcInfo;

intmax_t parse_maps_file(pid_t pid, ProcInfo *procinf, int num_iter); 

#endif  // __PROC_MAPS_H__
