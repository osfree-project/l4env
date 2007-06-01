/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#include <linux/netdevice.h>

#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/generic_io/libio.h>
#include <l4/dde_linux/dde.h>

#include <linuxemul.h>
#include <config.h>

extern int net_dev_init(void);
extern int loopback_only;

int init_emulation(long memsize)
{
  int err;

  // vars and inits
  l4io_info_t *io_info = NULL;

  LOGd(ORE_EMUL_DEBUG, "init l4io");
  // initialize L4io
  err = l4io_init(&io_info, L4IO_DRV_INVALID);
  if (err)
    {
      LOG_Error("io_init: %d (%s)", err, l4env_strerror(-err));
      return err;
    }

  LOGd(ORE_EMUL_DEBUG, "init_memsize");
  // init DDE memory
  err = l4dde_mm_init(memsize, memsize);
  if (err)
    {
      LOG_Error("dde_mm_init: %d (%s)", err, l4env_strerror(-err));
      return err;
    }

  LOGd(ORE_EMUL_DEBUG, "init dde process");
  err = l4dde_process_init();
  if (err)
    {
      LOG_Error("dde_process_init: %d (%s)", err, l4env_strerror(-err));
      return err;
    }

  LOGd(ORE_EMUL_DEBUG, "init dde time");
  err = l4dde_time_init();
  if (err)
    {
      LOG_Error("dde_time_init: %d (%s)", err, l4env_strerror(-err));
      return err;
    }

  LOGd(ORE_EMUL_DEBUG, "init soft irqs");
  err = l4dde_softirq_init();
  if (err)
    {
      LOG_Error("softirq_init: %d (%s)", err, l4env_strerror(-err));
      return err;
    }

  LOGd(ORE_EMUL_DEBUG, "init irqs");
  err = l4dde_irq_init();
  if (err)
    {
      LOG_Error("irq_init: %d (%s)", err, l4env_strerror(-err));
      return err;
    }

  LOGd(ORE_EMUL_DEBUG, "pci_init");
  err = l4dde_pci_init();
#if 0
  if (err)
    {
      LOG_Error("pci_init: %d (%s)", err, l4env_strerror(-err));
      return err;
    }
#endif

  LOGd(ORE_EMUL_DEBUG, "initialized DDE");

  skb_init();

  net_dev_init();

  if (!loopback_only)
      l4dde_do_initcalls();

  return 0;
}
