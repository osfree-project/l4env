/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4env/lib/include/__config
 * \brief  Default values for the environment config
 *
 * \date   02/14/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2002
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
#ifndef _L4ENV___CONFIG_H
#define _L4ENV___CONFIG_H

/* L4/L4Env includes */
#include <l4/dm_phys/dm_phys.h>

/*****************************************************************************
 *** default configuration
 *****************************************************************************/

/// default dataspace manager 
#define L4ENV_DEFAULT_DSM_NAME     L4DM_MEMPHYS_NAME

#endif /* !_L4ENV___CONFIG_H */
