#ifndef NFS_CONN_H
#define NFS_CONN_H

#include <stdint.h>

/* The connection to the export, NULL until nfs_connect() succeeds */
extern struct unfs4_context *nfs;

/* Takes ip= mask= gw= srv= export= uid= gid=, and ignores the rest */
void nfs_args(int argc, char *argv[]);
/* TCP connect and mount of the export. 0 on success */
int nfs_connect(void);
/* Opens a file read-only. NULL if it fails or the path does not fit */
uint8_t *nfs_open(const char *path);
/* Reads in pieces that fit the reassembly buffer. Bytes read, or -1 */
int nfs_pread(uint8_t *fh, void *buf, int count, uint64_t offset);

#endif
