#include <stdlib.h>
#include <string.h>

#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/util/macros.h>
#include <l4/sys/ipc.h>
#include <l4/generic_ts/generic_ts.h>
#include <l4/thread/thread.h>
#include <l4/names/libnames.h>

char LOG_tag[9] = "ipctest1";

int main(int argc, char **argv)
{
	int i;
	LOG("ipcmon_test");

	names_register("ipcmon_test");

	LOG("registered at names");

	for (i=0; i<20; i++) {
		LOG("I'm still printing stuff to LOG.");
		l4_sleep(1000);
	}

	l4_sleep_forever();

	return 0;
}
