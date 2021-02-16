// Copyright © 2018-2021 VMware, Inc. All Rights Reserved.
// SPDX-License-Identifier: BSD-2-Clause

#ifndef __UTIL_H__
#define __UTIL_H__

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define ASSERT(x)   assert((x))
#define ASSERTZ(x)  ASSERT(!(x))


#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define RESET "\x1B[0m"

#define pr_error(eno, func) \
               do { errno = eno; perror(KRED func); printf(RESET);} while (0)

#ifdef DEBUG_OUTPUT
#define pr_debug(fmt, ...)          \
    do {              \
          fprintf(stderr, "%s : %d : " fmt "\n",    \
                    __func__, __LINE__, ##__VA_ARGS__);  \
        } while (0)
#else
#define pr_debug(fmt, ...)
#endif



#define NAME1(t)  #t
#define NAME(t)   NAME1(t)

#define MAXUINT32       4294967295


#define CACHE_LINE    64
#define CACHE_ALIGN   __attribute__((aligned(CACHE_LINE)))

#define CL_SHIFT  (6)
#define CL_SIZE   (1ull << CL_SHIFT)
#define CL_MASK   (~(CL_SIZE - 1))

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;


struct PaddedUInt {
  u32 val;
  char pad_[CACHE_LINE - sizeof(u32)];
} CACHE_ALIGN;



struct PaddedVolatileUInt {
  volatile u32 val;
  char pad_[CACHE_LINE - sizeof(u32)];
} CACHE_ALIGN;


struct PaddedVolatileUInt64 {
  volatile u64 val;
  char pad_[CACHE_LINE - sizeof(u64)];
} CACHE_ALIGN;


#endif  // __UTIL_H__
