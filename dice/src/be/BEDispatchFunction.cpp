/**
 *	\file	dice/src/be/BEDispatchFunction.cpp
 *	\brief	contains the implementation of the class CBEDispatchFunction
 *
 *	\date	10/10/2003
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
#include "be/BEDispatchFunction.h"
#include "be/BESwitchCase.h"
#include "be/BEContext.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEMsgBufferType.h"
#include "be/BEOpcodeType.h"
#include "be/BEReplyCodeType.h"
#include "be/BEMarshaller.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BEComponent.h"

#include "fe/FEInterface.h"
#include "fe/FEStringAttribute.h"
#include "fe/FEOperation.h"

IMPLEMENT_DYNAMIC(CBEDispatchFunction);

CBEDispatchFunction::CBEDispatchFunction()
: m_vSwitchCases(RUNTIME_CLASS(CBESwitchCase))
{
    IMPLEMENT_DYNAMIC_BASE(CBEDispatchFunction, CBEInterfaceFunction);
}

CBEDispatchFunction::CBEDispatchFunction(CBEDispatchFunction & src)
: CBEInterfaceFunction(src),
  m_vSwitchCases(RUNTIME_CLASS(CBESwitchCase))
{
    m_vSwitchCases.Add(&(src.m_vSwitchCases));
    m_vSwitchCases.SetParentOfElements(this);
    m_sDefaultFunction = src.m_sDefaultFunction;
    IMPLEMENT_DYNAMIC_BASE(CBEDispatchFunction, CBEInterfaceFunction);
}

/**	\brief destructor of target class */
CBEDispatchFunction::~CBEDispatchFunction()
{
    m_vSwitchCases.DeleteAll();
}

/**	\brief creates the dispatch function for the given interface
 *	\param pFEInterface the respective front-end interface
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * The dispatch function return the reply code which determines if the server
 * loop should return a reply to the client or not.
 *
 * After we created the switch cases, we force them to reset the message buffer type of their functions.
 * This way, we use in all functions the message buffer type of this server loop.
 */
bool CBEDispatchFunction::CreateBackEnd(CFEInterface * pFEInterface, CBEContext * pContext)
{
    pContext->SetFunctionType(FUNCTION_DISPATCH);
	// set target file name
	SetTargetFileName(pFEInterface, pContext);
    // set name
    m_sName = pContext->GetNameFactory()->GetFunctionName(pFEInterface, pContext);

    if (!CBEInterfaceFunction::CreateBackEnd(pFEInterface, pContext))
        return false;

    // add functions
    if (!AddSwitchCases(pFEInterface, pContext))
        return false;

    // set own message buffer
    if (!AddMessageBuffer(pFEInterface, pContext))
        return false;

    // return type -> set to IPC reply code
	CBEReplyCodeType *pReplyType = pContext->GetClassFactory()->GetNewReplyCodeType();
	if (!pReplyType->CreateBackEnd(pContext))
	{
	    delete pReplyType;
		VERBOSE("CBEDispatchFunction::CreateBE failed because return var could not be set\n");
		return false;
	}
    String sReply = pContext->GetNameFactory()->GetReplyCodeVariable(pContext);
    if (!SetReturnVar(pReplyType, sReply, pContext))
    {
	    delete pReplyType;
        VERBOSE("CBEDispatchFunction::CreateBE failed because return var could not be set\n");
        return false;
    }

	// set message buffer type
    VectorElement *pIter = GetFirstSwitchCase();
    CBESwitchCase *pSwitchCase;
    while ((pSwitchCase = GetNextSwitchCase(pIter)) != 0)
    {
        pSwitchCase->SetMessageBufferType(pContext);
    }

	// check if interface has default function and add its name if available
    if (pFEInterface->FindAttribute(ATTR_DEFAULT_FUNCTION))
    {
        CFEStringAttribute *pDefaultFunc = ((CFEStringAttribute*)(pFEInterface->FindAttribute(ATTR_DEFAULT_FUNCTION)));
        m_sDefaultFunction = pDefaultFunc->GetString();
    }

	// add message buffer and environment as parameters

	return true;
}

/** \brief adds the message buffer parameter
 *  \param pFEInterface the respective front-end interface to use as reference
 *  \param pContext the context of the create process
 *  \return true if the creation was successful
 */
