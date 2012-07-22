#!/bin/sh

ROOT=`dirname $(readlink -f $0)`
source ${ROOT}/config.sh

wget -c ${GNU_MIRROR}/binutils/binutils-${BINVER}.tar.bz2
wget -c ${GNU_MIRROR}/gcc/gcc-${GCCVER}/gcc-core-${GCCVER}.tar.bz2
wget -c ${GNU_MIRROR}/gcc/gcc-${GCCVER}/gcc-g++-${GCCVER}.tar.bz2
wget -c ftp://sources.redhat.com/pub/newlib/newlib-${NEWLVER}.tar.gz

# EOF
