/*
 * \date    2009-04-07
 * \author  Dirk Vogt <dvogt@os.inf.tu-dresden.de>
 */

/*
 * This file is part of the DDEUSB package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */





#include <linux/input.h>

void input_event(struct input_dev *dev, unsigned int type, unsigned int code, int value)
{
}

struct input_dev *input_allocate_device(void)
{
	struct input_dev *dev;

	dev = kzalloc(sizeof(struct input_dev), GFP_KERNEL);
	if (dev) {
	}

	return dev;
}


int input_register_device(struct input_dev *dev)
{
	return 0;
}

void input_free_device(struct input_dev *dev)
{
	if (dev) {
		dev->name = NULL;
		kfree(dev);
	}
}

void input_unregister_device(struct input_dev *dev){
}
