#include <l4/names/libnames.h>
#include <l4/util/util.h>
#include "test-client.h"
#include <stdlib.h> // needed for malloc
#include <stdio.h>

l4_ssize_t l4libc_heapsize = 500*1024;

l4_threadid_t srv;
CORBA_Environment env = dice_default_environment;

char *str1 = "Hallo";
char *str2 = "Du!";
char *outstr = "Nanu?";

void *CORBA_alloc(unsigned long size) {
	return malloc(size);
}

int main(int argc, char **argv) {
	printf("Client(main): ask for name \"dcetst\"\n");
	while (names_waitfor_name("dcetst", &srv, 1000) == 0) {
		printf("Client(main): \"dcetst\" not available, keep trying...\n");
	}
	printf("Client(main): found \"dcetst\" at Names.");
	
	printf("\n*** TEST 1 ***\n");
	printf("Client(main): call test_ein_string with str1='%s'\n",str1);
	test_ein_string_call(&srv,str1,&env);
	
	printf("\n*** TEST 2 ***\n");
	printf("Client(main): call test_zwei_string with str1='%s', str2='%s'\n",
			str1,str2);
	test_zwei_string_call(&srv,str1,str2,&env);
	
	printf("\n*** TEST 3 ***\n");
	printf("Client(main): call test_gib_string with a = 42, instr = '%s'\n",str1);
	test_gib_string_call(&srv,42,str1,&outstr,&env);
	printf("Client(main): returned, outstr = '%s'\n",outstr);

	printf("\n\n\n");

	l4_sleep(2000);
	enter_kdebug("*#^test stopped");

	return 0;
}

