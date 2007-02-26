/**
 *  \file   dice/src/be/l4/TypeSpec-L4Types.h
 *  \brief  contains the declaration defines used with L4 types
 *
 *  \date   11/01/2004
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2004
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For different licensing schemes please contact
 * <contact@os.inf.tu-dresden.de>.
 */

/** preprocessing symbol to check header file */
#ifndef __DICE_TYPESPEC_L4TYPES_H__
#define __DICE_TYPESPEC_L4TYPES_H__

#include "TypeSpec-Type.h"

enum
{
    TYPE_L4_BASE = TYPE_MAX,  /**< ensure that we use distinct numbers */
    TYPE_MSGDOPE_SIZE,        /**< the l4 message size dope type */
    TYPE_MSGDOPE_SEND,        /**< the l4 message send dope type */
    TYPE_L4_MAX               /**< maximum value of the L4 types */
};

#endif /* !__DICE_TYPESPEC_L4TYPES_H__ */
