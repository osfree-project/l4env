/*
 * Remote serial-line source-level debugging for the Flux OS Toolkit.
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
 *	Loosely based on i386-stub.c from GDB 4.14
 */

#include <string.h>
#include <stdio.h>
#include <flux/gdb.h>
#include <flux/gdb_serial.h>
#include <flux/base_critical.h>
#include <flux/x86/types.h>
#include <flux/x86/gdb.h>
#include <stdlib.h>
#include <panic.h>


/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
/* at least NUMREGBYTES*2 are needed for register packets */
#define BUFMAX 400


/* These function pointers define how we send and receive characters
   over the serial line.  */
int (*gdb_serial_recv)(void);
void (*gdb_serial_send)(int ch);


/* True if we have an active connection to a remote debugger.
   This is used to avoid sending 'O' (console output) messages
   before the connection has been established or after it is closed,
   which tends to hang us and/or confuse the debugger.  */
static int connected;


static const char hexchars[]="0123456789abcdef";

static int hex(char ch)
{
	if ((ch >= '0') && (ch <= '9')) return (ch-'0');
	if ((ch >= 'a') && (ch <= 'f')) return (ch-'a'+10);
	if ((ch >= 'A') && (ch <= 'F')) return (ch-'A'+10);
	return (-1);
}

/* Scan for the sequence $<data>#<checksum> */
static void getpacket(char *buffer)
{
	unsigned char checksum;
	unsigned char xmitcsum;
	int  i;
	int  count;
	char ch;

	do {
		/* Wait around for the start character;
		   ignore all other characters.  */
		while ((ch = (gdb_serial_recv() & 0x7f)) != '$');
		checksum = 0;
		xmitcsum = -1;

		count = 0;

		/* Now, read until a # or end of buffer is found.  */
		while (count < BUFMAX) {
			ch = gdb_serial_recv() & 0x7f;
			if (ch == '#') break;
			checksum = checksum + ch;
			buffer[count] = ch;
			count = count + 1;
		}
		buffer[count] = 0;

		if (ch == '#')
		{
			xmitcsum = hex(gdb_serial_recv() & 0x7f) << 4;
			xmitcsum += hex(gdb_serial_recv() & 0x7f);
			if (checksum != xmitcsum)
				gdb_serial_send('-');  /* failed checksum */
			else
			{
				gdb_serial_send('+');  /* successful transfer */

				/* If a sequence char is present,
				   reply the sequence ID.  */
				if (buffer[2] == ':')
				{
					gdb_serial_send( buffer[0] );
					gdb_serial_send( buffer[1] );

					/* remove sequence chars from buffer */
					count = strlen(buffer);
					for (i=3; i <= count; i++)
						buffer[i-3] = buffer[i];
				}
			}
		}
	} while (checksum != xmitcsum);
}

/* send the packet in buffer.  */
#define SEND(ch) (gdb_serial_send(ch), checksum += (ch))
static void putpacket(char *buffer)
{
	unsigned char checksum;
	int  count;
	char ch;

	/*  $<packet info>#<checksum>. */
	do {
		gdb_serial_send('$');
		checksum = 0;
		count    = 0;

		while ((ch = buffer[count]) != '\0') {
			/*
			 * Unfortunately, the GDB serial protocol
			 * has no way of escaping special characters.
			 * Fortunately, special characters are not used
			 * within normal, automatically-generated messages.
			 * Unfortunately, they may occur in $O messages.
			 * Therefore, if we try to send a special character,
			 * replace it with a substitute string.
			 * so at least the user will at least see something
			 * other than completely bogus output.
			 */
			if (ch == '*')
			{
				SEND('('); SEND('S'); SEND('T');
				SEND('A'); SEND('R'); SEND(')');
			}
			else if (ch == '#')
			{
				SEND('('); SEND('H'); SEND('A');
				SEND('S'); SEND('H'); SEND(')');
			}
			else if (ch == '$')
			{
				SEND('('); SEND('D'); SEND('O'); SEND('L');
				SEND('L'); SEND('A'); SEND('R'); SEND(')');
			}
			else if ((buffer[count+1] == ch) &&
				 (buffer[count+2] == ch) &&
				 (buffer[count+3] == ch))
			{
				/*
				 * Run-length encode sequences of
				 * four or more repeated characters.
				 */
				unsigned char cc = ' ';
				for (count = count+4;
				     (buffer[count] == ch) && (cc < 126);
				     count++, cc++);
				SEND(ch);
				SEND('*');
				SEND(cc);
				continue;
			}
			else
			{
				/* Otherwise, just send the plain character. */
				SEND(ch);
			}

			count += 1;
		}

		gdb_serial_send('#');
		gdb_serial_send(hexchars[checksum >> 4]);
		gdb_serial_send(hexchars[checksum % 16]);

	} while ((gdb_serial_recv() & 0x7f) != '+');
}
#undef SEND

static void bin2hex(unsigned char *in, char *out, int len)
{
	while (len--)
	{
		*out++ = hexchars[*in >> 4];
		*out++ = hexchars[*in++ % 16];
	}
	*out = 0;
}

static int hex2bin(char *in, unsigned char *out, int len)
{
	while (len--)
	{
		int ch = (hex(in[0]) << 4) | hex(in[1]);
		if (ch < 0)
			return -1;
		*out++ = ch;
		in += 2;
	}
	return 0;
}

