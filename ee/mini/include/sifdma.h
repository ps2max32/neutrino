// Part of neutrino: the EE side without an SDK. See README.md.
#ifndef SIFDMA_H
#define SIFDMA_H

#include <tamtypes.h>

// SifDmaTransfer_t.attr
#define SIF_DMA_INT_O 0x04
#define SIF_DMA_ERT   0x40

enum {
    SIF_REG_SUBADDR = 2, // IOP's command buffer
    SIF_REG_SMFLAG  = 4, // IOP's flags
    // kept by the BIOS
    SIF_SYSREG_SUBADDR  = 0x80000000,
    SIF_SYSREG_MAINADDR = 0x80000001,
    SIF_SYSREG_RPCINIT  = 0x80000002,
};

// SIF_REG_SMFLAG
#define SIF_STAT_SIFINIT 0x10000
#define SIF_STAT_CMDINIT 0x20000
#define SIF_STAT_BOOTEND 0x40000

typedef struct t_SifDmaTransfer
{
    void *src;
    void *dest;
    int size;
    int attr;
} SifDmaTransfer_t;

// The BIOS's system calls
u32 SifSetDma(SifDmaTransfer_t *transfers, int count);
u32 iSifSetDma(SifDmaTransfer_t *transfers, int count);
int SifDmaStat(u32 id);
void SifStopDma(void);
void SifSetDChain(void);
void iSifSetDChain(void);
int SifSetReg(u32 reg, int value);
int SifGetReg(u32 reg);

// Write back and invalidate the data cache's lines
void SifWriteBackDCache(void *ptr, int size);

#endif
