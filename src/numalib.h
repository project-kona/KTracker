// Copyright © 2018-2021 VMware, Inc. All Rights Reserved.
// SPDX-License-Identifier: BSD-2-Clause

#ifndef __NUMALIB_H__
#define __NUMALIB_H__

#include <numa.h>

#include "util.h"

void pinThread(int core);
void checkThreadAffinity();
void setThreadPrioLow();
void setThreadPrioHigh();
void checkThreadPrio();
void createNumaAssignment(u32 **cores);

#endif  // __NUMALIB_H__
