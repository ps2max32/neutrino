EESIO_DEBUG?=0
IOPCORE_DEBUG?=0

PS2MAX ?= $(abspath $(CURDIR)/../..)
OUT    ?= $(PS2MAX)/build/apps/neutrino
# ps2sdk's prebuilt IOP modules
PS2MAX_PS2DEV ?= $(abspath $(PS2MAX)/../ps2dev/install)
PS2MAX_IRX    ?= $(PS2MAX_PS2DEV)/ps2sdk/iop/irx

all: iop ee

# The IOP modules, with LLVM (iop/Rules.make): $(OUT)/irx/*.irx
IOP_MODULES = atad_emu bdfs cdvdfsv cdvdman_emu cdvdman_esr1 cdvdman_esr2 fakemod fhi_bd gapfill hdlfs \
              imgdrv mc_emu ministack patch_freemem patch_membo patch_rc_uya smap udpbd usbd_null
IOP_MAKE    = $(MAKE) OUT=$(OUT) DEBUG=$(IOPCORE_DEBUG)

iop: $(IOP_MODULES:%=iop-%) iop-memcheck iop-dev9 iop-udpfs iop-smb iop-nfs iop-mmce

$(IOP_MODULES:%=iop-%): iop-%:
	$(IOP_MAKE) -C iop/$*
iop-memcheck:
	$(IOP_MAKE) -C iop/memcheck DEBUG=1
iop-dev9:
	$(IOP_MAKE) -C iop/dev9 DEV9_HIDDEN=1
	$(IOP_MAKE) -C iop/dev9 DEV9_NO_SHUTDOWN=1
iop-udpfs:
	$(IOP_MAKE) -C iop/udpfs UDPFS_BD=1
	$(IOP_MAKE) -C iop/udpfs UDPFS_IOMAN=1
	$(IOP_MAKE) -C iop/udpfs UDPFS_FHI=1
iop-smb:
	$(IOP_MAKE) -C iop/smb SMB_IOMAN=1
	$(IOP_MAKE) -C iop/smb SMB_FHI=1
iop-nfs:
	$(IOP_MAKE) -C iop/nfs NFS_IOMAN=1
	$(IOP_MAKE) -C iop/nfs NFS_FHI=1
iop-mmce:
	$(IOP_MAKE) -C iop/mmce
	$(IOP_MAKE) -C iop/mmce MMCE_FHI=1

# The EE side, with LLVM on ee/mini: ee_core.elf, neutrino.elf
ee:
	$(MAKE) -C ee/ee_core all OUT=$(OUT) EESIO_DEBUG=$(EESIO_DEBUG)
	$(MAKE) -C ee/loader  all OUT=$(OUT) DEBUG=0

clean:
	rm -rf $(OUT)

.PHONY: all iop ee clean copy copy_extra $(IOP_MODULES:%=iop-%) iop-memcheck iop-dev9 iop-udpfs iop-smb iop-nfs iop-mmce

