#!/bin/bash

if [ "$TARGET" = "x86_32" ]; then
	cross="i686-pc-nulnova"
	export QEMU="qemu-system-i386"
	export QEMU_FLAGS="-cpu phenom -m 256 -smp 4"
elif [ "$TARGET" = "x86_64" ]; then
	cross="x86_64-pc-nulnova"
	export QEMU="qemu-system-x86_64"
	export QEMU_FLAGS="-m 256 -smp 4"
else
	echo 'Please define $TARGET first!' >&2
	exit 1
fi

scons_args="-j8"
crossdir="$PWD/../cross/$TARGET/dist"
build="build/$TARGET"

if [ "$1" = "--help" ] || [ "$1" = "-h" ] || [ "$1" = "-?" ] ; then
	echo "Usage: $0 (dis=<app>|elf=<app>|debug=<app>|run|test)"
	exit 0
fi

scons $scons_args
case "$1" in
	run)
		./test --qemu="$QEMU" --qemu-flags="$QEMU_FLAGS" --build-dir="$PWD/$build"
		;;
	test)
		./unittests --qemu="$QEMU" --qemu-flags="$QEMU_FLAGS" --build-dir="$PWD/$build"
		;;
	dis=*)
		$crossdir/bin/$cross-objdump -SC $build/bin/apps/${1:4} | less
		;;
	elf=*)
		$crossdir/bin/$cross-readelf -a $build/bin/apps/${1:4} | less
		;;
	dbg=*)
		BUILD_DIR="$PWD/$build" novadebug ./test $build/bin/apps/${1:4}
		;;
esac

