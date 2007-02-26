#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/*** L4 INCLUDES ***/
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/l4vfs/extendable.h>
#include <l4/l4vfs/name_server.h>
#include <l4/l4vfs/basic_name_server.h>

char LOG_tag[9] = "svtest";
l4_ssize_t l4libc_heapsize = 500*1024;
static l4_threadid_t srv;


/*** MAIN FUNCTION ***/
int main(int argc, char **argv) {
	l4_threadid_t ns;
	int ret;

	/* mount server now */
	ns = l4vfs_get_name_server_threadid();
	while ((ret = l4vfs_attach_namespace(ns, 1000, "/", "/test3")) == 3)
	{
		l4_sleep(1000);
	}

	LOG("attached namespace");

	/* open fd=0 --> stdin */
	ret = open("/test3/vc0", O_RDONLY);

	/* open fd=1 --> stdout and fd=2 --> stderr - we need that for printf */
	ret = open("/test3/vc0", O_WRONLY);
	ret = open("/test3/vc0", O_WRONLY);

	printf("Server(main): ask for name \"ifconfig\"\n");
	while (names_waitfor_name("ifconfig", &srv, 2500) == 0) {
		printf("Server(main): \"ifconfig\" not available, keep trying...\n");
	}
	printf("Server(main): found \"ifconfig\" at Names.\n");

	return svtest_main(argc, argv);
}
