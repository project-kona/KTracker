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

/**************************************************************/

struct sigaction old_sigint_action;

void print_final_stats(pid_t pid) {
  char output[MAX_OUTPUT_LINE];
  char resultsfilename[MAX_FILE_LEN];
  sprintf(resultsfilename, "res_pbsim_%d.txt", pid);
  FILE *resultsfile = fopen(resultsfilename, "w");

  sprintf(output, "Iterations:%lu\nApp_time_ms:%.4f\nPaused_time_ms:%.4f\nWp_time_ms:%.4f\n",
          _num_iter, _app_time/1000, _paused_time, _wprotect_time/1000);

  fprintf(resultsfile, "%s", output);
  fclose(resultsfile);
}

void sigint_handler(int sig) {
  int r = 0;
  ASSERT(sig == SIGINT);
  printf("\nKTracker: Goodbye!\n");

  _stop_requested = 1;
}

int register_signal_handler(void) {
  int r = 0;
  struct sigaction sigint_action = {
    .sa_handler = sigint_handler
  };

  sigemptyset(&sigint_action.sa_mask);
  r = sigaction(SIGINT, &sigint_action, &old_sigint_action);
  if (r < 0) {
    pr_debug("could not register sigint handler");
  }
  return r;
}


/**************************************************************/

pid_t process_input(int argc, char *argv[]) {
  int opt;
  pid_t pid;

  while ((opt = getopt(argc, argv, "hp:c:")) != -1) {
    if (_config.parent == 1)
      /* remaining arguments are for the child process */
      break;
    switch (opt) {
      case 'h':
        usage(argc, argv);
        return 0;
      case 'p':
        printf("KTracker not running as parent\n");
        if (argc != 3) usage(argc, argv);
        pid = strtoull(optarg, NULL, 0);
        if (pid <= 0) {
          printf("Invalid pid %d\n", pid);
          exit(1);
        }
        break;
      case 'c':
        _config.parent = 1;
        printf("KTracker running as parent\n");
        pid = start_child(optarg, &(argv[2]));
        if (pid <= 0) {
          printf("Could not start child process %s\n", optarg);
          exit(1);
        }
        break;
      default:
        usage(argc, argv);
    }
  }

  return pid;
}


  /********* Dump memory to a file *******/

int open_memdump_file(pid_t pid, int num_iter) {
  int fd_memdump = 0;
#ifdef PB_MEMDUMP
  char full_filename[MAX_FILE_LEN];
  sprintf(full_filename, "proc_%d_memdump_%d.bin", pid, num_iter);
  fd_memdump = open(full_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR);
  assert(fd_memdump);
#endif
  return fd_memdump;
}

void write_memdump_file(int fd, void *data, size_t size) {
#ifdef PB_MEMDUMP
  /* write memory to a file */
  write(fd, data, size);
#endif
}

void close_memdump_file(int fd) {
#ifdef PB_MEMDUMP
  close(fd);
#endif
}


  /********* Write dirty cache lines to bin file *******/

int open_dcl_file(pid_t pid) {
  int fd_dcl= 0;
#ifdef PB_WRITE_DCL
  char full_filename[MAX_FILE_LEN];
  sprintf(full_filename, "dcl_%d.bin", pid);
  fd_dcl = open(full_filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR);
  assert(fd_dcl);
#endif
  return fd_dcl;
}

void write_dcl_file(int fd, void* data, size_t size) {
#ifdef PB_WRITE_DCL
  /* write memory to a file */
  ssize_t code = write(fd, data, size);
  if (code < 0) {
    pr_error(errno, "write in write_dcl_file");
    printf("Couldn't write DCL output\n");
  }
#endif
}

void close_dcl_file(int fd) {
#ifdef PB_WRITE_DCL
  close(fd);
#endif
}



void print_output(pid_t pid, const char *output) {
#if PB_WRITE_TEXT_OUTPUT
  char resultsfilename[MAX_FILE_LEN];
  sprintf(resultsfilename, "results%d.txt", pid);
  FILE *resultsfile = fopen(resultsfilename, "a");

  fprintf(resultsfile, "%s", output);
  fclose(resultsfile);
#endif
}

