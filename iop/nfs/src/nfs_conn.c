/*
 * The connection to the NFS export, shared by nfs_ioman.irx and nfs_fhi.irx:
 * unfs4 on the tiny TCP/IP stack, over smap.irx and ministack.irx.
 */
#include <fcntl.h>
#include <stdio.h>
#include <sysclib.h>
#include <thbase.h>

#include "ip.h"
#include "tcp.h"
#include "unfs4.h"
#include "slip.h"
#include "ps2iop-compat.h"
#include "nfs_conn.h"

#define BUF_SIZE 1500
static uint8_t tcp_buf[BUF_SIZE] __attribute__((aligned(4)));
static tcp_context_t tcp;

static uint32_t ip, mask = 0xffffff00, gw, srv, uid, gid;
/* A copy: argv is gone once _start() returns */
static char export[128];

struct unfs4_context *nfs;

static uint32_t parse_ip(const char *s)
{
        uint32_t v = 0, part = 0;

        for (; *s; s++) {
                if (*s == '.') {
                        v = (v << 8) | part;
                        part = 0;
                } else {
                        part = part * 10 + (*s - '0');
                }
        }
        return (v << 8) | part;
}

static uint32_t parse_num(const char *s)
{
        uint32_t v = 0;

        for (; *s >= '0' && *s <= '9'; s++) {
                v = v * 10 + (*s - '0');
        }
        return v;
}

void nfs_args(int argc, char *argv[])
{
        int i;

        for (i = 1; i < argc; i++) {
                if (!strncmp(argv[i], "ip=", 3)) ip = parse_ip(&argv[i][3]);
                else if (!strncmp(argv[i], "mask=", 5)) mask = parse_ip(&argv[i][5]);
                else if (!strncmp(argv[i], "gw=", 3)) gw = parse_ip(&argv[i][3]);
                else if (!strncmp(argv[i], "srv=", 4)) srv = parse_ip(&argv[i][4]);
                else if (!strncmp(argv[i], "uid=", 4)) uid = parse_num(&argv[i][4]);
                else if (!strncmp(argv[i], "gid=", 4)) gid = parse_num(&argv[i][4]);
                else if (!strncmp(argv[i], "export=", 7)) strncpy(export, &argv[i][7], sizeof(export) - 1);
        }
}

int nfs_connect(void)
{
        iop_sys_clock_t t;

        tcp.ip.pkt = tcp_buf;
        tcp.ip.pkt_size = BUF_SIZE;
        tcp.ip.saddr = htonl(ip);
        tcp.ip.daddr = htonl(srv);
        /*
         * A new port each time, the server may still have the last one.
         * Below 1024, as a Linux export wants unless it is "insecure".
         */
        GetSystemTime(&t);
        tcp.src_port = htons(512 + (t.lo & 0x1ff));
        tcp.dst_port = htons(2049);
        /* Before ps2smap_init(): its receive handler reads the port */
        ps2smap_set_port(&tcp.src_port);
        if (ps2smap_init(ip, mask, gw)) {
                return -1;
        }

        if (tcp_connect(&tcp) < 0) {
                printf("nfs: no connection to %d.%d.%d.%d\n", (int)(srv >> 24), (int)(srv >> 16) & 0xff, (int)(srv >> 8) & 0xff, (int)srv & 0xff);
                return -1;
        }
        unfs4_tcp = &tcp;
        nfs = unfs4_init_context(0, uid, gid);
        if (nfs == NULL) {
                return -1;
        }
        /* unfs4.h asks for xids from a clock */
        nfs->xid = t.lo;
        if (unfs4_mount(nfs, export)) {
                printf("nfs: mount of %s failed\n", export);
                free(nfs);
                nfs = NULL;
                return -1;
        }
        return 0;
}

uint8_t *nfs_open(const char *path)
{
        return unfs4_open(nfs, path, O_RDONLY);
}

int nfs_pread(uint8_t *fh, void *buf, int count, uint64_t offset)
{
        int done, n, rc;

        for (done = 0; done < count; done += rc) {
                n = count - done > 2048 ? 2048 : count - done;
                rc = unfs4_pread(nfs, fh, (uint8_t *)buf + done, n, offset + done);
                if (rc <= 0 || rc > n) {
                        return done ? done : -1;
                }
                if (rc < n) {
                        return done + rc;
                }
        }
        return done;
}
