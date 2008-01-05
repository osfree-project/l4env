/**
 * \file   l4io/server/include/static_cfg.h
 * \brief  Static configuration
 *
 * \date   2008-01-04
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 */
/* (c) 2008 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4IO_SERVER_INCLUDE___STATIC_CFG_H__
#define __L4IO_SERVER_INCLUDE___STATIC_CFG_H__

#include <l4/crtx/ctor.h>
#include <l4/generic_io/types.h>

#define SYS_CTRL "System Control"

#define AMBA_KMI_KBD   "AMBA KMI kbd"
#define AMBA_KMI_MOUSE "AMBA KMI mou"

#define AMBA_LCD_PL110 "AMBA PL110"

#define SMC91X         "smc91x"

enum {
  MAX_NUM_REGISTERED_DEVS = 10,
};

void register_device_group_fn(const char *, l4io_desc_device_t **, int num);

#define register_device_group(name, devspointer...)    \
      static l4io_desc_device_t *devs[] = \
       { devspointer }; \
      static void register_device_group_add(void) \
      { register_device_group_fn(name, devs, sizeof(devs) / sizeof(devs[0])); } \
      L4C_CTOR(register_device_group_add,0)



#endif /* ! __L4IO_SERVER_INCLUDE___STATIC_CFG_H__ */
