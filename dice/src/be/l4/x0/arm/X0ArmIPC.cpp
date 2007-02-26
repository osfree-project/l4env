/**
 *	\file	dice/src/be/l4/x0/arm/X0ArmIPC.h
 *	\brief	contains the declaration of the class CX0ArmIPC
 *
 *	\date	08/13/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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

#include "be/l4/x0/arm/X0ArmIPC.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEFunction.h"
#include "be/BEDeclarator.h"

#include "TypeSpec-Type.h"

IMPLEMENT_DYNAMIC(CX0ArmIPC);

CX0ArmIPC::CX0ArmIPC()
 : CL4X0BEIPC()
{
    IMPLEMENT_DYNAMIC_BASE(CX0ArmIPC, CL4X0BEIPC);
}

/** destroy IPC object */
CX0ArmIPC::~CX0ArmIPC()
{
}

/** \brief writes the call IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write the IPC for
 *  \param pContext the context of the write operation
 */
void CX0ArmIPC::WriteCall(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    CL4X0BEIPC::WriteCall(pFile, pFunction, pContext);
}

/** \brief writes the receive IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write to
 *  \param bAllowShortIPC true if short IPC is allowed
 *  \param pContext the context of the write operation
 */
void CX0ArmIPC::WriteReceive(CBEFile* pFile,  CBEFunction* pFunction,  bool bAllowShortIPC,  CBEContext* pContext)
{
    CL4X0BEIPC::WriteReceive(pFile, pFunction, bAllowShortIPC, pContext);
}

/** \brief write an IPC reply and receive operation
 *  \param pFile the file to write to
 *  \param pFunction the function to write it for
 *  \param bSendFlexpage true if a flexpage should be send (false, if the message buffer should determine this)
 *  \param bSendShortIPC true if a short IPC should be send (false, if message buffer should determine this)
 *  \param pContext the context of the write operation
 */
void CX0ArmIPC::WriteReplyAndWait(CBEFile* pFile,  CBEFunction* pFunction,  bool bSendFlexpage,  bool bSendShortIPC,  CBEContext* pContext)
{
    CL4X0BEIPC::WriteReplyAndWait(pFile, pFunction, bSendFlexpage, bSendShortIPC, pContext);
}

/** \brief write an IPC wait operation
 *  \param pFile the file to write to
 *  \param pFunction the function to write it for
 *  \param bAllowShortIPC true if short IPC can be used
 *  \param pContext the context of the write operation
 */
void CX0ArmIPC::WriteWait(CBEFile* pFile,  CBEFunction* pFunction,  bool bAllowShortIPC,  CBEContext* pContext)
{
    CL4X0BEIPC::WriteWait(pFile, pFunction, bAllowShortIPC, pContext);
}

/** \brief write an IPC send operation
 *  \param pFile the file to write to
 *  \param pFunction the function to write it for
 *  \param pContext the context of the write operation
 */
void CX0ArmIPC::WriteSend(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    CL4X0BEIPC::WriteSend(pFile, pFunction, pContext);
}
