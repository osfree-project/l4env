#include <stdio.h>

#include "term.h"
#include "dopelib.h"
#include "startup.h"

int main(int argc, char **argv) {

	char hist[1024];
	char dst[256];
	struct history_struct *history;
	
	native_startup(argc, argv);

	printf("termtest(main): term_init...\n");
	term_init("Terminal-Test");
	printf("termtest(main): term_init finished\n");
	
	term_printf("This is a string printed by term_printf().\n");
	term_printf("What about a \\ or a \"?\n");
	term_printf("Integer test: %d, 0x%x\n", 12345, 12345);
	term_printf("Float test: %f\n", 1.2345);


  term_printf(
"  possible command are\n"
"  a ... show information about all loaded applications at loader\n"
"  d ... show all dataspaces of simple_dm\n"
"  f ... set file provider\n"
"  k ... kill an application which was loaded by the loader using the task id\n"
"  l ... load a new application using tftp\n"
"  m ... dump dataspace manager's memory map into to L4 debug console\n"
"  n ... list all registered names at name server\n"
"  p ... dump dataspace manager's pool info to L4 debug console\n"
"  r ... dump rmgr memory info to L4 debug console\n"
"  ^ ... reboot machine\n");

	
	term_printf("Test of readline(): type some lines of text, [q][return] to go on\n");
	history = term_history_create(&hist[0], 1024);
	do {
		term_readline(&dst[0], 256, history);
	} while (dst[0] != 'q');
	
	term_printf("Test of getchar(): press some keys, [esc] to go on\n");
	while (1) {
		int ascii;
		ascii = term_getchar();
		if (ascii == 27) break;
		printf("term_getchar returned %c (ascii %d)\n", (char)ascii, ascii );
		term_printf("%c", (char)ascii);
	}
	
	term_deinit();
	return 0;
}
