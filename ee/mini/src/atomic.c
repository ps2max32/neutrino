// Part of neutrino: the EE side without an SDK. See README.md.
// The __atomic_* calls clang emits for picolibc's atomics: the R5900 has no
// LL/SC. One core: with the interrupts off, an operation is atomic.
#include <kernel.h>
#include <tamtypes.h>

#define OPS(N, T)                                                                                \
    T __atomic_load_##N(const volatile void *p, int m)                                          \
    {                                                                                            \
        (void)m;                                                                                 \
        int was = DIntr();                                                                       \
        T v     = *(const volatile T *)p;                                                        \
        if (was)                                                                                 \
            EIntr();                                                                             \
        return v;                                                                                \
    }                                                                                            \
    T __atomic_exchange_##N(volatile void *p, T v, int m)                                       \
    {                                                                                            \
        (void)m;                                                                                 \
        int was = DIntr();                                                                       \
        T o     = *(volatile T *)p;                                                              \
        *(volatile T *)p = v;                                                                    \
        if (was)                                                                                 \
            EIntr();                                                                             \
        return o;                                                                                \
    }                                                                                            \
    _Bool __atomic_compare_exchange_##N(volatile void *p, void *expected, T desired, int s, int f) \
    {                                                                                            \
        (void)s, (void)f;                                                                        \
        int was  = DIntr();                                                                      \
        T o      = *(volatile T *)p;                                                             \
        _Bool ok = o == *(T *)expected;                                                          \
        if (ok)                                                                                  \
            *(volatile T *)p = desired;                                                          \
        else                                                                                     \
            *(T *)expected = o;                                                                  \
        if (was)                                                                                 \
            EIntr();                                                                             \
        return ok;                                                                               \
    }

OPS(1, u8)
OPS(2, u16)
OPS(4, u32)
