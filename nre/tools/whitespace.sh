#!/bin/bash

find . -regex '.*\.\(h\|cc\|cpp\)$' | grep -v '^./build/' | while read l; do
    echo "Searching in \"$l\"..."
    grep --color -n '^\t*  *\t*[^\*[[:space:]]]*' $l
    echo ""
done

