// Part of neutrino: the EE side without an SDK. See README.md.
// What picolibc leaves to the system: files, stdio, the heap and the end.
// As ps2sdk's libcglue (Academic Free License version 2.0): open() goes to
// the ROM's FILEIO, or to fileXio once fileXioInit() is done; relative paths
// are in the working directory, at the start the ELF's directory; stdout and
// stderr are the IOP's tty (FILEIO's descriptor 1).
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fileXio_rpc.h>
#include <fileio.h>
#include <io_common.h>
#include <kernel.h>
#include <ps2sdkapi.h>

#define FD_MAX   32
#define PATH_MAX_ 256

extern int fileXio_active;

static struct
{
    int used;
    int fxio; // 1: fileXio, 0: FILEIO
    int iop_fd;
} fds[FD_MAX] = {
    {1, 0, 0},
    {1, 0, 1},
    {1, 0, 2},
};

static char cwd[PATH_MAX_ + 1] = "host:";

//---------------------------------------------------------------------------
// Paths, as libcglue's __path_absolute

enum sep { SEP_NOT_FOUND, SEP_NONE, SEP_POSIX, SEP_WINDOWS };

// The length of "drive:" (spaces before it included), or 0 without one
static int get_drive(const char *dev, enum sep *sep, int max)
{
    const char *d, *t;
    int name_len;

    for (d = dev; (d - dev) < max && *d == ' '; d++)
        ;
    for (t = d; (t - dev) < max && *t && *t != ':'; t++)
        ;
    if (*t != ':') {
        *sep = SEP_NOT_FOUND;
        return 0;
    }

    // Without the unit number
    for (name_len = t - d; name_len > 0 && d[name_len - 1] >= '0' && d[name_len - 1] <= '9'; name_len--)
        ;

    *sep = SEP_POSIX;
    if (name_len == 3 && (!memcmp(d, "rom", 3) || !memcmp(d, "hdd", 3)))
        *sep = SEP_NONE;
    else if (name_len == 5 && !memcmp(d, "cdrom", 5))
        *sep = SEP_WINDOWS;
    else if (name_len == 6 && !memcmp(d, "usbkbd", 6))
        *sep = SEP_NONE;

    return (t - dev) + 1;
}

// Remove "." and "..", double separators and a trailing one
static int normalize(char *out, int len, int posix)
{
    char s     = posix ? '/' : '\\';
    int out_len = strnlen(out, len) + 2;
    int i, j, first, next;

    if (out_len >= len)
        return -1;

    out[out_len - 1] = s;
    out[out_len]     = 0;

    for (i = 0; i < out_len; i++) {
        if (out[i] == '/' || out[i] == '\\')
            out[i] = s;
    }

    for (i = 0; i < out_len && out[i + 1]; i++) {
        if (out[i] == s && out[i + 1] == s) {
            for (j = i + 1; out[j]; j++)
                out[j] = out[j + 1];
            i--;
        }
    }

    for (i = 0; i < out_len && out[i] && out[i + 1] && out[i + 2]; i++) {
        if (out[i] == s && out[i + 1] == '.' && out[i + 2] == s) {
            for (j = i + 1; out[j]; j++)
                out[j] = out[j + 2];
            i--;
        }
    }

    first = next = 0;
    while (first < out_len && next < out_len) {
        if (out[next + 1] && out[next + 1] == '.' && out[next + 2] && out[next + 2] == '.' && out[next + 3] &&
            out[next + 3] == s) {
            for (j = 0; out[first + j + 1]; j++)
                out[first + j + 1] = out[next + j + 4];
            first = next = 0;
            continue;
        }

        first = next;
        for (next = first + 1; out[next] && out[next] != s; next++)
            ;
        if (!out[next])
            break;
    }

    for (i = 1; i < out_len && out[i]; i++)
        ;
    if (i > 1 && out[i - 1] == s)
        out[i - 1] = 0;

    return 0;
}

