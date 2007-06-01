/* L4 */
#include <l4/util/macros.h>
#include <l4/log/l4log.h>
#include <l4/dde_linux/dde.h>

/* local */
#include "liblinux.h"
#include "liblxdrv.h"

int liblxdrv_init()
{
	int err;

	if ((err = l4dde_pci_init())) {
		LOG_Error("initializing DDE_LINUX pci (%d)", err);
		return err;
	}
	if ((err = l4dde_irq_init())) {
		LOG_Error("initializing DDE_LINUX irq (%d)", err);
		return err;
	}

	return 0;
}
