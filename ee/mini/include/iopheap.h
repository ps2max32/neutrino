// Part of neutrino: the EE side without an SDK. See README.md.
#ifndef IOPHEAP_H
#define IOPHEAP_H

int SifInitIopHeap(void);
void SifExitIopHeap(void);
void *SifAllocIopHeap(int size);
int SifFreeIopHeap(void *addr);

#endif
