// Part of neutrino: the EE side without an SDK. See README.md.
#include <iopcontrol.h>
#include <iopheap.h>
#include <kernel.h>
#include <ps2lib_err.h>
#include <sifrpc.h>

static SifRpcClientData_t ih_cd;
static u32 ih_arg[16] ALIGNED(64); // a cache line, used uncached

int SifInitIopHeap(void)
{
    static int reboot_count = 0;
    int res;

    if (reboot_count != _iop_reboot_count) {
        reboot_count = _iop_reboot_count;
        SifExitIopHeap();
    }
    if (ih_cd.server != NULL)
        return 0;

    SifInitRpc(0);

    while ((res = SifBindRpc(&ih_cd, 0x80000003, 0)) >= 0 && ih_cd.server == NULL)
        nopdelay();

    return res < 0 ? -E_SIF_RPC_BIND : 0;
}

void SifExitIopHeap(void) { ih_cd.server = NULL; }

static int ih_call(int fno, u32 arg, u32 *res)
{
    vu32 *a = UNCACHED_SEG(ih_arg);

    if (SifInitIopHeap() < 0)
        return -E_LIB_API_INIT;

    SifWriteBackDCache(ih_arg, sizeof(ih_arg));
    a[0] = arg;
    if (SifCallRpc(&ih_cd, fno, 0, ih_arg, 4, ih_arg, 4, NULL, NULL) < 0)
        return -E_SIF_RPC_CALL;
    *res = a[0];

    return 0;
}

void *SifAllocIopHeap(int size)
{
    u32 addr;

    return ih_call(1, size, &addr) < 0 ? NULL : (void *)addr;
}

int SifFreeIopHeap(void *addr)
{
    u32 res;
    int err = ih_call(2, (u32)addr, &res);

    return err < 0 ? err : (int)res;
}
