/*****************************************************************************/
/**
 * \file   l4io/server/src/nopci.c
 * \brief  Dummy PCI implementation
 *
 * \date   2007-07-05
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/sys/types.h>
#include <l4/generic_io/types.h>
#include <l4/generic_io/generic_io-server.h>  /* IDL IPC interface */

/* local includes */
#include "pci.h"


/** Locate PCI device according to vendor and device id.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj  DICE corba object
 * \param  vendor_id        vendor id of PCI device
 * \param  device_id        device id of PCI device
 * \param  start_at         maybe start at this previously found device
 *
 * \retval pci_dev          PCI device found
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
long
l4_io_pci_find_device_component (CORBA_Object _dice_corba_obj,
                                 unsigned short vendor_id,
                                 unsigned short device_id,
                                 l4_io_pdev_t start_at,
                                 l4_io_pci_dev_t *pci_dev,
                                 CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_EINVAL;
}

/** Locate PCI device according to bus and slot info.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj  DICE corba object
 * \param  bus              bus number
 * \param  slot             slot number
 *
 * \retval pci_dev          PCI device found
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
long
l4_io_pci_find_slot_component (CORBA_Object _dice_corba_obj,
                               unsigned long bus,
                               unsigned long slot,
                               l4_io_pci_dev_t *pci_dev,
                               CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_EINVAL;
}

/** Locate PCI device according to device class id.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj  DICE corba object
 * \param  class_id         device class id of PCI device
 * \param  start_at         maybe start at this previously found device
 *
 * \retval pci_dev          PCI device found
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
long
l4_io_pci_find_class_component (CORBA_Object _dice_corba_obj,
                                unsigned long class_id,
                                l4_io_pdev_t start_at,
                                l4_io_pci_dev_t *pci_dev,
                                CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_EINVAL;
}

/** Read one byte of PCI configuration space.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj  DICE corba object
 * \param  pdev             PCI device
 * \param  offset           configuration register
 *
 * \retval val              content of configuration register
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
long
l4_io_pci_read_config_byte_component (CORBA_Object _dice_corba_obj,
                                      l4_io_pdev_t pdev,
                                      long offset,
                                      unsigned char *val,
                                      CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_EINVAL;
}

/** Read one word of PCI configuration space
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj  DICE corba object
 * \param  pdev             PCI device
 * \param  offset           configuration register
 *
 * \retval val              content of configuration register
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
long
l4_io_pci_read_config_word_component (CORBA_Object _dice_corba_obj,
                                      l4_io_pdev_t pdev,
                                      long offset,
                                      unsigned short *val,
                                      CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_EINVAL;
}

/** Read one double word of PCI configuration space.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj  DICE corba object
 * \param  pdev             PCI device
 * \param  offset           configuration register
 *
 * \retval val              content of configuration register
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
long
l4_io_pci_read_config_dword_component (CORBA_Object _dice_corba_obj,
                                       l4_io_pdev_t pdev,
                                       long offset,
                                       l4_uint32_t *val,
                                       CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_EINVAL;
}

/** Write one byte into PCI configuration space.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj  DICE corba object
 * \param  pdev             PCI device
 * \param  offset           configuration register
 * \param  val              configuration register modifier
 *
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
long
l4_io_pci_write_config_byte_component (CORBA_Object _dice_corba_obj,
                                       l4_io_pdev_t pdev,
                                       long offset,
                                       unsigned char val,
                                       CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_EINVAL;
}

/** Write one word into PCI configuration space.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj  DICE corba object
 * \param  pdev             PCI device
 * \param  offset           configuration register
 * \param  val              configuration register modifier
 *
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
long
l4_io_pci_write_config_word_component (CORBA_Object _dice_corba_obj,
                                       l4_io_pdev_t pdev,
                                       long offset,
                                       unsigned short val,
                                       CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_EINVAL;
}

/** Write one double word into PCI configuration space.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj  DICE corba object
 * \param  pdev             PCI device
 * \param  offset           configuration register
 * \param  val              configuration register modifier
 *
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
long
l4_io_pci_write_config_dword_component (CORBA_Object _dice_corba_obj,
                                        l4_io_pdev_t pdev,
                                        long offset,
                                        unsigned long val,
                                        CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_EINVAL;
}

/** Initialize device before it's used by a driver.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj  DICE corba object
 * \param  pdev             PCI device to be initialized
 *
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
long
l4_io_pci_enable_device_component (CORBA_Object _dice_corba_obj,
                                   l4_io_pdev_t pdev,
                                   CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_EINVAL;
}

/** Finalize device after use.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj  DICE corba object
 * \param  pdev             PCI device
 *
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
long
l4_io_pci_disable_device_component (CORBA_Object _dice_corba_obj,
                                    l4_io_pdev_t pdev,
                                    CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_EINVAL;
}

/** Enable Busmastering for PCI device.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj  DICE corba object
 * \param  pdev             PCI device to be initialized
 *
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
long
l4_io_pci_set_master_component (CORBA_Object _dice_corba_obj,
                                l4_io_pdev_t pdev,
                                CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_EINVAL;
}

/** Set power management state for PCI device.
 * \ingroup grp_pci
 *
 * \param  _dice_corba_obj  DICE corba object
 * \param  pdev             PCI device to be initialized
 * \param  state            new PM state (0 == D0, 3 == D3, etc.)
 *
 * \retval state            old PM state
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 */
long
l4_io_pci_set_power_state_component (CORBA_Object _dice_corba_obj,
                                     l4_io_pdev_t pdev,
                                     int *state,
                                     CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_EINVAL;
}

/** PCI module initialization.
 * \ingroup grp_pci
 *
 * \return 0 on success, negative error code otherwise
 *
 * Initialize PCIlib and setup base addresses.
 */
int io_pci_init(int list)
{
  /* nothing special */
  return 0;
}
