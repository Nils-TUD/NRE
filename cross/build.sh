#!/bin/sh

MAKE_ARGS=-j8
BUILD_BINUTILS=true
BUILD_GCC=true
BUILD_CPP=true

usage() {
	echo "Usage: $1 (x86_32|x86_64) [--rebuild]" >&2
	exit
}

if [ $# -ne 1 ] && [ $# -ne 2 ]; then
	usage $0
fi

ARCH="$1"
if [ "$ARCH" != "x86_32" ] && [ "$ARCH" != "x86_64" ]; then
	usage $0
fi

ROOT=`dirname $(readlink -f $0)`
BUILD=$ROOT/$ARCH/build
DIST=$ROOT/$ARCH/dist
SRC=$ROOT/$ARCH/src
HEADER=$ROOT/include

if [ "$2" == "--rebuild" ] || [ ! -d $DIST ]; then
	REBUILD=1
else
	REBUILD=0
fi

BINVER=2.21.1
GCCVER=4.6.1
NEWLVER=1.20.0

GCCCORE_ARCH=gcc-core-$GCCVER.tar.bz2
GCCGPP_ARCH=gcc-g++-$GCCVER.tar.bz2
BINUTILS_ARCH=binutils-"$BINVER"a.tar.bz2
NEWLIB_ARCH=newlib-$NEWLVER.tar.gz

# setup
export PREFIX=$DIST
if [ "$ARCH" = "x86_32" ]; then
	export TARGET=i686-pc-nulnova
else
	export TARGET=x86_64-pc-nulnova
fi
mkdir -p $BUILD/gcc $BUILD/binutils $BUILD/newlib

# cleanup
if [ $REBUILD -eq 1 ]; then
	if [ -d $SRC ]; then
		if $BUILD_BINUTILS && [ -d $BUILD/binutils ]; then
			cd $BUILD/binutils
			make distclean
			rm -Rf $SRC/binutils
		fi
		if $BUILD_GCC && [ -d $BUILD/gcc ]; then
			cd $BUILD/gcc
			make distclean
			rm -Rf $SRC/gcc
		fi
		if $BUILD_CPP && [ -d $BUILD/newlib ]; then
			cd $BUILD/newlib
			make distclean
			rm -Rf $SRC/newlib
		fi
		cd $ROOT
	fi
	mkdir -p $SRC
fi

# binutils
if $BUILD_BINUTILS; then
	if [ $REBUILD -eq 1 ]; then
		cat $BINUTILS_ARCH | bunzip2 | tar -C $SRC -xf -
		mv $SRC/binutils-$BINVER $SRC/binutils
		cd $ARCH && patch -p0 < binutils.diff
	fi
	cd $BUILD/binutils
	if [ $REBUILD -eq 1 ] || [ ! -f $BUILD/binutils/Makefile ]; then
		if [ "$ARCH" = "x86_32" ]; then
			$SRC/binutils/configure --target=$TARGET --prefix=$PREFIX --disable-nls
		else
			$SRC/binutils/configure --target=$TARGET --prefix=$PREFIX --disable-nls --enable-64-bit-bfd
		fi
		if [ $? -ne 0 ]; then
			exit 1
		fi
	fi
	make $MAKE_ARGS all && make install
	if [ $? -ne 0 ]; then
		exit 1
	fi
	cd $ROOT
fi

if $BUILD_GCC || $BUILD_CPP; then
	# put the include-files of newlib in the system-include-dir to pretend that we have a full libc
	# this is necessary for libgcc and libsupc++. we'll provide our own version of the few required
	# libc-functions later
	rm -Rf $DIST/$TARGET/include
	mkdir -p tmp
	cat $ROOT/$NEWLIB_ARCH | gunzip | tar -C tmp -xf - newlib-$NEWLVER/newlib/libc/include
	mv tmp/newlib-$NEWLVER/newlib/libc/include $DIST/$TARGET
	rm -Rf tmp
	
	# the mutexes should be initialized with 0
	sed --in-place -e 's/#define PTHREAD_MUTEX_INITIALIZER  ((pthread_mutex_t) 0xFFFFFFFF)/#define PTHREAD_MUTEX_INITIALIZER  ((pthread_mutex_t) 0)/g' $DIST/$TARGET/include/pthread.h
fi

# gcc
export PATH=$PATH:$PREFIX/bin
if $BUILD_GCC; then
	if [ $REBUILD -eq 1 ]; then
		cat $GCCCORE_ARCH | bunzip2 | tar -C $SRC -xf -
		cat $GCCGPP_ARCH | bunzip2 | tar -C $SRC -xf -
		mv $SRC/gcc-$GCCVER $SRC/gcc
		cd $ARCH && patch -p0 < gcc.diff
	fi
	cd $BUILD/gcc
	if [ $REBUILD -eq 1 ] || [ ! -f $BUILD/gcc/Makefile ]; then
		CFLAGS="-g -O2 -D_POSIX_THREADS -D_UNIX98_THREAD_MUTEX_ATTRIBUTES -DPTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP" $SRC/gcc/configure \
			  --target=$TARGET --prefix=$PREFIX --disable-nls \
			  --enable-languages=c,c++ --with-headers=$HEADER \
			  --disable-linker-build-id --with-gxx-include-dir=$HEADER/cpp --enable-threads=posix
		if [ $? -ne 0 ]; then
			exit 1
		fi
	fi
	make $MAKE_ARGS all-gcc && make install-gcc
	if [ $? -ne 0 ]; then
		exit 1
	fi
	ln -sf $DIST/bin/$TARGET-gcc $DIST/bin/$TARGET-cc

	# libgcc
	# first, generate crt*S.o and libc for libgcc_s. Its not necessary to have a full libc (we'll have
	# one later). But at least its necessary to provide the correct startup-files
	TMPCRT0=`tempfile`
	TMPCRT1=`tempfile`
	TMPCRTN=`tempfile`
	# crt0 can be empty
	echo ".section .init" >> $TMPCRT1
	echo ".global _init" >> $TMPCRT1
	echo "_init:" >> $TMPCRT1
	if [ "$ARCH" = "x86_32" ]; then
		echo "	push	%ebp" >> $TMPCRT1
		echo "	mov		%esp,%ebp" >> $TMPCRT1
	else
		echo "	push	%rbp" >> $TMPCRT1
		echo "	mov		%rsp,%rbp" >> $TMPCRT1
	fi
	echo ".section .fini" >> $TMPCRT1
	echo ".global _fini" >> $TMPCRT1
	echo "_fini:" >> $TMPCRT1
	if [ "$ARCH" = "x86_32" ]; then
		echo "	push	%ebp" >> $TMPCRT1
		echo "	mov		%esp,%ebp" >> $TMPCRT1
	else
		echo "	push	%rbp" >> $TMPCRT1
		echo "	mov		%rsp,%rbp" >> $TMPCRT1
	fi
	echo ".section .init" >> $TMPCRTN
	echo "	leave" >> $TMPCRTN
	echo "	ret" >> $TMPCRTN
	echo ".section .fini" >> $TMPCRTN
	echo "	leave" >> $TMPCRTN
	echo "	ret" >> $TMPCRTN

	# assemble them
	$TARGET-as -o $DIST/$TARGET/lib/crt0S.o $TMPCRT0 || exit 1
	$TARGET-as -o $DIST/$TARGET/lib/crt1S.o $TMPCRT1 || exit 1
	$TARGET-as -o $DIST/$TARGET/lib/crtnS.o $TMPCRTN || exit 1
	# build empty libc
	$TARGET-gcc -nodefaultlibs -nostartfiles -shared -Wl,-shared -Wl,-soname,libc.so \
	  -o $DIST/$TARGET/lib/libc.so $DIST/$TARGET/lib/crt0S.o || exit 1
	# cleanup
	rm -f $TMPCRT0 $TMPCRT1 $TMPCRTN
	
	# now build libgcc
	make $MAKE_ARGS all-target-libgcc && make install-target-libgcc
	if [ $? -ne 0 ]; then
		exit 1
	fi
	cd $ROOT
fi

# libsupc++
if $BUILD_CPP; then
	# libstdc++
	mkdir -p $BUILD/gcc/libstdc++-v3
	cd $BUILD/gcc/libstdc++-v3
	if [ $REBUILD -eq 1 ] || [ ! -f Makefile ]; then
		# pretend that we're using newlib
		CPP=$TARGET-cpp CXXFLAGS="-g -O2 -D_POSIX_THREADS -D_UNIX98_THREAD_MUTEX_ATTRIBUTES -DPTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP" \
			$SRC/gcc/libstdc++-v3/configure --host=$TARGET --prefix=$PREFIX \
			--disable-hosted-libstdcxx --disable-nls --with-newlib
		if [ $? -ne 0 ]; then
			exit 1
		fi
	fi
	cd include
	make $MAKE_ARGS && make install
	if [ $? -ne 0 ]; then
		exit 1
	fi
	cd ../libsupc++
	make $MAKE_ARGS && make install
	if [ $? -ne 0 ]; then
		exit 1
	fi
fi

# create basic symlinks
rm -Rf $DIST/$TARGET/include

# copy crt* to basic gcc-stuff
cp -f $BUILD/gcc/$TARGET/libgcc/crt*.o $DIST/lib/gcc/$TARGET/$GCCVER
