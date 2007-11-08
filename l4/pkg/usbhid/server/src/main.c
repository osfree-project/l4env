#include <l4/util/util.h>
#include <l4/util/macros.h>
#include <l4/log/l4log.h>

#include <l4/generic_io/libio.h>
#include <l4/dde_linux/dde.h>
#include <l4/dde_linux/ctor.h>

#include <l4/sys/ipc.h>
#include <l4/input/libinput.h>
#include <l4/names/libnames.h>

#include "internal.h"

static int dde_init(unsigned int vmem, unsigned int kmem)
{
	int err;
	static l4io_info_t *io_info_addr;

	l4io_init(&io_info_addr, L4IO_DRV_INVALID);
	Assert(io_info_addr);

	if ((err = l4dde_mm_init(vmem, kmem))) {
		LOG_Error("initializing DDE_LINUX mm (%d)", err);
		return err;
	}

	if ((err = l4dde_process_init())) {
		LOG_Error("initializing DDE_LINUX process-level (%d)", err);
		return err;
	}

	if ((err = l4dde_irq_init())) {
		LOG_Error("initializing DDE_LINUX irqs (%d)", err);
		return err;
	}

	if ((err = l4dde_softirq_init())) {
		LOG_Error("initializing DDE_LINUX softirqs (%d)", err);
		return err;
	}

	if ((err = l4dde_time_init())) {
		LOG_Error("initializing DDE_LINUX time (%d)", err);
		return err;
	}
	if ((err = l4dde_pci_init())) {
		LOG_Error("initializing DDE_LINUX pci (%d)", err);
		return err;
	}
	return 0;
}

/* FIXME proxy name and protocol hard-coded here and in pkg/input. */
static l4_threadid_t l4i_proxy = L4_INVALID_ID;

void callback(struct l4input *ev)
{
	if (l4_is_invalid_id(l4i_proxy)) return;

	l4_umword_t d0, d1;
	l4_msgdope_t res;

	d0 = ev->type | (ev->code << 16);
	d1 = ev->value;

	int error;
	static unsigned alex_drop_counter = 0;

	error = l4_ipc_send(l4i_proxy, 0, d0, d1, l4_ipc_timeout(1000,0,0,1), &res);
	if (error) alex_drop_counter++;
}


int main(int argc, char **argv)
{
	Assert(!dde_init(16*1024, 128*1024));
	l4dde_do_initcalls();
	
	/* FIXME long timeout */
	names_waitfor_name("l4i_proxy", &l4i_proxy, 20000);
	if (l4_is_invalid_id(l4i_proxy))
	  LOG("Did not find l4i_proxy");
	else
	  LOG("found l4i_proxy at %x.%x", l4i_proxy.id.task,
	      l4i_proxy.id.lthread);

	Assert(!l4input_internal_l4evdev_init(&callback));

	LOG("I'm going to sleep...");
	l4_sleep_forever();

	return 0;
}
