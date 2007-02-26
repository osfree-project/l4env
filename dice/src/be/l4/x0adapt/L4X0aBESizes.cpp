/**
 *    \file    dice/src/be/l4/x0adapt/L4X0aBESizes.cpp
 *    \brief   contains the implementation of the class CL4X0aBESizes
 *
 *    \date    06/01/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2004
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

#include "be/l4/x0adapt/L4X0aBESizes.h"

CL4X0aBESizes::CL4X0aBESizes()
 : CL4BESizes()
{
}

/** destroys the object */
CL4X0aBESizes::~CL4X0aBESizes()
{
}

/** \brief determines maximum number
 *  \param nDirection the direction to get max short IPC size for
 *  \return the number of bytes until which the short IPC is allowed
 *
 * method, which shall determine the maximum number of bytes of
 * a message for a short IPC
*/
int CL4X0aBESizes::GetMaxShortIPCSize(int nDirection)
{
// HACK: because X0 adaption has no _w3 IPC bindings,
// we return 2 words...

//    return 12;
    return 8;
}

/** \brief
 *determine maximum size of an environment type
*/
int CL4X0aBESizes::GetSizeOfEnvType(string sName)
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
