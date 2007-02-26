/**
 *	\file	dice/src/be/BEMarshalFunction.cpp
 *	\brief	contains the implementation of the class CBEMarshalFunction
 *
 *	\date	10/09/2003
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
#include "be/BEMarshalFunction.h"
#include "be/BEContext.h"
#include "be/BEType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEOpcodeType.h"
#include "be/BEReplyCodeType.h"
#include "be/BEMsgBufferType.h"
#include "be/BEUserDefinedType.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BEComponent.h"

#include "TypeSpec-Type.h"
#include "fe/FEOperation.h"
#include "fe/FETypedDeclarator.h"

IMPLEMENT_DYNAMIC(CBEMarshalFunction);

CBEMarshalFunction::CBEMarshalFunction()
 : CBEOperationFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CBEMarshalFunction, CBEOperationFunction);
}

CBEMarshalFunction::CBEMarshalFunction(CBEMarshalFunction & src)
: CBEOperationFunction(src)
{
    IMPLEMENT_DYNAMIC_BASE(CBEMarshalFunction, CBEOperationFunction);
}

/**	\brief destructor of target class */
CBEMarshalFunction::~CBEMarshalFunction()
{
}

/**	\brief creates the back-end marshal function
 *	\param pFEOperation the corresponding front-end operation
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * This function should only contain OUT parameters if it is on the component's side an
 * IN parameters if it is on the client's side.
 */
bool CBEMarshalFunction::CreateBackEnd(CFEOperation * pFEOperation, CBEContext * pContext)
{
    pContext->SetFunctionType(FUNCTION_MARSHAL);
	// set target file name
	SetTargetFileName(pFEOperation, pContext);
    // set name
    m_sName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);

    if (!CBEOperationFunction::CreateBackEnd(pFEOperation, pContext))
	{
        VERBOSE("%s failed because base function could not be created\n", __PRETTY_FUNCTION__);
	    return false;
	}

	if (IsComponentSide())
	{
		// return type -> set to IPC reply code
		CBEReplyCodeType *pReplyType = pContext->GetClassFactory()->GetNewReplyCodeType();
		if (!pReplyType->CreateBackEnd(pContext))
		{
			delete pReplyType;
			VERBOSE("CBERcvAnyFunction::CreateBE failed because return var could not be set\n");
			return false;
		}
		String sReply = pContext->GetNameFactory()->GetReplyCodeVariable(pContext);
		if (!SetReturnVar(pReplyType, sReply, pContext))
		{
			delete pReplyType;
			VERBOSE("CBERcvAnyFunction::CreateBE failed because return var could not be set\n");
			return false;
		}
	}
    // add parameters
    if (!AddMessageBuffer(pFEOperation->GetParentInterface(), pContext))
    {
        VERBOSE("%s failed because message buffer could not be created\n", __PRETTY_FUNCTION__);
	    return false;
	}

    return true;
}


/**	\brief writes the variable declarations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The variable declarations contains the return variable if needed. And a temporary variable if
 * we have any variable sized arrays. No message buffer definition (its an parameter).
 */
void CBEMarshalFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // do NOT declare return variable
    // check for temp
    if (HasVariableSizedParameters() || HasArrayParameters())
    {
        String sTmpVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
        String sOffsetVar = pContext->GetNameFactory()->GetOffsetVariable(pContext);
        pFile->PrintIndent("unsigned %s __attribute__ ((unused));\n", (const char*)sTmpVar);
        pFile->PrintIndent("unsigned %s __attribute__ ((unused));\n", (const char*)sOffsetVar);
    }
	// declare local exception variable if at component's side
    if (IsComponentSide())
		WriteExceptionWordDeclaration(pFile, true, pContext);
}

/**	\brief writes the variable initializations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation should initialize the pointers of the out variables. Do not
 * initialize the message buffer - this may overwrite the values we try to unmarshal.
 */
void CBEMarshalFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
}

/**	\brief writes the invocation of the message transfer
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation does nothing, because the unmarshalling does not contain a
 * message transfer.
 */
void CBEMarshalFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
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
void CBEMarshalFunction::WriteMarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext)
{
    if (IsComponentSide())
    {
        // start after exception
		nStartOffset += WriteMarshalException(pFile, nStartOffset, bUseConstOffset, pContext);
    }
    else
    {
        // start after opcode
        CBEOpcodeType *pOpcodeType = pContext->GetClassFactory()->GetNewOpcodeType();
        pOpcodeType->SetParent(this);
        if (pOpcodeType->CreateBackEnd(pContext))
            nStartOffset += pOpcodeType->GetSize();
        delete pOpcodeType;
    }
    // now unmarshal rest
    CBEOperationFunction::WriteMarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
}

