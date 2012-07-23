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

echo "This will download a precompiled cross-compiler for target $target into /opt/nre-cross-$target"
echo "Currently, the only ones available have been built on Ubuntu 12.04 hosts."
echo "It will probably work on other Ubuntu versions and other distributions" \
	"as well, but it hasn't been tested yet.\n"

host=""
while [ "$host" = "" ]; do
	echo "Please select your host platform:"
	echo "1. Ubuntu 12.04 i686"
	echo "2. Ubuntu 12.04 x86_64"
	echo "3. Other (build your own cross-compiler)"
	echo -n "Your choice (1, 2 or 3): "
	read choice
	if [ "$choice" = "1" ]; then
		host="ubuntu1204-i686"
	elif [ "$choice" = "2" ]; then
		host="ubuntu1204-x86_64"
	elif [ "$choice" = "3" ]; then
		echo "Ok, please use the build.sh to build your own compiler. Bye!"
		exit 0
	else
		echo "Invalid answer"
	fi
done

echo "\nDownloading cross-compiler for host $host and target $target..."
wget -c http://os.inf.tu-dresden.de/~nils/nre-cross-$host-$target.tar.gz

echo "\nExtracting it..."
$SUDO tar xfvz nre-cross-$host-$target.tar.gz -C /

echo "\nRemoving archive..."
rm -f nre-cross-$host-$target.tar.gz

