#! /bin/sh

#export FB_PROVIDER=l4dope-ux
MYPATH=`cd "${0%/*}" && pwd`

${0%/*}/../../../../tool/runux/qt3_app_con \
    "simple_ts" \
    "bmodfs" \
    "-d$MYPATH/bin/libloader.s.so" \
    "-d$MYPATH/bin/simple_file_server" \
    "-d$MYPATH/bin/simple_file_server_qt3" \
    "-d$MYPATH/bin/simple_file_server_data" \
    "-d$MYPATH/bin/fstab" \
    "-d$MYPATH/bin/qttest_qvv" \
    "-d$MYPATH/bin/file_servers.cfg" \
    "loader --fprov=BMODFS file_servers.cfg" \
    "l4exec" \
    "qttest_qrun -qws"

