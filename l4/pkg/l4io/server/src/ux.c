#include <l4/sys/vhw.h>
#include <l4/util/macros.h>
#include <l4/sigma0/kip.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>

#include "io.h"
#include "res.h"


int io_ux_init()
{
	l4_kernel_info_t *kip = l4sigma0_kip();
	if (!kip) return -L4_EUNKNOWN;

	struct l4_vhw_descriptor *vhw = l4_vhw_get(kip);
	if (!vhw) return -L4_EUNKNOWN;

	struct vhw_type
	{
		unsigned    id;
		const char *name;
	} ux_hw[] =
	{
		{ L4_TYPE_VHW_NET,         "netdev  " },
		{ L4_TYPE_VHW_FRAMEBUFFER, "gfxdev  " },
		{ L4_TYPE_VHW_INPUT,       "inputdev" },
		{ L4_TYPE_VHW_NONE,        "none" }
	};

	struct vhw_type *vhwt;
	for (vhwt = ux_hw; vhwt->id != L4_TYPE_VHW_NONE; vhwt++) {
		unsigned long base, size;
		unsigned prov, irq, fd;
		struct l4_vhw_entry *entry = l4_vhw_get_entry_type(vhw, vhwt->id);
		if (entry) {
			base = entry->mem_start;
			size = entry->mem_size;
			prov = entry->provider_pid;
			irq  = entry->irq_no;
			fd   = entry->fd;

			LOG_printf("Fiasco-UX %s is ["l4_addr_fmt","l4_addr_fmt") (prov=%d,irq=%d,fd=%d)\n",
			           vhwt->name, base, base + size, prov, irq, fd);

			if (size) callback_announce_mem_region(base, size);
		}
	}

	return 0;
}
