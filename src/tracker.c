// Copyright © 2018-2021 VMware, Inc. All Rights Reserved.
// SPDX-License-Identifier: BSD-2-Clause

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <dirent.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/prctl.h>
#include <sys/user.h>

#include <linux/ptrace.h>

#include "common.h"
#include "numalib.h"
#include "ptimer.h"
#include "pagemap.h"
#include "proc_maps.h"

#include "htable.h"

htable_t htable;

void find_dirty_data(pid_t pid, ProcInfo *procinf, int num_iter) {
  struct iovec local[1];
  struct iovec remote[1];
  uint8_t *page = (uint8_t*) aligned_alloc(PAGE_SIZE, PAGE_SIZE);
  local[0].iov_base = page;
  local[0].iov_len = PAGE_SIZE;
  remote[0].iov_len = PAGE_SIZE;

  int fd_memdump = open_memdump_file(pid, num_iter);
  int fd_dcl = open_dcl_file(pid);

#if PB_WRITE_DIRTYCL
  FILE *dirtyclfile;
  char dirtyclfilename[MAX_FILE_LEN];
  sprintf(dirtyclfilename, "dirtycl%d.txt", num_iter);
  dirtyclfile = fopen(dirtyclfilename, "a");
#endif

  htable_add_iter(&htable,num_iter);

  pr_debug("Writing %u mappings\n", procinf->num_proc_mappings);

  int numb;
  int size;
  int num_pages = 0;
  int num_dirty_cl = 0;
  int num_dirty_pages = 0;
  for (uint32_t i = 0; i < procinf->num_proc_mappings; ++i) {
    uint8_t *nextmem = (void*)procinf->proc_mappings[i].local_vaddr_start;
    num_pages = 0;
    /* We only look at writeable VMAs. Note: we can't skip read-only pages, because we might have missed the protection 
       change on the page, but it's a safe optimization to only look at VMAs that have w_perm */ 
    if (procinf->proc_mappings[i].w_perm && procinf->proc_mappings[i].r_perm) {
      size = 0;
      while (size < procinf->proc_mappings[i].size) {
        remote[0].iov_base = (void*)procinf->proc_mappings[i].vaddr_start + size;
        int r = process_vm_readv(pid, local, 1, remote, 1, 0);
        if (r < 0) {
          perror("Error reading memory region!");
          printf("Error in process_vm_readv %d at address %p\n", errno, remote[0].iov_base);
          if (errno == ESRCH) break;
        } else if (r != PAGE_SIZE) {
          printf("Didn't read full page, read %d bytes\n", r);
        } else {
          int diff_cl = 0;
          uint8_t *nextcl_local = nextmem;
          uint8_t *nextcl_remote = page;
          while (nextcl_remote < page + PAGE_SIZE) {
            if (memcmp(nextcl_local, nextcl_remote, CLSIZE) || !num_iter) {
              diff_cl++;
              uintptr_t vaddr = (uintptr_t)remote[0].iov_base + nextcl_local - nextmem;
              uintptr_t paddr = 0;
#if PB_GET_PHYSICAL_ADDR
              if (virt_to_phys_user(&paddr, pid, vaddr) != 0) paddr = 0;
#endif
#if PB_WRITE_DIRTYCL
              fprintf(dirtyclfile, "Dirty CL: vaddr %p paddr %p\n", (void*)vaddr, (void*)paddr);
#endif
              htable_add(&htable, vaddr, paddr);
            }
            nextcl_local += CLSIZE;
            nextcl_remote += CLSIZE;
          }

          write_memdump_file(fd_memdump, page, PAGE_SIZE);

          if (diff_cl) {
            memcpy(nextmem, page, PAGE_SIZE);
            num_dirty_cl += diff_cl;
            ++num_dirty_pages;
          }
          ++num_pages; 
        }
        size += PAGE_SIZE;
        nextmem += PAGE_SIZE;
      }
    }
  }

#if PB_WRITE_ITER_STATS
  printf("===========>>>>>>>>> iteration %d DIRTY CACHE LINES %d PAGES %d writable size: %lu cl2pages %0.2f cl2all %0.2f pages2all %0.2f\n",
      num_iter, num_dirty_cl, num_dirty_pages, procinf->size, 
      (double)num_dirty_cl*64*100/((double)num_dirty_pages * 4096), 
      (double)num_dirty_cl*64*100/procinf->size, 
      (double)num_dirty_pages*4096*100/procinf->size);
#endif
#if PB_WRITE_TEXT_OUTPUT
  char output[MAX_OUTPUT_LINE];
  sprintf(output, "\niteration: %d\ndirty_cl: %d\ndirty_pages: %d\ncl2pages: %0.2f\nwritable_size: %lu\ncl2all: %0.2f\npages2all: %0.2f\n",
      num_iter, num_dirty_cl, num_dirty_pages,
      (double)num_dirty_cl*64*100/((double)num_dirty_pages * 4096), procinf->size,
      (double)num_dirty_cl*64*100/procinf->size,
      (double)num_dirty_pages*4096*100/procinf->size);
  print_output(pid, output);
#endif

  htable_print(&htable, fd_dcl, num_iter);

#if PB_WRITE_DIRTYCL
  fclose(dirtyclfile); 
#endif

  close_memdump_file(fd_memdump);
  close_dcl_file(fd_dcl);

  free(page);
}

