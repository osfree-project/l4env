#! /bin/sh

#export FB_PROVIDER=l4dope-ux
MYPATH=`cd "${0%/*}" && pwd`

${0%/*}/../../../../tool/runux/qt3_app_con \
    "-d$MYPATH/images/Fantasy.jpg"          \
    "-d$MYPATH/images/GreenInfinity.jpg"   \
    "qttest_qvv -qws"

