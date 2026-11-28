// Part of neutrino: the EE side without an SDK. See README.md.
#ifndef GS_PRIVILEGED_H
#define GS_PRIVILEGED_H

#include <tamtypes.h>

#define GS_REG_PMODE    ((vu64 *)0x12000000)
#define GS_REG_SMODE2   ((vu64 *)0x12000020)
#define GS_REG_SYNCH1   ((vu64 *)0x12000040)
#define GS_REG_SYNCH2   ((vu64 *)0x12000050)
#define GS_REG_SYNCHV   ((vu64 *)0x12000060)
#define GS_REG_DISPLAY1 ((vu64 *)0x12000080)
#define GS_REG_DISPLAY2 ((vu64 *)0x120000A0)
#define GS_REG_BGCOLOR  ((vu64 *)0x120000E0)
#define GS_REG_CSR      ((vu64 *)0x12001000)
#define GS_REG_SIGLBLID ((vu64 *)0x12001080)

#define GS_SET_DISPLAY(DX, DY, MAGH, MAGV, DW, DH)                                                     \
    ((u64)((DX)&0xFFF) << 0 | (u64)((DY)&0x7FF) << 12 | (u64)((MAGH)&0xF) << 23 | (u64)((MAGV)&0x3) << 27 | \
     (u64)((DW)&0xFFF) << 32 | (u64)((DH)&0x7FF) << 44)
#define GS_SET_SMODE2(INT, FFMD, DPMS) ((u64)((INT)&1) << 0 | (u64)((FFMD)&1) << 1 | (u64)((DPMS)&3) << 2)

#endif
