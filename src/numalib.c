#define _GNU_SOURCE  
#include <errno.h>
#include <assert.h>
#include <numa.h>
#include <pthread.h>
#include <sched.h>

#include "numalib.h"

void pinThread(int core) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core, &cpuset);
  int retcode = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (retcode) pr_error(retcode, "pthread_setaffinitity_np");
}

void checkThreadAffinity() {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  int retcode = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (retcode) pr_error(retcode, "pthread_getaffinitity_np");
  else {
    printf("Thread affinity mask contains: ");
    for (int i = 0; i < CPU_SETSIZE; i++)
      if (CPU_ISSET(i, &cpuset))
        printf("%d ", i);
    printf("\n");
  }
}

void setThreadPrioLow() {
  struct sched_param param;
  int retcode, policy;

  policy = SCHED_FIFO;
  int pmin = sched_get_priority_min(policy);
  param.sched_priority = pmin;

  retcode = pthread_setschedparam(pthread_self(), policy, &param);
  if (retcode) pr_error(retcode, "pthread_setschedparam");
}

void setThreadPrioHigh() {
  struct sched_param param;
  int retcode, policy;

  policy = SCHED_FIFO;
  int pmax = sched_get_priority_max(policy);
  param.sched_priority = pmax;

  retcode = pthread_setschedparam(pthread_self(), policy, &param);
  if (retcode) pr_error(retcode, "pthread_setschedparam");
}

void checkThreadPrio() {
  struct sched_param param;
  int retcode, policy;

  retcode = pthread_getschedparam(pthread_self(), &policy, &param);
  if (retcode) pr_error(retcode, "pthread_getschedparam");

  printf("    policy=%s, priority=%d\n",
    (policy == SCHED_FIFO)  ? "SCHED_FIFO" :
    (policy == SCHED_RR)    ? "SCHED_RR" :
    (policy == SCHED_OTHER) ? "SCHED_OTHER" :
                              "Unknown",
    param.sched_priority);
}
