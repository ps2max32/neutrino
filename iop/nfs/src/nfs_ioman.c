/*
 * nfs_ioman.irx: the "nfs:" iomanX device for the loader, read-only.
 * nfs:/DVD/game.iso is DVD/game.iso in the export.
 *
 * Arguments: ip= mask= gw= srv= export= uid= gid=
 */
#include <errno.h>
#include <iomanX.h>
#include <io_common.h>
#include <irx.h>
#include <loadcore.h>
#include <stdio.h>
#include <sysclib.h>
#include <thsemap.h>

#include "unfs4.h"
#include "ps2iop-compat.h"
#include "nfs_conn.h"

IRX_ID("nfs_ioman", 1, 0);

#define MAX_FILES 4
static struct {
        uint8_t *fid;
        int64_t pos, size;
} files[MAX_FILES];

static int sema;

static int nfs_nop(void)
{
        return 0;
}

static int nfs_eio(void)
{
        return -EIO;
}

static int nfs_dev_open(iomanX_iop_file_t *f, const char *name, int flags, int mode)
{
        int i, rc = -EMFILE;

        if ((flags & FIO_O_RDWR) != FIO_O_RDONLY) {
                return -EROFS;
        }
        WaitSema(sema);
        if (nfs == NULL && nfs_connect()) {
                rc = -EIO;
                goto out;
        }
        for (i = 0; i < MAX_FILES; i++) {
                if (files[i].fid == NULL) {
                        break;
                }
        }
        if (i == MAX_FILES) {
                goto out;
        }
        files[i].fid = nfs_open(name);
        if (files[i].fid == NULL) {
                rc = -ENOENT;
                goto out;
        }
        files[i].size = unfs4_size(nfs, files[i].fid);
        if (files[i].size < 0) {
                unfs4_close(nfs, files[i].fid);
                files[i].fid = NULL;
                rc = -EIO;
                goto out;
        }
        files[i].pos = 0;
        f->privdata = &files[i];
        rc = 0;
out:
        SignalSema(sema);
        return rc;
}

static int nfs_dev_close(iomanX_iop_file_t *f)
{
        typeof(files[0]) *file = f->privdata;

        WaitSema(sema);
        unfs4_close(nfs, file->fid);
        file->fid = NULL;
        SignalSema(sema);
        return 0;
}

static int nfs_dev_read(iomanX_iop_file_t *f, void *buf, int size)
{
        typeof(files[0]) *file = f->privdata;
        int rc;

        if (size > file->size - file->pos) {
                size = file->size - file->pos;
        }
        if (size <= 0) {
                return 0;
        }
        WaitSema(sema);
        rc = nfs_pread(file->fid, buf, size, file->pos);
        SignalSema(sema);
        if (rc < 0) {
                return -EIO;
        }
        file->pos += rc;
        return rc;
}

static s64 nfs_dev_lseek64(iomanX_iop_file_t *f, s64 offset, int whence)
{
        typeof(files[0]) *file = f->privdata;

        if (whence == SEEK_CUR) {
                offset += file->pos;
        } else if (whence == SEEK_END) {
                offset += file->size;
        }
        if (offset < 0) {
                return -EINVAL;
        }
        file->pos = offset;
        return offset;
}

static int nfs_dev_lseek(iomanX_iop_file_t *f, int offset, int whence)
{
        return nfs_dev_lseek64(f, offset, whence);
}

static iomanX_iop_device_ops_t nfs_ops = {
        (void *)nfs_nop, /* init */
        (void *)nfs_nop, /* deinit */
        (void *)nfs_eio, /* format */
        nfs_dev_open,
        nfs_dev_close,
        nfs_dev_read,
        (void *)nfs_eio, /* write */
        nfs_dev_lseek,
        (void *)nfs_eio, /* ioctl */
        (void *)nfs_eio, /* remove */
        (void *)nfs_eio, /* mkdir */
        (void *)nfs_eio, /* rmdir */
        (void *)nfs_eio, /* dopen */
        (void *)nfs_eio, /* dclose */
        (void *)nfs_eio, /* dread */
        (void *)nfs_eio, /* getstat */
        (void *)nfs_eio, /* chstat */
        (void *)nfs_eio, /* rename */
        (void *)nfs_eio, /* chdir */
        (void *)nfs_eio, /* sync */
        (void *)nfs_eio, /* mount */
        (void *)nfs_eio, /* umount */
        nfs_dev_lseek64,
        (void *)nfs_eio, /* devctl */
        (void *)nfs_eio, /* symlink */
        (void *)nfs_eio, /* readlink */
        (void *)nfs_eio, /* ioctl2 */
};

static iomanX_iop_device_t nfs_dev = {
        "nfs", IOP_DT_FSEXT | IOP_DT_FS, 1, "nfs", &nfs_ops
};

int _start(int argc, char *argv[])
{
        iop_sema_t s = { 0, 0, 1, 1 };

        nfs_args(argc, argv);
        sema = CreateSema(&s);
        if (sema < 0 || AddDrv(&nfs_dev) < 0) {
                return MODULE_NO_RESIDENT_END;
        }
        return MODULE_RESIDENT_END;
}
