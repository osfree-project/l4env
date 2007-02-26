/*
 * \brief   Test for running a multithreaded Flips client
 * \author  Norman Feske
 * \date    2004-04-20
 *
 * Libflips creates Flips session on demand for each thread that calls
 * flips_socket functions. We use the same code as for the separate
 * idltest client and server.
 */


/*** L4 SPECIFIC INCLUDES ***/
#include <l4/thread/thread.h>
#include <l4/util/util.h>


l4_ssize_t l4libc_heapsize = 500*1024;
char LOG_tag[9] = "idlt-cs";

extern int client_main(int argc, char **argv);
extern int server_main(int argc, char **argv);


static void server_thread(void *arg) {
	server_main(0, NULL);
}


int main(int argc, char **argv) {
	printf("--- STARTED IDLTEST (CLIENT AND SERVER WITHIN ONE ADDRESS SPACE) ---\n");

	l4thread_create(server_thread, NULL, L4THREAD_CREATE_ASYNC);
	
	/* wait a second - you think this sucks? - you are right! */
	l4_usleep(1000*1000);

	client_main(0, NULL);
	return 0;
}
