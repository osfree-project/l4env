/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4io/server/src/pci.c
 * \brief  L4Env l4io I/O Server PCI module (wrapper to PCIlib)
 *
 * \date   05/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/sys/types.h>
#include <l4/generic_io/types.h>
#include <l4/generic_io/generic_io-server.h>	/* IDL IPC interface */

/* local includes */
#include "io.h"
#include "pci.h"
#include "pcilib.h"
#include "pcilib_pci.h"
#include "__config.h"
#include "__macros.h"

/** helper */
static inline void * _pci_get_addr(l4_io_pdev_t pdev)
{
  return pci_find_slot(pdev>>8, pdev&0xff);
}

/** \name Find PCI Devices (IPC interface)
 *
 * Locate PCI devices based on their properties.
 *
 * We use busno:devfn as PCI device handle. 
 *
 * @{ */

/** Locate PCI device according to vendor and device id.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj	DICE corba object
 * \param  vendor_id		vendor id of PCI device
 * \param  device_id		device id of PCI device
 * \param  start_at		maybe start at this previously found device
 *
 * \retval pci_dev		PCI device found
 * \retval _dice_corba_env	corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
l4_int32_t l4_io_pci_find_device_component(CORBA_Object _dice_corba_obj,
                                          l4_uint16_t vendor_id,
                                          l4_uint16_t device_id,
                                          l4_io_pdev_t start_at,
                                          l4_io_pci_dev_t *pci_dev,
                                          CORBA_Environment *_dice_corba_env)
{
  void *from, *pdev;
  unsigned int vendor = vendor_id;
  unsigned int device = device_id;

  if (vendor == 0xffff)
    vendor = ~0;
  if (device == 0xffff)
    device = ~0;

#if DEBUG_PCI
  DMSG("find [%04x/%04x] start at %x:%02x.%x ... ",
       vendor_id, device_id, start_at>>8, (start_at&0xff)>>3, start_at&7);
#endif

  if (!(from = _pci_get_addr(start_at)))
    return -L4_EINVAL;

  pdev = pci_find_device(vendor, device, from);

  if (!pdev)
    {
#if DEBUG_PCI
      DMSG("unsuccessfull\n");
#endif
      return -L4_ENOTFOUND;
    }

  memset(pci_dev, 0, sizeof(l4_io_pci_dev_t));
  pci_dev->handle = PCI_linux_to_io(pdev, (void *) pci_dev);

#if DEBUG_PCI
  printf("successfull (pdev=%x:%02x.%x)\n",
	 pci_dev->handle>>8, (pci_dev->handle&0xff)>>3, pci_dev->handle&7);
#endif

  return 0;
}

/** Locate PCI device according to bus and slot info.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj	DICE corba object
 * \param  bus			bus number
 * \param  slot			slot number
 *
 * \retval pci_dev		PCI device found
 * \retval _dice_corba_env	corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
l4_int32_t l4_io_pci_find_slot_component(CORBA_Object _dice_corba_obj,
                                         l4_uint32_t bus,
                                         l4_uint32_t slot,
                                         l4_io_pci_dev_t *pci_dev,
                                         CORBA_Environment *_dice_corba_env)
{
  void *pdev;

#if DEBUG_PCI
  DMSG("find %02x:%02x ... ", bus, slot);
#endif

  pdev = pci_find_slot(bus, slot);

  if (!pdev)
    {
#if DEBUG_PCI
      DMSG("unsuccessfull\n");
#endif
      return -L4_ENOTFOUND;
    }

  memset(pci_dev, 0, sizeof(l4_io_pci_dev_t));
  pci_dev->handle = PCI_linux_to_io(pdev, (void *) pci_dev);

#if DEBUG_PCI
  DMSG("successfull (pdev=%x:%02x.%x)\n",
       pci_dev->handle>>8, (pci_dev->handle&0xff)>>3, pci_dev->handle&7);
#endif

  return 0;
}

/** Locate PCI device according to device class id.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj	DICE corba object
 * \param  class_id		device class id of PCI device
 * \param  start_at		maybe start at this previously found device
 *
 * \retval pci_dev		PCI device found
 * \retval _dice_corba_env	corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
l4_int32_t l4_io_pci_find_class_component(CORBA_Object _dice_corba_obj,
                                          l4_uint32_t class_id,
                                          l4_io_pdev_t start_at,
                                          l4_io_pci_dev_t *pci_dev,
                                          CORBA_Environment *_dice_corba_env)
{
  void *from, *pdev;
  unsigned int class = class_id;

  class &= 0x00ffffff;

#if DEBUG_PCI
  DMSG("find <%06x>  start at %x:%02x.%x ... ",
       class_id, start_at>>8, (start_at&0xff)>>3, start_at&7);
#endif

  if (!(from =  _pci_get_addr(start_at)))
    return -L4_EINVAL;

  pdev = pci_find_class(class, from);

  if (!pdev)
    {
#if DEBUG_PCI
      DMSG("unsuccessfull\n");
#endif
      return -L4_ENOTFOUND;
    }

  memset(pci_dev, 0, sizeof(l4_io_pci_dev_t));
  pci_dev->handle = PCI_linux_to_io(pdev, (void *) pci_dev);

#if DEBUG_PCI
  DMSG("successfull (pdev=%x:%02x.%x)\n",
       pci_dev->handle>>8, (pci_dev->handle&0xff)>>3, pci_dev->handle&7);
#endif

  return 0;
}
/** @} */
/** \name Read PCI Configuration Space (IPC interface)
 *
 * Read the PCI configuration space portions (byte, word, dword).
 * @{ */

