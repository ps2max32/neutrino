// File Handle Interface for NFS (nfs_fhi.irx)
// The filehandles are kept in IOP memory, which an IOP reboot clears, so
// the files are looked up by path each time nfs_fhi.irx is loaded
#ifndef FHI_NFS_CONFIG_H
#define FHI_NFS_CONFIG_H


#include <stdint.h>
#include "fhi.h"


#define FHI_NFS_PATH_MAX 128

struct fhi_nfs_info
{
    char path[FHI_NFS_PATH_MAX]; /// Path on the export, without "nfs:"
    uint64_t size;               /// Size of the file in bytes
} __attribute__((packed));

struct fhi_nfs
{
    // Magic number to find
    uint32_t magic;

    // Files:
    // 0 = ISO
    struct fhi_nfs_info file[FHI_MAX_FILES];
} __attribute__((packed));


#endif
