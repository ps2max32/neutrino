/*
 * The connection to the SMB share, shared by smb_ioman.irx and smb_fhi.irx:
 * usmb2 on its tiny TCP/IP stack, over smap.irx and ministack.irx.
 */
#include <fcntl.h>
#include <stdio.h>
#include <sysclib.h>
#include <thbase.h>

#include "ip.h"
#include "tcp.h"
#include "usmb2.h"
#include "slip.h"
#include "ps2iop-compat.h"
#include "smb_conn.h"

#define BUF_SIZE 1500
static uint8_t tcp_buf[BUF_SIZE] __attribute__((aligned(4)));
static tcp_context_t tcp;

static uint32_t ip, mask = 0xffffff00, gw, srv;
/* Copies: argv is gone once _start() returns */
static char user[32], pass[32], share[64];

struct usmb2_context *smb;

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

void smb_args(int argc, char *argv[])
{
        int i;

        for (i = 1; i < argc; i++) {
                if (!strncmp(argv[i], "ip=", 3)) ip = parse_ip(&argv[i][3]);
                else if (!strncmp(argv[i], "mask=", 5)) mask = parse_ip(&argv[i][5]);
                else if (!strncmp(argv[i], "gw=", 3)) gw = parse_ip(&argv[i][3]);
                else if (!strncmp(argv[i], "srv=", 4)) srv = parse_ip(&argv[i][4]);
                else if (!strncmp(argv[i], "user=", 5)) strncpy(user, &argv[i][5], sizeof(user) - 1);
                else if (!strncmp(argv[i], "pass=", 5)) strncpy(pass, &argv[i][5], sizeof(pass) - 1);
                else if (!strncmp(argv[i], "share=", 6)) strncpy(share, &argv[i][6], sizeof(share) - 1);
        }
}

int smb_connect(void)
{
        iop_sys_clock_t t;
        char unc[96];
        int rc;

        tcp.ip.pkt = tcp_buf;
        tcp.ip.pkt_size = BUF_SIZE;
        tcp.ip.saddr = htonl(ip);
        tcp.ip.daddr = htonl(srv);
        /* A new port each time, the server may still have the last one */
        GetSystemTime(&t);
        tcp.src_port = htons(40000 + (t.lo & 0x3fff));
        tcp.dst_port = htons(445);
        if (ps2smap_init(ip, mask, gw)) {
                return -1;
        }
        ps2smap_set_port(&tcp.src_port);

        if (tcp_connect(&tcp) < 0) {
                printf("smb: no connection to %d.%d.%d.%d\n", (int)(srv >> 24), (int)(srv >> 16) & 0xff, (int)(srv >> 8) & 0xff, (int)srv & 0xff);
                return -1;
        }
        usmb2_tcp = &tcp;
        smb = usmb2_init_context(0, user, pass);
        if (smb == NULL) {
                printf("smb: login as %s failed\n", user);
                return -1;
        }
        sprintf(unc, "\\\\%d.%d.%d.%d\\%s", (int)(srv >> 24), (int)(srv >> 16) & 0xff, (int)(srv >> 8) & 0xff, (int)srv & 0xff, share);
        rc = usmb2_treeconnect(smb, unc);
        if (rc) {
                printf("smb: tree connect to %s failed\n", unc);
                free(smb);
                smb = NULL;
                return -1;
        }
        return 0;
}

uint8_t *smb_open(const char *path)
{
        char name[256];
        int i;

        while (*path == '/' || *path == '\\') {
                path++;
        }
        for (i = 0; path[i] && i < sizeof(name) - 1; i++) {
                name[i] = path[i] == '/' ? '\\' : path[i];
        }
        name[i] = 0;
        return usmb2_open(smb, name, O_RDONLY);
}

int smb_pread(uint8_t *fid, void *buf, int count, uint64_t offset)
{
        int done, n, rc;

        for (done = 0; done < count; done += rc) {
                n = count - done > 2048 ? 2048 : count - done;
                rc = usmb2_pread(smb, fid, (uint8_t *)buf + done, n, offset + done);
                if (rc <= 0 || rc > n) {
                        return done ? done : -1;
                }
                if (rc < n) {
                        return done + rc;
                }
        }
        return done;
}
