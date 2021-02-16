// Copyright © 2018-2021 VMware, Inc. All Rights Reserved.
// SPDX-License-Identifier: BSD-2-Clause

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

#include "proc_maps.h"


intmax_t parse_maps_file(pid_t pid, ProcInfo *procinf, int num_iter) {
  char buffer[BUFSIZ];
  char maps_file[BUFSIZ];
  int maps_fd;
  int offset = 0;

  snprintf(maps_file, sizeof(maps_file), "/proc/%ju/maps", (uintmax_t)pid);
  maps_fd = open(maps_file, O_RDONLY);
  if (maps_fd < 0) {
    perror("open maps");
    return -1;
  }
  int k = 0;
  intmax_t total_size = 0;
  int r_perm, w_perm, x_perm, private;
  for (;;) {
    ssize_t length = read(maps_fd, buffer + offset, sizeof buffer - offset);
    if (length <= 0) break;
    length += offset;
    for (size_t i = offset; i < (size_t)length; i++) {
      uintptr_t low = 0, high = 0;
      if (buffer[i] == '\n' && i) {
        const char *lib_name;
        size_t y;
        /* Parse a line from maps. Each line contains a range that contains many pages. */
        {
          /* low address */
          size_t x = i - 1;
          while (x && buffer[x] != '\n') x--;
          if (buffer[x] == '\n') x++;
          while (buffer[x] != '-' && x < sizeof buffer) {
            char c = buffer[x++];
            low *= 16;
            if (c >= '0' && c <= '9') {
              low += c - '0';
            } else if (c >= 'a' && c <= 'f') {
              low += c - 'a' + 10;
            } else {
              break;
            }
          }
          while (buffer[x] != '-' && x < sizeof buffer) x++;
          if (buffer[x] == '-') x++;
          /* high address */
          while (buffer[x] != ' ' && x < sizeof buffer) {
            char c = buffer[x++];
            high *= 16;
            if (c >= '0' && c <= '9') {
              high += c - '0';
            } else if (c >= 'a' && c <= 'f') {
              high += c - 'a' + 10;
            } else {
              break;
            }
          }

          /* permissions */
          x++;
          r_perm = w_perm = x_perm = 0;
          private = 0;
          while (buffer[x] != ' ' && x < sizeof buffer) {
            char c = buffer[x++];
            if (c == 'r') r_perm = 1;
            else if (c == 'w') w_perm = 1;
            else if (c == 'x') x_perm = 1;
            else if (c == 'p') private = 1;
          }

          /* skip 3 fields */
          for (int field = 0; field < 3; field++) {
            x++;
            while (buffer[x] != ' ' && x < sizeof buffer) x++;
          }

          /* lib name */
          lib_name = 0;
          while (buffer[x] == ' ' && x < sizeof buffer) x++;
          y = x;
          while (buffer[y] != '\n' && y < sizeof buffer) y++;
          buffer[y] = 0;
          lib_name = buffer + x;

          strcpy(procinf->proc_mappings[k].lib_name, lib_name);
          procinf->proc_mappings[k].vaddr_start = low;
          procinf->proc_mappings[k].vaddr_end = high;
          procinf->proc_mappings[k].size = high - low;
          procinf->proc_mappings[k].r_perm = r_perm;
          procinf->proc_mappings[k].w_perm = w_perm;
          procinf->proc_mappings[k].x_perm = x_perm;
          procinf->proc_mappings[k].private = private;
          procinf->proc_mappings[k].local_vaddr_start = 0;
          procinf->proc_mappings[k].local_vaddr_end = 0;

          /* This is the VMA permission, so we only look at writeable VMAs */
          /* Some VMAs can have w_perm, but not r_perm (e.g. infiniband) 
           * we can safely ignore these VMAs */
          if (w_perm & r_perm) {
            total_size += procinf->proc_mappings[k].size;
            /* we only keep VMAs we can read and that are writeable */
            ++k;
          }
        }
        buffer[y] = '\n';

      }
    }
  }

  procinf->num_proc_mappings = k;
  procinf->size = total_size;
  close(maps_fd);
  return total_size;
}

