#! /bin/sh

${0%/*}/../../../../tool/runux/fiasco -n8 names log dm_phys rtc-ux name_server \
    local_socks local_socks-test-server local_socks-test-client
