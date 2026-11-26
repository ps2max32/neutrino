// Part of neutrino: the EE side without an SDK. See README.md.
#ifndef KERNEL_H
#define KERNEL_H

#include <tamtypes.h>

#define UNCACHED_SEG(x) ((void *)((u32)(x) | 0x20000000))

// FlushCache
#define WRITEBACK_DCACHE  0
#define INVALIDATE_ICACHE 2

#define DMAC_SIF0 5

typedef struct
{
    int count;
    int max_count;
    int init_count;
    int wait_threads;
    u32 attr;
    u32 option;
} ee_sema_t;

// The BIOS's system calls
void LoadExecPS2(const char *filename, int argc, char **argv) __attribute__((noreturn));
int ExecPS2(void *entry, void *gp, int argc, char **argv);
int AddDmacHandler(int channel, int (*handler)(int channel), int next);
int RemoveDmacHandler(int channel, int id);
int _EnableDmac(int channel);
int _DisableDmac(int channel);
int GetThreadId(void);
int SleepThread(void);
int CreateSema(ee_sema_t *sema);
int DeleteSema(int id);
int iSignalSema(int id);
int WaitSema(int id);
void FlushCache(int operation);
void SetSyscall(int number, void *handler);
void _print(const char *fmt, ...);

// Interrupts: DIntr returns if they were enabled
int DIntr(void);
int EIntr(void);
int EnableDmac(int channel);
int DisableDmac(int channel);

// Write back and invalidate the data cache's lines: [start, end]
void SyncDCache(void *start, void *end);

// The handler of a system call, from the BIOS's table
void *GetSyscallHandler(int number);

static inline u32 ee_get_status(void)
{
    u32 status;
    __asm__ volatile("mfc0 %0, $12" : "=r"(status));
    return status;
}

static inline void ee_set_status(u32 status)
{
    __asm__ volatile("mtc0 %0, $12\n\tsync.p" : : "r"(status) : "memory");
}

// Kernel mode and back to user mode, with interrupts disabled
static inline void ee_kmode_enter(void) { ee_set_status(ee_get_status() & ~0x18); }
static inline void ee_kmode_exit(void) { ee_set_status(ee_get_status() | 0x10); }

static inline void nopdelay(void)
{
    for (int i = 0x100000; i > 0; i--)
        __asm__ volatile("nop\n\tnop\n\tnop\n\tnop\n\tnop");
}

#endif
