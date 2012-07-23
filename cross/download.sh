#!/bin/sh

SUDO=sudo

usage() {
	echo "Usage: $1 (x86_32|x86_64)" >&2
	exit
}

target="$1"
if [ "$target" != "x86_32" ] && [ "$target" != "x86_64" ]; then
	usage $0
fi

echo "This will download a precompiled cross-compiler into /opt/nre-cross-$target"
echo "Currently, the only ones available have been built on an Ubuntu 12.04 i686 host."
echo "It will probably work on other Ubuntu versions and other distributions" \
	"as well, but it hasn't been tested yet.\n"

echo -n "Do you think you have a compatible host platform? (yes/no) "
read dummy
if [ "$dummy" != "yes" ]; then
	echo "Ok, please use the build.sh to build your own compiler. Bye!"
	exit 0
fi

echo "\nDownloading cross compiler for target $target..."
wget -c http://os.inf.tu-dresden.de/~nils/nre-cross-ubuntu1204-i686-$target.tar.gz

echo "\nExtracting it..."
$SUDO tar xfvz nre-cross-ubuntu1204-i686-$target.tar.gz -C /

echo "\nRemoving archive..."
rm -f nre-cross-ubuntu1204-i686-$target.tar.gz

