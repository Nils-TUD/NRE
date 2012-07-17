#!/bin/bash

# config
cpus=`cat /proc/cpuinfo | grep '^processor[[:space:]]:' | wc -l`
jobs=-j$cpus
opts=""
hvdir="../nova-intel"
loader="../morbo/tftp/farnsworth"
bochscfg="bochs.cfg"

if [ "$NOVA_TARGET" = "x86_32" ]; then
	cross="i686-pc-nulnova"
	export QEMU="qemu-system-i386"
	export QEMU_FLAGS="-cpu phenom -m 256 -smp 1"
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

echo "Building for $NOVA_TARGET in $NOVA_BUILD mode using $cpus jobs..."

if [ ! -f $build/test.dump ]; then
	mkdir -p $build
	echo "" > $build/test.dump
fi

# build userland
scons $jobs $opts
if [ $? -ne 0 ]; then
	exit 1
fi

mv $build/test.dump $build/test_old.dump
$build/tools/dump/dump $build/bin/apps/test > $build/test.dump
if [ "`diff $build/test.dump $build/test_old.dump`" != "" ]; then
	scons $jobs $opts
fi
rm -f $build/test_old.dump

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
	prof=*)
		$build/tools/conv/conv i586 log.txt $build/bin/apps/${1:5} > result.xml
		#gdbtui --args $build/tools/conv/conv i586 log.txt $build/bin/apps/${1:5}
		;;
	bochs)
		mkdir -p $build/bin/boot/grub
		cp $root/tools/stage2_eltorito $build/bin/boot/grub
		novaboot --build-dir="$PWD/$build" --iso -- $2
		sed --in-place -e \
			's/\(ata0-master:.*\?path\)=.*\?,/\1='`echo $build/$2.iso | sed -e 's/\//\\\\\//g'`',/' $bochscfg
		bochs -f $bochscfg -q
		;;
	run)
		./$2 --qemu="$QEMU" --qemu-flags="$QEMU_FLAGS" --build-dir="$PWD/$build" | tee log.txt
		;;
	srv)
		./$2 --server --build-dir="$PWD/$build"
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
		BUILD_DIR="$PWD/$build" $build/tools/debug ./$2 $build/bin/apps/${1:4}
		kill `pgrep qemu`
		;;
	list)
		ls -1 $build/bin/apps | while read l; do	
			$crossdir/bin/$cross-readelf -S $build/bin/apps/$l | \
				grep "\.init" | awk "{ printf(\"%12s: %s\n\",\"$l\",\$5) }"
		done
		;;
esac

