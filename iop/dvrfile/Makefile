DVRFILE_FS  ?= 0
DVRFILE_FHI ?= 0

ifeq ($(DVRFILE_FS),1)
  IOP_OBJS     = dvrfile.o imports.o
  IOP_OBJS_DIR = obj.dvrfile/
  IOP_BIN      = dvrfile.irx
  IOP_CFLAGS  += -DDVRFILE_FS -DIOMANX_OLD_NAME_COMPATIBILITY=0 -DIOMANX_OLD_NAME_ADDDELDRV=0
else ifeq ($(DVRFILE_FHI),1)
  IOP_OBJS     = dvrfile_fhi.o exports.o imports.o
  IOP_OBJS_DIR = obj.dvrfile_fhi/
  IOP_BIN      = dvrfile_fhi.irx
  IOP_CFLAGS  += -DDVRFILE_FHI
endif

include ../Rules.make
