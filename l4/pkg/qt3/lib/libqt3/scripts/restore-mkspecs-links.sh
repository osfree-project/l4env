#! /bin/sh


(
    read l
    read t
    while [ ! -z "$l" ]
    do
	tt="`dirname \"$l\"`/$t"
	#echo "$l -> $t"
	[ -e "$tt" ] && ln -s "$t" "$l"
        read l
        read t
    done
) <<EOF
mkspecs/qws/linux-x86-g++/qplatformdefs.h
../../linux-g++/qplatformdefs.h
mkspecs/qws/linux-ipaq-g++/qplatformdefs.h
../linux-arm-g++/qplatformdefs.h
mkspecs/qws/linux-generic-g++/qplatformdefs.h
../../linux-g++/qplatformdefs.h
mkspecs/qws/linux-arm-g++/qplatformdefs.h
../../linux-g++/qplatformdefs.h
mkspecs/qws/linux-mips-g++/qplatformdefs.h
../../linux-g++/qplatformdefs.h
mkspecs/qws/solaris-generic-g++/qplatformdefs.h
../../solaris-g++/qplatformdefs.h
mkspecs/qws/freebsd-generic-g++/qplatformdefs.h
../../freebsd-g++/qplatformdefs.h
EOF

exit 0