/**	\brief clean up the mess
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation is empty, because there is nothing to clean up.
 */
void CBEMarshalFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{

}

/**	\brief adds the parameters of a front-end function to this function
 *	\param pFEOperation the front-end function
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * This implementation adds the return value to the parameter list. The return value is the
 * value returned by the component-function.
 *
 * Since this function is called before the rest of the above CreateBE function is executed, we
 * can assume, that the return variable is still the original function's return variable and
 * not the opcode return variable.
 */
bool CBEMarshalFunction::AddParameters(CFEOperation * pFEOperation, CBEContext * pContext)
{
    if (!GetReturnType()->IsVoid())
    {
        // create new parameter
        CBETypedDeclarator *pReturnParam = (CBETypedDeclarator*)(m_pReturnVar->Clone());
        CBEFunction::AddParameter(pReturnParam);
        AddSortedParameter(pReturnParam);
    }
    // call base class to add rest
    return CBEOperationFunction::AddParameters(pFEOperation, pContext);
}

/**	\brief adds a single parameter to this function
 *	\param pFEParameter the parameter to add
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * This function decides, which parameters to add and which not. The parameters to marshal are
 * for client-to-component transfer the IN parameters and for component-to-client transfer the OUT
 * and return parameters. We depend on the information set in m_bComponentSide.
 */
bool CBEMarshalFunction::AddParameter(CFETypedDeclarator * pFEParameter, CBEContext * pContext)
{
    if (IsComponentSide())
    {
	    // we transfer from the component to the client
        if (!(pFEParameter->FindAttribute(ATTR_OUT)))
		    // skip adding a parameter if it has no OUT
            return true;
    }
    else
    {
        if (!(pFEParameter->FindAttribute(ATTR_IN)))
		    // skip parameter if it has no IN
            return true;
    }
    return CBEOperationFunction::AddParameter(pFEParameter, pContext);
}

/** \brief checks if this parameter should be marshalled or not
 *  \param pParameter the parameter to check
 *  \param pContext the context of this marshalling
 *  \return true if this parameter should be marshalled
 *
 * Return true if the at component's side, the parameter has an OUT attribute, or
 * if at client's side the parameter has an IN attribute.
 */
bool CBEMarshalFunction::DoMarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext)
{
    if (IsComponentSide())
    {
	    if (pParameter->FindAttribute(ATTR_OUT))
            return true;
    }
    else
    {
        if (pParameter->FindAttribute(ATTR_IN))
            return true;
    }
    return false;
}

/** \brief checks if this parameter should be unmarshalled or not
 *  \param pParameter the parameter to check
 *  \param pContext the context of this unmarshalling
 *  \return true if this parameter should be unmarshalled
 */
bool CBEMarshalFunction::DoUnmarshalParameter(CBETypedDeclarator * pParameter, CBEContext * pContext)
{
    return false;
}

/** \brief writes the message buffer parameter
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \param bComma true if a comma has to be written before the parameter
 */
void CBEMarshalFunction::WriteAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma)
{
    assert(m_pMsgBuffer);
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
void CBEMarshalFunction::WriteCallAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma)
{
    assert(m_pMsgBuffer);
    if (bComma)
    {
        pFile->Print(",\n");
        pFile->PrintIndent("");
    }
	if (m_bCastMsgBufferOnCall)
		m_pMsgBuffer->GetType()->WriteCast(pFile, true, pContext);
    WriteCallParameter(pFile, m_pMsgBuffer, pContext);
    CBEOperationFunction::WriteCallAfterParameters(pFile, pContext, true);
}

/** \brief tries to find a parameter by its type
 *  \param sTypeName the name of the type
 *  \return a reference to the found parameter
 */
CBETypedDeclarator * CBEMarshalFunction::FindParameterType(String sTypeName)
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
bool CBEMarshalFunction::AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext)
{
    // get class's message buffer
    CBEClass *pClass = GetClass();
    assert(pClass);
    // get message buffer type
    CBEMsgBufferType *pMsgBuffer = pClass->GetMessageBuffer();
    assert(pMsgBuffer);
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
    // since we reply to a specific message, we have to set the correct counts
    m_pMsgBuffer->ZeroCounts(GetSendDirection());
    // since InitCounts uses MAX to determine counts, the receive direction will
    // have no effect
    m_pMsgBuffer->InitCounts(this, pContext);
    return true;
}