uint8_t *xmmap_proc_mapping(size_t size) {
  uint8_t *mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mem == MAP_FAILED) {
    perror("mmap failed for process mapping copy");
    assert(0);
  }
  return mem;
}

void update_local_maps(ProcInfo *procinf, ProcInfo *procinf2, int num_iter) {
  uint32_t k = 0;
  uint32_t k2 = 0; 
  uint8_t *mem = 0;
  size_t size = 0;
  assert(procinf);
  if (procinf2) {
    while ((k < procinf->num_proc_mappings) && (k2 < procinf2->num_proc_mappings)) {
      if (procinf2->proc_mappings[k2].vaddr_start == procinf->proc_mappings[k].vaddr_start) {
        if (procinf2->proc_mappings[k2].vaddr_end == procinf->proc_mappings[k].vaddr_end) {
          /* same mapping */
          procinf2->proc_mappings[k2].local_vaddr_start = procinf->proc_mappings[k].local_vaddr_start;
          procinf2->proc_mappings[k2].local_vaddr_end = procinf->proc_mappings[k].local_vaddr_end;
          k++; k2++;
        } else {
          /* same start, different end */
          // TODO: for now, we treat this as different mappings, but have to copy the old data. Could optimize this by reusing all or part of the old memory.
          if (procinf2->proc_mappings[k2].local_vaddr_start == 0) {
            size = procinf2->proc_mappings[k2].size;
            mem = xmmap_proc_mapping(size);
            memset(mem, 0, size);
            procinf2->proc_mappings[k2].local_vaddr_start = (uint64_t) mem;
          }
          if (procinf2->proc_mappings[k2].size < procinf->proc_mappings[k].size) {
            memcpy(mem, (void*)procinf->proc_mappings[k].local_vaddr_start, procinf2->proc_mappings[k2].size);
            k2++;  
          } else {
            /* (procinf2->proc_mappings[k2].size > procinf->proc_mappings[k].size) */
            memcpy(mem, (void*)procinf->proc_mappings[k].local_vaddr_start, procinf->proc_mappings[k].size);
            k++; 
          }
        }
      } else {
        if (procinf2->proc_mappings[k2].local_vaddr_start == 0) {
          size = procinf2->proc_mappings[k2].size;
          mem = xmmap_proc_mapping(size);
          memset(mem, 0, size);
          procinf2->proc_mappings[k2].local_vaddr_start = (uint64_t) mem;
        }
        if (procinf2->proc_mappings[k2].vaddr_start < procinf->proc_mappings[k].vaddr_start) {
          if (procinf2->proc_mappings[k2].vaddr_end <= procinf->proc_mappings[k].vaddr_start) {
            k2++;
          } else {
            if (procinf2->proc_mappings[k2].vaddr_end >= procinf->proc_mappings[k].vaddr_end) {
              mem = (void*)(procinf2->proc_mappings[k2].local_vaddr_start + procinf->proc_mappings[k].vaddr_start - procinf2->proc_mappings[k2].vaddr_start);
              memcpy(mem, (void*)procinf->proc_mappings[k].local_vaddr_start, procinf->proc_mappings[k].size);
              k++;
            } else {
              mem = (void*)(procinf2->proc_mappings[k2].local_vaddr_start + procinf->proc_mappings[k].vaddr_start - procinf2->proc_mappings[k2].vaddr_start);
              size = procinf2->proc_mappings[k2].vaddr_end - procinf->proc_mappings[k].vaddr_start; 
              memcpy(mem, (void*)procinf->proc_mappings[k].local_vaddr_start, size);
              k2++;
            }
          }
        } else {
          /* (procinf2->proc_mappings[k2].vaddr_start > procinf->proc_mappings[k].vaddr_start) */
          if (procinf2->proc_mappings[k2].vaddr_start >= procinf->proc_mappings[k].vaddr_end) {
            k++;
          } else {
            if (procinf2->proc_mappings[k2].vaddr_end >= procinf->proc_mappings[k].vaddr_end) {
              mem = (void*)(procinf->proc_mappings[k].local_vaddr_start + procinf2->proc_mappings[k2].vaddr_start - procinf->proc_mappings[k].vaddr_start);
              size = procinf->proc_mappings[k].vaddr_end - procinf2->proc_mappings[k2].vaddr_start;
              memcpy((void*)procinf2->proc_mappings[k2].local_vaddr_start, mem, size);
              k++;
            } else {
              mem = (void*)(procinf->proc_mappings[k].local_vaddr_start + procinf2->proc_mappings[k2].vaddr_start - procinf->proc_mappings[k].vaddr_start);
              memcpy((void*)procinf2->proc_mappings[k2].local_vaddr_start, mem, procinf2->proc_mappings[k2].size);
              k2++;
            }
          }
        }
      }
    }
  } else {
    /* allocate memory copy for the first time */
    for (k = 0; k < procinf->num_proc_mappings; ++k) {
      size = procinf->proc_mappings[k].size;
      assert(size > 0);
      mem = xmmap_proc_mapping(size);
      memset(mem, 0, size);
      procinf->proc_mappings[k].local_vaddr_start = (uint64_t)mem;
      procinf->proc_mappings[k].local_vaddr_end = (uint64_t) mem + size;
    }
  }
}


