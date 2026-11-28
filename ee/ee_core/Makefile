# Copyright 2009-2010, Ifcaro, jimmikaelkael & Polo
# Copyright 2006-2008 Polo
# Licenced under Academic Free License version 3.0
# Review OPNPS2LD README & LICENSE files for further details.
#
# ee_core.elf, with LLVM and ../mini (no SDK):
#
#   make [EESIO_DEBUG=1] [OUT=<dir>]    <dir>/ee_core.elf

PS2MAX ?= $(abspath $(CURDIR)/../../../..)
OUT    ?= $(PS2MAX)/build/apps/neutrino
PS2MAX_LLVM_BIN ?= $(PS2MAX)/build/toolchain/llvm/feature/bin

EE_BIN      = $(OUT)/ee_core.elf
EE_OBJS_DIR = $(OUT)/obj/ee_core$(if $(filter 1,$(EESIO_DEBUG)),.debug)

OBJS = crt0.o main.o iopmgr.o loadfile.o util.o patches.o patches_asm.o asm.o ps2logo.o \
       gsm_api.o ee_exception_l2.o cheat_engine.o cheat_api.o
MINI_OBJS = syscalls.o kernel.o string.o sifrpc.o iopheap.o sbv.o

EE_CC     = $(PS2MAX_LLVM_BIN)/clang
EE_LD     = $(PS2MAX_LLVM_BIN)/ld.lld
EE_CFLAGS = --target=mips64el -mcpu=r5900 -mabi=n32 -fno-pic -mno-abicalls -G0 -std=gnu17 -Wno-c23-extensions -ffreestanding \
            -fno-builtin -fno-strict-aliasing -Os -Wall -Werror -ffunction-sections -fdata-sections -MMD -MP \
            -D_EE -I. -Iinclude -I../../common/include -I../mini/include -I../mini/include/libc

ifeq ($(EESIO_DEBUG),1)
EE_CFLAGS += -D__EESIO_DEBUG
endif

ifeq ($(DTL_T10000),1)
	EE_CFLAGS += -D_DTL_T10000
endif

ifeq ($(LOAD_DEBUG_MODULES),1)
EE_CFLAGS += -D__LOAD_DEBUG_MODULES
	ifeq ($(DECI2_DEBUG),1)
	EE_CFLAGS += -D__DECI2_DEBUG
	endif
endif

EE_OBJS = $(OBJS:%=$(EE_OBJS_DIR)/%) $(MINI_OBJS:%=$(EE_OBJS_DIR)/mini/%)

all: $(EE_BIN)

$(EE_OBJS): Makefile

$(EE_OBJS_DIR)/mini/%.o: ../mini/src/%.c
	@mkdir -p $(@D)
	$(EE_CC) $(EE_CFLAGS) -c $< -o $@

$(EE_OBJS_DIR)/mini/%.o: ../mini/src/%.S
	@mkdir -p $(@D)
	$(EE_CC) $(EE_CFLAGS) -c $< -o $@

$(EE_OBJS_DIR)/%.o: src/%.c
	@mkdir -p $(@D)
	$(EE_CC) $(EE_CFLAGS) -c $< -o $@

$(EE_OBJS_DIR)/%.o: src/%.S
	@mkdir -p $(@D)
	$(EE_CC) $(EE_CFLAGS) -c $< -o $@

# $(EE_OBJS_DIR)/ee_core.elf keeps the symbols
$(EE_BIN): $(EE_OBJS) linkfile
	@mkdir -p $(@D)
	$(EE_LD) -m elf32ltsmipn32 -Tlinkfile -z max-page-size=128 --gc-sections -Map $(EE_OBJS_DIR)/ee_core.map \
	  -o $(EE_OBJS_DIR)/ee_core.elf $(EE_OBJS)
	$(PS2MAX_LLVM_BIN)/llvm-objcopy --strip-all $(EE_OBJS_DIR)/ee_core.elf $@

clean:
	rm -rf $(EE_OBJS_DIR) $(EE_BIN)

-include $(EE_OBJS:.o=.d)

.PHONY: all clean
