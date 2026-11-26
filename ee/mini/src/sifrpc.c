// Part of neutrino: the EE side without an SDK. See README.md.
// The SIF command and RPC client, on the BIOS. Both share one packet buffer
// for what the IOP sends and one table of RPC packets.
#include <iopcontrol.h>
#include <kernel.h>
#include <ps2lib_err.h>
#include <sifrpc.h>

#define PACKET_SIZE  64
#define PACKET_COUNT 8
#define SREG_COUNT   32
#define SREG_RPCINIT 0

#define DMAC_STAT      ((vu32 *)0x1000e010)
#define DMAC_STAT_SIF0 (1 << DMAC_SIF0)
#define SIF0_CHCR      ((vu32 *)0x1000c000)
#define CHCR_STR       0x100

// An RPC packet, in both directions: what follows rpc_id depends on the
// command
typedef struct
{
    SifCmdHeader_t header;
    int rec_id;
    void *pkt_addr;
    u32 rpc_id; // 0: free
    SifRpcClientData_t *cd;
    union
    {
        struct
        {
            u32 sid;
        } bind;
        struct
        {
            int fno;
            int send_size;
            void *recv;
            int recv_size;
            int rmode; // 1: the IOP sends RPC_END
            void *server;
        } call;
        struct
        {
            void *src;
            void *dest;
            int size;
        } rdata;
        struct
        {
            u32 cid; // the request's command
            void *server;
            void *buf;
            void *cbuf;
        } end;
    } u;
} ALIGNED(64) rpc_packet_t;

// What SIF_SYSREG_MAINADDR points to: the structure of the SCE libraries
static struct
{
    void *pktbuf;
    void *unused;
    void *iopbuf;
    void *sys_handlers;
    u32 nr_sys_handlers;
    void *usr_handlers;
    u32 nr_usr_handlers;
    int *sregs;
} ALIGNED(64) cmd_data;

static u8 pktbuf[128] ALIGNED(64);
static u8 unused[64] ALIGNED(64);
static struct
{
    SifCmdHeader_t header;
    void *buf;
} ALIGNED(64) addr_pkt;
static rpc_packet_t packets[PACKET_COUNT]; // used uncached
static u32 sys_handlers[32][3]; // for others: SIF_SYSREG_MAINADDR; not used here
static vu32 sregs[SREG_COUNT];
static u32 next_rpc_id;
static int handler_id;
static int rpc_init;

int _iop_reboot_count = 0;

//---------------------------------------------------------------------------
// The packet goes to the IOP's buffer after the data
static u32 send_cmd(u32 cid, void *pkt, int pktsize, void *src, void *dest, int size)
{
    SifDmaTransfer_t dmat[2];
    SifCmdHeader_t *header = UNCACHED_SEG(pkt);
    int count              = 0;

    header->cid   = cid;
    header->psize = pktsize;
    header->dsize = 0;
    header->dest  = NULL;

    if (size > 0) {
        header->dsize = size;
        header->dest  = dest;

        dmat[count].src  = src;
        dmat[count].dest = dest;
        dmat[count].size = size;
        dmat[count].attr = 0;
        count++;
    }

    dmat[count].src  = pkt;
    dmat[count].dest = cmd_data.iopbuf;
    dmat[count].size = pktsize;
    dmat[count].attr = SIF_DMA_ERT | SIF_DMA_INT_O;
    count++;

    return SifSetDma(dmat, count);
}

static void rpc_end(rpc_packet_t *p)
{
    SifRpcClientData_t *cd = p->cd;
    rpc_packet_t *request  = cd->pkt_addr;

    if (p->u.end.cid == SIF_CMD_RPC_BIND) {
        cd->server = p->u.end.server;
        cd->buf    = p->u.end.buf;
    }

    cd->pkt_addr = NULL;
    if (request != NULL)
        request->rpc_id = 0;
    if (cd->sema_id >= 0)
        iSignalSema(cd->sema_id);
}

