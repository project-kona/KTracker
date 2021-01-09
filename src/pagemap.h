#ifndef __PAGEMAP_H__
#define __PAGEMAP_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
  uint64_t pfn : 54;
  unsigned int soft_dirty : 1;
  unsigned int file_page : 1;
  unsigned int swapped : 1;
  unsigned int present : 1;
} PagemapEntry;

/* Parse the pagemap entry for the given virtual address.
   *
   *  * @param[out] entry      the parsed entry
   *  * @param[in]  pagemap_fd file descriptor to an open /proc/pid/pagemap file
   *  * @param[in]  vaddr      virtual address to get entry for
   *  * @return 0 for success, 1 for failure
   *  */
int pagemap_get_entry(PagemapEntry *entry, int pagemap_fd, uintptr_t vaddr);

/* Convert the given virtual address to physical using /proc/PID/pagemap.                                                                                                                              *                                                                                                                                                                                                     * @param[out] paddr physical address                                                                                                                                                                  * @param[in]  pid   process to convert for                                                                                                                                                            * @param[in] vaddr virtual address to get entry for                                                                                                                                                  
 * * @return 0 for success, 1 for failure
 * */
int virt_to_phys_user(uintptr_t *paddr, pid_t pid, uintptr_t vaddr);

uint64_t parse_pagemap_file(pid_t pid, uint64_t vaddr);

#endif  // __PAGEMAP_H__
