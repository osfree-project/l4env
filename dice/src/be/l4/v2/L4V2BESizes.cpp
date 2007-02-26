/**
 *    \file    dice/src/be/l4/v2/L4V2BESizes.cpp
 *    \brief   contains the implementation of the class CL4V2BESizes
 *
 *    \date    10/10/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "be/l4/v2/L4V2BESizes.h"

#include "TypeSpec-Type.h"

CL4V2BESizes::CL4V2BESizes()
{
}

/** \brief destroys object of this class */
CL4V2BESizes::~CL4V2BESizes()
{
}

/** \brief get the maximum message size in bytes for a short IPC
 *  \param nDirection the direction to check
 *  \return the max size in bytes
 *
 * The size of the opcode is neglected, because its added by the functions themselves
 * (sometimes no opcode is necessary).
 */
int CL4V2BESizes::GetMaxShortIPCSize(int nDirection)
{
    return 8;
}

/** \brief retrieves the size of an environment type
 *  \param sName the name of the type
 *  \return the size in bytes
 *
 * This implementation does support the L4  types:
 * - l4_fpage_t
 * - l4_msgdope_t
 * - l4_strdope_t
 */
int CL4V2BESizes::GetSizeOfEnvType(string sName)
{
    if (sName == "l4_fpage_t")
        return 4; // l4_umword_t
    if (sName == "l4_msgdope_t")
        return 4; // l4_umword_t
    if (sName == "l4_strdope_t")
        return 16; // 4*l4_umword_t
    if (sName == "l4_timeout_t")
        return 4; // l4_umword_t
    if (sName == "l4_threadid_t")
        return 8;
    if (sName == "CORBA_Object")
        return 4; // sizeof(l4_threadid_t*)
    if (sName == "CORBA_Object_base")
        return 8; // sizeof(l4_threadid_t)
    if (sName == "CORBA_Environment")
        return 24; // 4(major+repos_id) + 4(param+ipc_error) + 4(timeout) + 4(rcv_fpage) + 4(malloc ptr) + 4(free ptr)
    if (sName == "CORBA_Server_Environment")
        return 72; // + 4(user_data) + 4(ptrs_count) + 4*DICE_PTRS_MAX(dice_ptrs)
    return CBESizes::GetSizeOfEnvType(sName);
}

/** \brief returns a value for the maximum  size of a specific type
 *  \param nFEType the type to get the max size for
 *  \return the maximum size of an array of that type
 *
 * This function is used to determine a maximum size of an array of a specifc
 * type if the parameter has no maximum size attribute.
 */
int CL4V2BESizes::GetMaxSizeOfType(int nFEType)
{
    int nSize = CBESizes::GetMaxSizeOfType(nFEType);
    switch (nFEType)
    {
    case TYPE_CHAR:
    case TYPE_CHAR_ASTERISK:
        nSize = 1024;
        break;
    default:
        break;
    }
    return nSize;
}
