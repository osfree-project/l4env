/**
 *    \file    dice/src/be/l4/v4/L4V4BESizes.cpp
 *    \brief    contains the implementation of the class CL4V4BESizes
 *
 *    \date    01/08/2004
 *    \author    Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
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

#include "be/l4/v4/L4V4BESizes.h"

CL4V4BESizes::CL4V4BESizes()
 : CL4BESizes()
{
}

/** \brief destroys object of this class */
CL4V4BESizes::~CL4V4BESizes()
{
}

/** \brief get the maximum message size in bytes for a short IPC
 *  \param nDirection the direction to check
 *  \return the max size in bytes
 *
 * For generic V4 we return 0, because all IPC loads into MRs.
 */
int CL4V4BESizes::GetMaxShortIPCSize(int nDirection)
{
    return 0;
}

/** \brief retrieves the size of an environment type
 *  \param sName the name of the type
 *  \return the size in bytes
 *
 * This implementation does support the L4 V4 types:
 * - StringItem
 * - MapItem
 * - GrantItem
 */
int CL4V4BESizes::GetSizeOfEnvType(string sName)
{
    if (sName == "StringItem")
        return 8;
    if (sName == "MapItem")
        return 8;
    if (sName == "GrantItem")
        return 8;
    return CL4BESizes::GetSizeOfEnvType(sName);
}
