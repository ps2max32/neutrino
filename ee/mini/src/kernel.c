// Part of neutrino: the EE side without an SDK. See README.md.
// Interrupts, the data cache, the BIOS's table of system calls, IOP memory
#include <kernel.h>
#include <sifdma.h>
#include <smem.h>

#define STATUS_EIE 0x10000

int DIntr(void)
{
    if (!(ee_get_status() & STATUS_EIE))
        return 0;

    // di does not always take effect: try again
    do {
        __asm__ volatile("di\n\tsync.p" : : : "memory");
    } while (ee_get_status() & STATUS_EIE);

    return 1;
}

int EIntr(void)
{
    int enabled = (ee_get_status() & STATUS_EIE) != 0;

    __asm__ volatile("ei" : : : "memory");

    return enabled;
}

static int with_intr_disabled(int (*syscall)(int), int channel)
{
    int enabled = DIntr();
    int res     = syscall(channel);

    __asm__ volatile("sync.l");
    if (enabled)
        EIntr();

    return res;
}

int EnableDmac(int channel) { return with_intr_disabled(_EnableDmac, channel); }
int DisableDmac(int channel) { return with_intr_disabled(_DisableDmac, channel); }

void SifWriteBackDCache(void *ptr, int size)
{
    u32 line = (u32)ptr & ~63;
    u32 end  = (u32)ptr + size;

    if (size <= 0)
        return;

    for (; line < end; line += 64) // DHWBIN: hit, write back, invalidate
        __asm__ volatile("sync.l\n\tcache 0x18, 0(%0)\n\tsync.l" : : "r"(line) : "memory");
}

void SyncDCache(void *start, void *end) { SifWriteBackDCache(start, (u32)end - (u32)start + 1); }

void *GetSyscallHandler(int number)
{
    u32 *code = (u32 *)0x800002f0; // the kernel's system call exception: lui, (other), lw with the table's address
    int enabled = DIntr();
    u32 mode    = ee_get_status() & 0x18;

    ee_kmode_enter();
    u32 *table    = (u32 *)((code[0] << 16) | (code[2] & 0xFFFF));
    void *handler = (void *)table[number];
    ee_set_status(ee_get_status() | mode);

    if (enabled)
        EIntr();

    return handler;
}

u32 smem_write(void *addr, void *buf, u32 size)
{
    vu32 *dst    = (vu32 *)((u32)addr + 0xbc000000); // IOP memory, uncached
    u32 *src     = buf;
    int enabled  = DIntr();

    ee_kmode_enter();
    for (u32 i = 0; i < size / 4; i++)
        dst[i] = src[i];
    ee_kmode_exit();

    if (enabled)
        EIntr();

    return size;
}