/** \brief gets the direction, which the marshal-parameters have
 *  \return if at client's side DIRECTION_IN, if at server's side DIRECTION_OUT
 *
 * Since this function ignores marshalling parameter this value should be irrelevant
 */
int CBEMarshalFunction::GetSendDirection()
{
    return IsComponentSide() ? DIRECTION_OUT : DIRECTION_IN;
}

/** \brief gets the direction of the unmarshal-parameters
 *  \return if at client's side DIRECTION_OUT, if at server's side DIRECTION_IN
 */
int CBEMarshalFunction::GetReceiveDirection()
{
    return IsComponentSide() ? DIRECTION_IN : DIRECTION_OUT;
}

/**	\brief calculates the size of the function's parameters
 *	\param nDirection the direction to count
 *  \param pContext the context of this calculation
 *	\return the size of the parameters
 *
 * The reply function has the return value as a parameter. The base class' GetSize function adds the size of the return
 * type to the sum of the parameters. Thus the status code for the IPC is counted even though it
 * shouldn't. We have to subtract it from the calculated size.
 */
int CBEMarshalFunction::GetSize(int nDirection, CBEContext *pContext)
{
    int nSize = CBEOperationFunction::GetSize(nDirection, pContext);
	if (nDirection & DIRECTION_OUT)
	    nSize += pContext->GetSizes()->GetExceptionSize();
    return nSize;
}

/**	\brief calculates the size of the function's parameters
 *	\param nDirection the direction to count
 *  \param pContext the context of this calculation
 *	\return the size of the parameters
 */
int CBEMarshalFunction::GetFixedSize(int nDirection,  CBEContext* pContext)
{
    int nSize = CBEOperationFunction::GetFixedSize(nDirection, pContext);
	if (nDirection & DIRECTION_OUT)
	    nSize += pContext->GetSizes()->GetExceptionSize();
    return nSize;
}

/** \brief do not count this function's return var
 *  \param nDirection the direction to count
 *  \param pContext the context of the counting
 *  \return zero
 *
 * The return var (or the variable in m_pReturnValue is the IPC reply code,
 * which should not be counted as a parameter.
 */
int CBEMarshalFunction::GetMaxReturnSize(int nDirection, CBEContext * pContext)
{
    return 0;
}

/** \brief do not count this function's return var
 *  \param nDirection the direction to count
 *  \param pContext the context of the counting
 *  \return zero
 *
 * The return var (or the variable in m_pReturnValue is the IPC reply code,
 * which should not be counted as a parameter.
 */
int CBEMarshalFunction::GetFixedReturnSize(int nDirection, CBEContext * pContext)
{
    return 0;
}

/** \brief do not count this function's return var
 *  \param nDirection the direction to count
 *  \param pContext the context of the counting
 *  \return zero
 *
 * The return var (or the variable in m_pReturnValue is the IPC reply code,
 * which should not be counted as a parameter.
 */
int CBEMarshalFunction::GetReturnSize(int nDirection, CBEContext * pContext)
{
    return 0;
}

/**	\brief writes the return statement of the function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The Marshal function always returns DICE_REPLY for now.
 */
void CBEMarshalFunction::WriteReturn(CBEFile * pFile, CBEContext * pContext)
{
	if (!GetReturnVariable() ||
	    GetReturnType()->IsVoid())
	{
	    pFile->PrintIndent("return;\n");
        return;
	}

    pFile->PrintIndent("return DICE_REPLY;\n");
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return true if should be written
 *
 * A marshal function is written if client's side and IN or if component's side
 * and one of the parameters has an OUT.
 */
bool CBEMarshalFunction::DoWriteFunction(CBEFile * pFile, CBEContext * pContext)
{
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEHeaderFile)))
		if (!IsTargetFile((CBEHeaderFile*)pFile))
			return false;
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEImplementationFile)))
		if (!IsTargetFile((CBEImplementationFile*)pFile))
			return false;
	if (pFile->IsOfFileType(FILETYPE_CLIENT) &&
		(FindAttribute(ATTR_IN)))
		return true;
	if (pFile->GetTarget()->IsKindOf(RUNTIME_CLASS(CBEComponent)))
	{
	    VectorElement *pIter = GetFirstParameter();
		CBETypedDeclarator *pParameter;
		while ((pParameter = GetNextParameter(pIter)) != 0)
		{
		    if (pParameter->FindAttribute(ATTR_OUT))
			    return true;
		}
		if (GetReturnType() &&
			!GetReturnType()->IsVoid())
			return true;
	}
	return false;
}