/* Similar to getline(), except gets process pid task IDs.
 * Returns positive (number of TIDs in list) if success,
 * otherwise 0 with errno set. */
size_t get_tids(pid_t **const listptr, size_t *const sizeptr, const pid_t pid) {
    char     dirname[64];
    DIR     *dir;
    pid_t   *list;
    size_t   size, used = 0;

    if (!listptr || !sizeptr || pid < (pid_t)1) {
        errno = EINVAL;
        return (size_t)0;
    }

    if (*sizeptr > 0) {
        list = *listptr;
        size = *sizeptr;
    } else {
        list = *listptr = NULL;
        size = *sizeptr = 0;
    }

    if (snprintf(dirname, sizeof dirname, "/proc/%d/task/", (int)pid) >= (int)sizeof dirname) {
        errno = ENOTSUP;
        return (size_t)0;
    }

    dir = opendir(dirname);
    if (!dir) {
        errno = ESRCH;
        return (size_t)0;
    }

    while (1) {
        struct dirent *ent;
        int            value;
        char           dummy;

        errno = 0;
        ent = readdir(dir);
        if (!ent)
            break;

        /* Parse TIDs. Ignore non-numeric entries. */
        if (sscanf(ent->d_name, "%d%c", &value, &dummy) != 1)
            continue;

        /* Ignore obviously invalid entries. */
        if (value < 1)
            continue;

        /* Make sure there is room for another TID. */
        if (used >= size) {
            size = (used | 127) + 128;
            list = realloc(list, size * sizeof list[0]);
            if (!list) {
                closedir(dir);
                errno = ENOMEM;
                return (size_t)0;
            }
            *listptr = list;
            *sizeptr = size;
        }

        /* Add to list. */
        list[used++] = (pid_t)value;
    }
    if (errno) {
        const int saved_errno = errno;
        closedir(dir);
        errno = saved_errno;
        return (size_t)0;
    }
    if (closedir(dir)) {
        errno = EIO;
        return (size_t)0;
    }

    /* None? */
    if (used < 1) {
        errno = ESRCH;
        return (size_t)0;
    }

    /* Make sure there is room for a terminating (pid_t)0. */
    if (used >= size) {
        size = used + 1;
        list = realloc(list, size * sizeof list[0]);
        if (!list) {
            errno = ENOMEM;
            return (size_t)0;
        }
        *listptr = list;
        *sizeptr = size;
    }

    /* Terminate list; done. */
    list[used] = (pid_t)0;
    errno = 0;
    return used;
}



size_t add_tid(pid_t **const listptr, size_t *const sizeptr, size_t listsize, const pid_t pid) {
    char     dirname[64];
    DIR     *dir;
    pid_t   *list;
    size_t   size, used = listsize;

    if (!listptr || !sizeptr || pid < (pid_t)1) {
        errno = EINVAL;
        return (size_t)0;
    }

    if (*sizeptr > 0) {
        list = *listptr;
        size = *sizeptr;
    } else {
        list = *listptr = NULL;
        size = *sizeptr = 0;
    }

    /* Make sure there is room for another TID. */
    if (used >= size) {
      size = (used | 127) + 128;
      list = realloc(list, size * sizeof list[0]);
      if (!list) {
        errno = ENOMEM;
        return (size_t)0;
      }
      *listptr = list;
      *sizeptr = size;
    }

    /* Add to list. */
    list[used++] = pid;

    /* Make sure there is room for a terminating (pid_t)0. */
    if (used >= size) {
      size = used + 1;
      list = realloc(list, size * sizeof list[0]);
      if (!list) {
        errno = ENOMEM;
        return (size_t)0;
      }
      *listptr = list;
      *sizeptr = size;
    }

    /* Terminate list; done. */
    list[used] = (pid_t)0;
    errno = 0;
    return used;
}


void timer_app_start(PTimer *pt, pid_t pid) {
#if defined (GET_APP_TIME)
  pr_debug("Starting app timer\n");
  ptimer_start(pt);
#endif
}

