/**
 *	\file	dice/src/be/BERcvFunction.cpp
 *	\brief	contains the implementation of the class CBERcvFunction
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

#include "be/BERcvFunction.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEClient.h"
#include "be/BEComponent.h"
#include "be/BEImplementationFile.h"
#include "be/BEHeaderFile.h"
#include "be/BEMsgBufferType.h"

#include "fe/FEOperation.h"
#include "fe/FETypeSpec.h"

IMPLEMENT_DYNAMIC(CBERcvFunction);

CBERcvFunction::CBERcvFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CBERcvFunction, CBEOperationFunction);
}

CBERcvFunction::CBERcvFunction(CBERcvFunction & src):CBEOperationFunction(src)
{
    IMPLEMENT_DYNAMIC_BASE(CBERcvFunction, CBEOperationFunction);
}

/**	\brief destructor of target class */
CBERcvFunction::~CBERcvFunction()
{

}

/**	\brief writes the variable declarations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The variable declarations of the call function include the message buffer for send and receive.
 */
void CBERcvFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // declare message buffer
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
void CBERcvFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
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
void CBERcvFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    pFile->PrintIndent("/* invoke */\n");
}

/**	\brief clean up the mess
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation cleans up allocated memory inside this function
 */
void CBERcvFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{
    pFile->PrintIndent("/* clean up */\n");
}

/**	\brief creates the back-end receive function
 *	\param pFEOperation the corresponding front-end operation
 *	\param pContext the context of the code generation
 *	\return true is successful
 *
 * This implementation calls the base class' implementation and then sets the name
 * of the function.
 */
bool CBERcvFunction::CreateBackEnd(CFEOperation * pFEOperation, CBEContext * pContext)
{
    pContext->SetFunctionType(FUNCTION_RECV);
	// set target file name
	SetTargetFileName(pFEOperation, pContext);
    // set name
    m_sName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);

    if (!CBEOperationFunction::CreateBackEnd(pFEOperation, pContext))
        return false;

    // set return type to void
    CBEType *pReturnType = pContext->GetClassFactory()->GetNewType(TYPE_VOID);
    pReturnType->SetParent(this);
    if (!pReturnType->CreateBackEnd(false, 0, TYPE_VOID, pContext))
    {
        VERBOSE("CBERcvFunction::CreateBE failed because return type could not be created\n");
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
 * We always return false, because this function only receives data
 */
bool CBERcvFunction::DoMarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext)
{
    return false;
}

/** \brief check if this parameter should be unmarshalled
 *  \param pParameter the parameter to be checked
 *  \param pContext the context of this unmarshalling
 *  \return true if it should be unmarshalled
 *
 * This implementation should unpack the out parameters from the returned message structure
 */
bool CBERcvFunction::DoUnmarshalParameter(CBETypedDeclarator * pParameter, CBEContext * pContext)
{
    if (pParameter->FindAttribute(ATTR_OUT))
        return true;
    return false;
}

/** \brief write check code for the opcode
 *  \param pFile the file to write to
 *  \param pContext the context of this operation
 */
void CBERcvFunction::WriteOpcodeCheck(CBEFile *pFile, CBEContext *pContext)
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
    {                                                getenv("CPP");
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
 *  \return true if shall write
 *
 * A receive function should be written on the client's side if the directional attribute
 * for the function is OUT, and at the component's side if the directional attribute is IN.
 * Otherwise this function is not written.
 */
bool CBERcvFunction::DoWriteFunction(CBEFile * pFile, CBEContext * pContext)
{
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEHeaderFile)))
		if (!IsTargetFile((CBEHeaderFile*)pFile))
			return false;
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEImplementationFile)))
		if (!IsTargetFile((CBEImplementationFile*)pFile))
			return false;
	if (pFile->IsOfFileType(FILETYPE_CLIENT) && (FindAttribute(ATTR_OUT)))
		return true;
	if (pFile->IsOfFileType(FILETYPE_COMPONENT) && (FindAttribute(ATTR_IN)))
		return true;
	return false;
}

/** \brief gets the direction this function sends to
 *  \return depending on communication side DIRECTION_IN or DIRECTION_OUT
 *
 * If this function is at client side, this is DIRECTION_IN, if not its DIRECTION_OUT.
 * Since this function only receives it should not matter.
 */
int CBERcvFunction::GetSendDirection()
{
    return IsComponentSide() ? DIRECTION_OUT : DIRECTION_IN;
}

/** \brief gets the direction this function receives from
 *  \return depending on communication side DIRECTION_IN or DIRECTION_OUT
 */
int CBERcvFunction::GetReceiveDirection()
{
    return IsComponentSide() ? DIRECTION_IN : DIRECTION_OUT;
}
