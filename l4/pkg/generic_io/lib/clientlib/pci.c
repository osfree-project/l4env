/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_io/lib/clientlib/pci.c
 * \brief  L4Env I/O Client Library PCI Support Wrapper
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
#include <l4/sys/types.h>
#include <l4/env/errno.h>

/* io includes */
#include <l4/generic_io/libio.h>

/* local */
#include "internal.h"
#include "__macros.h"

/*****************************************************************************/
/**
 * \name One-to-one Mapping of PCI library to IDL functions w/ error handling
 *
 * @{ */
/*****************************************************************************/
int l4io_pci_find_slot(unsigned int bus, unsigned int slot, l4io_pci_dev_t * pci_dev)
{
  int err;
  CORBA_Environment _env = dice_default_environment;

  err = l4_io_pci_find_slot_call(&io_l4id, bus, slot, (l4_io_pci_dev_t *) pci_dev, &_env);

  /* done */
  return DICE_ERR(err, &_env);
}

int l4io_pci_find_device(unsigned short vendor, unsigned short device,
                         l4io_pdev_t start, l4io_pci_dev_t * pci_dev)
{
  int err;
  CORBA_Environment _env = dice_default_environment;

  err = l4_io_pci_find_device_call(&io_l4id, vendor, device, start,
                                   (l4_io_pci_dev_t *) pci_dev, &_env);

  /* done */
  return DICE_ERR(err, &_env);
}

int l4io_pci_find_class(unsigned long class,
                        l4io_pdev_t start, l4io_pci_dev_t * pci_dev)
{
  int err;
  CORBA_Environment _env = dice_default_environment;

  err = l4_io_pci_find_class_call(&io_l4id, class, start, (l4_io_pci_dev_t *)pci_dev, &_env);

  /* done */
  return DICE_ERR(err, &_env);
}

int l4io_pci_readb_cfg(l4io_pdev_t pdev, int pos, l4_uint8_t * val)
{
  int err;
  CORBA_Environment _env = dice_default_environment;

  err = l4_io_pci_read_config_byte_call(&io_l4id, pdev, pos, val, &_env);

  /* done */
  return DICE_ERR(err, &_env);
}
int l4io_pci_readw_cfg(l4io_pdev_t pdev, int pos, l4_uint16_t * val)
{
  int err;
  CORBA_Environment _env = dice_default_environment;

  err = l4_io_pci_read_config_word_call(&io_l4id, pdev, pos, val, &_env);

  /* done */
  return DICE_ERR(err, &_env);
}
int l4io_pci_readl_cfg(l4io_pdev_t pdev, int pos, l4_uint32_t * val)
{
  int err;
  CORBA_Environment _env = dice_default_environment;

  err = l4_io_pci_read_config_dword_call(&io_l4id, pdev, pos, val, &_env);

  /* done */
  return DICE_ERR(err, &_env);
}

int l4io_pci_writeb_cfg(l4io_pdev_t pdev, int pos, l4_uint8_t val)
{
  int err;
  CORBA_Environment _env = dice_default_environment;

  err = l4_io_pci_write_config_byte_call(&io_l4id, pdev, pos, val, &_env);

  /* done */
  return DICE_ERR(err, &_env);
}
int l4io_pci_writew_cfg(l4io_pdev_t pdev, int pos, l4_uint16_t val)
{
  int err;
  CORBA_Environment _env = dice_default_environment;

  err = l4_io_pci_write_config_word_call(&io_l4id, pdev, pos, val, &_env);

  /* done */
  return DICE_ERR(err, &_env);
}
int l4io_pci_writel_cfg(l4io_pdev_t pdev, int pos, l4_uint32_t val)
{
  int err;
  CORBA_Environment _env = dice_default_environment;

  err = l4_io_pci_write_config_dword_call(&io_l4id, pdev, pos, val, &_env);

  /* done */
  return DICE_ERR(err, &_env);
}

int l4io_pci_enable(l4io_pdev_t pdev)
{
  int err;
  CORBA_Environment _env = dice_default_environment;

  err = l4_io_pci_enable_device_call(&io_l4id, pdev, &_env);

  /* done */
  return DICE_ERR(err, &_env);
}

int l4io_pci_disable(l4io_pdev_t pdev)
{
  int err;
  CORBA_Environment _env = dice_default_environment;

  err = l4_io_pci_disable_device_call(&io_l4id, pdev, &_env);

  /* done */
  return DICE_ERR(err, &_env);
}

void l4io_pci_set_master(l4io_pdev_t pdev)
{
  int err;
  CORBA_Environment _env = dice_default_environment;

  err = l4_io_pci_set_master_call(&io_l4id, pdev, &_env);

  /* done */
  DICE_ERR(err, &_env);
}

int l4io_pci_set_pm(l4io_pdev_t pdev, int state)
{
  int err;
  int old = state;
  CORBA_Environment _env = dice_default_environment;

  err = l4_io_pci_set_power_state_call(&io_l4id, pdev, &old, &_env);

  /* done */
  DICE_ERR(err, &_env);

  return old;
}
/** @} */