/** Read one byte of PCI configuration space.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj	DICE corba object
 * \param  pdev			PCI device
 * \param  offset		configuration register
 *
 * \retval val			content of configuration register
 * \retval _dice_corba_env	corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
l4_int32_t l4_io_pci_read_config_byte_component(CORBA_Object _dice_corba_obj,
                                                l4_io_pdev_t pdev,
                                                l4_int32_t offset,
                                                l4_uint8_t *val,
                                                CORBA_Environment *_dice_corba_env)
{
  void *pci_dev =  _pci_get_addr(pdev);

  if (!pci_dev)
    return -L4_EINVAL;

#if DEBUG_PCI_RW
  DMSG("read %x:%02x.%x byte %d\n", pdev>>8, (pdev&0xff)>>3, pdev&7, offset);
#endif

  return pci_read_config_byte(pci_dev, offset, val);
}

/** Read one word of PCI configuration space
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj	DICE corba object
 * \param  pdev			PCI device
 * \param  offset		configuration register
 *
 * \retval val			content of configuration register
 * \retval _dice_corba_env	corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
l4_int32_t l4_io_pci_read_config_word_component(CORBA_Object _dice_corba_obj,
                                                l4_io_pdev_t pdev,
                                                l4_int32_t offset,
                                                l4_uint16_t *val,
                                                CORBA_Environment *_dice_corba_env)
{
  void *pci_dev =  _pci_get_addr(pdev);

  if (!pci_dev)
    return -L4_EINVAL;

#if DEBUG_PCI_RW
  DMSG("read %x:%02x.%x word %d\n", pdev>>8, (pdev&0xff)>>3, pdev&7, offset);
#endif

  return pci_read_config_word(pci_dev, offset, val);
}

/** Read one double word of PCI configuration space.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj	DICE corba object
 * \param  pdev			PCI device
 * \param  offset		configuration register
 *
 * \retval val			content of configuration register
 * \retval _dice_corba_env	corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
l4_int32_t l4_io_pci_read_config_dword_component(CORBA_Object _dice_corba_obj,
                                                 l4_io_pdev_t pdev,
                                                 l4_int32_t offset,
                                                 l4_uint32_t *val,
                                                 CORBA_Environment *_dice_corba_env)
{
  void *pci_dev =  _pci_get_addr(pdev);

  if (!pci_dev)
    return -L4_EINVAL;

#if DEBUG_PCI_RW
  DMSG("read %x:%02x.%x dword %d\n", pdev>>8, (pdev&0xff)>>3, pdev&7, offset);
#endif

  return pci_read_config_dword(pci_dev, offset, val);
}
/** @} */
/** \name Modify PCI Configuration Space (IPC interface)
 *
 * Write portions of PCI configuration space (bytes, word, dwords).
 *
 * \todo Maybe check for well-known region that should _not_ be
 * altered. Implementation.
 *
 * @{ */

/** Write one byte into PCI configuration space.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj	DICE corba object
 * \param  pdev			PCI device
 * \param  offset		configuration register
 * \param  val			configuration register modifier
 *
 * \retval _dice_corba_env	corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
l4_int32_t l4_io_pci_write_config_byte_component(CORBA_Object _dice_corba_obj,
                                                 l4_io_pdev_t pdev,
                                                 l4_int32_t offset,
                                                 l4_uint8_t val,
                                                 CORBA_Environment *_dice_corba_env)
{
  void *pci_dev =  _pci_get_addr(pdev);

  if (!pci_dev)
    return -L4_EINVAL;

#if DEBUG_PCI_RW
  DMSG("write %x:%02x.%x byte %d (val 0x%02x)\n",
       pdev>>8, (pdev&0xff)>>3, pdev&7, offset, val);
#endif

  if (pci_write_config_byte(pci_dev, offset, val))
    {
#if DEBUG_PCI_RW
      ERROR("writing PCI configuration register");
#endif
      return -L4_EUNKNOWN;
    }

  return 0;
}

/** Write one word into PCI configuration space.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj	DICE corba object
 * \param  pdev			PCI device
 * \param  offset		configuration register
 * \param  val			configuration register modifier
 *
 * \retval _dice_corba_env	corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
l4_int32_t l4_io_pci_write_config_word_component(CORBA_Object _dice_corba_obj,
                                                 l4_io_pdev_t pdev,
                                                 l4_int32_t offset,
                                                 l4_uint16_t val,
                                                 CORBA_Environment *_dice_corba_env)
{
  void *pci_dev =  _pci_get_addr(pdev);

  if (!pci_dev)
    return -L4_EINVAL;

#if DEBUG_PCI_RW
  DMSG("write %x:%02x.%x word %d (val 0x%02x)\n",
       pdev>>8, (pdev&0xff)>>3, pdev&7, offset, val);
#endif

  if (pci_write_config_word(pci_dev, offset, val))
    {
#if DEBUG_PCI_RW
      ERROR("writing PCI configuration register");
#endif
      return -L4_EUNKNOWN;
    }

  return 0;
}

/** Write one double word into PCI configuration space.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj	DICE corba object
 * \param  pdev			PCI device
 * \param  offset		configuration register
 * \param  val			configuration register modifier
 *
 * \retval _dice_corba_env	corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
l4_int32_t l4_io_pci_write_config_dword_component(CORBA_Object _dice_corba_obj,
                                                  l4_io_pdev_t pdev,
                                                  l4_int32_t offset,
                                                  l4_uint32_t val,
                                                  CORBA_Environment *_dice_corba_env)
{
  void *pci_dev =  _pci_get_addr(pdev);

  if (!pci_dev)
    return -L4_EINVAL;

#if DEBUG_PCI_RW
  DMSG("write %x:%02x.%x dword %d (val 0x%02x)\n",
       pdev>>8, (pdev&0xff)>>3, pdev&7, offset, val);
#endif

  if (pci_write_config_dword(pci_dev, offset, val))
    {
#if DEBUG_PCI_RW
      ERROR("writing PCI configuration register");
#endif
      return -L4_EUNKNOWN;
    }

  return 0;
}
/** @} */
/** \name PCI Device Initialization (IPC interface)
 *
 * Activation, Busmastering and Power Management (PM).
 *
 * @{ */

