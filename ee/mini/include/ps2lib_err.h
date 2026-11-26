// Part of neutrino: the EE side without an SDK. See README.md.
#ifndef PS2LIB_ERR_H
#define PS2LIB_ERR_H

enum {
    E_IOP_NO_MEMORY   = 400,
    E_LIB_API_INIT    = 0xd601,
    E_LIB_SEMA_CREATE = 0xd602,
    E_SIF_PKT_ALLOC   = 0xd610,
    E_SIF_PKT_SEND    = 0xd611,
    E_SIF_RPC_BIND    = 0xd612,
    E_SIF_RPC_CALL    = 0xd613,
};

#endif
