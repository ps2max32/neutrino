// Part of neutrino: the EE side without an SDK. See README.md.
// Numbers of the BIOS's system calls. Negative: from an interrupt handler.
#ifndef SYSCALLNR_H
#define SYSCALLNR_H

#define __NR_SetGsCrt         0x02
#define __NR_LoadExecPS2      0x06
#define __NR_ExecPS2          0x07
#define __NR_AddDmacHandler   0x12
#define __NR_RemoveDmacHandler 0x13
#define __NR__EnableDmac      0x16
#define __NR__DisableDmac     0x17
#define __NR_GetThreadId      0x2f
#define __NR_SleepThread      0x32
#define __NR_SetupThread      0x3c
#define __NR_CreateSema       0x40
#define __NR_DeleteSema       0x41
#define __NR_iSignalSema      (-0x43)
#define __NR_WaitSema         0x44
#define __NR_FlushCache       0x64
#define __NR_SifStopDma       0x6b
#define __NR_SetSyscall       0x74
#define __NR__print           0x75
#define __NR_SifDmaStat       0x76
#define __NR_SifSetDma        0x77
#define __NR_iSifSetDma       (-0x77)
#define __NR_SifSetDChain     0x78
#define __NR_iSifSetDChain    (-0x78)
#define __NR_SifSetReg        0x79
#define __NR_SifGetReg        0x7a

#endif
