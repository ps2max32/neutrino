// Part of neutrino: the EE side without an SDK. See README.md.
#ifndef SIFRPC_H
#define SIFRPC_H

#include <sifcmd.h>

#define SIF_RPC_M_NOWAIT 1 // do not wait for the end of the call

typedef struct
{
    void *pkt_addr; // the packet of the running request
    u32 rpc_id;
    int sema_id; // signalled at the end of it, or -1
    void *server; // NULL: not bound
    void *buf; // the server's, for the data sent
} SifRpcClientData_t;

typedef SifRpcClientData_t SifRpcReceiveData_t;

void SifInitRpc(int mode);
void SifExitRpc(void);
int SifBindRpc(SifRpcClientData_t *cd, u32 sid, int mode);
// end_function and end_param: NULL
int SifCallRpc(SifRpcClientData_t *cd, int fno, int mode, void *send, int ssize, void *recv, int rsize, void *end_function,
               void *end_param);
// Read IOP memory. The EE's buffer is 64-byte aligned and not in the cache.
int SifRpcGetOtherData(SifRpcReceiveData_t *rd, void *src, void *dest, int size, int mode);

#endif
