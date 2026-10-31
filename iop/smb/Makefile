# usmb2 on the tiny TCP/IP stack, over smap.irx and ministack.irx
# https://github.com/sahlberg/usmb2
USMB2_DIR ?= usmb2
SMB_IOMAN ?= 0
SMB_FHI   ?= 0

IOP_CFLAGS  += -mno-check-zero-division -I../smap/include -I../ministack/include \
	-I$(USMB2_DIR) -I$(USMB2_DIR)/z80 -DRS232_TPS=100 \
	-DUSMB2_FEATURE_TINY_TCP -DUSMB2_FEATURE_REASSEMBLY -DUSMB2_FEATURE_NTLM

IOP_OBJS_COMMON = smb_conn.o ps2smap.o usmb2.o ntlm.o md4c.o md5.o hmac-md5.o \
	ps2iop-compat.o ip.o tcp.o imports.o

ifeq ($(SMB_IOMAN),1)
  IOP_CFLAGS += -DFEATURE_SMB_IOMAN
  IOP_OBJS = smb_ioman.o $(IOP_OBJS_COMMON)
  IOP_OBJS_DIR = obj.smb_ioman/
  IOP_BIN = smb_ioman.irx
else ifeq ($(SMB_FHI),1)
  IOP_CFLAGS += -DFEATURE_SMB_FHI
  IOP_OBJS = smb_fhi.o exports.o $(IOP_OBJS_COMMON)
  IOP_OBJS_DIR = obj.smb_fhi/
  IOP_BIN = smb_fhi.irx
endif

include ../Rules.make

$(IOP_OBJS_DIR)%.o: $(USMB2_DIR)/%.c
	$(DIR_GUARD)
	$(IOP_C_COMPILE) -c $< -o $@

$(IOP_OBJS_DIR)%.o: $(USMB2_DIR)/z80/%.c
	$(DIR_GUARD)
	$(IOP_C_COMPILE) -c $< -o $@

# usmb2's hand-written MIPS asm for its sources, about 3.4 KB smaller than the
# C (usmb2/mips/README). USMB2_ASM=0 builds the C instead.
USMB2_ASM ?= 1
ifeq ($(USMB2_ASM),1)
USMB2_ASM_OBJS = md5 md4c hmac-md5 ntlm ip tcp ps2smap usmb2 ps2iop-compat
$(addprefix $(IOP_OBJS_DIR),$(addsuffix .o,$(USMB2_ASM_OBJS))): $(IOP_OBJS_DIR)%.o: $(USMB2_DIR)/mips/%.S
	$(DIR_GUARD)
	$(IOP_C_COMPILE) -c $< -o $@
endif
