// Part of neutrino: the EE side without an SDK. See README.md.
#ifndef USBHDFSD_COMMON_H
#define USBHDFSD_COMMON_H

#include <tamtypes.h>

// ioctl2 of bdm's file systems
#define USBMASS_IOCTL_GET_DRIVERNAME    0x0003
#define USBMASS_IOCTL_GET_FRAGLIST      0x0005
#define USBMASS_IOCTL_GET_DEVICE_NUMBER 0x0006

typedef struct bd_fragment
{
    u64 sector;
    u32 count;
} __attribute__((packed)) bd_fragment_t;

#endif
