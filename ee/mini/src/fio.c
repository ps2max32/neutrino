// Part of neutrino: the EE side without an SDK. See README.md.
// The client of the ROM's FILEIO (SIF RPC 0x80000001), as ps2sdk's (Academic
// Free License version 2.0). One call at a time: neutrino has one thread.
#include <fileio.h>
#include <iopcontrol.h>
#include <kernel.h>
#include <ps2lib_err.h>
#include <sifrpc.h>
#include <string.h>

#define FIO_F_OPEN  0
#define FIO_F_CLOSE 1
#define FIO_F_READ  2
#define FIO_F_WRITE 3
#define FIO_F_LSEEK 4

#define FIO_PATH_MAX 256

// The IOP sends the parts of a read before and after the 16-byte aligned
// middle here
struct read_data
{
    u32 size1;
    u32 size2;
    void *dest1;
    void *dest2;
    u8 buf1[16];
    u8 buf2[16];
};

static SifRpcClientData_t fio_cd;
static union
{
    struct
    {
        int mode;
        char name[FIO_PATH_MAX];
    } ALIGNED(16) open;
    struct
    {
        int fd;
        void *ptr;
        int size;
        struct read_data *read_data;
    } ALIGNED(16) read;
    struct
    {
        int fd;
        const void *ptr;
        u32 size;
        u32 mis;
        u8 aligned[16];
    } ALIGNED(16) write;
    struct
    {
        int fd;
        int offset;
        int whence;
    } ALIGNED(16) lseek;
    int words[16];
} arg ALIGNED(64);
static int recv[16] ALIGNED(64);
static struct read_data read_data ALIGNED(64);

int fioInit(void)
{
    static int reboot_count = 0;
    int res;

    if (reboot_count != _iop_reboot_count) {
        reboot_count = _iop_reboot_count;
        fioExit();
    }
    if (fio_cd.server != NULL)
        return 0;

    SifInitRpc(0);
    while ((res = SifBindRpc(&fio_cd, 0x80000001, 0)) >= 0 && fio_cd.server == NULL)
        nopdelay();

    return res < 0 ? res : 0;
}

void fioExit(void) { fio_cd.server = NULL; }

static int call(int fno, int size)
{
    int res = fioInit();

    if (res < 0)
        return res;

    SifWriteBackDCache(recv, sizeof(recv));
    res = SifCallRpc(&fio_cd, fno, 0, &arg, size, recv, 4, NULL, NULL);

    return res < 0 ? res : ((volatile int *)UNCACHED_SEG(recv))[0];
}

int fioOpen(const char *path, int flags)
{
    arg.open.mode = flags;
    strncpy(arg.open.name, path, FIO_PATH_MAX - 1);
    arg.open.name[FIO_PATH_MAX - 1] = 0;

    return call(FIO_F_OPEN, sizeof(arg.open));
}

int fioClose(int fd)
{
    arg.words[0] = fd;

    return call(FIO_F_CLOSE, 4);
}

int fioRead(int fd, void *buf, int size)
{
    struct read_data *rd = UNCACHED_SEG(&read_data);
    int res;

    arg.read.fd        = fd;
    arg.read.ptr       = buf;
    arg.read.size      = size;
    arg.read.read_data = &read_data;

    SifWriteBackDCache(&read_data, sizeof(read_data));
    if (!((u32)buf & 0x20000000))
        SifWriteBackDCache(buf, size);

    res = call(FIO_F_READ, sizeof(arg.read));

    if (rd->size1 && rd->dest1)
        memcpy(rd->dest1, rd->buf1, rd->size1);
    if (rd->size2 && rd->dest2)
        memcpy(rd->dest2, rd->buf2, rd->size2);

    return res;
}

int fioWrite(int fd, const void *buf, int size)
{
    u32 mis = 0;

    // What comes before the 16-byte aligned rest goes with the arguments
    if ((u32)buf & 15) {
        mis = 16 - ((u32)buf & 15);
        if (mis > (u32)size)
            mis = size;
    }

    arg.write.fd   = fd;
    arg.write.ptr  = buf;
    arg.write.size = size;
    arg.write.mis  = mis;
    if (mis)
        memcpy(arg.write.aligned, buf, mis);

    if (!((u32)buf & 0x20000000))
        SifWriteBackDCache((void *)buf, size);

    return call(FIO_F_WRITE, sizeof(arg.write));
}

int fioLseek(int fd, int offset, int whence)
{
    arg.lseek.fd     = fd;
    arg.lseek.offset = offset;
    arg.lseek.whence = whence;

    return call(FIO_F_LSEEK, sizeof(arg.lseek));
}
