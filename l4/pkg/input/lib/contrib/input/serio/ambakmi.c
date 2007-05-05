/*
 *  linux/drivers/input/serio/ambakmi.c
 *
 *  Copyright (C) 2000-2003 Deep Blue Solutions Ltd.
 *  Copyright (C) 2002 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/err.h>

#include <asm/io.h>
#include <asm/hardware/amba_kmi.h>

#include <l4/sigma0/sigma0.h>
#include <l4/arm_drivers/common.h>

static unsigned long kmi0_base;

enum {
  // realview eb
#if 0
  KMI0_PHYS_BASE = 0x10006000,
  KMI0_SIZE      = 0x1000,
#endif
  // integrator cp
#if 1
  KMI0_PHYS_BASE = 0x18000000,
  KMI0_SIZE      = 0x1000,
#endif



  // DATA
  DATA_RESET          = 0xff,
  DATA_RESET_RESPONSE = 0xaa,

  // CLKDIV
  CLKDIV_DIVISOR = 2,   /* 8MHz = 24MHz / (1 + CLKDIV_DIVISOR) */
};


#define KMI_BASE	(kmi0_base)

struct amba_kmi_port {
	struct serio		*io;
	struct clk		*clk;
	void  *base;
	unsigned int		irq;
	unsigned int		divisor;
	unsigned int		open;
};

static irqreturn_t amba_kmi_int(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned int status = readb(KMIIR);
	int handled = IRQ_NONE;

	while (status & KMIIR_RXINTR) {
		serio_interrupt(dev_id, readb(KMIDATA), 0, regs);
		status = readb(KMIIR);
		handled = IRQ_HANDLED;
	}

	return handled;
}

static int amba_kmi_write(struct serio *io, unsigned char val)
{
	unsigned int timeleft = 10000; /* timeout in 100ms */

	while ((readb(KMISTAT) & KMISTAT_TXEMPTY) == 0 && timeleft--)
		udelay(10);

	if (timeleft)
		writeb(val, KMIDATA);

	return timeleft ? 0 : SERIO_TIMEOUT;
}

static int amba_kmi_open(struct serio *io)
{
	unsigned int divisor;
	int ret;

	divisor = CLKDIV_DIVISOR;
	writeb(divisor, KMICLKDIV);
	writeb(KMICR_EN, KMICR);

	ret = request_irq(20, amba_kmi_int, 0, "kmi-pl050", io);
	if (ret) {
		printk(KERN_ERR "kmi: failed to claim IRQ%d\n", 20);
		writeb(0, KMICR);
		return ret;
	}

	writeb(KMICR_EN | KMICR_RXINTREN, KMICR);
	return 0;
}

static void amba_kmi_close(struct serio *io)
{
	writeb(0, KMICR);
	free_irq(20, NULL);
}

static int amba_kmi_probe(struct device *dev, void *id)
{
	struct serio *io;
	int ret;
	l4_threadid_t s0 = L4_NIL_ID;

	/* Initialize keyboard */
	s0.id.task = 2;

	if ((kmi0_base = arm_driver_map_io_region(KMI0_PHYS_BASE, KMI0_SIZE)) == -1)
	  {
	    printf("arm_driver_map_io_region failed\n");
	    return -1;
	  }
	printf("pl050: got memory, virtual base at %lx\n", kmi0_base);

	// setup clock and enable
	writeb(0, KMICR);
	writeb(CLKDIV_DIVISOR, KMICLKDIV);
	writeb(KMICR_EN | KMICR_RXINTREN, KMICR);

	// reset controller
	writeb(DATA_RESET, KMIDATA);
	// clear data again
	readb(KMIDATA);

	printf("pl050: waiting for reset %lx %lx\n", KMIDATA, KMISTAT);
	// wait for reset response
	while (1) {
		if (readb(KMISTAT) & KMISTAT_RXFULL) {
		  unsigned long val = readb(KMIDATA);
		  printf("val = %ld\n", val);
		//	if (val == DATA_RESET_RESPONSE)
				break;
		}

		l4_sleep(2);
	}
	printf("received kbd reset\n");

	io = kmalloc(sizeof(struct serio), GFP_KERNEL);
	if (!io) {
		ret = -ENOMEM;
		goto out;
	}

	memset(io, 0, sizeof(struct serio));

	io->type	= SERIO_8042;
	io->write	= amba_kmi_write;
	io->open	= amba_kmi_open;
	io->close	= amba_kmi_close;
	strlcpy(io->name, "1", sizeof(io->name));
	strlcpy(io->phys, "2", sizeof(io->phys));
	//io->dev.parent	= &dev->dev;

	serio_register_port(io);
	return 0;

 out:
	kfree(io);
	return ret;
}

static int __init amba_kmi_init(void)
{
	return amba_kmi_probe(NULL, NULL);
}

static void __exit amba_kmi_exit(void)
{
}

module_init(amba_kmi_init);
module_exit(amba_kmi_exit);

MODULE_AUTHOR("Russell King <rmk@arm.linux.org.uk>");
MODULE_DESCRIPTION("AMBA KMI controller driver");
MODULE_LICENSE("GPL");
