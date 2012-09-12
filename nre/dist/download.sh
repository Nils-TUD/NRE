#!/bin/sh

if [ ! -f SConstruct ]; then
	echo "Please call this script from the nre directory!" >&2
	exit 1
fi

mkdir -p dist/imgs
cd dist/imgs

files="bzImage-3.1.0-32 initrd-js.lzma escape.bin escape_cmos.bin escape_fs.bin \
	escape_ata.bin escape_pci.bin escape_romdisk.bin escape.iso escape-hd.img"

for f in $files; do
	wget -c http://os.inf.tu-dresden.de/~nils/imgs/$f
done

