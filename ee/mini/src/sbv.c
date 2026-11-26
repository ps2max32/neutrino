// Part of neutrino: the EE side without an SDK. See README.md.
// Adds LoadModuleBuffer to the ROM's LOADFILE, as ps2sdk's sbv library
// (Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>, Academic Free
// License version 2.0): LOADCORE's lists and LOADFILE's code are read by SIF
// and changed by SIF DMA.
#include <iopheap.h>
#include <kernel.h>
#include <sbv_patches.h>
#include <sifrpc.h>

#define JAL(addr)  (0x0c000000 | (0x3ffffff & ((addr) >> 2)))
#define HI16(addr) (0x3c110000 | (((addr) >> 16) & 0xffff)) // lui $s1
#define LO16(addr) (0x36310000 | ((addr)&0xffff))           // ori $s1, $s1

// IOP
typedef struct
{
    u32 next;
    u32 name;
    u16 version;
    u16 newflags;
    u16 id;
    u16 unused;
    u32 entry;
    u32 gp;
    u32 text_start;
    u32 text_size;
    u32 data_size;
    u32 bss_size;
} mod_info_t;

typedef struct
{
    u32 prev;
    u32 caller;
    u16 version;
    u16 flags;
    char name[8];
    u32 exports[];
} exp_lib_t;

#define BUF_SIZE 0x300 // CDVDMAN's exports too

static union
{
    u8 bytes[BUF_SIZE];
    u32 words[BUF_SIZE / 4];
    mod_info_t mod;
    exp_lib_t lib;
} buf ALIGNED(64);

