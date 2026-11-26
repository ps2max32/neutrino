// Part of neutrino: the EE side without an SDK. See README.md.
// The client of the ROM's LOADFILE (SIF RPC 0x80000006), as ps2sdk's
// (Academic Free License version 2.0). ee_core has its own, changed one.
#include <iopcontrol.h>
#include <iopheap.h>
#include <kernel.h>
#include <loadfile.h>
#include <ps2lib_err.h>
#include <sifrpc.h>
#include <string.h>

static SifRpcClientData_t lf_cd;
static union
{
    struct _lf_module_load_arg load;
    struct _lf_module_buffer_load_arg buffer;
} arg;

int SifLoadFileInit(void)
{
    static int reboot_count = 0;
    int res;

    if (reboot_count != _iop_reboot_count) {
        reboot_count = _iop_reboot_count;
        SifLoadFileExit();
    }
    if (lf_cd.server != NULL)
        return 0;

    SifInitRpc(0);
    while ((res = SifBindRpc(&lf_cd, 0x80000006, 0)) >= 0 && lf_cd.server == NULL)
        nopdelay();

    return res < 0 ? -E_SIF_RPC_BIND : 0;
}

void SifLoadFileExit(void) { lf_cd.server = NULL; }

int _SifLoadModule(const char *path, int arg_len, const char *args, int *modres, int fno, int dontwait)
{
    if (SifLoadFileInit() < 0)
        return -SCE_EBINDMISS;

    memset(&arg.load, 0, sizeof(arg.load));
    strncpy(arg.load.path, path, LF_PATH_MAX - 1);
    if (args && arg_len) {
        arg.load.p.arg_len = arg_len > LF_ARG_MAX ? LF_ARG_MAX : arg_len;
        memcpy(arg.load.args, args, arg.load.p.arg_len);
    }

    if (SifCallRpc(&lf_cd, fno, dontwait ? SIF_RPC_M_NOWAIT : 0, &arg.load, sizeof(arg.load), &arg.load, 8, NULL, NULL) < 0)
        return -SCE_ECALLMISS;

    if (modres)
        *modres = arg.load.modres;
    return arg.load.p.result;
}

int SifLoadModule(const char *path, int arg_len, const char *args)
{
    return _SifLoadModule(path, arg_len, args, NULL, LF_F_MOD_LOAD, 0);
}

static int load_module_buffer(void *ptr, int arg_len, const char *args, int *modres)
{
    if (SifLoadFileInit() < 0)
        return -SCE_EBINDMISS;

    memset(&arg.buffer, 0, sizeof(arg.buffer));
    arg.buffer.p.ptr = ptr;
    if (args && arg_len) {
        arg.buffer.q.arg_len = arg_len > LF_ARG_MAX ? LF_ARG_MAX : arg_len;
        memcpy(arg.buffer.args, args, arg.buffer.q.arg_len);
    }

    if (SifCallRpc(&lf_cd, LF_F_MOD_BUF_LOAD, 0, &arg.buffer, sizeof(arg.buffer), &arg.buffer, 8, NULL, NULL) < 0)
        return -SCE_ECALLMISS;

    if (modres)
        *modres = arg.buffer.q.modres;
    return arg.buffer.p.result;
}

int SifExecModuleBuffer(void *ptr, u32 size, u32 arg_len, const char *args, int *mod_res)
{
    SifDmaTransfer_t dmat;
    void *iop_addr;
    u32 id;
    int res;

    size = (size + 15) & ~15;
    if ((iop_addr = SifAllocIopHeap(size)) == NULL)
        return -E_IOP_NO_MEMORY;

    dmat.src  = ptr;
    dmat.dest = iop_addr;
    dmat.size = size;
    dmat.attr = 0;
    SifWriteBackDCache(ptr, size);
    do {
        id = SifSetDma(&dmat, 1);
    } while (!id);
    while (SifDmaStat(id) >= 0)
        ;

    res = load_module_buffer(iop_addr, arg_len, args, mod_res);
    SifFreeIopHeap(iop_addr);

    return res;
}