static int path_absolute(const char *in, char *out, int len)
{
    int in_len = strlen(in);
    enum sep sep;
    int dr = get_drive(in, &sep, in_len);
    char s = sep == SEP_POSIX ? '/' : '\\';
    int out_len;

    if (sep != SEP_NOT_FOUND && (sep == SEP_NONE || in[dr] == s)) {
        // "drive:" without separators, or "drive:/": absolute
        out_len = snprintf(out, len, "%s", in);
    } else if (sep != SEP_NOT_FOUND && in[dr - 1] == ':') {
        // "drive:" without the "/" after it
        out_len = snprintf(out, len, "%.*s%c%s", dr, in, s, in + dr);
    } else if (in[0] == '\\' || in[0] == '/') {
        // Without "drive:": the working directory's
        out_len = snprintf(out, len, "%.*s%s", get_drive(cwd, &sep, strlen(cwd)), cwd, in);
    } else {
        out_len = snprintf(out, len, "%s%c%s", cwd, s, in);
    }
    if (out_len >= len)
        return -1;

    dr = get_drive(out, &sep, out_len);
    return normalize(out + dr, len - dr, sep == SEP_POSIX);
}

int chdir(const char *path)
{
    char dest[PATH_MAX_ + 1];

    if (path_absolute(path, dest, sizeof(dest)) < 0) {
        errno = ENAMETOOLONG;
        return -1;
    }
    strcpy(cwd, dest);
    return 0;
}

// crt0: the ELF's directory, or "host:"
void __init_cwd(int argc, char **argv)
{
    char *p, *s = NULL;

    if (argc == 0) {
        chdir("host:");
        return;
    }
    for (p = argv[0]; *p; p++) {
        if (*p == '/' || *p == '\\' || *p == ':')
            s = p;
    }
    if (s == NULL) {
        chdir("host:");
        return;
    }

    char backup = *(++s);
    *s          = 0;
    chdir(argv[0]);
    *s = backup;
}

//---------------------------------------------------------------------------
// Files

static int set_errno(int res)
{
    if (res >= 0)
        return res;
    errno = -res;
    return -1;
}

static int valid(int fd) { return fd >= 0 && fd < FD_MAX && fds[fd].used; }

int open(const char *path, int flags, ...)
{
    char full[PATH_MAX_ + 1];
    int iop_flags = (flags & 3) + 1; // O_RDONLY..O_RDWR to IOP_O_RDONLY..IOP_O_RDWR
    int fd, res;

    if (flags & O_NONBLOCK)
        iop_flags |= IOP_O_NBLOCK;
    if (flags & O_APPEND)
        iop_flags |= IOP_O_APPEND;
    if (flags & O_CREAT)
        iop_flags |= IOP_O_CREAT;
    if (flags & O_TRUNC)
        iop_flags |= IOP_O_TRUNC;
    if (flags & O_EXCL)
        iop_flags |= IOP_O_EXCL;

    if (path_absolute(path, full, sizeof(full)) < 0) {
        errno = ENAMETOOLONG;
        return -1;
    }

    for (fd = 3; fd < FD_MAX && fds[fd].used; fd++)
        ;
    if (fd == FD_MAX) {
        errno = ENFILE;
        return -1;
    }

    res = fileXio_active ? fileXioOpen(full, iop_flags, 0666) : fioOpen(full, iop_flags);
    if (res < 0)
        return set_errno(res);

    fds[fd].used   = 1;
    fds[fd].fxio   = fileXio_active;
    fds[fd].iop_fd = res;
    return fd;
}

int close(int fd)
{
    int res;

    if (!valid(fd) || fd < 3) {
        errno = EBADF;
        return -1;
    }
    res = fds[fd].fxio ? fileXioClose(fds[fd].iop_fd) : fioClose(fds[fd].iop_fd);
    fds[fd].used = 0;
    return set_errno(res);
}

ssize_t read(int fd, void *buf, size_t n)
{
    if (!valid(fd)) {
        errno = EBADF;
        return -1;
    }
    return set_errno(fds[fd].fxio ? fileXioRead(fds[fd].iop_fd, buf, n) : fioRead(fds[fd].iop_fd, buf, n));
}

