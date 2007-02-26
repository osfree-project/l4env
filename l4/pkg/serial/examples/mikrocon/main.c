/*!
 * \file   serial/examples/mikrocon/main.c
 * \brief  Example showing the use of the serial lib
 *
 * \date   10/21/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <stdio.h>
#include <string.h>
#include <l4/sys/kdebug.h>
#include <l4/rmgr/librmgr.h>
#include <l4/util/reboot.h>
#include <l4/serial/serial.h>

int main(void){
    int err;

    rmgr_init();

    if((err=l4serial_init(5, 1))){
	printf("l4serial_init(5,1): %d\n\r ", err);
	return 1;
    }
    while(1){
	if((err = l4serial_readbyte())<0){
	    printf("l4serial_readbyte(): %d\n\r", err);
	    continue;
	}
	printf("read: %d (%c)\n", err, err);
	switch(err){
	case 'm':
	    err = l4serial_outstring("foobarlongstringhoho\n\r",
				     strlen("foobarlongstringhoho\n\r"));
	    break;
	case '^':
	    printf("rebooting...\n");
	    l4util_reboot();
	    break;
	case 'l':
	    err = l4serial_outstring("hoho\n\r", 5);
	    break;
	case 'd':
	    enter_kdebug("");
	    break;
	}
    }
}
