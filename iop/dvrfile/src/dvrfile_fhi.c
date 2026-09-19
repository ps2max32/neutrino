/*
 * dvrfile_fhi - File Handle Interface for the DVR filesystem
 *
 * Minimal FHI implementation for game boot scenarios.
 * Uses pre-opened DVR-side file handles stored in fhi_fileid struct
 * (populated by EE loader during LE phase via dvrfile.irx).
 *
 * DVRP firmware keeps the file open across the IOP reboot, so this module can
 * serve sector reads/writes directly through pvrdrv using dvrfile's own command
 * protocol (0x110D lseek64 / 0x1111 read / 0x1119 write).
 */

#include "irx_imports.h"
#include "fhi_fileid_config.h"
#include "mprintf.h"

#define MODNAME "dvrfile_fhi"

IRX_ID(MODNAME, 1, 1);

/* Module settings - populated by EE loader via MODULE_SETTINGS_MAGIC scan */
struct fhi_fileid fhi = {MODULE_SETTINGS_MAGIC};

extern struct irx_export_table _exp_fhi;

/* Serializes the shared DVRP command state (pvrdrv handshake buffers) */
static int g_sema = -1;

/* Buffer for unaligned DMA reads */
static int RBUF[32768];

/*
 * Execute a DVRP command and check ack/status.
 * Returns 0 on success and stores the result word in *retval
 */
static int fhi_check_cmdack(int (*func)(drvdrv_exec_cmd_ack *), drvdrv_exec_cmd_ack *cmdack, int *retval)
{
    if (func(cmdack)) {
        *retval = -EIO;
        return -1;
    }
    if (cmdack->comp_status) {
        *retval = -EIO;
        return -1;
    }
    *retval = (cmdack->return_result_word[0] << 16) + cmdack->return_result_word[1];
    return 0;
}

/*
 * FHI interface: get file size in 512-byte sectors
 */
uint32_t fhi_size(int file_handle)
{
    if (file_handle < 0 || file_handle >= FHI_MAX_FILES)
        return 0;

    return fhi.file[file_handle].size / 512;
}

/*
 * FHI interface: read sectors from file
 */
int fhi_read(int file_handle, void *buffer, unsigned int sector_start, unsigned int sector_count)
{
    int dvrp_fd;
    int retval;
    int remain;
    char *out_buf;
    drvdrv_exec_cmd_ack cmdack;

    if (file_handle < 0 || file_handle >= FHI_MAX_FILES)
        return 0;

    dvrp_fd = fhi.file[file_handle].id;
    if (dvrp_fd < 0)
        return 0;

    WaitSema(g_sema);

    {
        /* Seek the persistent DVRP fd to sector_start*512 (SEEK_SET) */
        s64 offset = (s64)sector_start * 512;

        cmdack.command = 0x110D;
        cmdack.input_word[0] = (dvrp_fd >> 16) & 0xFFFF;
        cmdack.input_word[1] = dvrp_fd;
        cmdack.input_word[2] = (offset >> 48) & 0xFFFF;
        cmdack.input_word[3] = (offset >> 32) & 0xFFFF;
        cmdack.input_word[4] = (offset >> 16) & 0xFFFF;
        cmdack.input_word[5] = offset;
        cmdack.input_word[6] = 0; /* SEEK_SET */
        cmdack.input_word[7] = 0;
        cmdack.input_word_count = 8;
        cmdack.timeout = 10000000;
        if (fhi_check_cmdack(&DvrdrvExecCmdAckComp, &cmdack, &retval)) {
            SignalSema(g_sema);
            return 0;
        }
    }

    remain = sector_count * 512;
    out_buf = (char *)buffer;

    cmdack.command = 0x1111;
    cmdack.input_word[0] = (dvrp_fd >> 16) & 0xFFFF;
    cmdack.input_word[1] = dvrp_fd;
    cmdack.input_word_count = 4;
    cmdack.timeout = 10000000;

    while (remain > 0) {
        int chunk_size;
        int read_size;
        int use_bounce;

        chunk_size = (remain > 0x4000) ? 0x4000 : remain;

        /* DMA requires a 128-byte multiple and a 4-byte aligned target buffer. */
        use_bounce = ((chunk_size & 0x7F) != 0) || (((u32)out_buf & 3) != 0);

        cmdack.input_word[2] = (chunk_size >> 16) & 0xFFFF;
        cmdack.input_word[3] = chunk_size;
        cmdack.output_buffer = use_bounce ? (void *)RBUF : (void *)out_buf;

        if (fhi_check_cmdack(&DvrdrvExecCmdAckDmaRecvComp, &cmdack, &read_size)) {
            SignalSema(g_sema);
            return 0;
        }

        if (read_size <= 0)
            break;

        if (use_bounce)
            memcpy(out_buf, RBUF, read_size);

        remain -= read_size;
        out_buf += read_size;
    }

    SignalSema(g_sema);
    return (remain == 0) ? sector_count : 0;
}

