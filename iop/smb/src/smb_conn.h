#ifndef SMB_CONN_H
#define SMB_CONN_H

#include <stdint.h>

/* The connection to the share, NULL until smb_connect() succeeds */
extern struct usmb2_context *smb;

/* Takes ip= mask= gw= srv= user= pass= share=, and ignores the rest */
void smb_args(int argc, char *argv[]);
/* TCP connect, login and tree connect. 0 on success */
int smb_connect(void);
/*
 * usmb2 puts the name length in UTF-16 bytes in one byte of a 512-byte
 * request, so names are at most 127 characters
 */
#define SMB_NAME_MAX 127

/* Opens a file read-only, with the path in either '/' or '\' form.
 * NULL if it fails or the path is longer than SMB_NAME_MAX */
uint8_t *smb_open(const char *path);
/* Reads in pieces that fit the reassembly buffer. Bytes read, or -1 */
int smb_pread(uint8_t *fid, void *buf, int count, uint64_t offset);

#endif
