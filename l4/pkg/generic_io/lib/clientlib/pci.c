/* $Id$ */
/*****************************************************************************/
/**
 * \file	generic_io/lib/src/pci.c
 *
 * \brief	L4Env I/O Client Library PCI Support Wrapper
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/

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
  sm_exc_t _exc;

  err = l4_io_pci_find_slot(io_l4id, bus, slot, (l4_io_pci_dev_t *) pci_dev, &_exc);

  /* done */
  return FLICK_ERR(err, &_exc);
}

int l4io_pci_find_device(unsigned short vendor, unsigned short device,
			 l4io_pdev_t start, l4io_pci_dev_t * pci_dev)
{
  int err;
  sm_exc_t _exc;

  err = l4_io_pci_find_device(io_l4id, vendor, device, start,
			      (l4_io_pci_dev_t *) pci_dev, &_exc);

  /* done */
  return FLICK_ERR(err, &_exc);
}

int l4io_pci_find_class(unsigned long class,
			l4io_pdev_t start, l4io_pci_dev_t * pci_dev)
{
  int err;
  sm_exc_t _exc;

  err = l4_io_pci_find_class(io_l4id, class, start, (l4_io_pci_dev_t *)pci_dev, &_exc);

  /* done */
  return FLICK_ERR(err, &_exc);
}

int l4io_pci_readb_cfg(l4io_pdev_t pdev, int pos, l4_uint8_t * val)
{
  int err;
  sm_exc_t _exc;

  err = l4_io_pci_read_config_byte(io_l4id, pdev, pos, val, &_exc);

  /* done */
  return FLICK_ERR(err, &_exc);
}
int l4io_pci_readw_cfg(l4io_pdev_t pdev, int pos, l4_uint16_t * val)
{
  int err;
  sm_exc_t _exc;

  err = l4_io_pci_read_config_word(io_l4id, pdev, pos, val, &_exc);

  /* done */
  return FLICK_ERR(err, &_exc);
}
int l4io_pci_readl_cfg(l4io_pdev_t pdev, int pos, l4_uint32_t * val)
{
  int err;
  sm_exc_t _exc;

  err = l4_io_pci_read_config_dword(io_l4id, pdev, pos, val, &_exc);

  /* done */
  return FLICK_ERR(err, &_exc);
}

int l4io_pci_writeb_cfg(l4io_pdev_t pdev, int pos, l4_uint8_t val)
{
  int err;
  sm_exc_t _exc;

  err = l4_io_pci_write_config_byte(io_l4id, pdev, pos, val, &_exc);

  /* done */
  return FLICK_ERR(err, &_exc);
}
int l4io_pci_writew_cfg(l4io_pdev_t pdev, int pos, l4_uint16_t val)
{
  int err;
  sm_exc_t _exc;

  err = l4_io_pci_write_config_word(io_l4id, pdev, pos, val, &_exc);

  /* done */
  return FLICK_ERR(err, &_exc);
}
int l4io_pci_writel_cfg(l4io_pdev_t pdev, int pos, l4_uint32_t val)
{
  int err;
  sm_exc_t _exc;

  err = l4_io_pci_write_config_dword(io_l4id, pdev, pos, val, &_exc);

  /* done */
  return FLICK_ERR(err, &_exc);
}

int l4io_pci_enable(l4io_pdev_t pdev)
{
  int err;
  sm_exc_t _exc;

  err = l4_io_pci_enable_device(io_l4id, pdev, &_exc);

  /* done */
  return FLICK_ERR(err, &_exc);
}

int l4io_pci_disable(l4io_pdev_t pdev)
{
  int err;
  sm_exc_t _exc;

  err = l4_io_pci_disable_device(io_l4id, pdev, &_exc);

  /* done */
  return FLICK_ERR(err, &_exc);
}

void l4io_pci_set_master(l4io_pdev_t pdev)
{
  int err;
  sm_exc_t _exc;

  err = l4_io_pci_set_master(io_l4id, pdev, &_exc);

  /* done */
  FLICK_ERR(err, &_exc);
}

int l4io_pci_set_pm(l4io_pdev_t pdev, int state)
{
  int err;
  int old = state;
  sm_exc_t _exc;

  err = l4_io_pci_set_power_state(io_l4id, pdev, &old, &_exc);

  /* done */
  FLICK_ERR(err, &_exc);

  return old;
}
/** @} */