void proc_trace(pid_t pid, pid_t *tid, size_t tids) {
  printf("Proc Trace \n");
  ProcInfo *procinf = (ProcInfo *) malloc(sizeof(ProcInfo));
  ProcInfo *procinf2 = (ProcInfo *) malloc(sizeof(ProcInfo));
  procinf->proc_mappings = (Mapping *)malloc(MAX_PROC_MAPPINGS * sizeof(Mapping));
  procinf2->proc_mappings = (Mapping *)malloc(MAX_PROC_MAPPINGS * sizeof(Mapping));

  ProcInfo *tmp;
  _num_iter = 0;

  htable_init(&htable);

#if defined (NO_DIRTY_TRACKING)
#elif defined (NO_DIRTY_TRACKING_SLEEP)
  sleep(DO_NOTHING_TIME);
#else
  parse_maps_file(pid, procinf, _num_iter);
  intmax_t size = procinf->size;
  update_local_maps(procinf, NULL, _num_iter);
  find_dirty_data(pid, procinf, _num_iter);
#endif

  /* main tracer loop*/
  while(!_stop_requested) {
    ++_num_iter;
    proc_resume_all(pid, tid, tids);
    pr_debug("Iteration: %d Sleeping for %d seconds...\n", _num_iter, SLEEP_TIME);
    sleep(SLEEP_TIME);
    if (_stop_requested) break;
    int paused = proc_pause_all(pid, tid, tids);

    if (!paused || _stop_requested) break;

#if defined (NO_DIRTY_TRACKING)
#elif defined (NO_DIRTY_TRACKING_SLEEP)
    sleep(DO_NOTHING_TIME);
#else
    parse_maps_file(pid, procinf2, _num_iter);
    update_local_maps(procinf, procinf2, _num_iter);
    find_dirty_data(pid, procinf2, _num_iter);

    tmp = procinf;
    procinf = procinf2;
    procinf2 = tmp;
#endif
  }

  print_final_stats(pid);

  free(procinf->proc_mappings);
  free(procinf2->proc_mappings);
  free(procinf);
  free(procinf2);
}


int main(int argc, char **argv) {
  _stop_requested = 0;
  _app_time = _paused_time = _wprotect_time = 0;
  _config.parent = 0;

  ptimer_init(&_wp_ptimer);
  ptimer_init(&_paused_timer);
  ptimer_init(&_app_timer);

  ASSERTZ(register_signal_handler());

  pid_t pid = process_input(argc, argv);
  proc_seize(pid);

  tid = 0;
  tids = 0;
  tids_max = 0;
  num_procs = 1;

  tids = get_tids(&tid, &tids_max, pid);
  if (!tids) {
    /* process exited */
    assert(0);
  } else {
    assert(tid[0] == pid);
    printf("Tid %p tids %lu tids_max %lu\n", tid, tids, tids_max);
    for (int i = 0; i < tids; i++) {
      printf("TID %d\n", tid[i]);
    }
  }


  proc_seize_and_pause(pid, tid, tids);


  proc_trace(pid, tid, tids);

  return 0;
}
