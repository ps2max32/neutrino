// Part of neutrino: the EE side without an SDK. See README.md.
// The client of fileXio.irx (SIF RPC 0xb0b0b00), as ps2sdk's (Academic Free
// License version 2.0). One call at a time: neutrino has one thread.
#include <fileXio_rpc.h>
#include <iopcontrol.h>
#include <kernel.h>
#include <ps2lib_err.h>
#include <sifrpc.h>
#include <string.h>

#define FILEXIO_IRX 0xb0b0b00

enum {
    FILEXIO_OPEN    = 9,
    FILEXIO_CLOSE   = 10,
    FILEXIO_READ    = 11,
    FILEXIO_WRITE   = 12,
    FILEXIO_LSEEK   = 13,
    FILEXIO_IOCTL2  = 27,
    FILEXIO_LSEEK64 = 28,
};

#define CTL_BUF_SIZE 2048

static SifRpcClientData_t fxio_cd;
static union
{
    struct
    {
        char pathname[512];
        int flags;
        int mode;
    } open;
    struct
    {
        int fd;
        void *buffer;
        int size;
        void *intr_data;
    } read;
    struct
    {
        int fd;
        const void *buffer;
        int size;
        unsigned int unaligned_len;
        unsigned char unaligned[64];
    } write;
    struct
    {
        int fd;
        u32 offset;
        int whence;
    } lseek;
    struct
    {
        int fd;
        u32 offset_lo;
        u32 offset_hi;
        int whence;
    } lseek64;
    struct
    {
        int fd;
        u8 arg[CTL_BUF_SIZE];
        int cmd;
        int arglen;
        void *buf;
        int buflen;
        void *intr_data;
    } ioctl2;
    int words[16];
} pkt ALIGNED(64);
static u32 recv[16] ALIGNED(64);

// What the IOP sends besides: the unaligned ends of a read, or the result of
// an ioctl2
static union
{
    struct
    {
        int ssize;
        int esize;
        void *sbuf;
        void *ebuf;
        u8 sbuffer[64];
        u8 ebuffer[64];
    } rests;
    struct
    {
        void *dest;
        int len;
        u8 buf[CTL_BUF_SIZE];
        int padding[2];
    } ctl;
} intr_data ALIGNED(64);

int fileXio_active = 0; // open() uses fileXio (posix.c)

int fileXioInit(void)
{
    static int reboot_count = 0;
    int res;

    if (reboot_count != _iop_reboot_count) {
        reboot_count = _iop_reboot_count;
        fileXioExit();
    }
    if (fxio_cd.server != NULL)
        return 0;

    SifInitRpc(0);
    while ((res = SifBindRpc(&fxio_cd, FILEXIO_IRX, 0)) >= 0 && fxio_cd.server == NULL)
        nopdelay();

    if (res < 0)
        return res;

    fileXio_active = 1;
    return 0;
}

void fileXioExit(void)
{
    fxio_cd.server = NULL;
    fileXio_active = 0;
}

static int call(int fno, int size, int rsize)
{
    int res = fileXioInit();

    if (res < 0)
        return res;

    SifWriteBackDCache(recv, sizeof(recv));
    res = SifCallRpc(&fxio_cd, fno, 0, &pkt, size, recv, rsize, NULL, NULL);

    return res < 0 ? res : (int)((vu32 *)UNCACHED_SEG(recv))[0];
}

int fileXioOpen(const char *path, int flags, int mode)
{
    strncpy(pkt.open.pathname, path, sizeof(pkt.open.pathname) - 1);
    pkt.open.pathname[sizeof(pkt.open.pathname) - 1] = 0;
    pkt.open.flags = flags;
    pkt.open.mode  = mode;

    return call(FILEXIO_OPEN, sizeof(pkt.open), 4);
}

int fileXioClose(int fd)
{
    pkt.words[0] = fd;

    return call(FILEXIO_CLOSE, 4, 4);
}

int fileXioRead(int fd, void *buf, int size)
{
    typeof(intr_data.rests) *rests = UNCACHED_SEG(&intr_data);
    int res;

    pkt.read.fd        = fd;
    pkt.read.buffer    = buf;
    pkt.read.size      = size;
    pkt.read.intr_data = &intr_data;

    SifWriteBackDCache(&intr_data, sizeof(intr_data.rests));
    if (!((u32)buf & 0x20000000))
        SifWriteBackDCache(buf, size);

    res = call(FILEXIO_READ, sizeof(pkt.read), 4);

    if (rests->ssize)
        memcpy(rests->sbuf, rests->sbuffer, rests->ssize);
    if (rests->esize)
        memcpy(rests->ebuf, rests->ebuffer, rests->esize);

    return res;
}

int fileXioWrite(int fd, const void *buf, int size)
{
    unsigned int miss = 0;

    if ((u32)buf & 63) {
        miss = 64 - ((u32)buf & 63);
        if (miss > (unsigned int)size)
            miss = size;
    }

    pkt.write.fd            = fd;
    pkt.write.buffer        = buf;
    pkt.write.size          = size;
    pkt.write.unaligned_len = miss;
    memcpy(pkt.write.unaligned, buf, miss);

    if (!((u32)buf & 0x20000000))
        SifWriteBackDCache((void *)buf, size);

    return call(FILEXIO_WRITE, sizeof(pkt.write), 4);
}

int fileXioLseek(int fd, int offset, int whence)
{
    pkt.lseek.fd     = fd;
    pkt.lseek.offset = offset;
    pkt.lseek.whence = whence;

    return call(FILEXIO_LSEEK, sizeof(pkt.lseek), 4);
}

s64 fileXioLseek64(int fd, s64 offset, int whence)
{
    vu32 *r = UNCACHED_SEG(recv);
    int res = fileXioInit();

    if (res < 0)
        return res;

    pkt.lseek64.fd        = fd;
    pkt.lseek64.offset_lo = (u32)offset;
    pkt.lseek64.offset_hi = (u32)(offset >> 32);
    pkt.lseek64.whence    = whence;

    SifWriteBackDCache(recv, sizeof(recv));
    res = SifCallRpc(&fxio_cd, FILEXIO_LSEEK64, 0, &pkt, sizeof(pkt.lseek64), recv, 8, NULL, NULL);
    if (res < 0)
        return res;

    return (s64)(((u64)r[1] << 32) | r[0]);
}

int fileXioIoctl2(int fd, int cmd, void *arg, unsigned int arglen, void *buf, unsigned int buflen)
{
    typeof(intr_data.ctl) *ctl = UNCACHED_SEG(&intr_data);
    int res;

    if (arglen > CTL_BUF_SIZE)
        arglen = CTL_BUF_SIZE;
    if (buflen > CTL_BUF_SIZE)
        buflen = CTL_BUF_SIZE;
    if (arglen)
        memcpy(pkt.ioctl2.arg, arg, arglen);

    pkt.ioctl2.fd        = fd;
    pkt.ioctl2.cmd       = cmd;
    pkt.ioctl2.arglen    = arglen;
    pkt.ioctl2.buf       = buf;
    pkt.ioctl2.buflen    = buflen;
    pkt.ioctl2.intr_data = &intr_data;

    SifWriteBackDCache(buf, buflen);
    if (buflen) {
        SifWriteBackDCache(&intr_data, sizeof(intr_data.ctl));
        ctl->len = 0;
    }

    res = call(FILEXIO_IOCTL2, sizeof(pkt.ioctl2), 4);

    if (buflen && ctl->len > 0)
        memcpy(ctl->dest, ctl->buf, ctl->len);

    return res;
}
