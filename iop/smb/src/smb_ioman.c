/*
 * smb_ioman.irx: the "smb:" iomanX device for the loader, read-only.
 * smb:/DVD/game.iso is DVD\game.iso on the share.
 *
 * Arguments: ip= mask= gw= srv= user= pass= share=
 */
#include <errno.h>
#include <iomanX.h>
#include <io_common.h>
#include <irx.h>
#include <loadcore.h>
#include <stdio.h>
#include <sysclib.h>
#include <thsemap.h>

#include "usmb2.h"
#include "ps2iop-compat.h"
#include "smb_conn.h"

IRX_ID("smb_ioman", 1, 0);

#define MAX_FILES 4
static struct {
        uint8_t *fid;
        int64_t pos, size;
} files[MAX_FILES];

static int sema;

static int smb_nop(void)
{
        return 0;
}

static int smb_eio(void)
{
        return -EIO;
}

static int smb_dev_open(iomanX_iop_file_t *f, const char *name, int flags, int mode)
{
        int i, rc = -EMFILE;

        if ((flags & FIO_O_RDWR) != FIO_O_RDONLY) {
                return -EROFS;
        }
        WaitSema(sema);
        if (smb == NULL && smb_connect()) {
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
        files[i].fid = smb_open(name);
        if (files[i].fid == NULL) {
                rc = -ENOENT;
                goto out;
        }
        /*
         * usmb2_size() returns the low 32 bits of EndOfFile, and leaves
         * all 64 of them in the reply in buf, for ISOs over 4 GB.
         */
        if (usmb2_size(smb, files[i].fid) == -1) {
                usmb2_close(smb, files[i].fid);
                files[i].fid = NULL;
                rc = -EIO;
                goto out;
        }
        memcpy(&files[i].size, &smb->buf[16], 8);
        files[i].pos = 0;
        f->privdata = &files[i];
        rc = 0;
out:
        SignalSema(sema);
        return rc;
}

static int smb_dev_close(iomanX_iop_file_t *f)
{
        typeof(files[0]) *file = f->privdata;

        WaitSema(sema);
        usmb2_close(smb, file->fid);
        file->fid = NULL;
        SignalSema(sema);
        return 0;
}

static int smb_dev_read(iomanX_iop_file_t *f, void *buf, int size)
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
        rc = smb_pread(file->fid, buf, size, file->pos);
        SignalSema(sema);
        if (rc < 0) {
                return -EIO;
        }
        file->pos += rc;
        return rc;
}

static s64 smb_dev_lseek64(iomanX_iop_file_t *f, s64 offset, int whence)
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

static int smb_dev_lseek(iomanX_iop_file_t *f, int offset, int whence)
{
        return smb_dev_lseek64(f, offset, whence);
}

static iomanX_iop_device_ops_t smb_ops = {
        (void *)smb_nop, /* init */
        (void *)smb_nop, /* deinit */
        (void *)smb_eio, /* format */
        smb_dev_open,
        smb_dev_close,
        smb_dev_read,
        (void *)smb_eio, /* write */
        smb_dev_lseek,
        (void *)smb_eio, /* ioctl */
        (void *)smb_eio, /* remove */
        (void *)smb_eio, /* mkdir */
        (void *)smb_eio, /* rmdir */
        (void *)smb_eio, /* dopen */
        (void *)smb_eio, /* dclose */
        (void *)smb_eio, /* dread */
        (void *)smb_eio, /* getstat */
        (void *)smb_eio, /* chstat */
        (void *)smb_eio, /* rename */
        (void *)smb_eio, /* chdir */
        (void *)smb_eio, /* sync */
        (void *)smb_eio, /* mount */
        (void *)smb_eio, /* umount */
        smb_dev_lseek64,
        (void *)smb_eio, /* devctl */
        (void *)smb_eio, /* symlink */
        (void *)smb_eio, /* readlink */
        (void *)smb_eio, /* ioctl2 */
};

static iomanX_iop_device_t smb_dev = {
        "smb", IOP_DT_FSEXT | IOP_DT_FS, 1, "smb", &smb_ops
};

int _start(int argc, char *argv[])
{
        iop_sema_t s = { 0, 0, 1, 1 };

        smb_args(argc, argv);
        sema = CreateSema(&s);
        if (sema < 0 || AddDrv(&smb_dev) < 0) {
                return MODULE_NO_RESIDENT_END;
        }
        return MODULE_RESIDENT_END;
}
