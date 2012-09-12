#!/bin/sh
SUDO=sudo

if [ $# -ne 3 ]; then
	echo "Usage: $0 <diskfile> <megabytes> <dir>" >&2
	exit 1
fi

diskmount=`mktemp -d`
tmpfile=`mktemp`
hdd=$1
mb=$2
dir=$3

# disk geometry (512 * 31 * 63 ~= 1 mb)
secsize=512
hdheads=31
hdtracksecs=63
hdcyl=$mb
totalsecs=$(($hdheads * $hdtracksecs * $hdcyl))
totalbytes=$(($totalsecs * $secsize))

# partitions
part1offset=2048
part1blocks=$(($totalsecs / 4))
part2offset=$((2048 + $part1blocks * 2))
part2blocks=$((($totalsecs - 2048 - $part1blocks * 2) / 2))

createFS() {
	lodev=`$SUDO losetup -f`
	$SUDO losetup -o$(($1 * 512)) $lodev $hdd || true
	$SUDO mke2fs -t ext3 -b 1024 $lodev $2
	$SUDO losetup -d $lodev
}

copyFiles() {
	lodev=`$SUDO losetup -f`
	$SUDO mount -text3 -oloop=$lodev,offset=$(($1 * 512)) $hdd $diskmount
	$SUDO cp -R $2 $diskmount
	i=0
	$SUDO umount -d $diskmount > /dev/null 2>&1
	while [ $? != 0 ]; do
		i=$(($i + 1))
		if [ $i -ge 10 ]; then
			echo "Unmount failed after $i tries" >&2
			exit 1
		fi
		$SUDO umount -d $diskmount > /dev/null 2>&1
	done
}

# create image
dd if=/dev/zero of=$hdd bs=512 count=$totalsecs

# create partitions
lodev=`$SUDO losetup -f`
$SUDO losetup $lodev $hdd
echo "n" > $tmpfile && \
	echo "p" >> $tmpfile && \
	echo "1" >> $tmpfile && \
	echo "" >> $tmpfile && \
	echo `expr $part2offset - 1` >> $tmpfile && \
	echo "n" >> $tmpfile && \
	echo "p" >> $tmpfile && \
	echo "2" >> $tmpfile && \
	echo "" >> $tmpfile && \
	echo "" >> $tmpfile && \
	echo "a" >> $tmpfile && \
	echo "1" >> $tmpfile && \
	echo "w" >> $tmpfile
$SUDO fdisk -u -C $hdcyl -S $hdtracksecs -H $hdheads $lodev < $tmpfile || true
$SUDO losetup -d $lodev

# create ext3 filesystems
createFS $part1offset $part1blocks
createFS $part2offset $part2blocks

# copy some content on part1
copyFiles $part1offset $dir

$SUDO rm -Rf $tmpfile $diskmount

