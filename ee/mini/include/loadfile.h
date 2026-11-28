// Part of neutrino: the EE side without an SDK. See README.md.
#ifndef LOADFILE_H
#define LOADFILE_H

#include <tamtypes.h>

#define LF_PATH_MAX 252
#define LF_ARG_MAX  252

enum {
    LF_F_MOD_LOAD     = 0,
    LF_F_ELF_LOAD     = 1,
    LF_F_MG_MOD_LOAD  = 4,
    LF_F_MOD_BUF_LOAD = 6, // from sbv_patch_enable_lmb or a newer LOADFILE
};

#define SCE_EBINDMISS 0x10000
#define SCE_ECALLMISS 0x10001
#define SCE_ELOADMISS 0x10003

typedef struct
{
    u32 epc;
    u32 gp;
    u32 sp;
    u32 dummy;
} t_ExecData;

// The arguments and results of LOADFILE, received by DMA: in cache lines of
// their own.
struct _lf_module_load_arg
{
    union
    {
        int arg_len;
        int result;
    } p;
    int modres;
    char path[LF_PATH_MAX];
    char args[LF_ARG_MAX];
} ALIGNED(64);

struct _lf_elf_load_arg
{
    u32 epc;
    u32 gp;
    char path[LF_PATH_MAX];
    char secname[LF_ARG_MAX];
} ALIGNED(64);

struct _lf_module_buffer_load_arg
{
    union
    {
        void *ptr;
        int result;
    } p;
    union
    {
        int arg_len;
        int modres;
    } q;
    char unused[LF_PATH_MAX];
    char args[LF_ARG_MAX];
} ALIGNED(64);

int SifLoadFileInit(void);
void SifLoadFileExit(void);
int _SifLoadModule(const char *path, int arg_len, const char *args, int *modres, int fno, int dontwait);
int SifLoadModule(const char *path, int arg_len, const char *args);
int SifLoadElf(const char *path, t_ExecData *data);
int SifExecModuleBuffer(void *ptr, u32 size, u32 arg_len, const char *args, int *mod_res);

#endif
