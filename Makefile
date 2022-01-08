ISO_IMAGE = bin/disk.iso

.DEFAULT_GOAL := default

.PHONY: default
default:
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
	cp initrd.tar iso_root/
	xorriso -as mkisofs -b limine-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-eltorito-efi.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(ISO_IMAGE)
	limine/limine-install $(ISO_IMAGE)
	rm -rf iso_root

.PHONY: clean
clean:
	rm -f $(ISO_IMAGE)
	$(MAKE) -C kernel clean

.PHONY: distclean
distclean: clean
	rm -rf limine
	$(MAKE) -C kernel distclean
