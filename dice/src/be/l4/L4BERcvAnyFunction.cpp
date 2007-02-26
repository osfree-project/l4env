/**
 *	\file	dice/src/be/l4/L4BERcvAnyFunction.cpp
 *	\brief	contains the implementation of the class CL4BERcvAnyFunction
 *
 *	\date	Thu Jul 4 2002
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

#include "be/l4/L4BERcvAnyFunction.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEClassFactory.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "be/BEOperationFunction.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BEIPC.h"

#include "TypeSpec-Type.h"
#include "fe/FEAttribute.h"

IMPLEMENT_DYNAMIC(CL4BERcvAnyFunction);

CL4BERcvAnyFunction::CL4BERcvAnyFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4BERcvAnyFunction, CBERcvAnyFunction);
}

/** \brief cleans up the mess */
CL4BERcvAnyFunction::~CL4BERcvAnyFunction()
{
}

/**	\brief writes the invocation call to thetarget file
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The wait any function simply waits for any message and unmarshals the opcode. Since the message buffer is a
 * referenced parameter, we know for sure, that the "buffer" is a pointer.
 */
void CL4BERcvAnyFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    // init size and send dope
    ((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteSendDopeInit(pFile, pContext);

    WriteIPC(pFile, pContext);
    WriteIPCErrorCheck(pFile, pContext);
}

/** \brief writes the unmarshalling code for this function
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start unmarshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param pContext the context of the write operation
 *
 * The wait-any function does only unmarshal the opcode. We can print this code by hand. We should use
 * a marshaller anyways.
 */
void CL4BERcvAnyFunction::WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext)
{
    if (((CL4BEMsgBufferType*) m_pMsgBuffer)->HasReceiveFlexpages())
    {
        // we have to always check if this was a flexpage IPC
        String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
        pFile->PrintIndent("if (l4_ipc_fpage_received(%s))\n", (const char*)sResult);
        WriteFlexpageOpcodePatch(pFile, pContext);  // does indent itself
        pFile->PrintIndent("else\n");
        pFile->IncIndent();
        WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
        pFile->DecIndent();
    }
    else
    {
        WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
    }
}

/**	\brief writes the variable declarations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The variable declarations of the wait-any function only contains so-called helper variables.
 * This is the result variable.
 */
void CL4BERcvAnyFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // first call base class
    CBERcvAnyFunction::WriteVariableDeclaration(pFile, pContext);

    // write result variable
    String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0};\n", (const char *) sResult);
}

/**	\brief write the error checking code for the IPC
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 */
void CL4BERcvAnyFunction::WriteIPCErrorCheck(CBEFile * pFile, CBEContext * pContext)
{
	String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
	pFile->PrintIndent("/* test for IPC errors */\n");
	pFile->PrintIndent("if (L4_IPC_IS_ERROR(%s))\n", (const char *) sResult);
	pFile->PrintIndent("{\n");
	pFile->IncIndent();
	// set opcode to zero value
	if (m_pReturnVar)
	    m_pReturnVar->WriteSetZero(pFile, pContext);
    if (!m_sErrorFunction.IsEmpty())
        pFile->PrintIndent("%s(%s);\n", (const char*)m_sErrorFunction, (const char*)sResult);
	// set exception if not set already
	VectorElement *pIter = m_pCorbaEnv->GetFirstDeclarator();
	CBEDeclarator *pDecl = m_pCorbaEnv->GetNextDeclarator(pIter);
	pFile->PrintIndent("if (");
	pDecl->WriteName(pFile, pContext);
	if (pDecl->GetStars() > 0)
	    pFile->Print("->");
	else
	    pFile->Print(".");
	pFile->Print("major == CORBA_NO_EXCEPTION)\n");
	pFile->IncIndent();
	// set exception
	pFile->PrintIndent("CORBA_exception_set(");
	if (pDecl->GetStars() == 0)
	    pFile->Print("&");
	pDecl->WriteName(pFile, pContext);
	pFile->Print(",\n");
	pFile->IncIndent();
	pFile->PrintIndent("CORBA_SYSTEM_EXCEPTION,\n");
	pFile->PrintIndent("CORBA_DICE_INTERNAL_IPC_ERROR,\n");
	pFile->PrintIndent("0);\n");
	pFile->DecIndent();
	pFile->DecIndent();
	// return (skip unmarshalling)
	WriteReturn(pFile, pContext);
	pFile->DecIndent();
	pFile->PrintIndent("}\n");
}

