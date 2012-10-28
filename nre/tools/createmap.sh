#!/bin/sh

if [ $# -ne 1 ]; then
    echo "Usage: $0 <ELF-file>" >&2
    exit 1
fi

nm $1 | grep " T " | awk '
function filt(arg)
{
    c = "c++filt -n "arg
    c | getline res
    close(c)
    return res
}

{
    print $1" "filt($3)
}
'
