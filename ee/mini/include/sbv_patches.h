// Part of neutrino: the EE side without an SDK. See README.md.
#ifndef SBV_PATCHES_H
#define SBV_PATCHES_H

// Adds LoadModuleBuffer (LF_F_MOD_BUF_LOAD) to the ROM's LOADFILE
int sbv_patch_enable_lmb(void);

#endif
