// Part of neutrino: the EE side without an SDK. See README.md.
// The ROM's FILEIO client (ioman's files over SIF RPC). Its descriptors are
// the IOP's; 0, 1 and 2 are the IOP's tty.
#ifndef FILEIO_H
#define FILEIO_H

int fioInit(void);
void fioExit(void);
int fioOpen(const char *path, int flags);
int fioClose(int fd);
int fioRead(int fd, void *buf, int size);
int fioWrite(int fd, const void *buf, int size);
int fioLseek(int fd, int offset, int whence);

#endif
