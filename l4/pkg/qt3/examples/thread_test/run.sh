#! /bin/sh

#export FB_PROVIDER=l4dope-ux

#${0%/*}/../../../../tool/runux/qt3_app_con \
#    "qttest_thread_test"

${0%/*}/../../../..//tool/runux/fiasco \
    -n8 -n9 names log dm_phys rtc-ux name_server \
    qttest_thread_test

