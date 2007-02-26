/****************************************************************
 * ORe network device setup and utilities.                      *
 *                                                              *
 * Bjoern Doebel <doebel@os.inf.tu-dresden.de>                  *
 * 2005-08-10                                                   *
 ****************************************************************/

#include <linux/netdevice.h>

#include <l4/sys/types.h>
#include <l4/util/l4_macros.h>
#include <l4/rmgr/librmgr.h>
#include <l4/dde_linux/dde.h>
#include <l4/names/libnames.h>

#include "ore-local.h"

/*****************************************************************************
 * List all available network devices.
 *****************************************************************************/
l4_int32_t list_network_devices(void)
{
  struct net_device *dev;
  char mesg[25];

  LOGd_Enter(ORE_DEBUG_INIT);

  for (dev = dev_base; dev; dev = dev->next)
    {
        sprintf(mesg, "Device = %4s, MAC = ", dev->name);
        LOG_MAC_s(1, mesg, dev->dev_addr);
/*      LOG("Device: %4s, MAC=%02x:%02x:%02x:%02x:%02x:%02x, base=%u, irq %d",
          dev->name, dev->dev_addr[0], dev->dev_addr[1], dev->dev_addr[2],
          dev->dev_addr[3], dev->dev_addr[4], dev->dev_addr[5],
          (l4_uint32_t)dev->base_addr, dev->irq);
          */
    }

  return 0;
}

/*****************************************************************************
 * Open the "real" network devices. Return the number of opened interfaces.
 *****************************************************************************/
l4_int32_t open_network_devices(void)
{
  struct net_device *dev;
  l4_int32_t err, cnt = 0;

  LOGd_Enter(ORE_DEBUG_INIT);

  for (dev = dev_base; dev; dev=dev->next)
    {
      LOGd(ORE_DEBUG_INIT, "opening %s", dev->name);

      // beam us to promiscuous mode, so that we can receive packets that
      // are not meant for the NIC's MAC address --> we need that, because
      // ORe clients have different MAC addresses
      if ((err = dev_change_flags(dev, dev->flags | IFF_PROMISC)) != 0)
        {
          LOGd(ORE_DEBUG_INIT, "%s could not be set to promiscuous mode.",
              dev->name);
        }
      else
        {
          LOGd(ORE_DEBUG_INIT, "set interface to promiscuous mode: %d", err);
        }

      err = dev_open(dev);
      if (err)
        {
          LOGl("error opening %s : %d (%s)", dev->name, err,
               l4env_strerror(-err));
          return err;
        }

      cnt++;

      LOGd(ORE_DEBUG_INIT,"attaching irq");
      if (dev->irq)
        {
          err = l4dde_irq_set_prio(dev->irq, IRQ_HANDLER_PRIO);
          if (err)
            {
              LOG_Error("cannot set priority of IRQ %d to %d",
                        dev->irq, IRQ_HANDLER_PRIO);
              return err;
            }

          err = l4dde_set_deferred_irq_handler(dev->irq, irq_handler, 0);
          if (err)
            {
              LOG_Error("set irq_handler failed: %d (%s)",
                        err, l4env_strerror(-err));
              return err;
            }
        }
    }

  return cnt;
}
