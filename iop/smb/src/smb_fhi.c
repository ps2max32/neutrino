/*
 * smb_fhi.irx: the File Handle Interface on an SMB share, for the emulators
 * in the game. The loader fills in the path and size of each file. After
 * every IOP reboot a thread connects again and reopens them, and until it
 * has fhi_size() is 0, which cdvdman_emu waits on.
 *
 * Arguments: ip= mask= gw= srv= user= pass= share=
 */
#include <irx.h>
#include <loadcore.h>
#include <stdio.h>
#include <thbase.h>
#include <thsemap.h>

#include "usmb2.h"
#include "smb_conn.h"
#include "fhi_smb_config.h"

IRX_ID("smb_fhi", 1, 0);

/* Module settings - populated by EE loader via MODULE_SETTINGS_MAGIC scan */
struct fhi_smb fhi = {MODULE_SETTINGS_MAGIC};

static uint8_t *fids[FHI_MAX_FILES];
static volatile int ready;
static int sema;

extern struct irx_export_table _exp_fhi __attribute__((section("data")));

uint32_t fhi_size(int file_handle)
{
        if (!ready || file_handle < 0 || file_handle >= FHI_MAX_FILES || fids[file_handle] == NULL) {
                return 0;
        }
        return fhi.file[file_handle].size / 512;
}

int fhi_read(int file_handle, void *buffer, unsigned int sector_start, unsigned int sector_count)
{
        int i, rc = -1;

        if (fhi_size(file_handle) == 0) {
                return 0;
        }
        WaitSema(sema);
        for (i = 0; i < 3 && rc != sector_count * 512; i++) {
                rc = smb_pread(fids[file_handle], buffer, sector_count * 512, (uint64_t)sector_start * 512);
        }
        SignalSema(sema);
        if (rc != sector_count * 512) {
                printf("smb_fhi: read of %u sectors at %u failed\n", sector_count, sector_start);
                return 0;
        }
        return sector_count;
}

int fhi_write(int file_handle, const void *buffer, unsigned int sector_start, unsigned int sector_count)
{
        return 0;
}

static void connect_thread(void *arg)
{
        int i;

        while (smb_connect()) {
                DelayThread(1000 * 1000);
        }
        for (i = 0; i < FHI_MAX_FILES; i++) {
                if (fhi.file[i].path[0]) {
                        fids[i] = smb_open(fhi.file[i].path);
                        if (fids[i] == NULL) {
                                printf("smb_fhi: open %s failed\n", fhi.file[i].path);
                        }
                }
        }
        ready = 1;
}

int _start(int argc, char *argv[])
{
        iop_thread_t thread = { TH_C, 0, connect_thread, 0x800, 0x40 };
        iop_sema_t s = { 0, 0, 1, 1 };
        int tid;

        smb_args(argc, argv);
        sema = CreateSema(&s);
        if (sema < 0 || RegisterLibraryEntries(&_exp_fhi) != 0) {
                return MODULE_NO_RESIDENT_END;
        }
        tid = CreateThread(&thread);
        if (tid < 0 || StartThread(tid, NULL) < 0) {
                return MODULE_NO_RESIDENT_END;
        }
        return MODULE_RESIDENT_END;
}
