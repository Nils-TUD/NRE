#!/bin/sh

dest=dist/imgs

if [ ! -f tools/disk.py ]; then
	echo "Please call this script from the nre subdirectory!" >&2
	exit 1
fi

# create_disk($target,$source)
create_disk() {
	dd if=/dev/zero of="$1" bs=512 count=4
	dd if="$2" of="$1" conv=notrunc
}

# create_iso($target,$source)
create_iso() {
	genisoimage -U -iso-level 3 -input-charset ascii -no-emul-boot -b \
		boot/grub/stage2_eltorito -boot-load-size 4 -boot-info-table -f -o "$1" "$2"
}

# build dummy disk files
create_disk $dest/hd1.img dist/hd1.hddcontent
create_disk $dest/hd2.img dist/hd2.hddcontent

# build an iso image
create_iso $dest/test.iso dist/iso

# build a 20MB disk with 2 ext3 partitions
tools/disk.py create $dest/hd3.img --part ext3 10 dist/iso --part ext2 12 -

