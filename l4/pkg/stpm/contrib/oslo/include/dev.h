/*
 * \brief header used for DEV protection
 * \date    2006-10-25
 * \author  Bernhard Kauer <kauer@tudos.org>
 */
/*
 * Copyright (C) 2006  Bernhard Kauer <kauer@tudos.org>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the OSLO package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#pragma once

enum pci_constants {
  PCI_ADDR_PORT = 0xcf8,
  PCI_DATA_PORT = 0xcfc,
  PCI_CONF_HDR_CMD = 4,
  PCI_CONF_HDR_CAP = 52,
  PCI_CAP_OFFSET = 1,
};

enum dev_constants {
  DEV_PCI_DEVICE_ID = 0x11031022,
  DEV_PCI_CAP_ID = 0x0f,
  DEV_OFFSET_OP = 4,
  DEV_OFFSET_DATA = 8,
};

enum dev_registers {
  DEV_REG_BASE_LO,
  DEV_REG_BASE_HI,
  DEV_REG_MAP,
  DEV_REG_CAP,
  DEV_REG_CR,
  DEV_REG_ERR_STATUS,
  DEV_REG_ERR_ADDR_LO,
  DEV_REG_ERR_ADDR_HI,
};

enum dev_cr {
  DEV_CR_EN     = 1<<0,
  DEV_CR_CLEAR  = 1<<1,
  DEV_CR_IOSPEN = 1<<2,
  DEV_CR_MCE    = 1<<3,
  DEV_CR_INVD   = 1<<4,
  DEV_CR_SLDEV  = 1<<5,
  DEV_CR_PROBE  = 1<<6,
};


int disable_dev_protection();
int pci_iterate_devices();
unsigned pci_read_long(unsigned addr);
unsigned pci_find_device_per_class(unsigned short class);
