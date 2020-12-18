#ifndef __COMMON_H__
#define __COMMON_H__

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

#include "numalib.h"
#include "ptimer.h"
#include "pagemap.h"
#include "proc_maps.h"


#define MAX_FILE_LEN 128
#define MAX_OUTPUT_LINE 256 
#define CLSIZE 64
// TODO: increase this if needed by checking sysctl 
#define MAX_PROC_MAPPINGS  65530
#define SLEEP_TIME         1  /* seconds */
#define DO_NOTHING_TIME    1  /* seconds */

//#if defined (GET_WP_TIME)
//#undef GET_PAUSED_TIME
//#endif

volatile int _stop_requested;
int _parent_core, _child_core;
double _paused_time, _app_time, _wprotect_time;
PTimer _wp_ptimer, _paused_timer, _app_timer;
uint64_t _num_iter;

pid_t *tid;
size_t tids;
size_t tids_max;
size_t num_procs;


struct Config_ {
  int parent;
} _config;
typedef struct Config_ Config; 

/***********************************************************************/

void print_final_stats();
int register_signal_handler(void);

pid_t process_input(int argc, char *argv[]); 

int open_memdump_file(pid_t pid, int num_iter);
void write_memdump_file(int fd, void *data, size_t size);
void close_memdump_file(int fd); 

int open_dcl_file(pid_t pid);
void write_dcl_file(int fd, void* data, size_t size);
void close_dcl_file(int fd); 

size_t get_tids(pid_t **const listptr, size_t *const sizeptr, const pid_t pid); 

void print_output(pid_t pid, const char *output);
void proc_seize(pid_t pid);
void proc_seize_and_pause(pid_t pid, pid_t *tid, size_t tids); 

int proc_pause_all(pid_t pid, pid_t *loc_tid, size_t loc_tids);
void proc_resume_all(pid_t pid, pid_t *loc_tid, size_t loc_tids); 
void usage(int argc, char **argv); 
void get_numa_config(); 
pid_t start_child(const char *cmd, char * const args[]);

#endif  // __COMMON_H__