// The handler of LOADFILE's call 6. At 96: the result (id, unused, modres).
static const u32 lmb_code[] = {
    0x27bdffd8, //   addiu  $sp, -40
    0xafb00018, //   sw     $s0, 0x18($sp)
    0xafbf0020, //   sw     $ra, 0x20($sp)
    0x00808021, //   move   $s0, $a0
    0x8c840000, //   lw     $a0, 0($a0)
    0x0c000000, //   jal    [LoadModuleBuffer]
    0xafb1001c, //    sw    $s1, 0x1c($sp)
    0x3c110000, //   lui    $s1, [HI16(result)]
    0x04400008, //   bltz   $v0, 1f
    0x36310000, //    ori   $s1, [LO16(result)]
    0x00402021, //   move   $a0, $v0
    0x26250008, //   addiu  $a1, $s1, 8
    0x8e060004, //   lw     $a2, 4($s0)
    0x26070104, //   addiu  $a3, $s0, 0x104
    0x26280004, //   addiu  $t0, $s1, 4
    0x0c000000, //   jal    [StartModule]
    0xafa80010, //    sw    $t0, 0x10($sp)
    0xae220000, // 1:sw     $v0, 0($s1)
    0x02201021, //   move   $v0, $s1
    0x8fbf0020, //   lw     $ra, 0x20($sp)
    0x8fb1001c, //   lw     $s1, 0x1c($sp)
    0x8fb00018, //   lw     $s0, 0x18($sp)
    0x03e00008, //   jr     $ra
    0x27bd0028, //    addiu $sp, 40
    0x00000000, 0x00000000,
    0x7962424c, 0x00004545, // "LBbyEE"
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

static int iop_read(u32 addr, u32 size)
{
    SifRpcReceiveData_t rd;

    SifWriteBackDCache(&buf, sizeof(buf));
    return SifRpcGetOtherData(&rd, (void *)addr, &buf, size, 0);
}

// src: 64-byte aligned, written back
static void iop_write(u32 addr, void *src, u32 size)
{
    SifDmaTransfer_t dmat;
    u32 id;

    dmat.src  = src;
    dmat.dest = (void *)addr;
    dmat.size = size;
    dmat.attr = 0;
    do {
        id = SifSetDma(&dmat, 1);
    } while (!id);
    while (SifDmaStat(id) >= 0)
        ;
}

static int iop_write_word(u32 addr, u32 value)
{
    if (iop_read(addr & ~63, 64) < 0)
        return -1;
    buf.words[(addr & 63) / 4] = value;
    SifWriteBackDCache(&buf, 64);
    iop_write(addr & ~63, &buf, 64);
    return 0;
}

static int same(const void *a, const char *b, int n)
{
    const char *p = a;

    while (n--) {
        if (*p++ != *b++)
            return 0;
    }
    return 1;
}

// The last export library: LOADCORE's function 3 (GetLoadcoreInternalData)
// is lui, addiu of the list's address
static u32 last_library(void)
{
    u32 *w = buf.words;

    if (iop_read(0x800, sizeof(mod_info_t)) < 0)
        return 0;
    if (iop_read(buf.mod.next, sizeof(mod_info_t)) < 0)
        return 0;

    // LOADCORE's library is at the end of its text. SYSMEM's, at 0x830, is
    // before it in the list.
    if (iop_read(buf.mod.text_start + buf.mod.text_size - 512, 512) < 0)
        return 0;
    int i;
    for (i = 0; i < (512 - 36) / 4; i++) {
        if (w[i] == 0x830 && same(&w[i + 3], "loadcore", 8))
            break;
    }
    if (i == (512 - 36) / 4)
        return 0;

    if (iop_read(w[i + 5 + 3], 8) < 0)
        return 0;
    if ((w[0] & 0xffff0000) != 0x3c020000 || (w[1] & 0xffff0000) != 0x24420000)
        return 0;
    if (iop_read((w[0] << 16) + (s16)w[1], 8) < 0)
        return 0;

    return w[0];
}

int sbv_patch_enable_lmb(void)
{
    static u32 code[sizeof(lmb_code) / 4] ALIGNED(64);
    u32 start_module, load_module_buffer;
    u32 addr;

    // MODLOAD's exports 8 and 10
    for (addr = last_library(); addr != 0; addr = buf.lib.prev) {
        if (iop_read(addr, BUF_SIZE) < 0)
            return -1;
        if (same(buf.lib.name, "modload", 8))
            break;
    }
    if (addr == 0)
        return -1;
    for (int i = 0; i < 16; i++) {
        if (buf.lib.exports[i] == 0)
            return -1;
    }
    start_module       = buf.lib.exports[8];
    load_module_buffer = buf.lib.exports[10];

    // LOADFILE. A module's record, then its name.
    mod_info_t mod;
    for (addr = 0x800; addr != 0; addr = mod.next) {
        if (iop_read(addr, sizeof(mod_info_t)) < 0)
            return -1;
        mod = buf.mod;
        if (mod.name != 0 && iop_read(mod.name, 64) >= 0 && same(&buf, "LoadModuleByEE", 14))
            break;
    }
    if (addr == 0 || mod.text_size < 0x4c4 + 128)
        return -1;

    // The dispatcher of its calls is at the same place in all ROMs
    u32 dispatch = mod.text_start + 0x4c4;
    u32 *w       = buf.words;
    if (iop_read(dispatch, 128) < 0)
        return -1;
    if (w[0] != 0x27bdffe8 || w[1] != 0x2c820006 || w[2] != 0x14400003 || w[3] != 0xafbf0010 || w[5] != 0x00001021 ||
        w[6] != 0x00041080)
        return 1;
    u32 jump_table = (w[7] << 16) + (s16)w[9];

    addr = (u32)SifAllocIopHeap(sizeof(code));
    if (addr == 0)
        return -1;
    for (u32 i = 0; i < sizeof(code) / 4; i++)
        code[i] = lmb_code[i];
    code[5]  = JAL(load_module_buffer);
    code[7]  = HI16(addr + 96);
    code[9]  = LO16(addr + 96);
    code[15] = JAL(start_module);
    SifWriteBackDCache(code, sizeof(code));
    iop_write(addr, code, sizeof(code));

    // Entry 6 of the jump table, and then that there are 7 of them
    if (iop_write_word(jump_table + 6 * 4, addr) < 0)
        return -1;
    return iop_write_word(dispatch + 4, 0x2c820007);
}
