#! /bin/sh

#export FB_PROVIDER=l4dope-ux

${0%/*}/../../../../tool/runux/qt3_app_con \
    "qttest_server -qws" \
    "qttest_client"
#    "qttest_client2"
