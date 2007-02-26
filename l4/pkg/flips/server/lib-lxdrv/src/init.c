/* L4 */
#include <l4/util/macros.h>
#include <l4/log/l4log.h>
#include <l4/generic_io/libio.h>
#include <l4/dde_linux/dde.h>

/* local */
#include "liblinux.h"
#include "liblxdrv.h"

int liblxdrv_init()
{
	int err;

	l4io_info_t *l4io_page = (l4io_info_t *)liblinux_get_l4io_info();
	Assert(l4io_page);

	if ((err = l4dde_pci_init())) {
		LOG_Error("initializing DDE_LINUX pci (%d)", err);
		return err;
	}
	if ((err = l4dde_irq_init(l4io_page->omega0))) {
		LOG_Error("initializing DDE_LINUX irq (%d)", err);
		return err;
	}

	return 0;
}
