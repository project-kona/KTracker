#ifndef __PTIMER_H__
#define __PTIMER_H__

#include <stdint.h>
#include "util.h"

struct PTimer_ {
  unsigned long long _frequency;
  unsigned long long _startT;
  unsigned long long _stopT;
};

typedef struct PTimer_ PTimer;

void ptimer_init(PTimer *pt);
void ptimer_reset(PTimer *pt);
void ptimer_start(PTimer *pt);
void ptimer_stop(PTimer *pt);

double ptimer_getElapsedSec(PTimer *pt);
double ptimer_getElapsedMSec(PTimer *pt);
double ptimer_getElapsedUSec(PTimer *pt);

#endif  // __PTIMER_H__

