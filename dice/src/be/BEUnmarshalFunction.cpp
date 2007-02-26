/**
 *	\file	dice/src/be/BEUnmarshalFunction.cpp
 *	\brief	contains the implementation of the class CBEUnmarshalFunction
 *
 *	\date	01/20/2002
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

#include "be/BEUnmarshalFunction.h"
#include "be/BEContext.h"
#include "be/BEComponent.h"
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BEOpcodeType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEAttribute.h"
#include "be/BEClient.h"
#include "be/BEComponent.h"
#include "be/BEImplementationFile.h"
#include "be/BEHeaderFile.h"
#include "be/BEMsgBufferType.h"
#include "be/BEUserDefinedType.h"

#include "fe/FETypeSpec.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"

IMPLEMENT_DYNAMIC(CBEUnmarshalFunction);

CBEUnmarshalFunction::CBEUnmarshalFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CBEUnmarshalFunction, CBEOperationFunction);
}

CBEUnmarshalFunction::CBEUnmarshalFunction(CBEUnmarshalFunction & src)
:CBEOperationFunction(src)
{
    IMPLEMENT_DYNAMIC_BASE(CBEUnmarshalFunction, CBEOperationFunction);
}

/**	\brief destructor of target class */
CBEUnmarshalFunction::~CBEUnmarshalFunction()
{

}

/**	\brief creates the back-end unmarshal function
 *	\param pFEOperation the corresponding front-end operation
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * This function should only contain IN parameters if it is on the component's side an
 * OUT parameters if it is on the client's side.
 */
bool CBEUnmarshalFunction::CreateBackEnd(CFEOperation * pFEOperation, CBEContext * pContext)
{
    pContext->SetFunctionType(FUNCTION_UNMARSHAL);
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
    // add parameters
    if (!AddMessageBuffer(pFEOperation->GetParentInterface(), pContext))
	    return false;

    return true;
}

/**	\brief writes the variable declarations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The variable declarations contains the return variable if needed. And a temporary variable if
 * we have any variable sized arrays. No message buffer definition (its an parameter).
 */
void CBEUnmarshalFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    m_pReturnVar->WriteZeroInitDeclaration(pFile, pContext);
    // check for temp
    int nDirection = IsComponentSide()?DIRECTION_IN:DIRECTION_OUT;
    if (HasVariableSizedParameters(nDirection) || HasArrayParameters(nDirection))
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
 * This implementation should initialize the pointers of the out variables. Do not
 * initialize the message buffer - this may overwrite the values we try to unmarshal.
 */
void CBEUnmarshalFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
}

/**	\brief writes the invocation of the message transfer
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation does nothing, because the unmarshalling does not contain a
 * message transfer.
 */
void CBEUnmarshalFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{

}

/**	\brief writes the unmarshalling of the message
 *	\param pFile the file to write to
 *  \param nStartOffset the offset where to start unmarshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *	\param pContext the context of the write operation
 *
 * This implementation iterates over the parameters (except the message buffer itself) and
 * unmarshals them.
 */
void CBEUnmarshalFunction::WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext)
{
    if (IsComponentSide())
    {
        // start after opcode
        CBEOpcodeType *pOpcodeType = pContext->GetClassFactory()->GetNewOpcodeType();
        pOpcodeType->SetParent(this);
        if (pOpcodeType->CreateBackEnd(pContext))
            nStartOffset += pOpcodeType->GetSize();
        delete pOpcodeType;
    }
    else
    {
        // first unmarshl return value
        nStartOffset += WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
    }
    // now unmarshal rest
    CBEOperationFunction::WriteUnmarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
}

/**	\brief clean up the mess
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation is empty, because there is nothing to clean up.
 */
void CBEUnmarshalFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{

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
bool CBEUnmarshalFunction::AddParameter(CFETypedDeclarator * pFEParameter, CBEContext * pContext)
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

/** \brief checks if this parameter should be marshalled or not
 *  \param pParameter the parameter to check
 *  \param pContext the context of this marshalling
 *  \return true if this parameter should be marshalled
 *
 * Always return false, because this function does only unmarshal parameters
 */
bool CBEUnmarshalFunction::DoMarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext)
{
    return false;
}

/** \brief checks if this parameter should be unmarshalled or not
 *  \param pParameter the parameter to check
 *  \param pContext the context of this unmarshalling
 *  \return true if this parameter should be unmarshalled
 */