// DMAC handler: the IOP has sent a packet
static int sif0_handler(int channel)
{
    vu32 *buf = cmd_data.pktbuf;
    union
    {
        u32 words[sizeof(pktbuf) / 4];
        SifCmdHeader_t header;
        rpc_packet_t rpc;
    } packet;
    u32 size = ((volatile u8 *)buf)[0];

    if (size == 0)
        goto out;
    if (size > sizeof(pktbuf))
        size = sizeof(pktbuf);

    for (u32 i = 0; i < (size + 3) / 4; i++)
        packet.words[i] = buf[i];
    ((volatile u8 *)buf)[0] = 0;

    // Ready for the next one
    iSifSetDChain();

    switch (packet.header.cid) {
        case SIF_CMD_CHANGE_SADDR:
            cmd_data.iopbuf = (void *)packet.words[4];
            break;
        case SIF_CMD_SET_SREG:
            if (packet.words[4] < SREG_COUNT)
                sregs[packet.words[4]] = packet.words[5];
            break;
        case SIF_CMD_RPC_END:
            rpc_end(&packet.rpc);
            break;
    }

out:
    // A handler ends with the interrupts enabled
    __asm__ volatile("sync.l\n\tei" : : : "memory");
    return 0;
}

static void exit_cmd(void)
{
    if (handler_id > 0) {
        DisableDmac(DMAC_SIF0);
        RemoveDmacHandler(DMAC_SIF0, handler_id);
        handler_id = 0;
    }
}

static void init_cmd(void)
{
    cmd_data.pktbuf          = UNCACHED_SEG(pktbuf);
    cmd_data.unused          = UNCACHED_SEG(unused);
    cmd_data.sys_handlers    = sys_handlers;
    cmd_data.nr_sys_handlers = 32;
    cmd_data.usr_handlers    = NULL;
    cmd_data.nr_usr_handlers = 0;
    cmd_data.sregs           = (int *)sregs;

    for (int i = 0; i < SREG_COUNT; i++)
        sregs[i] = 0;

    // What is used uncached must not be in the cache
    FlushCache(WRITEBACK_DCACHE);
    SifWriteBackDCache(pktbuf, sizeof(pktbuf));
    SifWriteBackDCache(packets, sizeof(packets));
    SifWriteBackDCache(&addr_pkt, sizeof(addr_pkt));
    for (int i = 0; i < PACKET_COUNT; i++)
        ((rpc_packet_t *)UNCACHED_SEG(packets))[i].rpc_id = 0;

    if (*DMAC_STAT & DMAC_STAT_SIF0)
        *DMAC_STAT = DMAC_STAT_SIF0;
    if (!(*SIF0_CHCR & CHCR_STR))
        SifSetDChain();

    handler_id = AddDmacHandler(DMAC_SIF0, sif0_handler, 0);
    EnableDmac(DMAC_SIF0);

    vu32 *addr = UNCACHED_SEG(&addr_pkt);
    addr[4]    = (u32)cmd_data.pktbuf;

    cmd_data.iopbuf = (void *)SifGetReg(SIF_SYSREG_SUBADDR);
    if (cmd_data.iopbuf != NULL) {
        // The IOP's SIFCMD runs already: just our buffer
        addr[3] = 0;
        send_cmd(SIF_CMD_CHANGE_SADDR, &addr_pkt, 20, NULL, NULL, 0);
    } else {
        while (!(SifGetReg(SIF_REG_SMFLAG) & SIF_STAT_CMDINIT))
            ;

        cmd_data.iopbuf = (void *)SifGetReg(SIF_REG_SUBADDR);
        SifSetReg(SIF_SYSREG_SUBADDR, (int)cmd_data.iopbuf);
        SifSetReg(SIF_SYSREG_MAINADDR, (int)&cmd_data);
        addr[3] = 0; // opt: initialise SIFCMD
        send_cmd(SIF_CMD_INIT_CMD, &addr_pkt, 20, NULL, NULL, 0);
    }
}

//---------------------------------------------------------------------------
void SifInitRpc(int mode)
{
    static int reboot_count = 0;

    if (reboot_count != _iop_reboot_count) {
        reboot_count = _iop_reboot_count;
        SifExitRpc();
    }
    if (rpc_init)
        return;
    rpc_init = 1;

    init_cmd();

    if (SifGetReg(SIF_SYSREG_RPCINIT))
        return;

    // Not addr_pkt: that one may still be on its way
    vu32 *init = UNCACHED_SEG(&packets[PACKET_COUNT - 1]);
    init[3]    = 1; // opt: initialise SIFRPC
    send_cmd(SIF_CMD_INIT_CMD, &packets[PACKET_COUNT - 1], 16, NULL, NULL, 0);
    while (!sregs[SREG_RPCINIT])
        ;
    SifSetReg(SIF_SYSREG_RPCINIT, 1);
}

