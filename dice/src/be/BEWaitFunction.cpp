/**
 *	\file	dice/src/be/BEWaitFunction.cpp
 *	\brief	contains the implementation of the class CBEWaitFunction
 *
 *	\date	01/14/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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

#include "be/BEWaitFunction.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEAttribute.h"
#include "be/BEClient.h"
#include "be/BEComponent.h"
#include "be/BEImplementationFile.h"
#include "be/BEHeaderFile.h"
#include "be/BEMsgBufferType.h"

#include "fe/FETypeSpec.h"
#include "fe/FEAttribute.h"
#include "fe/FEOperation.h"

IMPLEMENT_DYNAMIC(CBEWaitFunction);

CBEWaitFunction::CBEWaitFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CBEWaitFunction, CBEOperationFunction);
}

CBEWaitFunction::CBEWaitFunction(CBEWaitFunction & src):CBEOperationFunction(src)
{
    IMPLEMENT_DYNAMIC_BASE(CBEWaitFunction, CBEOperationFunction);
}

/**	\brief destructor of target class */
CBEWaitFunction::~CBEWaitFunction()
{

}

/**	\brief writes the variable declarations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The variable declarations includes the definition of the message buffer.
 */
void CBEWaitFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // define message buffer
    ASSERT(m_pMsgBuffer);
    m_pMsgBuffer->WriteDefinition(pFile, false, pContext);
    // check for temp
    if (HasVariableSizedParameters() || HasArrayParameters())
    {
        String sTmpVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
        String sOffsetVar = pContext->GetNameFactory()->GetOffsetVariable(pContext);
        pFile->PrintIndent("unsigned %s __attribute__ ((unused));\n", (const char*)sTmpVar);
        pFile->PrintIndent("unsigned %s __attribute__ ((unused));\n", (const char*)sOffsetVar);
    }
}

/**	\brief writes the variable initializations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation should initialize the message buffer and the pointers of the out variables.
 */
void CBEWaitFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    ASSERT(m_pMsgBuffer);
    m_pMsgBuffer->WriteInitialization(pFile, pContext);
}

/**	\brief writes the invocation of the message transfer
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation calls the underlying message trasnfer mechanisms
 */
void CBEWaitFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    pFile->PrintIndent("/* invoke */\n");
}

/**	\brief clean up the mess
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation cleans up allocated memory inside this function
 */
void CBEWaitFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{
    pFile->PrintIndent("/* clean up */\n");
}

/**	\brief creates the back-end wait function
 *	\param pFEOperation the corresponding front-end operation
 *	\param pContext the context of the code generation
 *	\return true if successful
 */
bool CBEWaitFunction::CreateBackEnd(CFEOperation * pFEOperation, CBEContext * pContext)
{
    pContext->SetFunctionType(FUNCTION_WAIT);
	// set target file name
	SetTargetFileName(pFEOperation, pContext);
    // set name
    m_sName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);

    if (!CBEOperationFunction::CreateBackEnd(pFEOperation, pContext))
        return false;

    // set return type
    CBEType *pReturnType = pContext->GetClassFactory()->GetNewType(TYPE_VOID);
    pReturnType->SetParent(this);
    if (!pReturnType->CreateBackEnd(false, 0, TYPE_VOID, pContext))
      {
	  delete pReturnType;
	  return false;
      }
    CBEType *pOldType = m_pReturnVar->ReplaceType(pReturnType);
    delete pOldType;

    return true;
}

/** \brief checks if this parameter should be marshalled
 *  \param pParameter the parameter to be marshalled
 *  \param pContext the context of this marshalling
 *  \return true if this parameter should be marshalled
 *
 * Always return false, because this function does only receive data.
 */
bool CBEWaitFunction::DoMarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext)
{
    return false;
}

/** \brief checks if this parameter should be unmarshalled
 *  \param pParameter the parameter to be unmarshalled
 *  \param pContext the context of this unmarshalling
 *  \return true if this parameter should be unmarshalled
 *
 * This implementation should unpack the in parameters from the message structure
 */
bool CBEWaitFunction::DoUnmarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext)
{
    if (pParameter->FindAttribute(ATTR_IN))
        return true;
    return false;
}

/** \brief writes opcode check code
 *  \param pFile the file to write to
 *  \param pContext the context of the write
 */
void CBEWaitFunction::WriteOpcodeCheck(CBEFile *pFile, CBEContext *pContext)
{
    String sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);
    pFile->PrintIndent("if (*(%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])) != %s)\n", (const char*)m_sOpcodeConstName);
    pFile->PrintIndent("{\n");
    pFile->IncIndent();
    String sException = pContext->GetNameFactory()->GetCorbaEnvironmentVariable(pContext);
    pFile->PrintIndent("CORBA_ exception_set(%s,\n", (const char*)sException);
    pFile->IncIndent();
    pFile->PrintIndent("CORBA_USER_EXCEPTION,\n");
    pFile->PrintIndent("__CORBA_Exception_Repository[CORBA_DICE_EXCEPTION_WRONG_OPCODE],\n");
    pFile->PrintIndent("0);\n");
    pFile->DecIndent();
    pFile->PrintIndent("return");
    if (GetReturnType())
    {
        if (!GetReturnType()->IsVoid())
        {
            String sReturn = pContext->GetNameFactory()->GetReturnVariable(pContext);
            pFile->Print("%s", (const char*)sReturn);
        }
    }
    pFile->Print(";\n");
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return true if should be written
 *
 * A wait function should be written at client's side if OUT attribute or
 * at component's side if IN attribute.
 */
bool CBEWaitFunction::DoWriteFunction(CBEFile * pFile, CBEContext * pContext)
{
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEHeaderFile)))
		if (!IsTargetFile((CBEHeaderFile*)pFile))
			return false;
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEImplementationFile)))
		if (!IsTargetFile((CBEImplementationFile*)pFile))
			return false;
	if (pFile->GetTarget()->IsKindOf(RUNTIME_CLASS(CBEClient)) &&
		(FindAttribute(ATTR_OUT)))
		return true;
	if (pFile->GetTarget()->IsKindOf(RUNTIME_CLASS(CBEComponent)) &&
		(FindAttribute(ATTR_IN)))
		return true;
	return false;
}

/** \brief gets the direction for the sending data
 *  \return if at client's side DIRECTION_IN, else DIRECTION_OUT
 *
 * Since this function ignores the send part, this value should be not interesting
 */
int CBEWaitFunction::GetSendDirection()
{
    return IsComponentSide() ? DIRECTION_OUT : DIRECTION_IN;
}

/** \brief gets the direction for the receiving data
 *  \return if at client's side DIRECTION_OUT, else DIRECTION_IN
 */
int CBEWaitFunction::GetReceiveDirection()
{
    return IsComponentSide() ? DIRECTION_IN : DIRECTION_OUT;
}
