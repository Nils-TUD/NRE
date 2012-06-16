#!/bin/bash

# config
jobs="-j8"
hvdir="../nova-intel"
loader="../morbo/tftp/farnsworth"

if [ "$NOVA_TARGET" = "x86_32" ]; then
	cross="i686-pc-nulnova"
	export QEMU="qemu-system-i386"
	export QEMU_FLAGS="-cpu phenom -m 256 -smp 8"
elif [ "$NOVA_TARGET" = "x86_64" ]; then
	cross="x86_64-pc-nulnova"
	export QEMU="qemu-system-x86_64"
	export QEMU_FLAGS="-m 256 -smp 4"
else
	echo 'Please define $NOVA_TARGET to x86_32 or x86_64 first!' >&2
	exit 1
fi

if [ "$NOVA_BUILD" != "debug" ]; then
	NOVA_BUILD="release"
fi

crossdir="$PWD/../cross/$NOVA_TARGET/dist"
build="build/$NOVA_TARGET-$NOVA_BUILD"
root=$(dirname $(readlink -f $0))

if [ "$1" = "--help" ] || [ "$1" = "-h" ] || [ "$1" = "-?" ] ; then
	echo "Usage: $0 (dis=<app>|elf=<app>|debug=<app>|run|test)"
	exit 0
fi

echo "Building for $NOVA_TARGET in $NOVA_BUILD mode..."

# build userland
scons $jobs
if [ $? -ne 0 ]; then
	exit 1
fi

cd $hvdir/build
# adjust build-flags depending on build-type
if [ "`grep 'OFLAGS[[:space:]]*:=[[:space:]]*-O0' Makefile`" != "" ]; then
	if [ "$NOVA_BUILD" != "debug" ]; then
		# it should be release, but isn't
		sed --in-place -e 's/OFLAGS[[:space:]]*:=[[:space:]]*-O0.*/OFLAGS\t\t:= -Os -g/' Makefile
	fi
else
	if [ "$NOVA_BUILD" != "release" ]; then
		# it should be debug, but isn't
		sed --in-place -e 's/OFLAGS[[:space:]]*:=[[:space:]]*.*/OFLAGS\t\t:= -O0 -g -DDEBUG/' Makefile
	fi
fi
# build NOVA
ARCH=$NOVA_TARGET make $jobs
if [ $? -ne 0 ]; then
	exit 1
fi
cp hypervisor-$NOVA_TARGET $root/$build/bin/apps/hypervisor
cd $root

# copy loader
cp $loader $build/bin/apps/chainloader

# run the specified command, if any
case "$1" in
	run)
		./test --qemu="$QEMU" --qemu-flags="$QEMU_FLAGS" --build-dir="$PWD/$build" | tee log.txt
		;;
	srv=*)
		./${1:4} --server --build-dir="$PWD/$build"
		;;
	test)
		./unittests --qemu="$QEMU" --qemu-flags="$QEMU_FLAGS" --build-dir="$PWD/$build" | tee log.txt
		;;
	dis=*)
		$crossdir/bin/$cross-objdump -SC $build/bin/apps/${1:4} | less
		;;
	elf=*)
		$crossdir/bin/$cross-readelf -a $build/bin/apps/${1:4} | less
		;;
	trace=*)
		tools/backtrace $build/bin/apps/${1:6}
		;;
	dbg=*)
		BUILD_DIR="$PWD/$build" $build/tools/debug ./test $build/bin/apps/${1:4}
		;;
esac

