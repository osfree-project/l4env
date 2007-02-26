/* Copyright (C) 2001-2003 by
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

#include "be/l4/x0/L4X0BESizes.h"

IMPLEMENT_DYNAMIC(CL4X0BESizes)

CL4X0BESizes::CL4X0BESizes()
 : CL4BESizes()
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0BESizes, CL4BESizes);
}


CL4X0BESizes::~CL4X0BESizes()
{
}

/** \brief determines maximum number 
 *  \param nDirection the direction to get max short IPC size for
 *  \return the number of bytes until which the short IPC is allowed
 *
 * method, which shall determine the maximum number of bytes of
 * a message for a short IPC
*/
int CL4X0BESizes::GetMaxShortIPCSize(int nDirection)
{
    return 12;
}

/** \brief 
 *determine maximum size of an environment type
*/
int CL4X0BESizes::GetSizeOfEnvType(String sName)
{
    if (sName == "l4_fpage_t")
        return 4; // l4_umword_t
    if (sName == "l4_msgdope_t")
        return 4; // l4_umword_t
    if (sName == "l4_strdope_t")
        return 16; // 4*l4_umword_t
    if (sName == "l4_timeout_t")
        return 4; // l4_umword_t
    if (sName == "CORBA_Object")
        return 4; // sizeof(l4_threadid_t)
    if (sName == "CORBA_Environment")
        return 28; // 4(major+repos_id) + 4(param) + 4(ipc_error) + 4(timeout) + 4(rcv_fpage) + 4(user_data) + 4(malloc ptr)
    return CBESizes::GetSizeOfEnvType(sName);
}
