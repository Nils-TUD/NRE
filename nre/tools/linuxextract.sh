#!/bin/sh

if [ $# -ne 1 ]; then
	echo "Usage: $0 <linux-image>" >&2
	exit 1
fi

binary=$1
gziphead="1f 8b 08 00"

echo "Searching for gzip header..." >&2
begin=`od -A d -t x1 $binary | grep -m 1 "$gziphead"`
# cut off the leading zeros; otherwise its interpreted as octal
off1=`echo $begin | sed -e "s/^00*//" | cut -d ' ' -f 1`
# count the number of spaces between the offset and the byte-sequence; this will
# tell us how many bytes are in front of that sequence
off2=`echo $begin | sed -e "s/^[[:xdigit:]]* \(.*\) $gziphead/\1/" | grep -o ' ' | wc -l`
offset=$(($off1 + $off2 + 1))
printf "Found gzip header at offset %x\n" $offset >&2

echo "Unpacking gzip file to stdout..." >&2
dd if=$binary bs=1 skip=$offset | zcat

