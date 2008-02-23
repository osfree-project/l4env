/* L4 */
#include <l4/util/macros.h>
#include <l4/util/parse_cmd.h>
#include <l4/names/libnames.h>

//This causes trouble, because we mix here uclibc includes
//with dde/linux kernel includes
//#include <stdlib.h>
//Therefore we define exit here explicitly
extern void exit(int status);

/* local */
#include "flips-server.h"
#include "local_s.h"

/* loglib tag */
char LOG_tag[9] = "flips";

l4_ssize_t l4libc_heapsize = 1024 * 1024;
const int l4thread_max_threads = 64;

/** MAIN */
int main(int argc, const char **argv)
{
	l4thread_t notif_l4t = L4THREAD_INVALID_ID;
	CORBA_Server_Environment env = dice_default_server_environment;
	int error, use_dhcp = 0;

	if ((error = parse_cmdline(&argc, &argv,
		'd', "dhcp", "enable dhcp",
		PARSE_CMD_SWITCH, 1, &use_dhcp,
		0))) {
		switch (error) {
			case -1: LOG_printf("Bad parameter for parse_cmdline()\n"); break;
			case -2: LOG_printf("Out of memory in parse_cmdline()\n"); break;
			default: LOG_printf("Error %d in parse_cmdline()\n", error); break;
		}
		exit(-1);
	}

	/* keep all options open */
	env.malloc = CORBA_alloc;
	env.free = CORBA_free;

	LOG_printf("Starting FLIPS server\n");

	if (liblinux_init(1024*1024, 1024*1024, use_dhcp))
		exit(1);

	notif_l4t = l4thread_create_named((l4thread_fn_t) notify_thread,
	                                  ".notify",
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

	LOG_printf("FLIPS server is up ...\n");

	flips_server_loop(&env);

	exit(0);
}