bool CBEDispatchFunction::AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext)
{
    // get class's message buffer
    CBEClass *pClass = GetClass();
    assert(pClass);
    // get message buffer type
    CBEMsgBufferType *pMsgBuffer = pClass->GetMessageBuffer();
    assert(pMsgBuffer);
    // msg buffer has to be initialized with correct values,
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

/**	\brief adds the functions for the given front-end interface
 *	\param pFEInterface the interface to add the functions for
 *	\param pContext the context of the code generation
 */
bool CBEDispatchFunction::AddSwitchCases(CFEInterface * pFEInterface, CBEContext * pContext)
{
    if (!pFEInterface)
        return true;

    VectorElement *pIter = pFEInterface->GetFirstOperation();
    CFEOperation *pFEOperation;
    while ((pFEOperation = pFEInterface->GetNextOperation(pIter)) != 0)
    {
	    // skip OUT functions
		if (pFEOperation->FindAttribute(ATTR_OUT))
		    continue;
        CBESwitchCase *pFunction = pContext->GetClassFactory()->GetNewSwitchCase();
        AddSwitchCase(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveSwitchCase(pFunction);
            delete pFunction;
            return false;
        }
    }

    pIter = pFEInterface->GetFirstBaseInterface();
    CFEInterface *pFEBase;
    while ((pFEBase = pFEInterface->GetNextBaseInterface(pIter)) != 0)
    {
        if (!AddSwitchCases(pFEBase, pContext))
            return false;
    }

    return true;
}

/**	\brief adds a new function to the functions vector
 *	\param pFunction the function to add
 */
void CBEDispatchFunction::AddSwitchCase(CBESwitchCase * pFunction)
{
    if (!pFunction)
        return;
    m_vSwitchCases.Add(pFunction);
    pFunction->SetParent(this);
}

/**	\brief removes a function from the functions vector
 *	\param pFunction the function to remove
 */
void CBEDispatchFunction::RemoveSwitchCase(CBESwitchCase * pFunction)
{
    if (!pFunction)
        return;
    m_vSwitchCases.Remove(pFunction);
}

/**	\brief retrieves a pointer to the first function
 *	\return a pointer to the first function
 */
VectorElement *CBEDispatchFunction::GetFirstSwitchCase()
{
    return m_vSwitchCases.GetFirst();
}

/**	\brief retrieves reference to the next function
 *	\param pIter the pointer to the next function
 *	\return a reference to the next function
 */
CBESwitchCase *CBEDispatchFunction::GetNextSwitchCase(VectorElement * &pIter)
{
    if (!pIter)
	    return 0;
    CBESwitchCase *pRet = (CBESwitchCase *) pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
	    return GetNextSwitchCase(pIter);
    return pRet;
}

/** \brief writes the message buffer parameter
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \param bComma true if a comma has to be written before the parameter
 */
void CBEDispatchFunction::WriteAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma)
{
    assert(m_pMsgBuffer);
    if (bComma)
    {
        pFile->Print(",\n");
        pFile->PrintIndent("");
    }
    WriteParameter(pFile, m_pMsgBuffer, pContext);
	// writes environment parameter
    CBEInterfaceFunction::WriteAfterParameters(pFile, pContext, true);
}

/** \brief writes the message buffer parameter
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \param bComma true if a comma has to be written before the parameter
 */
void CBEDispatchFunction::WriteCallAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma)
{
    assert(m_pMsgBuffer);
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
	// writes environment parameter
    CBEInterfaceFunction::WriteCallAfterParameters(pFile, pContext, true);
}

/**	\brief writes the variable declarations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * Variables declared for the dispatch include:
 * - the opcode
 */
void CBEDispatchFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
	// declare local exception variable
	// if we do handle opcode exception
	if (m_sDefaultFunction.IsEmpty())
		WriteExceptionWordDeclaration(pFile, false, pContext);

	// write opcode
    CBEOpcodeType *pOpcodeType = pContext->GetClassFactory()->GetNewOpcodeType();
    if (!pOpcodeType->CreateBackEnd(pContext))
    {
        delete pOpcodeType;
        return;
    }
    String sOpcodeVar = pContext->GetNameFactory()->GetOpcodeVariable(pContext);
    pFile->PrintIndent("");
    pOpcodeType->Write(pFile, pContext);
    pFile->Print(" %s;\n", (const char *) sOpcodeVar);
    delete pOpcodeType;

	// reply code
    m_pReturnVar->WriteInitDeclaration(pFile, String("DICE_REPLY"), pContext);
}