void timer_app_stop(PTimer *pt, pid_t pid) {
#if defined (GET_APP_TIME)
  ptimer_stop(pt);
  pr_debug("Stopping app timer\n");

  double elapsed = ptimer_getElapsedUSec(pt);
  ptimer_reset(pt);
  _app_time += elapsed;

  pr_debug("App_time(us):%.4f\n", elapsed);
  pr_debug("Total_app_time(us):%.4f\n", _app_time);

  char output[MAX_OUTPUT_LINE];
  sprintf(output, "app_time_us: %.4f\n", elapsed);
  print_output(pid, output);

  sprintf(output, "total_app_time_us: %.4f\n", _app_time);
  print_output(pid, output);
#endif
}

void timer_paused_start(PTimer *pt, pid_t pid) {
#if defined (GET_PAUSED_TIME)
  ptimer_start(pt);
#endif
}

void timer_paused_stop(PTimer *pt, pid_t pid) {
#if defined (GET_PAUSED_TIME)
  ptimer_stop(pt);

  double elapsed = ptimer_getElapsedMSec(pt);
  ptimer_reset(pt);
  _paused_time += elapsed;

  pr_debug("Paused time (ms):%.4f\n", elapsed);
  pr_debug("Total Paused time (ms):%.4f\n", _paused_time);

  char output[MAX_OUTPUT_LINE];
  sprintf(output, "paused_time_ms: %.4f\n", elapsed);
  print_output(pid, output);

  sprintf(output, "total_paused_time_ms: %.4f\n", _paused_time);
  print_output(pid, output);
#endif
}


void timer_wptime_start(PTimer *pt, pid_t pid) {
#if defined (GET_WP_TIME)
  ptimer_start(pt);
#endif
}

void timer_wptime_stop(PTimer *pt, pid_t pid) {
#if defined (GET_WP_TIME)
  ptimer_stop(pt);

  double elapsed = ptimer_getElapsedUSec(pt);
  ptimer_reset(pt);
  _wprotect_time += elapsed;

  pr_debug("Write protect elapsed time (us):%.4f\n", elapsed);
  pr_debug("Total Write protect elapsed time (us):%.4f\n", _wprotect_time);

  char output[MAX_OUTPUT_LINE];
  sprintf(output, "wp_time_us: %.4f\ntotal_wp_time_us: %.4f\n", elapsed, _wprotect_time);
  print_output(pid, output);
#endif
}


void write_protect_pages(pid_t pid) {
#if defined (WPROTECT)
  char filename[MAX_FILE_LEN];

  pr_debug("write-protecting pages!\n");
  sprintf(filename, "/proc/%d/clear_refs", pid);
  int fd = open(filename, O_WRONLY, S_IRUSR);
  assert(fd);
  char c[1] = "4";
  timer_wptime_start(&_wp_ptimer, pid);
  write(fd, c, sizeof(c));
  close(fd);
  timer_wptime_stop(&_wp_ptimer, pid);
#endif
}

int proc_is_stopping_signal(int signal) {
  return (signal == SIGSTOP || signal == SIGTSTP || signal == SIGTTIN || signal == SIGTTOU);
}

void proc_seize(pid_t pid) {
  int r = ptrace(PTRACE_SEIZE, pid, 0, PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK | PTRACE_O_TRACEEXEC | PTRACE_O_TRACEEXIT); 
  if (r <0) {
    perror("Can't seize tracee!");
    assert(0);
  }
  printf("Seize successful\n");
}

void proc_attach(pid_t pid) {
  int r = ptrace(PTRACE_ATTACH, pid, 0, 0);
  if (r <0) assert(0);
  r = waitpid(pid, NULL, 0);
  if (r < 0) assert(0);
}

void proc_detach(pid_t pid) {
  int r = ptrace(PTRACE_DETACH, pid, 0, 0);
  if (r < 0) assert(0);
  printf("Proc detach successful\n");
}

