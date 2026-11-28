// Part of neutrino: the EE side without an SDK. See README.md.
#ifndef EE_COP0_DEFS_H
#define EE_COP0_DEFS_H

#define EE_COP0_Status   $12
#define EE_COP0_Cause    $13
#define EE_COP0_ErrorEPC $30

// Cause: the level 2 exception
#define EE_CAUSE_BD2 (1 << 30)
#define EE_EXC2_DBG  3
#define M_EE_GET_CAUSE_EXC2(cause) (((cause) >> 16) & 7)

// Breakpoint control register
#define EE_BPC_DRE (1 << 30) // data read
#define EE_BPC_DWE (1 << 29) // data write
#define EE_BPC_DUE (1 << 21) // data, user mode
#define EE_BPC_DKE (1 << 19) // data, kernel mode
#define EE_BPC_BED (1 << 15) // breakpoint exceptions disabled

#endif
