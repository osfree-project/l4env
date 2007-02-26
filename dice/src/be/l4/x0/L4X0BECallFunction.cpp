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

#include "be/l4/x0/L4X0BECallFunction.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEIPC.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BEFile.h"
#include "be/BEContext.h"

#include "TypeSpec-Type.h"

IMPLEMENT_DYNAMIC(CL4X0BECallFunction);

CL4X0BECallFunction::CL4X0BECallFunction()
 : CL4BECallFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0BECallFunction, CL4BECallFunction);
}


CL4X0BECallFunction::~CL4X0BECallFunction()
{
}

/** \brief Writes the variable declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * If we have a short IPC in both direction, we only need the result dope,
 * and two dummy dwords.
 */
void CL4X0BECallFunction::WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext)
{
	CL4BECallFunction::WriteVariableDeclaration(pFile, pContext); // msgdope, return var, msgbuffer
    if (((CL4BEIPC*)m_pComm)->UseAssembler(this, pContext) &&
	    ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(0, pContext))
	{
		// write dummys
		CBENameFactory *pNF = pContext->GetNameFactory();
		String sDummy = pNF->GetDummyVariable(pContext);
		String sMWord = pNF->GetTypeName(TYPE_MWORD, false, pContext, 0);
		// write dummy variable
		pFile->PrintIndent("%s %s __attribute__((unused));\n",
		                    (const char*)sMWord, (const char*)sDummy);
	}
}

/* \brief write the marshalling code for the short IPC
 * \param pFile the file to write to
 * \param nStartOffset the offset where to start marshalling
 * \param bUseConstOffset true if nStartOffset can be used
 * \param pContext the context of the write operation
 *
 * If we have a short IPC into both direction, we skip the marshalling.
 */
void CL4X0BECallFunction::WriteMarshalling(CBEFile * pFile,  int nStartOffset,
                           bool & bUseConstOffset,  CBEContext * pContext)
{
    if (!(((CL4BEIPC*)m_pComm)->UseAssembler(this, pContext) &&
	     ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(0, pContext)))
	    CL4BECallFunction::WriteMarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
}

/* \brief write the unmarshalling code for the short IPC
 * \param pFile the file to write to
 * \param nStartOffset the offset where to start marshalling
 * \param bUseConstOffset true if nStartOffset can be used
 * \param pContext the context of the write operation
 *
 * If we have a short IPC in both direction, we can skip the unmarshalling.
 */
void CL4X0BECallFunction::WriteUnmarshalling(CBEFile * pFile,  int nStartOffset,
                            bool & bUseConstOffset,  CBEContext * pContext)
{
    if (((CL4BEIPC*)m_pComm)->UseAssembler(this, pContext) &&
	    ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(0, pContext))
	    // unmarshal the exception from its word representation
		WriteEnvExceptionFromWord(pFile, pContext);
    else
        CL4BECallFunction::WriteUnmarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
}

/** \brief writes the invocation code for the short IPC
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * If we have a short IPC in both direction, we write assembler code
 * directly.
 */
void CL4X0BECallFunction::WriteInvocation(CBEFile * pFile,  CBEContext * pContext)
{
    if (((CL4BEIPC*)m_pComm)->UseAssembler(this, pContext) &&
	    ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(0, pContext))
    {
	    // skip send dope init
	    WriteIPC(pFile, pContext);
		WriteIPCErrorCheck(pFile, pContext);
    }
	else
        CL4BECallFunction::WriteInvocation(pFile, pContext);
}

