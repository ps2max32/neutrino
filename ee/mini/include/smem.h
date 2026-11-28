// Part of neutrino: the EE side without an SDK. See README.md.
#ifndef SMEM_H
#define SMEM_H

#include <tamtypes.h>

// Write IOP memory directly, in words
u32 smem_write(void *addr, void *buf, u32 size);

#endif
