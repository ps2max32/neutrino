// Part of neutrino: the EE side without an SDK. See README.md.
#ifndef IOPCONTROL_H
#define IOPCONTROL_H

#include <tamtypes.h>

// Who resets the IOP adds one: the RPC clients are then unbound
extern int _iop_reboot_count;

int SifIopReset(const char *arg, int mode);
int SifIopSync(void);

#endif
