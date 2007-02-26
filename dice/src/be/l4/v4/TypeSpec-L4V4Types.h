/**
 *  \file   dice/src/be/l4/v4/TypeSpec-L4V4Types.h
 *  \brief  contains the declaration defines used with L4 version 4 types
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
#ifndef __DICE_TYPESPEC_L4V4TYPES_H__
#define __DICE_TYPESPEC_L4V4TYPES_H__

#include "be/l4/TypeSpec-L4Types.h"

/** \brief the L4 version 4 specific types
 */
enum
{
    TYPE_L4V4_BASE = TYPE_L4_MAX, /**< ensure that values are disjunct */
    TYPE_MSGTAG,                  /**< L4 V4 message tag type */
    TYPE_L4V4_MAX                 /**< maximum value of the L4 V4 types */
};

#endif /* !__DICE_TYPESPEC_L4V4TYPES_H__ */
