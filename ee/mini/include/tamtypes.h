// Part of neutrino: the EE side without an SDK. See README.md.
#ifndef TAMTYPES_H
#define TAMTYPES_H

#include <stddef.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef unsigned int u128 __attribute__((mode(TI), aligned(16)));

typedef volatile u8 vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile u64 vu64;

#define ALIGNED(x) __attribute__((aligned(x)))

static inline u32 _lw(u32 addr) { return *(vu32 *)addr; }
static inline void _sw(u32 val, u32 addr) { *(vu32 *)addr = val; }

#endif