/*
 * FHI interface: write sectors to file
 */
int fhi_write(int file_handle, const void *buffer, unsigned int sector_start, unsigned int sector_count)
{
    int dvrp_fd;
    int retval;
    int remain;
    int unaligned_size;
    char *in_buf;
    drvdrv_exec_cmd_ack cmdack;

    if (file_handle < 0 || file_handle >= FHI_MAX_FILES)
        return 0;

    dvrp_fd = fhi.file[file_handle].id;
    if (dvrp_fd < 0)
        return 0;

    in_buf = (char *)buffer;
    remain = sector_count * 512;

    WaitSema(g_sema);

    /* DMA send needs a 4-byte aligned input buffer; copy the head bytes
       into the shared buffer when the source is not aligned. */
    unaligned_size = 0;
    if (((u32)in_buf & 3) != 0) {
        unaligned_size = 4 - ((u32)in_buf & 3);
        if (unaligned_size > remain)
            unaligned_size = remain;
        memcpy(RBUF, in_buf, unaligned_size);
    }

    /* Random access: seek the persistent DVRP fd to sector_start*512 (SEEK_SET) */
    {
        s64 offset = (s64)sector_start * 512;

        cmdack.command = 0x110D;
        cmdack.input_word[0] = (dvrp_fd >> 16) & 0xFFFF;
        cmdack.input_word[1] = dvrp_fd;
        cmdack.input_word[2] = (offset >> 48) & 0xFFFF;
        cmdack.input_word[3] = (offset >> 32) & 0xFFFF;
        cmdack.input_word[4] = (offset >> 16) & 0xFFFF;
        cmdack.input_word[5] = offset;
        cmdack.input_word[6] = 0; /* SEEK_SET */
        cmdack.input_word[7] = 0;
        cmdack.input_word_count = 8;
        cmdack.timeout = 10000000;
        if (fhi_check_cmdack(&DvrdrvExecCmdAckComp, &cmdack, &retval)) {
            SignalSema(g_sema);
            return 0;
        }
    }

    cmdack.command = 0x1119;
    cmdack.input_word[0] = (dvrp_fd >> 16) & 0xFFFF;
    cmdack.input_word[1] = dvrp_fd;
    cmdack.input_word_count = 4;
    cmdack.timeout = 10000000;

    while (remain > 0) {
        int chunk_size;

        chunk_size = (remain > 0x4000) ? 0x4000 : remain;
        if (unaligned_size != 0)
            chunk_size = unaligned_size;

        cmdack.input_word[2] = (chunk_size >> 16) & 0xFFFF;
        cmdack.input_word[3] = chunk_size;

        if (unaligned_size != 0) {
            cmdack.input_buffer = (void *)RBUF;
            unaligned_size = 0;
        } else {
            cmdack.input_buffer = (void *)in_buf;
        }
        cmdack.input_buffer_length = chunk_size;

        if (fhi_check_cmdack(&DvrdrvExecCmdAckDmaSendComp, &cmdack, &retval)) {
            SignalSema(g_sema);
            return 0;
        }
        if (retval <= 0) {
            SignalSema(g_sema);
            return 0;
        }

        remain -= retval;
        in_buf += retval;
    }

    SignalSema(g_sema);
    return sector_count;
}

int _start(int argc, char *argv[])
{
    iop_sema_t smp;

    (void)argc;
    (void)argv;

    smp.attr = 0;
    smp.initial = 1;
    smp.max = 1;
    smp.option = 0;
    g_sema = CreateSema(&smp);
    if (g_sema < 0) {
        M_DEBUG("failed to create semaphore\n");
        return MODULE_NO_RESIDENT_END;
    }

    if (RegisterLibraryEntries(&_exp_fhi) != 0) {
        M_DEBUG("fhi library already registered\n");
        return MODULE_NO_RESIDENT_END;
    }

    M_DEBUG("ready (dvrp fd=%d, size=%u bytes)\n", fhi.file[0].id, (unsigned int)fhi.file[0].size);
    return MODULE_RESIDENT_END;
}