copy:
	rm -rf ee/loader/modules
	mkdir -p ee/loader/modules
	cp $(OUT)/irx/atad_emu.irx           ee/loader/modules
	cp $(OUT)/irx/bdfs.irx                   ee/loader/modules
	cp $(OUT)/irx/cdvdfsv.irx             ee/loader/modules
	cp $(OUT)/irx/cdvdman_emu.irx     ee/loader/modules
	cp $(OUT)/irx/cdvdman_esr1.irx   ee/loader/modules
	cp $(OUT)/irx/cdvdman_esr2.irx   ee/loader/modules
	cp $(OUT)/irx/dev9_hidden.irx            ee/loader/modules
	cp $(OUT)/irx/dev9_ns.irx                ee/loader/modules
	cp $(OUT)/irx/fakemod.irx             ee/loader/modules
	cp $(OUT)/irx/fhi_bd.irx               ee/loader/modules
	cp $(OUT)/irx/hdlfs.irx                 ee/loader/modules
	cp $(OUT)/irx/imgdrv.irx               ee/loader/modules
	cp $(OUT)/irx/mc_emu.irx               ee/loader/modules
	cp $(OUT)/irx/patch_freemem.irx ee/loader/modules
	cp $(OUT)/irx/patch_membo.irx     ee/loader/modules
	cp $(OUT)/irx/patch_rc_uya.irx   ee/loader/modules
	cp $(OUT)/irx/udpbd.irx                 ee/loader/modules
	cp $(OUT)/irx/smap.irx                   ee/loader/modules
	cp $(OUT)/irx/ministack.irx         ee/loader/modules
	cp $(OUT)/irx/udpfs_bd.irx              ee/loader/modules
	cp $(OUT)/irx/udpfs_ioman.irx           ee/loader/modules
	cp $(OUT)/irx/udpfs_fhi.irx             ee/loader/modules
	cp $(OUT)/irx/smb_ioman.irx               ee/loader/modules
	cp $(OUT)/irx/smb_fhi.irx                 ee/loader/modules
	cp $(OUT)/irx/nfs_ioman.irx               ee/loader/modules
	cp $(OUT)/irx/nfs_fhi.irx                 ee/loader/modules
	cp $(OUT)/ee_core.elf                  ee/loader/modules
	cp $(OUT)/neutrino.elf $(OUT)/version.txt ee/loader
	cp $(PS2MAX_IRX)/udnl.irx              ee/loader/modules
	cp $(PS2MAX_IRX)/udnl-t300.irx         ee/loader/modules
	cp $(PS2MAX_IRX)/eesync.irx            ee/loader/modules
	cp $(PS2MAX_IRX)/iomanX.irx            ee/loader/modules
	cp $(PS2MAX_IRX)/fileXio.irx           ee/loader/modules
	cp $(PS2MAX_IRX)/bdm.irx               ee/loader/modules
	cp $(PS2MAX_IRX)/bdmfs_fatfs.irx       ee/loader/modules
	cp $(PS2MAX_IRX)/ata_bd.irx            ee/loader/modules
	cp $(PS2MAX_IRX)/ps2hdd-bdm.irx        ee/loader/modules
	cp $(PS2MAX_IRX)/usbd_mini.irx         ee/loader/modules
	cp $(PS2MAX_IRX)/usbmass_bd_mini.irx   ee/loader/modules
	cp $(PS2MAX_IRX)/mx4sio_bd_mini.irx    ee/loader/modules
	cp $(PS2MAX_IRX)/iLinkman.irx          ee/loader/modules
	cp $(PS2MAX_IRX)/IEEE1394_bd_mini.irx  ee/loader/modules
	cp $(OUT)/irx/mmceman.irx                 ee/loader/modules
	cp $(OUT)/irx/mmcefhi.irx                 ee/loader/modules

copy_extra:
	cp $(OUT)/irx/gapfill.irx             ee/loader/modules
	cp $(OUT)/irx/memcheck.irx           ee/loader/modules
	cp $(OUT)/irx/usbd_null.irx         ee/loader/modules

format:
	find . -type f -a \( -iname \*.h -o -iname \*.c \) | xargs clang-format -i

# Start on PS2 (ps2link/ps2client)
run: all copy copy_extra
	$(MAKE) -C ee/loader run

# Start on PS2 (ps2link/ps2client), using mmce device and quickboot
run_mmce_qb: all copy copy_extra
	$(MAKE) -C ee/loader run_mmce_qb

# Copy neutrino to UDPBD shared drive, then run nhddl
PS2_IP ?= 192.168.1.10
UDPBD_BD = /dev/zd0p1
run_nhddl: all copy copy_extra
	mkdir -p temp
	sudo mount $(UDPBD_BD) temp
	sudo cp    README.md              temp
	sudo cp -R ee/loader/config       temp
	sudo cp -R ee/loader/modules      temp
	sudo cp    ee/loader/neutrino.elf temp
	sudo cp    ee/loader/version.txt  temp
	sudo umount $(UDPBD_BD)
	rmdir temp
	ps2client -h $(PS2_IP) execee host:nhddl.elf

# Start on PCSX2
sim: all copy copy_extra
	$(MAKE) -C ee/loader sim

# Mount first partition of block device used in PCSX2 testing (ATA or USB)
sim_mount:
	losetup -Pf ee/loader/bd_exfat.raw

RELEASE_DIR = ./releases/neutrino_$(shell git describe --tags --exclude=latest --always)
release: all copy
	mkdir -p                     $(RELEASE_DIR)
	cp    README.md              $(RELEASE_DIR)
	cp -R ee/loader/config       $(RELEASE_DIR)
	rm -f $(RELEASE_DIR)/config/pcsx2*
	cp -R ee/loader/modules      $(RELEASE_DIR)
	cp    ee/loader/neutrino.elf $(RELEASE_DIR)
	cp    ee/loader/version.txt  $(RELEASE_DIR)
	mkdir -p                     $(RELEASE_DIR)/udpfs_server/compressed_iso
	cp    pc/*.py                $(RELEASE_DIR)/udpfs_server
	cp    pc/compressed_iso/*.py $(RELEASE_DIR)/udpfs_server/compressed_iso
	7z a -t7z $(RELEASE_DIR).7z $(RELEASE_DIR)/*