/**	\brief writes the variable initializations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation should initialize the message buffer and the pointers of the out variables.
 * The CROBA stuff does not need to be set, because it is set by the first wait function.
 * This function is also used to cast the server loop parameter to the CORBA_Environment if it is
 * used.
 */
void CBEDispatchFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    // create fake opcode variable
	CBETypedDeclarator *pOpcode = pContext->GetClassFactory()->GetNewTypedDeclarator();
	pOpcode->SetParent(this);

	// get opcode from message buffer
    CBEOpcodeType *pOpcodeType = pContext->GetClassFactory()->GetNewOpcodeType();
    pOpcodeType->SetParent(pOpcode);
    if (!pOpcodeType->CreateBackEnd(pContext))
    {
        delete pOpcodeType;
        return;
    }
    String sOpcodeVar = pContext->GetNameFactory()->GetOpcodeVariable(pContext);
	if (!pOpcode->CreateBackEnd(pOpcodeType, sOpcodeVar, pContext))
	{
		delete pOpcodeType;
	    delete pOpcode;
		return;
	}
	bool bUseConstOffset = true;
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    pMarshaller->Unmarshal(pFile, pOpcode, 0, bUseConstOffset, true, pContext);
    delete pMarshaller;
    delete pOpcodeType;
	delete pOpcode;
}

/**	\brief writes the switch statement
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 */
void CBEDispatchFunction::WriteSwitch(CBEFile * pFile, CBEContext * pContext)
{
    String sOpcodeVar = pContext->GetNameFactory()->GetOpcodeVariable(pContext);

    pFile->PrintIndent("switch (%s)\n", (const char *) sOpcodeVar);
    pFile->PrintIndent("{\n");

    // iterate over functions
    assert(m_pClass);
    VectorElement *pIter = GetFirstSwitchCase();
    CBESwitchCase *pFunction;
    while ((pFunction = GetNextSwitchCase(pIter)) != 0)
    {
        pFunction->Write(pFile, pContext);
    }

    // writes default case
    WriteDefaultCase(pFile, pContext);

    pFile->PrintIndent("}\n");
}

/** \brief writes the default case of the switch statetment
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * The default case is usually empty. It is only used if the opcode does not match any
 * of the defined opcodes. Because the switch statement expects a valid opcode after the
 * function returns, it hastohave the format:
 * &lt;opcode type&gt; &lt;name&gt;(&lt;corba object&gt;*, &lt;msgbuffer type&gt;*, &lt;corba environment&gt;*)
 *
 * An alternative is to call the wait-any function.
 */
void CBEDispatchFunction::WriteDefaultCase(CBEFile *pFile, CBEContext *pContext)
{
    pFile->PrintIndent("default:\n");
    pFile->IncIndent();
    if (m_sDefaultFunction.IsEmpty())
        WriteDefaultCaseWithoutDefaultFunc(pFile, pContext);
    else
        WriteDefaultCaseWithDefaultFunc(pFile, pContext);
    pFile->PrintIndent("break;\n");
    pFile->DecIndent();
}

/** \brief writes the code in the default case if the default function is not available
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBEDispatchFunction::WriteDefaultCaseWithoutDefaultFunc(CBEFile *pFile, CBEContext *pContext)
{
    String sOpcode = pContext->GetNameFactory()->GetOpcodeVariable(pContext);
	pFile->PrintIndent("/* unknown opcode */\n");
	WriteSetWrongOpcodeException(pFile, pContext);
	// send reply
	String sReply = pContext->GetNameFactory()->GetReplyCodeVariable(pContext);
    pFile->PrintIndent("%s = DICE_REPLY;\n", (const char*)sReply);
	// erase the exception
// 	VectorElement *pIter = m_pCorbaEnv->GetFirstDeclarator();
// 	CBEDeclarator *pEnv = m_pCorbaEnv->GetNextDeclarator(pIter);
// 	pFile->PrintIndent("CORBA_exception_free(");
// 	if (pEnv->GetStars() == 0)
// 		pFile->Print("&");
// 	pEnv->WriteName(pFile, pContext);
// 	pFile->Print(");\n");
}