bool CBEUnmarshalFunction::DoUnmarshalParameter(CBETypedDeclarator * pParameter, CBEContext * pContext)
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
bool CBEUnmarshalFunction::HasAdditionalReference(CBEDeclarator * pDeclarator, CBEContext * pContext, bool bCall)
{
    CBETypedDeclarator *pParameter = GetParameter(pDeclarator, bCall);
    if (!pParameter)
        return false;
    ASSERT(pParameter->IsKindOf(RUNTIME_CLASS(CBETypedDeclarator)));
    if (pParameter->FindAttribute(ATTR_IN))
    {
        if ((pDeclarator->GetStars() == 0) && (pDeclarator->GetArrayDimensionCount() == 0))
            return true;
        if ((pParameter->FindAttribute(ATTR_STRING)) &&
             pParameter->GetType()->IsOfType(TYPE_CHAR) &&
            (pDeclarator->GetStars() < 2))
            return true;
        if ((pParameter->FindAttribute(ATTR_SIZE_IS) ||
            pParameter->FindAttribute(ATTR_LENGTH_IS) ||
            pParameter->FindAttribute(ATTR_MAX_IS)) &&
            (pDeclarator->GetArrayDimensionCount() == 0))
            return true;
    }
    return CBEOperationFunction::HasAdditionalReference(pDeclarator, pContext, bCall);
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return true if should be written
 *
 * An unmarshal function is written if client's side and OUT or if component's side.
 */
bool CBEUnmarshalFunction::DoWriteFunction(CBEFile * pFile, CBEContext * pContext)
{
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEHeaderFile)))
		if (!IsTargetFile((CBEHeaderFile*)pFile))
			return false;
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEImplementationFile)))
		if (!IsTargetFile((CBEImplementationFile*)pFile))
			return false;
	if (pFile->IsOfFileType(FILETYPE_CLIENT) &&
		(FindAttribute(ATTR_OUT)))
		return true;
	return pFile->GetTarget()->IsKindOf(RUNTIME_CLASS(CBEComponent));
}

/** \brief writes the message buffer parameter
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \param bComma true if a comma has to be written before the parameter
 */
void CBEUnmarshalFunction::WriteAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma)
{
    ASSERT(m_pMsgBuffer);
    if (bComma)
    {
        pFile->Print(",\n");
        pFile->PrintIndent("");
    }
    WriteParameter(pFile, m_pMsgBuffer, pContext);
    CBEOperationFunction::WriteAfterParameters(pFile, pContext, true);
}

/** \brief writes the message buffer call parameter
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \param bComma true if a comma has to be written before the declarators
 *
 * This is also where the environment variable is written. If the server has a
 * parameter of type Corba-Environment, it is a pointer in the server loop and
 * when calling the unmarshal function, needs no reference making '&'.
 */
void CBEUnmarshalFunction::WriteCallAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma)
{
    ASSERT(m_pMsgBuffer);
    if (bComma)
    {
        pFile->Print(",\n");
        pFile->PrintIndent("");
    }
    if (m_pMsgBuffer->HasReference())
    {
        if (m_bCastMsgBufferOnCall)
            m_pMsgBuffer->GetType()->WriteCast(pFile, true, pContext);
        pFile->Print("&");
    }
    WriteCallParameter(pFile, m_pMsgBuffer, pContext);
    CBEOperationFunction::WriteCallAfterParameters(pFile, pContext, true);
}

/** \brief tries to find a parameter by its type
 *  \param sTypeName the name of the type
 *  \return a reference to the found parameter
 */
CBETypedDeclarator * CBEUnmarshalFunction::FindParameterType(String sTypeName)
{
    if (m_pMsgBuffer)
    {
        CBEType *pType = m_pMsgBuffer->GetType();
        if (pType->IsKindOf(RUNTIME_CLASS(CBEUserDefinedType)))
        {
            if (((CBEUserDefinedType*)pType)->GetName() == sTypeName)
                return m_pMsgBuffer;
        }
        if (pType->HasTag(sTypeName))
            return m_pMsgBuffer;
    }
    return CBEOperationFunction::FindParameterType(sTypeName);
}

/** \brief adds the specific message buffer parameter for this function
 *  \param pFEInterface the respective front-end interface to use as reference
 *  \param pContext the context of the create process
 *  \return true if the create process was successful
 *
 * Instead of creating a whole new message buffer type, we use the existing type
 * of the class as a user defined type.
 */
bool CBEUnmarshalFunction::AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext)
{
    // get class's message buffer
    CBEClass *pClass = GetClass();
    ASSERT(pClass);
    // get message buffer type
    CBEMsgBufferType *pMsgBuffer = pClass->GetMessageBuffer();
    ASSERT(pMsgBuffer);
    // msg buffer not yet initialized
    pMsgBuffer->InitCounts(pClass, pContext);
    // create own message buffer
    m_pMsgBuffer = pContext->GetClassFactory()->GetNewMessageBufferType();
    m_pMsgBuffer->SetParent(this);
    if (!m_pMsgBuffer->CreateBackEnd(pMsgBuffer, pContext))
    {
        delete m_pMsgBuffer;
        m_pMsgBuffer = 0;
        VERBOSE("%s failed because message buffer could not be created\n", __PRETTY_FUNCTION__);
        return false;
    }
    return true;
}

/** \brief gets the direction, which the marshal-parameters have
 *  \return if at client's side DIRECTION_IN, if at server's side DIRECTION_OUT
 *
 * Since this function ignores marshalling parameter this value should be irrelevant
 */
int CBEUnmarshalFunction::GetSendDirection()
{
    return IsComponentSide() ? DIRECTION_OUT : DIRECTION_IN;
}

/** \brief gets the direction of the unmarshal-parameters
 *  \return if at client's side DIRECTION_OUT, if at server's side DIRECTION_IN
 */
int CBEUnmarshalFunction::GetReceiveDirection()
{
    return IsComponentSide() ? DIRECTION_IN : DIRECTION_OUT;
}
