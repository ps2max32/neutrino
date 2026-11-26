// Part of neutrino: the EE side without an SDK. See README.md.
#ifndef SIFCMD_H
#define SIFCMD_H

#include <sifdma.h>

typedef struct t_SifCmdHeader
{
    u32 psize : 8;
    u32 dsize : 24;
    void *dest;
    u32 cid;
    u32 opt;
} SifCmdHeader_t;

#define SIF_CMD_CHANGE_SADDR 0x80000000
#define SIF_CMD_SET_SREG     0x80000001
#define SIF_CMD_INIT_CMD     0x80000002
#define SIF_CMD_RESET_CMD    0x80000003
#define SIF_CMD_RPC_END      0x80000008
#define SIF_CMD_RPC_BIND     0x80000009
#define SIF_CMD_RPC_CALL     0x8000000a
#define SIF_CMD_RPC_RDATA    0x8000000c

#define RESET_ARG_MAX 79

typedef struct _iop_reset_pkt
{
    struct t_SifCmdHeader header;
    int arglen;
    int mode;
    char arg[RESET_ARG_MAX + 1];
} SifCmdResetData_t;

#endif
