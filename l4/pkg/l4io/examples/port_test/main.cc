/**
 * \file   l4io/examples/port_test/main.cc
 * \brief  L4io request/release I/O port test
 *
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/generic_io/libio.h>

char LOG_tag[9] = "io_port";

static l4io_info_t *io_info_addr = NULL;


int main(void)
{
	int error;
	l4io_drv_t drv = L4IO_DRV_INVALID;

	if ((error = l4io_init(&io_info_addr, drv))) {
		LOG_Error("initalizing libio: %d (%s)", error, l4env_strerror(-error));
		return 1;
	}

	LOG_printf("I/O port test running...\n");

	enum Port_state { FREE, ALLOC, DENIED, INVALID };

	struct Ports
	{
		Port_state      state;
		unsigned short  addr;
		unsigned short  len;
		const char     *name;
	} ports[] =
	{
		{ FREE, 0x0060, 1, "i8042 data"    },
		{ FREE, 0x0064, 1, "i8042 command" },
		{ FREE, 0x0020, 2, "PIC"           },
		{ FREE, 0x0070, 2, "RTC"           },
		{ FREE, 0x0080, 1, "diag port"     },
		{ FREE, 0x0278, 3, "parallel I/O"  },
		{ FREE, 0x02f8, 8, "serial I/O"    },
		{ FREE, 0x0cf8, 8, "PCI conf"      },
		{ FREE, 0x3001, 0x2fe, "nasty boy" },
		{ INVALID, 0, 0, 0 }
	};

	/* request ports in array */
	for (Ports *p = ports; p->state != INVALID; ++p) {
		if (p->state != FREE) continue;

		int err = l4io_request_region(p->addr, p->len);
		p->state = err ? DENIED : ALLOC;

		LOG_printf("REQ [%04x,%04x) (%s) => %s\n",
		           p->addr, p->addr + p->len, p->name,
		           err ? "FAILED" : "OK");
	}

	/* release allocated ports */
	for (Ports *p = ports; p->state != INVALID; ++p) {
		if (p->state != ALLOC) continue;

		int err = l4io_release_region(p->addr, p->len);
		p->state = err ? DENIED : FREE;

		LOG_printf("REL [%04x,%04x) (%s) => %s\n",
		           p->addr, p->addr + p->len, p->name,
		           err ? "FAILED" : "OK");
	}

	/* request free ports again */
	for (Ports *p = ports; p->state != INVALID; ++p) {
		if (p->state != FREE) continue;

		int err = l4io_request_region(p->addr, p->len);
		p->state = err ? DENIED : ALLOC;

		LOG_printf("REQ [%04x,%04x) (%s) => %s\n",
		           p->addr, p->addr + p->len, p->name,
		           err ? "FAILED" : "OK");
	}

	return 0;
}