/** Initialize device before it's used by a driver.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj	DICE corba object
 * \param  pdev			PCI device to be initialized
 *
 * \retval _dice_corba_env	corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
l4_int32_t l4_io_pci_enable_device_component(CORBA_Object _dice_corba_obj,
                                             l4_io_pdev_t pdev,
                                             CORBA_Environment *_dice_corba_env)
{
  int err;
  void *pci_dev =  _pci_get_addr(pdev);

  if (!pci_dev)
    return -L4_EINVAL;

  err = pci_enable_device(pci_dev);

  if (err)
    {
#if DEBUG_PCI
      DMSG("enable %x:%02x.%x unsuccessfull (err=%d)\n",
	   pdev>>8, (pdev&0xff)>>3, pdev&7, err);
#endif
      return err;
    }

#if DEBUG_PCI
  DMSG("enable %x:%02x.%x successfull\n", pdev>>8, (pdev&0xff)>>3, pdev&7);
#endif

  return 0;
}

/** Finalize device after use.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj	DICE corba object
 * \param  pdev			PCI device
 *
 * \retval _dice_corba_env	corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
l4_int32_t l4_io_pci_disable_device_component(CORBA_Object _dice_corba_obj,
                                              l4_io_pdev_t pdev,
                                              CORBA_Environment *_dice_corba_env)
{
  void *pci_dev =  _pci_get_addr(pdev);

  if (!pci_dev)
    return -L4_EINVAL;

  pci_disable_device(pci_dev);

#if DEBUG_PCI
  DMSG("disable %x:%02x.%x successfull\n", pdev>>8, (pdev&0xff)>>3, pdev&7);
#endif

  return 0;
}

/** Enable Busmastering for PCI device.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj	DICE corba object
 * \param  pdev			PCI device to be initialized
 *
 * \retval _dice_corba_env	corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
l4_int32_t l4_io_pci_set_master_component(CORBA_Object _dice_corba_obj,
                                          l4_io_pdev_t pdev,
                                          CORBA_Environment *_dice_corba_env)
{
  void *pci_dev =  _pci_get_addr(pdev);

  if (!pci_dev)
    return -L4_EINVAL;

  pci_set_master(pci_dev);

#if DEBUG_PCI
  DMSG("busmastering for %x:%02x.%x enabled\n",
       pdev>>8, (pdev&0xff)>>3, pdev&7);
#endif

  return 0;
}

/** Set power management state for PCI device.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj	DICE corba object
 * \param  pdev			PCI device to be initialized
 * \param  state		new PM state (0 == D0, 3 == D3, etc.)
 *
 * \retval state		old PM state
 * \retval _dice_corba_env	corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
l4_int32_t l4_io_pci_set_power_state_component(CORBA_Object _dice_corba_obj,
                                               l4_io_pdev_t pdev,
                                               l4_int32_t *state,
                                               CORBA_Environment *_dice_corba_env)
{
  int old;
  void *pci_dev =  _pci_get_addr(pdev);

  if (!pci_dev)
    return -L4_EINVAL;

  old = pci_set_power_state(pci_dev, *state);

#if DEBUG_PCI
  DMSG("%x:%02x.%x goes from %d to %d PM\n",
       pdev>>8, (pdev&0xff)>>3, pdev&7, old, *state);
#endif

  *state = old;

  return 0;
}
/** @} */
/** PCI module initialization.
 * \ingroup grp_pci
 *
 * \return 0 on success, negative error code otherwise
 *
 * Initialize PCIlib and setup base addresses.
 */
int io_pci_init()
{
  return PCI_init();
}
