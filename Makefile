ISO_IMAGE = bin/disk.iso

.DEFAULT_GOAL := cleanbuild

.PHONY: cleanbuild
cleanbuild:
	make clean
	make all

.PHONY: all
all: $(ISO_IMAGE)

.PHONY: limine
limine:
	make -C limine

.PHONY: kernel
kernel:
	$(MAKE) -C kernel

$(ISO_IMAGE): limine kernel
	rm -rf iso_root
	mkdir -p iso_root
	cp kernel/bin/kernel.elf \
		limine.cfg limine/limine.sys limine/limine-cd.bin limine/limine-eltorito-efi.bin iso_root/
	cp ext2-ramdisk-image.ext2 iso_root/
	xorriso -as mkisofs -b limine-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-eltorito-efi.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(ISO_IMAGE)
	limine/limine-install $(ISO_IMAGE)
	rm -rf iso_root

.PHONY: ramdisk
ramdisk:
# Create mount point for ext2 ramdisk image
	sudo rm -rf ramdisk-mountpoint
	mkdir -p ramdisk-mountpoint
# Create ext2 ramdisk image
	dd if=/dev/zero of=ext2-ramdisk-image.ext2 bs=1024 count=8192
	mke2fs -F ext2-ramdisk-image.ext2
# Mount it on ramdisk-mountpoint/
	sudo mount -o loop ext2-ramdisk-image.ext2 ramdisk-mountpoint
# Copy sysroot/system-root/ contents into it
	sudo cp -r xbstrap-build/system-root/* ramdisk-mountpoint
# Copy root-directory/ contents into it
	sudo cp -r root-directory/* ramdisk-mountpoint
# Unmount
	sudo umount ramdisk-mountpoint

.PHONY: clean
clean:
	rm -f $(ISO_IMAGE)
	$(MAKE) -C kernel clean

.PHONY: distclean
distclean: clean
	rm -rf limine
