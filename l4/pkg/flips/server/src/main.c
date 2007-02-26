/* L4 */
#include <l4/util/macros.h>
#include <l4/names/libnames.h>

#include <stdlib.h>

/* local */
#include "flips-server.h"
#include "local.h"

/* loglib tag */
char LOG_tag[9] = "flips-0";

l4_ssize_t l4libc_heapsize = 1024 * 1024;

/** MAIN */
int main(int argc, char **argv)
{
	l4thread_t notif_l4t = L4THREAD_INVALID_ID;
	CORBA_Server_Environment env = dice_default_server_environment;

	LOG_printf("Starting FLIPS server\n");

	if (liblinux_init(640*1024, 1024*1024))
		exit(1);
	l4dde_do_initcalls();

	notif_l4t = l4thread_create((l4thread_fn_t) notify_thread,
                                 NULL, L4THREAD_CREATE_SYNC);

	if (notif_l4t <= 0)
	{
		LOG_Error("No thread available.");
		exit(1);
	}

	if (!names_register("FLIPS")) {
		LOG_Error("Cannot register at nameserver");
		exit(2);
	}

	flips_server_loop(&env);

	exit(0);
}

