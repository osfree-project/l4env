#!/bin/sh

# This script file is for the maintainer of Qt for DROPS. It is used to generate
# the scripts, which restore the links from the original tarball, which can't be
# imported into CVS.

if [ -z "$2" ]
then
    echo "usage: `basename $0` DIR SCRIPTNAME"
    echo " e.g.: `basename $0` qt-embedded-free-3.3.3/ restore-include-links.sh"
    exit 1
fi

BASE="$1"
TARGET="$2"

echo -e "#! /bin/sh\n\n" > "$TARGET"


cat >> "$TARGET" <<EOF
(
    read l
    read t
    while [ ! -z "\$l" ]
    do
	tt="\`dirname \"\$l\"\`/\$t"
	#echo "\$l -> \$t"
	[ -e "\$tt" ] && ln -s "\$t" "\$l"
        read l
        read t
    done
) <<EOF
EOF
find "$BASE" -type l -printf "%P\n%l\n" >> "$TARGET"
echo "EOF" >> "$TARGET"
echo -e "\nexit 0\n" >> "$TARGET"

chmod +x "$TARGET"


