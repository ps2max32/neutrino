// Part of neutrino: the EE side without an SDK. See README.md.
// Reset the IOP, as ps2sdk's SifIopReset (Academic Free License version 2.0)
#include <iopcontrol.h>
#include <kernel.h>
#include <sifcmd.h>

int SifIopReset(const char *arg, int mode)
{
    static SifCmdResetData_t reset_pkt ALIGNED(64);
    SifDmaTransfer_t dmat;
    int arglen = 0;

    _iop_reboot_count++; // the RPC clients bind again
    SifStopDma();

    if (arg != NULL) {
        for (arglen = 0; arg[arglen] != 0 && arglen < RESET_ARG_MAX; arglen++)
            reset_pkt.arg[arglen] = arg[arglen];
    }
    reset_pkt.header.psize = sizeof(reset_pkt);
    reset_pkt.header.cid   = SIF_CMD_RESET_CMD;
    reset_pkt.arglen       = arglen;
    reset_pkt.mode         = mode;

    dmat.src  = &reset_pkt;
    dmat.dest = (void *)SifGetReg(SIF_SYSREG_SUBADDR);
    dmat.size = sizeof(reset_pkt);
    dmat.attr = SIF_DMA_ERT | SIF_DMA_INT_O;
    SifWriteBackDCache(&reset_pkt, sizeof(reset_pkt));

    SifSetReg(SIF_REG_SMFLAG, SIF_STAT_BOOTEND);
    if (!SifSetDma(&dmat, 1))
        return 0;

    SifSetReg(SIF_REG_SMFLAG, SIF_STAT_SIFINIT);
    SifSetReg(SIF_REG_SMFLAG, SIF_STAT_CMDINIT);
    SifSetReg(SIF_SYSREG_RPCINIT, 0);
    SifSetReg(SIF_SYSREG_SUBADDR, 0);

    return 1;
}
