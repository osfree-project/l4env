#! /bin/sh

# usage: diff-against-contrib.sh [--print-only]
#          use '--print-only' option to just print filenames instead of a diff

find . -type f -exec test -f "../contrib/qt-embedded-free/{}" \; -printf "%P\n" | grep -v CVS | \
(
    read a;
    while [ ! -z "$a" ]
    do
        if [ "$1" = "--print-only" ]
        then
            echo "$a"
        else
            diff --ignore-matching-lines="^\*\* $Id" -u "../contrib/qt-embedded-free/$a" "$a"
        fi
        read a
    done
)