void SifExitRpc(void)
{
    exit_cmd();
    rpc_init = 0;
}

static rpc_packet_t *get_packet(SifRpcClientData_t *cd)
{
    rpc_packet_t *p = UNCACHED_SEG(packets);
    int enabled     = DIntr();

    for (int i = 0; i < PACKET_COUNT; i++, p++) {
        if (p->rpc_id == 0) {
            if (++next_rpc_id == 0)
                next_rpc_id = 1;
            p->rpc_id   = next_rpc_id;
            p->rec_id   = (i << 16) | 5; // 4: the index is valid, 1: in use
            p->pkt_addr = p;
            p->cd       = cd;
            if (enabled)
                EIntr();

            cd->pkt_addr = p;
            cd->rpc_id   = p->rpc_id;
            cd->sema_id  = -1;
            return p;
        }
    }

    if (enabled)
        EIntr();
    return NULL;
}

// Send the request and, unless SIF_RPC_M_NOWAIT, wait for its end
static int request(SifRpcClientData_t *cd, u32 cid, rpc_packet_t *p, int mode, void *src, void *dest, int size)
{
    void *pkt = (void *)((u32)p & ~0x20000000);

    if (!(mode & SIF_RPC_M_NOWAIT)) {
        ee_sema_t sema;

        sema.max_count  = 1;
        sema.init_count = 0;
        sema.option     = 0;
        cd->sema_id     = CreateSema(&sema);
        if (cd->sema_id < 0) {
            p->rpc_id = 0;
            return -E_LIB_SEMA_CREATE;
        }
    }

    if (!send_cmd(cid, pkt, PACKET_SIZE, src, dest, size)) {
        p->rpc_id = 0;
        if (cd->sema_id >= 0)
            DeleteSema(cd->sema_id);
        return -E_SIF_PKT_SEND;
    }

    if (!(mode & SIF_RPC_M_NOWAIT)) {
        WaitSema(cd->sema_id);
        DeleteSema(cd->sema_id);
    }

    return 0;
}

int SifBindRpc(SifRpcClientData_t *cd, u32 sid, int mode)
{
    rpc_packet_t *p = get_packet(cd);

    if (p == NULL)
        return -E_SIF_PKT_ALLOC;

    cd->server    = NULL;
    p->u.bind.sid = sid;

    return request(cd, SIF_CMD_RPC_BIND, p, mode, NULL, NULL, 0);
}

int SifCallRpc(SifRpcClientData_t *cd, int fno, int mode, void *send, int ssize, void *recv, int rsize, void *end_function,
               void *end_param)
{
    rpc_packet_t *p = get_packet(cd);

    if (p == NULL)
        return -E_SIF_PKT_ALLOC;

    p->u.call.fno       = fno;
    p->u.call.send_size = ssize;
    p->u.call.recv      = recv;
    p->u.call.recv_size = rsize;
    p->u.call.rmode     = (mode & SIF_RPC_M_NOWAIT) ? 0 : 1;
    p->u.call.server    = cd->server;

    SifWriteBackDCache(send, ssize);
    SifWriteBackDCache(recv, rsize);

    return request(cd, SIF_CMD_RPC_CALL, p, mode, send, cd->buf, ssize);
}

int SifRpcGetOtherData(SifRpcReceiveData_t *rd, void *src, void *dest, int size, int mode)
{
    rpc_packet_t *p = get_packet(rd);

    if (p == NULL)
        return -E_SIF_PKT_ALLOC;

    p->u.rdata.src  = src;
    p->u.rdata.dest = dest;
    p->u.rdata.size = size;

    return request(rd, SIF_CMD_RPC_RDATA, p, mode, NULL, NULL, 0);
}

//---------------------------------------------------------------------------
int SifIopSync(void) { return (SifGetReg(SIF_REG_SMFLAG) & SIF_STAT_BOOTEND) != 0; }