void gdb_serial_signal(int *inout_signo, struct gdb_state *state)
{
	int signo = *inout_signo;
	static char inbuf[BUFMAX+1];
	static char outbuf[BUFMAX+1];

	base_critical_enter();

	if (!gdb_serial_send || !gdb_serial_recv) {
		base_critical_leave();
		return;
	}

	/* Reply to the host that a signal has occurred.  */
	sprintf(outbuf, "S%02x", signo);
	putpacket(outbuf);

	/* OK, now we're officially connected.  */
	connected = 1;

	/* Handle commands sent by the remote debugger.  */
	while (1)
	{
		/* Return a blank response to messages we don't understand.  */
		outbuf[0] = 0;

		getpacket(inbuf);
		switch (inbuf[0])
		{
			case '?':
			{
				sprintf(outbuf, "S%02x", signo);
				break;
			}
			case 'g': /* return the value of the CPU registers */
			{
				/* Send it over like a memory dump.  */
				bin2hex((char*)state, outbuf, sizeof(*state));

				break;
			}
			case 'G' : /* set the CPU registers - return OK */
			{
				/* Unpack the new register state.  */
				if (hex2bin(&inbuf[1], (char*)state,
					    sizeof(*state)))
				{
					strcpy(outbuf, "E04");
					break;
				}

				strcpy(outbuf, "OK");
				break;
			}

			/* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
			case 'm' :
			{
				vm_offset_t addr, len;
				char *ptr, *ptr2;

				/* Read the start address and length.
				   If invalid, send back an error code.  */
				strcpy(outbuf, "E01");
				addr = strtoul(&inbuf[1], &ptr, 16);
				if ((ptr == &inbuf[1]) || (*ptr != ','))
					break;
				ptr++;
				len = strtoul(ptr, &ptr2, 16);
				if ((ptr2 == ptr) || (len*2 > BUFMAX))
					break;

				/* Copy the data into a kernel buffer.
				   If a fault occurs, return an error code.  */
				if (gdb_copyin(addr, inbuf, len))
				{
					strcpy(outbuf, "E03");
					break;
				}

				/* Convert it to hex data in the outbuf.  */
				bin2hex(inbuf, outbuf, len);

				break;
			}

			/* MAA..AA,LLLL: Write LLLL bytes at AA.AA return OK */
			case 'M' :
			{
				vm_offset_t addr, len;
				char *ptr, *ptr2;

				/* Read the start address and length.
				   If invalid, send back an error code.  */
				strcpy(outbuf, "E02");
				addr = strtoul(&inbuf[1], &ptr, 16);
				if ((ptr == &inbuf[1]) || (*ptr != ','))
					break;
				ptr++;
				len = strtoul(ptr, &ptr2, 16);
				if ((ptr2 == ptr) || (*ptr2 != ':'))
					break;
				ptr2++;
				if (ptr2 + len*2 > &inbuf[BUFMAX])
					break;

				/* Convert the hex input data to binary.  */
				if (hex2bin(ptr2, outbuf, len))
					break;

				/* Copy the data into its final place.
				   If a fault occurs, return an error code.  */
				if (gdb_copyout(outbuf, addr, len))
				{
					strcpy(outbuf, "E03");
					break;
				}

				strcpy(outbuf, "OK");
				break;
			}

			/* cAA..AA Continue at address AA..AA(optional) */
			/* sAA..AA Step one instruction from AA..AA(optional) */
			case 'c' :
			case 's' :
			{
				vm_offset_t new_eip;
				char *ptr;

				/* Try to read optional parameter;
				   leave EIP unchanged if none.  */
				new_eip = strtoul(&inbuf[1], &ptr, 16);
				if (ptr > &inbuf[1])
					state->eip = new_eip;

				/* Set the trace flag appropriately.  */
				gdb_set_trace_flag(inbuf[0] == 's', state);

				/* Return and "consume" the signal
				   that caused us to be called.  */
				*inout_signo = 0;
				base_critical_leave();
				return;
			}

			/* CssAA..AA Continue with signal ss */
			/* SssAA..AA Step one instruction with signal ss */
			case 'C' :
			case 'S' :
			{
				vm_offset_t new_eip;
				int nsig;
				char *ptr;

				/* Read the new signal number.  */
				nsig = hex(inbuf[1]) << 4 | hex(inbuf[2]);
				if ((nsig <= 0) || (nsig >= 32))
				{
					strcpy(outbuf, "E05");
					break;
				}

				/* Try to read optional parameter;
				   leave EIP unchanged if none.  */
				new_eip = strtoul(&inbuf[3], &ptr, 16);
				if (ptr > &inbuf[3])
					state->eip = new_eip;

				/* Set the trace flag appropriately.  */
				gdb_set_trace_flag(inbuf[0] == 'S', state);

				/* Return and "consume" the signal
				   that caused us to be called.  */
				*inout_signo = nsig;
				base_critical_leave();
				return;
			}

			case 'k': /* kill the program */
				connected = 0;
				panic("Program terminated by debugger");
		}

		/* reply to the request */
		putpacket(outbuf);
	}
	base_critical_leave();

}

void gdb_serial_putchar(int ch)
{
       char outbuf[4];

	base_critical_enter();
	if (!gdb_serial_send || !gdb_serial_recv) {
		base_critical_leave();
		return;
	}

	if (!connected) {
		gdb_serial_send(ch);
		base_critical_leave();
		return;
	}

	outbuf[0] = 'O';
       outbuf[1] = hexchars[ch >> 4];
       outbuf[2] = hexchars[ch & 15];
       outbuf[3] = 0;
	putpacket(outbuf);
	base_critical_leave();
}

