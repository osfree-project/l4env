/**
 *	\file	dice/src/be/BEWaitFunction.cpp
 *	\brief	contains the implementation of the class CBEWaitFunction
 *
 *	\date	01/14/2002
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

#include "be/BEWaitFunction.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEAttribute.h"
#include "be/BEClient.h"
#include "be/BEComponent.h"
#include "be/BEImplementationFile.h"
#include "be/BEHeaderFile.h"
#include "be/BEMsgBufferType.h"

#include "TypeSpec-Type.h"
#include "fe/FEAttribute.h"
#include "fe/FEOperation.h"
#include "fe/FETypedDeclarator.h"

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
    assert(m_pMsgBuffer);
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
    assert(m_pMsgBuffer);
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
	{
        VERBOSE("%s failed because base function could not be created\n", __PRETTY_FUNCTION__);
        return false;
	}

    // set return type
    CBEType *pReturnType = pContext->GetClassFactory()->GetNewType(TYPE_VOID);
    pReturnType->SetParent(this);
    if (!pReturnType->CreateBackEnd(false, 0, TYPE_VOID, pContext))
	{
        VERBOSE("%s failed because return type could not be created\n", __PRETTY_FUNCTION__);
		delete pReturnType;
		return false;
	}
    CBEType *pOldType = m_pReturnVar->ReplaceType(pReturnType);
    delete pOldType;

    // add message buffer
    if (!AddMessageBuffer(pFEOperation, pContext))
	{
        VERBOSE("%s failed because message buffer could not be created\n", __PRETTY_FUNCTION__);
        return false;
	}

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
    if (IsComponentSide())
    {
        if (pParameter->FindAttribute(ATTR_IN))
            return true;
    }
    else
    {
        if (pParameter->FindAttribute(ATTR_OUT))
            return true;
    }
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
    pFile->PrintIndent("CORBA_exception_set(%s,\n", (const char*)sException);
    pFile->IncIndent();
    pFile->PrintIndent("CORBA_SYSTEM_EXCEPTION,\n");
    pFile->PrintIndent("CORBA_DICE_EXCEPTION_WRONG_OPCODE,\n");
    pFile->PrintIndent("0);\n");
    pFile->DecIndent();
	WriteReturn(pFile, pContext);
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

/** \brief checks whether a given parameter needs an additional reference pointer
 *  \param pDeclarator the decl to check
 *  \param pContext the context of the operation
 *  \param bCall true if the parameter is a call parameter
 *  \return true if we need a reference
 *
 * This implementation checks for the special condition to give an extra reference.
 * Since the declarator may also belong to an attribute, we have to check this as well.
 * (The size_is declarator belongs to a parameter, but has to be checked as well).
 *
 * Another possibility is, that members of structures or union are checked. To avoid
 * giving them unwanted references, we search for the parameter.
 *
 * An additional reference is also given to the [in, string] parameters.
 *
 * (The message buffer parameter needs no additional reference, it is itself a pointer.)
 *
 *
 * Because every parameter is set inside this functions, all parameters should be referenced.
 * Since OUT parameters are already referenced, we only need to add an asterisk to IN parameters.
 * This function is only called when iterating over the declarators of a typed declarator. Thus the
 * direction of a parameter can be found out by checking the attributes of the declarator's parent.
 * To be sure, whether this parameter needs an additional star, we check the existing number of stars.
 *
 * We also need to add an asterisk to the message buffer parameter.
 */
bool CBEWaitFunction::HasAdditionalReference(CBEDeclarator * pDeclarator, CBEContext * pContext, bool bCall)
{
    CBETypedDeclarator *pParameter = GetParameter(pDeclarator, bCall);
    if (!pParameter)
        return false;
    assert(pParameter->IsKindOf(RUNTIME_CLASS(CBETypedDeclarator)));
    if (pParameter->FindAttribute(ATTR_IN))
    {
		CBEType *pType = pParameter->GetType();
		CBEAttribute *pAttr;
		if ((pAttr = pParameter->FindAttribute(ATTR_TRANSMIT_AS)) != 0)
			pType = pAttr->GetAttrType();
		int nArrayDimensions = pDeclarator->GetArrayDimensionCount() - pType->GetArrayDimensionCount();
        if ((pDeclarator->GetStars() == 0) && (nArrayDimensions <= 0))
            return true;
        if ((pParameter->FindAttribute(ATTR_STRING)) &&
             pType->IsOfType(TYPE_CHAR) &&
            (pDeclarator->GetStars() < 2))
            return true;
        if ((pParameter->FindAttribute(ATTR_SIZE_IS) ||
            pParameter->FindAttribute(ATTR_LENGTH_IS) ||
            pParameter->FindAttribute(ATTR_MAX_IS)) &&
            (nArrayDimensions <= 0))
            return true;
    }
    return CBEOperationFunction::HasAdditionalReference(pDeclarator, pContext, bCall);
}

/**	\brief adds a single parameter to this function
 *	\param pFEParameter the parameter to add
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * This function decides, which parameters to add and which don't. The parameters to unmarshal are
 * for client-to-component transfer the IN parameters and for component-to-client transfer the OUT
 * and return parameters. We depend on the information set in m_bComponentSide.
 */
bool CBEWaitFunction::AddParameter(CFETypedDeclarator * pFEParameter, CBEContext * pContext)
{
    if (IsComponentSide())
    {
        if (!(pFEParameter->FindAttribute(ATTR_IN)))
            return true;
    }
    else
    {
        if (!(pFEParameter->FindAttribute(ATTR_OUT)))
            return true;
    }
    return CBEOperationFunction::AddParameter(pFEParameter, pContext);
}
