#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#include "pagemap.h"

int pagemap_get_entry(PagemapEntry *entry, int pagemap_fd, uintptr_t vaddr) {
  size_t nread;
  ssize_t ret;
  uint64_t data;

  nread = 0;
  while (nread < sizeof(data)) {
    ret = pread(pagemap_fd, &data, sizeof(data),
        (vaddr / sysconf(_SC_PAGE_SIZE)) * sizeof(data) + nread);
    nread += ret;
    if (ret <= 0) {
      return 1;
    }
  }
  entry->pfn = data & (((uint64_t)1 << 54) - 1);
  entry->soft_dirty = (data >> 54) & 1;
  entry->file_page = (data >> 61) & 1;
  entry->swapped = (data >> 62) & 1;
  entry->present = (data >> 63) & 1;
  return 0;
}

int virt_to_phys_user(uintptr_t *paddr, pid_t pid, uintptr_t vaddr) { 
  char pagemap_file[BUFSIZ];
  int pagemap_fd;

  snprintf(pagemap_file, sizeof(pagemap_file), "/proc/%ju/pagemap", (uintmax_t)pid);
  pagemap_fd = open(pagemap_file, O_RDONLY);
  if (pagemap_fd < 0) {
    return 1;
  }
  PagemapEntry entry;
  if (pagemap_get_entry(&entry, pagemap_fd, vaddr)) {
    return 1;
  }
  close(pagemap_fd);
  *paddr = (entry.pfn * sysconf(_SC_PAGE_SIZE)) + (vaddr % sysconf(_SC_PAGE_SIZE));
  return 0;
}

uint64_t parse_pagemap_file(pid_t pid, uint64_t vaddr) {
  char buffer[BUFSIZ];
  char pagemap_file[BUFSIZ];
  int offset = 0;
  int pagemap_fd;

  snprintf(pagemap_file, sizeof(pagemap_file), "/proc/%ju/pagemap", (uintmax_t)pid);
  pagemap_fd = open(pagemap_file, O_RDONLY);
  if (pagemap_fd < 0) {
    perror("open pagemap");
    return 0;
  }

  uintmax_t paddr = 0;
  PagemapEntry entry;
  if (!pagemap_get_entry(&entry, pagemap_fd, vaddr)) {
    paddr = (uintmax_t) entry.pfn;
  }
  close(pagemap_fd);
  return paddr;
}