void proc_resume(pid_t pid, int signal) {
  /* signal could be ignored by the tracee depending 
   * on the stop status it is in */
  int status;
  pr_debug("Resuming process %d\n", pid);
  int r = ptrace(PTRACE_CONT, pid, 0, signal);
  if (r < 0) {
    perror("can't resume process");
    printf("Can't resume process %d\n", pid);
    return;
  }
  pr_debug("Process %d resumed\n", pid);
}

void proc_resume_all(pid_t pid, pid_t *loc_tid, size_t loc_tids) {
  pr_debug("Resume all %lu processes!\n", tids);
  write_protect_pages(pid);
  timer_app_start(&_app_timer, pid);
  for (int i = 0; i < tids; ++i) {
    if (tid[i] != 0)
      proc_resume(tid[i], 0);
  }
  timer_paused_stop(&_paused_timer, pid);
}


int proc_stillrunning(pid_t pid, int index) {
  if (index < 0) return 1;
  return (tid[index] == pid);
}


void proc_mark_exited(pid_t pid) {
  for (int i = 0; i < tids; ++i) {
    if (tid[i] == pid) tid[i] = 0;
  }
}

int proc_is_new(pid_t pid) {
  for (int i = 0; i < tids; ++i) {
    if (tid[i] == pid) return 0;
  }
  return 1;
}

int proc_check_status(pid_t pid) {
  int status = 0;
  pid_t newpid = 0;
  pid_t curpid = pid;
  int r = 0;
  while (1) {
    r = 0;
    if (pid) {
      r = waitpid(pid, &status, 0);
      pr_debug("waitpid response %d pid %d\n", r, pid);
      assert(r == pid);
    } else {
      r = waitpid(-1, &status, WNOHANG);
      assert(r >= 0);
      if (r == 0) {
        pr_debug("Children didn't change state!\n");
        break;
      }
    }
    curpid = r;
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
      printf("Child %d finished execution\n", curpid);
      num_procs--;
      proc_mark_exited(curpid);
      /* This pid exited, don't wait for it anymore */
      if (curpid == pid) pid = 0;
    } else if (WIFSTOPPED(status)) {
      int signal = WSTOPSIG(status);
      pr_debug("Child stopped by signal %d\n", signal);
      if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_EXIT << 8))) {
        printf("Child TRACEEXIT detected for pid %d!\n", curpid);
        proc_mark_exited(curpid);
        proc_resume(curpid, 0);
      } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_FORK << 8))) {
        printf("Child Event Fork detected for pid %d\n", curpid);
      } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_VFORK << 8))) {
        printf("Child event Vfork detected for pid %d\n", curpid);
      } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_VFORK_DONE << 8))) {
        printf("Child event vfork_done detected for pid %d\n", curpid);
      } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_EXEC << 8))) {
        printf("Child Event Exec detected for pid %d\n", curpid);
        // TODO: we continue, because OK for case where KTracker runs in parent mode
        // but otherwise we need to do more work (destroy potential threads, etc.)
        /* if we're waiting for this thread and it performed an exec at the same time with pause, 
           it might not get the pause */
        if (curpid == pid) pid = 0;
        else proc_resume(curpid, 0);
      } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_SECCOMP << 8))) {
        printf("Child Event Seccomp detected for pid %d\n", curpid);
      } else if (status>> 8 == (SIGTRAP | (PTRACE_EVENT_CLONE << 8))) {
        ptrace(PTRACE_GETEVENTMSG, r, NULL, &newpid);
        printf("Child event clone detected for pid %d, with child pid %d\n", curpid, newpid);
        proc_resume(curpid, 0);
      } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_STOP << 8))) {
        if (proc_is_new(curpid)) {
          pr_debug("Ptrace_Event_stop detected (automatically attached child pid %d)\n", curpid);
          tids = add_tid(&tid, &tids_max, tids, curpid);
          num_procs++;
          proc_resume(curpid, 0);
        } else {
          assert(pid && (pid == curpid));
          pr_debug("Proc %d paused\n", pid);
          pid = 0;
        }
      } else if (proc_is_stopping_signal(signal)) {
        /* TODO: could be group-stop */
        printf("Potential group-stop (not handled right now) %d\n", signal);
        proc_resume(curpid, signal);
      } else {
        /* TODO: other stop events are possible */
        pr_debug("Child unknown stop event detected %d signal %d for pid %d \n", status >> 8, signal, curpid);

        /* could be signal-delivery-stop, send signal to app */
        proc_resume(curpid, signal);
      }
      } else {
        pr_debug("Child pid %d  changed state, but didn't stop\n", curpid);
      }
    }
}

