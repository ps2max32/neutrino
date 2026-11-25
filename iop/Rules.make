# Building an IOP module with LLVM: clang for mipsel-none-irx and ld.lld --irx,
# set up by maxsdk's iop.mk. Each module's Makefile sets
#
#   IOP_OBJS     its objects, as ps2sdk's; imports.o stands for src/imports.lst,
#                exports.o for src/exports.tab (irxtab.py)
#   IOP_BIN      the .irx, if not called as the directory
#   IOP_CFLAGS   anything more
#
#   make [DEBUG=1] [OUT=<dir>]    <dir>/irx/<module>.irx
#
# PS2MAX is the ps2max tree: its LLVM and maxsdk, and ps2sdk's IOP headers.

NEUTRINO_IOP := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
PS2MAX ?= $(abspath $(NEUTRINO_IOP)/../../..)
OUT    ?= $(PS2MAX)/build/apps/neutrino
MAXSDK ?= $(PS2MAX)/libs/maxsdk

IOP_SRC_DIR ?= src/
IOP_BIN     ?= $(notdir $(CURDIR)).irx

# maxsdk's IOP_CFLAGS replace what is there
MODULE_CFLAGS := $(IOP_CFLAGS)
include $(MAXSDK)/iop/iop.mk

# neutrino's headers first: some have the same name as ps2sdk's
IOP_CFLAGS := -D_IOP $(filter-out -std=%,$(IOP_CFLAGS)) -std=gnu17 -Wno-c23-extensions -Os $(if $(filter 1,$(DEBUG)),-DDEBUG) \
              -Iinclude -Isrc -I../common -I../../common/include $(MODULE_CFLAGS) $(IOP_INCS)
IOP_C_COMPILE = $(IOP_CC) $(IOP_CFLAGS) -MMD -MP

# a directory of its own for each variant of a module
IOP_IRX       = $(OUT)/irx/$(IOP_BIN)
IOP_OBJS_DIR := $(OUT)/obj/$(IOP_BIN:.irx=)$(if $(filter 1,$(DEBUG)),.debug)/
DIR_GUARD     = @mkdir -p $(@D)

IOP_TABLES  = $(if $(filter exports.o,$(IOP_OBJS)),exports_tab.o exports_c.o)
# the export table first, at the start of .text
IOP_LINK   := $(addprefix $(IOP_OBJS_DIR),$(IOP_TABLES) $(filter-out imports.o exports.o,$(IOP_OBJS)))
IOP_ILB     = $(if $(filter imports.o,$(IOP_OBJS)),$(IOP_OBJS_DIR)imports.ilb)
BUILD_DEPS  = Makefile $(NEUTRINO_IOP)/Rules.make $(MAXSDK)/iop/iop.mk

all: $(IOP_IRX)

$(IOP_LINK): $(BUILD_DEPS)

$(IOP_OBJS_DIR)%.o: $(IOP_SRC_DIR)%.c
	$(DIR_GUARD)
	$(IOP_C_COMPILE) -c $< -o $@

# the headers too: they say the libraries' versions and entries
$(IOP_OBJS_DIR)imports.ilb: $(IOP_SRC_DIR)imports.lst $(NEUTRINO_IOP)/irxtab.py \
                            $(wildcard $(IOP_SRC_DIR)*.h include/*.h $(NEUTRINO_IOP)/*/include/*.h) $(BUILD_DEPS)
	$(DIR_GUARD)
	python3 $(NEUTRINO_IOP)/irxtab.py import $@ $(IOP_CC) $(IOP_CFLAGS)

$(IOP_OBJS_DIR)exports_tab.S: $(IOP_SRC_DIR)exports.tab $(NEUTRINO_IOP)/irxtab.py $(BUILD_DEPS)
	$(DIR_GUARD)
	python3 $(NEUTRINO_IOP)/irxtab.py export $@ $(@D)/exports_c.c $(IOP_CC) $(IOP_CFLAGS)

$(IOP_OBJS_DIR)exports_c.c: $(IOP_OBJS_DIR)exports_tab.S

$(IOP_OBJS_DIR)exports_tab.o: $(IOP_OBJS_DIR)exports_tab.S
	$(IOP_CC) -c $< -o $@

$(IOP_OBJS_DIR)exports_c.o: $(IOP_OBJS_DIR)exports_c.c
	$(IOP_C_COMPILE) -c $< -o $@

# $(IOP_OBJS_DIR)module.irx keeps the symbols, for looking up addresses
$(IOP_IRX): $(IOP_LINK) $(IOP_ILB)
	@mkdir -p $(@D)
	$(IOP_CC) -o $(IOP_OBJS_DIR)module.irx $^ -Wl,--gc-sections $(IOP_LDFLAGS)
	$(IOP_CC) -s -o $@ $^ -Wl,--gc-sections $(IOP_LDFLAGS)

clean:
	rm -rf $(IOP_OBJS_DIR) $(IOP_IRX)

-include $(IOP_LINK:.o=.d)

.PHONY: all clean
.SECONDARY:
