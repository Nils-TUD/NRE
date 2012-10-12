#!/bin/sh

TARGET=$1
URL=$2
REVISION=$3
BRANCH=$4

rm -rf "$TARGET" || exit 1
git clone "$URL" $TARGET || exit 1

( cd $TARGET && git checkout -b $BRANCH "$REVISION" ) || exit 1

shift 4

while [ $# -ne 0 ]; do
    ( cd $TARGET && git apply "$1" && git commit -am "$1" ) || exit 1
    shift
done

# EOF
