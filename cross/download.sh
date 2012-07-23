#!/bin/sh

SUDO=sudo

echo "This will download a precompiled cross-compiler into /opt"
echo "Currently, the only ones available have been built on an Ubuntu 12.04 i686 host."
echo "It will probably work on other Ubuntu versions and other distributions" \
	"as well, but it hasn't been tested yet.\n"
/bin/echo -e "\033[1mIf you don't have a compatible platform, please use the build.sh to build" \
	"the cross-compiler yourself.\033[0m\n"

target=""
while [ "$target" = "" ]; do
	echo "Pick target:"
	echo "1. x86_32"
	echo "2. x86_64"
	echo -n "Your choice (1 or 2): "
	read choice

	if [ "$choice" = "1" ]; then
		target="x86_32"
	elif [ "$choice" = "2" ]; then
		target="x86_64"
	else
		echo "Invalid answer"
	fi
done

echo "\nDownloading cross compiler for target $target..."
wget -c http://os.inf.tu-dresden.de/~nils/nre-cross-ubuntu1204-i686-$target.tar.gz

echo "\nExtracting it..."
$SUDO tar xfvz nre-cross-ubuntu1204-i686-$target.tar.gz -C /

echo "\nRemoving archive..."
rm -f nre-cross-ubuntu1204-i686-$target.tar.gz

