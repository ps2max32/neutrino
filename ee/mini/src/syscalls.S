// Part of neutrino: the EE side without an SDK. See README.md.
#include <syscallnr.h>

    .set    noreorder

.macro SYSCALL name, number
    .section .text.\name, "ax", @progbits
    .p2align 3
    .globl  \name
    .type   \name, @function
\name:
    li      $3, \number
    syscall
    jr      $31
    nop
    .size   \name, . - \name
.endm

    SYSCALL LoadExecPS2, __NR_LoadExecPS2
    SYSCALL ExecPS2, __NR_ExecPS2
    SYSCALL AddDmacHandler, __NR_AddDmacHandler
    SYSCALL RemoveDmacHandler, __NR_RemoveDmacHandler
    SYSCALL _EnableDmac, __NR__EnableDmac
    SYSCALL _DisableDmac, __NR__DisableDmac
    SYSCALL GetThreadId, __NR_GetThreadId
    SYSCALL SleepThread, __NR_SleepThread
    SYSCALL CreateSema, __NR_CreateSema
    SYSCALL DeleteSema, __NR_DeleteSema
    SYSCALL iSignalSema, __NR_iSignalSema
    SYSCALL WaitSema, __NR_WaitSema
    SYSCALL FlushCache, __NR_FlushCache
    SYSCALL SifStopDma, __NR_SifStopDma
    SYSCALL SetSyscall, __NR_SetSyscall
    SYSCALL _print, __NR__print
    SYSCALL SifDmaStat, __NR_SifDmaStat
    SYSCALL SifSetDma, __NR_SifSetDma
    SYSCALL SifSetDChain, __NR_SifSetDChain
    SYSCALL iSifSetDChain, __NR_iSifSetDChain
    SYSCALL SifSetReg, __NR_SifSetReg
    SYSCALL SifGetReg, __NR_SifGetReg
