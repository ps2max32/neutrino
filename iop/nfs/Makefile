# unfs4 on the tiny TCP/IP stack, over smap.irx and ministack.irx
# https://github.com/sahlberg/unfs4
# The tiny stack and the IOP helpers come from usmb2, in ../smb.
UNFS4_DIR ?= unfs4
USMB2_DIR ?= ../smb/usmb2
NFS_IOMAN ?= 0
NFS_FHI   ?= 0

IOP_CFLAGS  += -mno-check-zero-division -I../smap/include -I../ministack/include \
	-I$(UNFS4_DIR) -I$(USMB2_DIR) -I$(USMB2_DIR)/z80 -DRS232_TPS=100 \
	-DUNFS4_FEATURE_TINY_TCP -DUNFS4_FEATURE_REASSEMBLY \
	-DUSMB2_FEATURE_TINY_TCP -DUNFS4_SIZE=512

IOP_OBJS_COMMON = nfs_conn.o ps2smap.o unfs4.o ps2iop-compat.o ip.o tcp.o imports.o

ifeq ($(NFS_IOMAN),1)
  IOP_CFLAGS += -DFEATURE_NFS_IOMAN
  IOP_OBJS = nfs_ioman.o $(IOP_OBJS_COMMON)
  IOP_OBJS_DIR = obj.nfs_ioman/
  IOP_BIN = nfs_ioman.irx
else ifeq ($(NFS_FHI),1)
  IOP_CFLAGS += -DFEATURE_NFS_FHI
  IOP_OBJS = nfs_fhi.o exports.o $(IOP_OBJS_COMMON)
  IOP_OBJS_DIR = obj.nfs_fhi/
  IOP_BIN = nfs_fhi.irx
endif

include ../Rules.make

$(IOP_OBJS_DIR)%.o: $(UNFS4_DIR)/%.c
	$(DIR_GUARD)
	$(IOP_C_COMPILE) -c $< -o $@

$(IOP_OBJS_DIR)%.o: $(USMB2_DIR)/%.c
	$(DIR_GUARD)
	$(IOP_C_COMPILE) -c $< -o $@

$(IOP_OBJS_DIR)%.o: $(USMB2_DIR)/z80/%.c
	$(DIR_GUARD)
	$(IOP_C_COMPILE) -c $< -o $@
