/*
 * some dummy functions...
 * \date    2009-04-07
 * \author  Dirk Vogt <dvogt@os.inf.tu-dresden.de>
 */

/*
 * This file is part of the DDEUSB package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


#include <linux/input.h>


void input_ff_destroy(struct input_dev * dev){
}

int input_ff_event (	struct input_dev *  	iev,
 	unsigned int  	type,
 	unsigned int  	code,
 	int  	value){
return 0;
}


void add_input_randomness(unsigned int type, unsigned int code, unsigned int value) {
}

int register_chrdev(unsigned int major, const char*name, const struct file_operations *ops) {
	return 0;
}
