#!/bin/bash
if [ "$1" == "" ]; then
	echo "usage: $0 <contrib dir>"
	exit 1
fi

set -e

# make path absolute
contrib=`cd $1 && pwd`;

for d in `find $contrib -type d -printf '%P\n'`; do
	mkdir -p $d
done
for d in `find $contrib -type f -a ! \( -name aclocal.m4 \) -printf '%P\n'`; do
	if [ ! -e "$d" ]; then
		ln -s $contrib/$d $d
	fi
done