ssize_t write(int fd, const void *buf, size_t n)
{
    if (!valid(fd)) {
        errno = EBADF;
        return -1;
    }
    return set_errno(fds[fd].fxio ? fileXioWrite(fds[fd].iop_fd, buf, n) : fioWrite(fds[fd].iop_fd, buf, n));
}

off_t lseek(int fd, off_t offset, int whence)
{
    if (!valid(fd)) {
        errno = EBADF;
        return -1;
    }
    return set_errno(fds[fd].fxio ? fileXioLseek(fds[fd].iop_fd, offset, whence)
                                  : fioLseek(fds[fd].iop_fd, offset, whence));
}

long long lseek64(int fd, long long offset, int whence)
{
    long long res;

    if (!valid(fd)) {
        errno = EBADF;
        return -1;
    }
    if (!fds[fd].fxio) // FILEIO has 32 bits only
        return lseek(fd, (off_t)offset, whence);

    res = fileXioLseek64(fds[fd].iop_fd, offset, whence);
    if (res < 0) {
        errno = (int)-res;
        return -1;
    }
    return res;
}

int ps2sdk_get_iop_fd(int fd)
{
    if (!valid(fd)) {
        errno = EBADF;
        return -1;
    }
    return fds[fd].iop_fd;
}

//---------------------------------------------------------------------------
// stdio: stdout and stderr by line to the tty, and fopen() for reading

static char line[256];
static unsigned line_len;

static int out_flush(FILE *f)
{
    (void)f;
    if (line_len)
        fioWrite(1, line, line_len);
    line_len = 0;
    return 0;
}

static int out_put(char c, FILE *f)
{
    line[line_len++] = c;
    if (c == '\n' || line_len == sizeof(line))
        out_flush(f);
    return (unsigned char)c;
}

static FILE out = FDEV_SETUP_STREAM(out_put, NULL, out_flush, _FDEV_SETUP_WRITE);
FILE *const stdin  = &out;
FILE *const stdout = &out;
FILE *const stderr = &out;

struct rfile
{
    FILE file; // first: a FILE * is one of these
    int fd;
    int pos;
    int len;
    char buf[512];
};

static int file_get(FILE *f)
{
    struct rfile *r = (struct rfile *)f;

    if (r->pos == r->len) {
        int n = read(r->fd, r->buf, sizeof(r->buf));
        if (n <= 0)
            return n == 0 ? _FDEV_EOF : _FDEV_ERR;
        r->len = n;
        r->pos = 0;
    }
    return (unsigned char)r->buf[r->pos++];
}

FILE *fopen(const char *path, const char *mode)
{
    struct rfile *r;
    int fd;

    if (strcmp(mode, "r") && strcmp(mode, "rb")) {
        errno = EINVAL;
        return NULL;
    }
    if ((fd = open(path, O_RDONLY)) < 0)
        return NULL;
    if ((r = calloc(1, sizeof(*r))) == NULL) {
        close(fd);
        errno = ENOMEM;
        return NULL;
    }

    r->file.get   = file_get;
    r->file.flags = __SRD;
    r->fd         = fd;
    return &r->file;
}

int fclose(FILE *f)
{
    struct rfile *r = (struct rfile *)f;
    int res         = close(r->fd);

    free(r);
    return res;
}

//---------------------------------------------------------------------------
// The heap: from the end of the program to the stack (crt0)

extern char _end[];
extern char _heap_end[];

void *sbrk(ptrdiff_t incr)
{
    static char *brk = _end;
    char *prev       = brk;

    if (incr > _heap_end - brk) {
        errno = ENOMEM;
        return (void *)-1;
    }
    brk += incr;
    return prev;
}

void _exit(int status)
{
    out_flush(&out);
    __asm__ volatile("move $4, %0\n\tli $3, 4\n\tsyscall" : : "r"(status) : "$3", "$4", "memory"); // Exit
    for (;;)
        ;
}
