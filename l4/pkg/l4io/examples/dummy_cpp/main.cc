/**
 * \file   l4io/examples/dummy_cpp/main.cc
 * \brief  L4Env I/O client example (dumb C++ dummy)
 *
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/util/util.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>

#include <l4/generic_io/libio.h>

char LOG_tag[9] = "io_dummy";

/* exported by libio */
extern unsigned long jiffies;
extern unsigned long HZ;

static l4io_info_t *io_info_addr;

static void do_jiffies(unsigned int num)
{
	unsigned long stamp;

	LOG_Enter("HZ = %lu jiffies = %lu @ %p num = %u", HZ, jiffies, &jiffies, num);

	for (; num; num--) {
		/* wait HZ jiffies (1 s) */
		stamp = jiffies + HZ;
		while (jiffies < stamp) l4_usleep(900*(stamp-jiffies));

		LOG_printf("period = %lu jiffies ... xtime = {%ld, %ld}\n",
		           HZ, io_info_addr->xtime.tv_sec, io_info_addr->xtime.tv_usec);
	}
}

static const char * get_class_string(unsigned dev_class)
{
	/* check class code and if appropriate also sub-class code */
	switch (dev_class >> 16) {
	case 0x01:
		/* mass storage */
		switch ((dev_class >> 8) & 0xff) {
		case 0x00: return "SCSI storage controller";
		case 0x01: return "IDE interface";
		case 0x05: return "ATA controller";
		default:   return "Unknown mass storage controller";
		}
	case 0x02:
		/* network */
		switch ((dev_class >> 8) & 0xff) {
		case 0x00: return "Ethernet controller";
		default:   return "Unknown network controller";
		}
	case 0x03: return "Display controller";
	case 0x04:
		/* multimedia */
		switch ((dev_class >> 8) & 0xff) {
		case 0x00: return "Multimedia video device";
		case 0x01: return "Multimedia audio device";
		default:   return "Unknown multimedia controller";
		}
	case 0x05: return "Memory controller";
	case 0x06:
		/* bridge */
		switch ((dev_class >> 8) & 0xff) {
		case 0x00: return "Host bridge";
		case 0x01: return "PCI/ISA bridge";
		case 0x04: return "PCI/PCI bridge";
		case 0x05: return "PCI/PCMCIA bridge";
		case 0x07: return "PCI/Cardbus bridge";
		default:   return "Unknown bridge";
		}
	case 0x07:
		/* Simple communication */
		switch ((dev_class >> 8) & 0xff) {
		case 0x03: return "Modem";
		default:   return "Unknown simple communication controller";
		}
	case 0x0c:
		/* Serial bus */
		switch ((dev_class >> 8) & 0xff) {
		case 0x00: return "Firewire controller";
		case 0x03:
			/* USB controller */
			switch (dev_class & 0xff) {
			case 0x00: return "USB UHCI controller";
			case 0x10: return "USB OHCI controller";
			case 0x20: return "USB EHCI controller";
			default:   return "Unknown USB controller";
			}
		case 0x05: return "SMBus controller";
		default:   return "Unknown serial bus controller";
		}

	default: return "Unknown class";
	}
}

static void list_pcidevs()
{
	LOG_printf("PCI device list:\n");
	l4io_pdev_t start = 0;

	while (true) {
		l4io_pci_dev_t pci_dev;

		int err = l4io_pci_find_device(~0, ~0, start, &pci_dev);
		if (err) break;

		LOG_printf("  %02x:%02x.%x %s: %s\n",
		           pci_dev.bus, pci_dev.devfn >> 3, pci_dev.devfn & 0x7,
		           get_class_string(pci_dev.dev_class), pci_dev.name);

		start = pci_dev.handle;
	}
}


static void list_descriptors()
{
	LOG_printf("Device descriptor list:\n");
	l4io_desc_device_t *dev = l4io_desc_first_device(io_info_addr);

	while (dev) {
		LOG_printf("  device \"%s\" has %d resource descriptors:\n", dev->id, dev->num_resources);

		int i;
		for (i = 0; i < dev->num_resources; i++)
			LOG_printf("    %s: %08lx-%08lx\n",
			           dev->resources[i].type == L4IO_RESOURCE_IRQ ? " IRQ" :
			           dev->resources[i].type == L4IO_RESOURCE_MEM ? " MEM" :
			           dev->resources[i].type == L4IO_RESOURCE_PORT ? "PORT" : "????",
			           dev->resources[i].start, dev->resources[i].end);

		dev = l4io_desc_next_device(dev);
	}
}

int main(void)
{
	int error;
	l4io_drv_t drv = L4IO_DRV_INVALID;

	LOG("Hello World! io_dummy is up ...");

	if ((error = l4io_init(&io_info_addr, drv))) {
		LOG_Error("initalizing libio: %d (%s)", error, l4env_strerror(-error));
		return 1;
	}

	LOG("libio was initialized.");
	if (io_info_addr->omega0)
		LOG_printf("L4IO is running an omega0.\n");
	else
		LOG_printf("L4IO doesn't handle Interrupts.\n");

	list_pcidevs();
	list_descriptors();
	do_jiffies(13);

	return 0;
}