/** \brief writes the code in the default case if the default function is available
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBEDispatchFunction::WriteDefaultCaseWithDefaultFunc(CBEFile *pFile, CBEContext *pContext)
{
    String sOpcode = pContext->GetNameFactory()->GetOpcodeVariable(pContext);
	String sReturn = pContext->GetNameFactory()->GetReturnVariable(pContext);
	String sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
	String sObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
	String sEnv = pContext->GetNameFactory()->GetCorbaEnvironmentVariable(pContext);

	pFile->PrintIndent("return %s(%s, %s, %s);\n", (const char*)m_sDefaultFunction,
					(const char*)sObj, (const char*)sMsgBuffer, (const char*)sEnv);
}

/** \brief writes function declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This implementation adds the decalartion of the default function if defined and used. And
 * it has to declare the reply-any-wait-any function
 */
void CBEDispatchFunction::WriteFunctionDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // call base
    CBEInterfaceFunction::WriteFunctionDeclaration(pFile, pContext);
    // add declaration of default function
    pFile->Print("\n");
    if (!m_sDefaultFunction.IsEmpty())
    {
        CBEMsgBufferType *pMsgBuffer = m_pClass->GetMessageBuffer();
        String sMsgBufferType = pMsgBuffer->GetAlias()->GetName();
        // int &lt;name&gt;(&lt;corba object&gt;, &lt;msg buffer type&gt;*, &lt;corba environment&gt;*)
        pFile->Print("CORBA_int %s(CORBA_Object, %s*, CORBA_Environment*);\n", (const char*)m_sDefaultFunction,
                     (const char*)sMsgBufferType);
    }
}

/** \brief writes the code to set and marshal the exception codes for a wrong opcode
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBEDispatchFunction::WriteSetWrongOpcodeException(CBEFile* pFile, CBEContext* pContext)
{
	// set the exception in the environment
	VectorElement *pIter = m_pCorbaEnv->GetFirstDeclarator();
	CBEDeclarator *pEnv = m_pCorbaEnv->GetNextDeclarator(pIter);
	pFile->PrintIndent("CORBA_exception_set(");
	if (pEnv->GetStars() == 0)
		pFile->Print("&");
	pEnv->WriteName(pFile, pContext);
	pFile->Print(",\n");
	pFile->IncIndent();
	pFile->PrintIndent("CORBA_SYSTEM_EXCEPTION,\n");
	pFile->PrintIndent("CORBA_DICE_EXCEPTION_WRONG_OPCODE,\n");
	pFile->PrintIndent("0);\n");
	pFile->DecIndent();
	// copy from environment to local exception variable
	WriteExceptionWordInitialization(pFile, pContext);
	// marshal exception variable
	bool bUseConstOffset = true;
	WriteMarshalException(pFile, 0, bUseConstOffset, pContext);
}

/** \brief test fi this function should be written
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return  true if this function should be written
 *
 * A server loop is only written at the component's side.
 */
bool CBEDispatchFunction::DoWriteFunction(CBEFile * pFile, CBEContext * pContext)
{
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEHeaderFile)))
		if (!IsTargetFile((CBEHeaderFile*)pFile))
			return false;
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEImplementationFile)))
	{
		if (!IsTargetFile((CBEImplementationFile*)pFile))
			return false;
		// do not write this function implementation if
		// option NO_DISPATCHER is set
	    if (pContext->IsOptionSet(PROGRAM_NO_DISPATCHER))
		    return false;
	}
	return pFile->GetTarget()->IsKindOf(RUNTIME_CLASS(CBEComponent));
}

/**	\brief writes the invocation of the message transfer
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 */
void CBEDispatchFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
}

/**	\brief clean up the mess
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 */
void CBEDispatchFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{
}

/**	\brief writes the server loop's function body
 *	\param pFile the target file
 *	\param pContext the context of the write operation
 */
void CBEDispatchFunction::WriteBody(CBEFile * pFile, CBEContext * pContext)
{
    // write variable declaration and initialization
    WriteVariableDeclaration(pFile, pContext);
    WriteVariableInitialization(pFile, pContext);
    // write loop (contains switch)
    WriteSwitch(pFile, pContext);
    // write clean up
    WriteCleanup(pFile, pContext);
    // write return
    WriteReturn(pFile, pContext);
}
