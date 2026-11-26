# The MMCE modules, from ps2-mmce's mmceman (the submodule mmceman, a branch
# with mmcefhi, neutrino's file handle interface):
#   make            mmceman.irx (the full driver, for the loader)
#   make MMCE_FHI=1 mmcefhi.irx (the FHI, for neutrino's emulation)
MMCE_DIR ?= mmceman
MMCE_FHI ?= 0

IOP_SRC_DIR = $(MMCE_DIR)/mmceman/src/
# its own sources, whose warnings are not ours
IOP_CFLAGS += -I$(IOP_SRC_DIR) -I$(MMCE_DIR)/mmceman/include -Wno-pointer-sign

ifeq ($(MMCE_FHI),1)
  IOP_CFLAGS += -DMMCEDRV -DFHI
  IOP_OBJS = ioplib.o sio2man_hook.o mmce_sio2.o exports.o imports.o mmcedrv.o mmce_fhi.o
  IOP_BIN = mmcefhi.irx
else
  IOP_CFLAGS += -DIOMANX_OLD_NAME_COMPATIBILITY=0 -DIOMANX_OLD_NAME_ADDDELDRV=0
  IOP_OBJS = ioplib.o sio2man_hook.o mmce_sio2.o mmce_cmds.o mmce_fs.o imports.o mmceman.o
  IOP_BIN = mmceman.irx
endif

include ../Rules.make