int proc_pause(pid_t pid, int index) {
  int r;
  pr_debug("Pausing process: %d\n", pid);
  if (proc_stillrunning(pid, index)) {
    r = ptrace(PTRACE_INTERRUPT, pid, 0, 0);
    if (r < 0) {
      pr_debug("PId is %d errno %d\n", pid, errno);
      perror("Can't pause tracee!");
      return 0;
    } else {
      pr_debug("PTRACE_INTERRUPT returned %d\n", r);

      proc_check_status(pid);

      pr_debug("Process %d paused\n", pid);
      return 1;
    }
  } else {
    printf("Cannot pause process %d, already finished\n", pid);
    return 0;
  }
}


int proc_pause_all(pid_t pid, pid_t *loc_tid, size_t loc_tids) {
  int r = 0;
  proc_check_status(0);
  pr_debug("Pause all %lu processes!\n", tids);
  timer_paused_start(&_paused_timer, pid);
  /* Note! tids could change (only increase) in proc_check_status if 
    a thread does a clone. This is the expected behavior, as we want to pause 
    the new thread as well. */
  for (int i = 0; i < tids; ++i) {
    if (tid[i] != 0)
      r = r | proc_pause(tid[i], i);
  }
  timer_app_stop(&_app_timer, pid);
  return r;
}

void proc_setoptions(pid_t pid, int options) {
  int r = ptrace(PTRACE_SETOPTIONS, pid, 0, options);
  assert(r >= 0);
  pr_debug("Process set options %d\n", options);
}

void proc_seize_and_pause(pid_t pid, pid_t *tid, size_t tids) {
  pr_debug("Proc seize and pause\n");

  timer_paused_start(&_paused_timer, pid);
  for (int i = 0; i < tids; ++i) {
    if (tid[i] != pid) {
      proc_seize(tid[i]);
    }
    proc_pause(tid[i], i);
  }
}

void usage(int argc, char **argv) {
  printf("Usage: sudo %s -p <pid>\n", argv[0]);
  printf("Usage: sudo %s -c <binary> <args-for-binary>\n", argv[0]);
  exit(1);
}

void get_numa_config() {
  int ret;
  struct bitmask *mask = numa_allocate_cpumask();
  u32 max_node = numa_max_node();
  u32 max_cores = numa_num_configured_cpus();
  u32 k = 0;

  _parent_core = _child_core = 0;
  if (max_node == 0) {
    if (max_cores > 1) {
      _child_core = 0;
      _parent_core = 1;
    }
  } else {
    /* put child on node 0 */
    numa_node_to_cpus(0, mask);
    _child_core = 0;
    for (u32 j = 0; j < max_cores; ++j) {
      if (numa_bitmask_isbitset(mask, j)) {
        _child_core = j;
        break;
      }
    }

    /* put tracker on node 1 */
    numa_node_to_cpus(1, mask);
    _parent_core = 0;
    for (u32 j = 0; j < max_cores; ++j) {
      if (numa_bitmask_isbitset(mask, j)) {
        _parent_core = j;
        break;
      }
    }
  }
}

pid_t start_child(const char *cmd, char * const args[]) {
  /* get NUMA configuration */
  get_numa_config();

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork failed");
    return 0;
  } else if (pid == 0) {
    /* child process */
#if PB_PIN_THREADS
    pinThread(_child_core);
    printf("Child: running on core %d\n", _child_core);
#endif
    sleep(1);

    int r = execvp(cmd, args);
    if (r < 0) {
      perror("Child: execv failed");
    }
  } else {
    /* parent process */
#if PB_PIN_THREADS
    pinThread(_parent_core);
    printf("Parent: running on core %d\n", _parent_core);
#endif
    printf("Parent: Child pid is: %d\n", pid);

    return pid;
  }
}
