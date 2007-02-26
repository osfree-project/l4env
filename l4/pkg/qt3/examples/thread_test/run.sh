#! /bin/sh

${0%/*}/../../../..//tool/runux/fiasco -m128     \
    -n8 -n9 names log dm_phys rtc-ux name_server \
    qt3_thread_test

