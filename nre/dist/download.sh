#!/bin/sh

if [ ! -f SConstruct ]; then
	echo "Please call this script from the nre directory!" >&2
	exit 1
fi

mkdir -p dist/imgs
cd dist/imgs
wget -c http://os.inf.tu-dresden.de/~nils/imgs/bzImage-3.1.0-32
wget -c http://os.inf.tu-dresden.de/~nils/imgs/initrd-js.lzma

