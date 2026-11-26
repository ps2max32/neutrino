// Part of neutrino: the EE side without an SDK. See README.md.
// fileXio.irx's client (iomanX's files over SIF RPC). Its descriptors are the
// IOP's.
#ifndef FILEXIO_RPC_H
#define FILEXIO_RPC_H

#include <tamtypes.h>

int fileXioInit(void);
void fileXioExit(void);
int fileXioOpen(const char *path, int flags, int mode);
int fileXioClose(int fd);
int fileXioRead(int fd, void *buf, int size);
int fileXioWrite(int fd, const void *buf, int size);
int fileXioLseek(int fd, int offset, int whence);
s64 fileXioLseek64(int fd, s64 offset, int whence);
int fileXioIoctl2(int fd, int cmd, void *arg, unsigned int arglen, void *buf, unsigned int buflen);

#endif
