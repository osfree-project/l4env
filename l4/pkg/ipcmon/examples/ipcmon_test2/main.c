#include <stdlib.h>
#include <string.h>

#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/util/macros.h>
#include <l4/sys/ipc.h>
#include <l4/generic_ts/generic_ts.h>
#include <l4/thread/thread.h>
#include <l4/names/libnames.h>
#include <l4/ipcmon/ipcmon.h>

char LOG_tag[9] = "ipctest2";

int main(int argc, char **argv)
{
	l4_threadid_t LOG_tid;
	l4_threadid_t ipcmon_tid;
	l4_threadid_t ipcmon_test_tid;

	LOG("ipcmon_test2 starting");
	names_register("ipcmon_test2");

	LOG("I'm going to sleep for 10 seconds.");

	l4_sleep(10000);

	if (!names_query_name("stdlogV05", &LOG_tid)) {
		LOG("could not query stdlogV05 - terminating");
		l4_sleep_forever();
	}

	if (!names_query_name("ipcmon", &ipcmon_tid)) {
		LOG("could not query IPCMon - terminating");
		l4_sleep_forever();
	}

	if (!names_query_name("ipcmon_test", &ipcmon_test_tid)) {
		LOG("could not query IPCMon_Test - terminating");
		l4_sleep_forever();
	}

	LOG("LOG is at "l4util_idfmt, l4util_idstr(LOG_tid));
	LOG("IPCMon is at "l4util_idfmt, l4util_idstr(ipcmon_tid));
	LOG("IPCMon_Test is at "l4util_idfmt, l4util_idstr(ipcmon_test_tid));

	LOG("now revoking comm rights from ipcmon_test.");
	l4ipcmon_deny(ipcmon_tid, ipcmon_test_tid, LOG_tid);

	while(1);


	return 0;
}
