/*
 * Simple polling serial console for the Flux OS toolkit
 * Copyright (C) 1996-1994 Sleepless Software
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *	Author: Bryan Ford
 */

#include <flux/base_critical.h>
#include <flux/x86/pio.h>
#include <flux/x86/pc/com_cons.h>

static int ser_io_base;
int com_cons_try_getchar(void);

int com_cons_getchar(void)
{
	int ch;

	base_critical_enter();

	if (ser_io_base == 0) {
		base_critical_leave();
		return -1;
	}

	/* Wait for a character to arrive.  */
	while (!(inb(ser_io_base + 5) & 0x01));

	/* Grab it.  */
	ch = inb(ser_io_base + 0);

	base_critical_leave();
	return ch;
}

int 
com_cons_try_getchar(void)
{
	int ch = -1;

	base_critical_enter();

	if (ser_io_base == 0) {
		base_critical_leave();
		return -1;
	}

	/* character available?  */
	if (inb(ser_io_base + 5) & 0x01) {
		/* Grab it.  */
		ch = inb(ser_io_base + 0);
	}

	base_critical_leave();
	return ch;
}

void com_cons_putchar(int ch)
{
	base_critical_enter();

	if (ser_io_base == 0) {
		base_critical_leave();
		return;
	}
	
	/* Wait for the transmit buffer to become available.  */
	while (!(inb(ser_io_base + 5) & 0x20));

	outb(ser_io_base + 0, ch);

	base_critical_leave();
}

void com_cons_init(int com_port)
{
	unsigned char dfr;
	unsigned divisor;

	base_critical_enter();

	switch (com_port)
	{
		case 1:	ser_io_base = 0x3f8; break;
		case 2:	ser_io_base = 0x2f8; break;
		case 3:	ser_io_base = 0x3e8; break;
		case 4:	ser_io_base = 0x2e8; break;
	}

	/* If no com_params was supplied, use a common default.  */

	dfr = 0x00;
	dfr |= 0x03; 

	/* Convert the baud rate into a divisor latch value.  */
	divisor = 115200 / 115200; //com_params->c_ospeed;

	/* Initialize the serial port.  */
	outb(ser_io_base + 3, 0x80 | dfr);	/* DLAB = 1 */
	outb(ser_io_base + 0, divisor & 0xff);
	outb(ser_io_base + 1, divisor >> 8);
	outb(ser_io_base + 3, 0x03 | dfr);	/* DLAB = 0, frame = 8N1 */
	outb(ser_io_base + 1, 0x00);	/* no interrupts enabled */
	outb(ser_io_base + 4, 0x0b);	/* OUT2, RTS, and DTR enabled */

	/* Clear all serial interrupts.  */
	inb(ser_io_base + 6);	/* ID 0: read RS-232 status register */
	inb(ser_io_base + 2);	/* ID 1: read interrupt identification reg */
	inb(ser_io_base + 0);	/* ID 2: read receive buffer register */
	inb(ser_io_base + 5);	/* ID 3: read serialization status reg */

	base_critical_leave();
}

void com_cons_enable_receive_interrupt(void)
{
	outb(ser_io_base + 1, 0x01);	/* interrupt on received data */
}