/** \brief writes a patch to find the opcode if flexpage were received
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This function may receive messages from different function. Because we don't know at
 * compile time, which function sends, we don't know if the message contains a flexpage.
 * If it does the unmarshalled opcode is wrong. Because flexpages have to come first in
 * the message buffer, the opcode cannot be the first parameter. We have to check this
 * condition and get the opcode from behind the flexpages.
 *
 * First we get the number of flexpages of the interface. If it has none, we don't need
 * this extra code. If it has a fixed number of flexpages (either none is sent or one, but
 * if flexpages are sent it is always the same number of flexpages) we can hard code
 * the offset were to find the opcode. If we have different numbers of flexpages (one
 * function may send one, another sends two) we have to use code which can deal with a variable
 * number of flexpages.
 */
void CL4BERcvAnyFunction::WriteFlexpageOpcodePatch(CBEFile *pFile, CBEContext *pContext)
{
    if (!(((CL4BEMsgBufferType*)m_pMsgBuffer)->HasReceiveFlexpages()))
        return;
    bool bFixedNumberOfFlexpages = true;
    int nNumberOfFlexpages = m_pClass->GetParameterCount(TYPE_FLEXPAGE, bFixedNumberOfFlexpages);
    // if fixed number  (should be true for only one flexpage as well)
    if (bFixedNumberOfFlexpages)
    {
        // the fixed offset (where to find the opcode) is:
        // offset = 8*nMaxNumberOfFlexpages + 8
        bool bUseConstOffset = true;
        pFile->IncIndent();
        WriteUnmarshalReturn(pFile, 8*nNumberOfFlexpages+8, bUseConstOffset, pContext);
        pFile->DecIndent();
    }
    else
    {
        // the variable offset can be determined by searching for the delimiter flexpage
        // which is two zero dwords
        pFile->PrintIndent("{\n");
        pFile->IncIndent();
        // search for delimiter flexpage
        String sTempVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
        // init temp var
        pFile->PrintIndent("%s = 0;\n", (const char*)sTempVar);
        pFile->PrintIndent("while ((");
        m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
        pFile->Print("[%s] != 0) && (", (const char*)sTempVar);
        m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
        pFile->Print("[%s+4] != 0)) %s += 8;\n", (const char*)sTempVar, (const char*)sTempVar);
        // now sTempVar points to the delimiter flexpage
        // we have to add another 8 bytes to find the opcode, because UnmarshalReturn does only use
        // temp-var
        pFile->PrintIndent("%s += 8;\n", (const char*)sTempVar);
        // now unmarshal opcode
        bool bUseConstOffset = false;
        WriteUnmarshalReturn(pFile, 0, bUseConstOffset, pContext);
        pFile->DecIndent();
        pFile->PrintIndent("}\n");
    }
}

/** \brief initializes the variables
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BERcvAnyFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    // call base class
    CBERcvAnyFunction::WriteVariableInitialization(pFile, pContext);
    if (m_pMsgBuffer)
    {
        // init receive flexpage
        ((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteReceiveFlexpageInitialization(pFile, pContext);
        // init indirect strings
        ((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteReceiveIndirectStringInitialization(pFile, pContext);
    }
}

/** \brief writes the IPC code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BERcvAnyFunction::WriteIPC(CBEFile *pFile, CBEContext *pContext)
{
    assert(m_pComm);
	((CL4BEIPC*)m_pComm)->WriteReceive(pFile, this, false, pContext);
}
