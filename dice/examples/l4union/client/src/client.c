#include <l4/names/libnames.h>
#include <l4/util/util.h> // needed for l4_sleep
#include <l4/util/reboot.h>
#include "test-client.h"
#include <stdlib.h>
#include <stdio.h>

l4_ssize_t l4libc_heapsize = 500*1024;
char LOG_tag[9] = "tstclnt";

l4_threadid_t srv;
CORBA_Environment env = dice_default_environment;

char *str1 = "Hallo";
char *str2 = "Du!";

void *CORBA_alloc(unsigned long size) {
	return malloc(size);
}

int main(int argc, char **argv) {
	dope_event e;

	printf("Client(main): ask for name \"dcetst\"\n");
	while (names_waitfor_name("dcetst", &srv, 1000) == 0) {
		printf("Client(main): \"dcetst\" not available, keep trying...\n");
	}
	printf("Client(main): found \"dcetst\" at Names.");
	
	printf("\n*** TEST 1 ***\n");
	e._d = 1;
	e._u.command.cmd = "testcommand_1";
	printf("Client(main): send event with type 1 (command = \"%s\")\n",e._u.command.cmd);
	test_event_call(&srv,&e,"test_bindarg_1",&env);

	printf("\n*** TEST 2 ***\n");
	e._d = 2;
	e._u.motion.rel_x = 1;
	e._u.motion.rel_y = 2;
	e._u.motion.abs_x = 3;
	e._u.motion.abs_y = 4;
	printf("Client(main): send event with type 2 (motion = 1,2,3,4)\n");
	test_event_call(&srv,&e,"test_bindarg_2",&env);
	
	printf("\n*** TEST 3 ***\n");
	e._d = 3;
	e._u.press.code = 42;
	printf("Client(main): send event with type 3 (press code = 42)\n");
	test_event_call(&srv,&e,"test_bindarg_3",&env);
	
	printf("\n*** TEST 4 ***\n");
	e._d = 4;
	e._u.release.code = 24;
	printf("Client(main): send event with type 4 (release code = 24)\n");
	test_event_call(&srv,&e,"test_bindarg_4",&env);

	printf("*** UNION TEST FINISHED. ***\n\n\n");

	l4_sleep(2000);
	l4util_reboot();
	return 0;
}

