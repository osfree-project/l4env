#! /bin/sh

#export FB_PROVIDER=l4dope-ux
MYPATH=`cd "${0%/*}" && pwd`

${0%/*}/../../../../tool/runux/qt3_app_con \
    "-d$MYPATH/qt.png" \
    "-d$MYPATH/trolltech.gif" \
    "-d$MYPATH/tt-logo.png" \
    "-d$MYPATH/textfile.txt" \
    "-d$MYPATH/../../lib/libqt3/lib/fonts/c0648bt_.pfb" \
    "-d$MYPATH/../../lib/libqt3/lib/fonts/c0632bt_.pfb" \
    "-d$MYPATH/../../lib/libqt3/lib/fonts/c0649bt_.pfb" \
    "qttest_widgets -qws"

