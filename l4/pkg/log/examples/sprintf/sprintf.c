/*!
 * \file   log/examples/sprintf/sprintf.c
 * \brief  sprintf example
 *
 * \date   02/27/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */

#include <l4/log/l4log.h>
#include <stdio.h>
#include <string.h>

int main(void){
	char buf[100];
	int i;
	
	memset(buf, 'x', sizeof(buf));
	sprintf(buf, "Example with a number (%d), a string (%s), and a hex number (%#x)",
	        4712, "Hi there", 0xdeadbeef);
	LOGl("sprintf called, putting the buffer now...");
	puts(buf);
	i = snprintf(buf, 38, "Example with a string %s", "limited in length");
	puts(buf);
	printf("('th' should be missing)\n");
	
	return 0;
}
