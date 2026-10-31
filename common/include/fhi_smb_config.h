// File Handle Interface for SMB (smb_fhi.irx)
// SMB file handles do not survive an IOP reboot, so the files are
// reopened by path each time smb_fhi.irx is loaded
#ifndef FHI_SMB_CONFIG_H
#define FHI_SMB_CONFIG_H


#include <stdint.h>
#include "fhi.h"


#define FHI_SMB_PATH_MAX 128

struct fhi_smb_info
{
    char path[FHI_SMB_PATH_MAX]; /// Path on the share, without "smb:"
    uint64_t size;               /// Size of the file in bytes
} __attribute__((packed));

struct fhi_smb
{
    // Magic number to find
    uint32_t magic;

    // Files:
    // 0 = ISO
    struct fhi_smb_info file[FHI_MAX_FILES];
} __attribute__((packed));


#endif
