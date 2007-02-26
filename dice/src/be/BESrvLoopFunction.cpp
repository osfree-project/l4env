/**
 *	\file	dice/src/be/BESrvLoopFunction.cpp
 *	\brief	contains the implementation of the class CBESrvLoopFunction
 *
 *	\date	01/21/2002
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

#include "be/BESrvLoopFunction.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEOpcodeType.h"
#include "be/BEReplyCodeType.h"
#include "be/BESwitchCase.h"
#include "be/BEWaitAnyFunction.h"
#include "be/BEReplyAnyWaitAnyFunction.h"
#include "be/BEDispatchFunction.h"
#include "be/BERoot.h"
#include "be/BEComponent.h"
#include "be/BEImplementationFile.h"
#include "be/BEHeaderFile.h"
#include "be/BEMsgBufferType.h"
#include "be/BEDeclarator.h"

#include "TypeSpec-Type.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "fe/FEStringAttribute.h"

IMPLEMENT_DYNAMIC(CBESrvLoopFunction);

CBESrvLoopFunction::CBESrvLoopFunction()
{
    m_pWaitAnyFunction = 0;
    m_pReplyAnyWaitAnyFunction = 0;
	m_pDispatchFunction = 0;
    IMPLEMENT_DYNAMIC_BASE(CBESrvLoopFunction, CBEInterfaceFunction);
}

CBESrvLoopFunction::CBESrvLoopFunction(CBESrvLoopFunction & src)
: CBEInterfaceFunction(src)
{
    m_pWaitAnyFunction = src.m_pWaitAnyFunction;
    m_pReplyAnyWaitAnyFunction = src.m_pReplyAnyWaitAnyFunction;
	m_pDispatchFunction = src.m_pDispatchFunction;
    IMPLEMENT_DYNAMIC_BASE(CBESrvLoopFunction, CBEInterfaceFunction);
}

/**	\brief destructor of target class */
CBESrvLoopFunction::~CBESrvLoopFunction()
{
}

/**	\brief creates the server loop function for the given interface
 *	\param pFEInterface the respective front-end interface
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * A server loop function does usually not return anything. However, it might be possible to return a
 * status code or something similar. As parameters one might use timeouts or similar.
 *
 * After we created the switch cases, we force them to reset the message buffer type of their functions.
 * This way, we use in all functions the message buffer type of this server loop.
 */
bool CBESrvLoopFunction::CreateBackEnd(CFEInterface * pFEInterface, CBEContext * pContext)
{
    pContext->SetFunctionType(FUNCTION_SRV_LOOP);
	// set target file name
	SetTargetFileName(pFEInterface, pContext);
    // set name
    m_sName = pContext->GetNameFactory()->GetFunctionName(pFEInterface, pContext);

    if (!CBEInterfaceFunction::CreateBackEnd(pFEInterface, pContext))
        return false;

    // set own message buffer
    if (!AddMessageBuffer(pFEInterface, pContext))
        return false;

	// CORBA_Object should not have any pointers (its a pointer type itself)
	// set Corba parameters to variables without pointers
	if (m_pCorbaEnv)
	{
		if (!DoUseParameterAsEnv(pContext))
		{
			VectorElement *pIter = m_pCorbaEnv->GetFirstDeclarator();
			CBEDeclarator *pDecl = m_pCorbaEnv->GetNextDeclarator(pIter);
			pDecl->IncStars(-pDecl->GetStars());
		}
	}

	// no parameters added

    CBERoot *pRoot = GetRoot();
    assert(pRoot);
    // search for wait any function
    int nOldType = pContext->SetFunctionType(FUNCTION_WAIT_ANY);
    String sFuncName = pContext->GetNameFactory()->GetFunctionName(pFEInterface, pContext);
    pContext->SetFunctionType(nOldType);
    m_pWaitAnyFunction = (CBEWaitAnyFunction*)pRoot->FindFunction(sFuncName);
    assert(m_pWaitAnyFunction);
    if (m_pCorbaObject)
	{
	    VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
		CBEDeclarator *pDecl = m_pCorbaObject->GetNextDeclarator(pIter);
		m_pWaitAnyFunction->SetCallVariable(pDecl->GetName(), pDecl->GetStars(), pDecl->GetName(), pContext);
	}
    if (m_pCorbaEnv)
	{
	    VectorElement *pIter = m_pCorbaEnv->GetFirstDeclarator();
		CBEDeclarator *pDecl = m_pCorbaEnv->GetNextDeclarator(pIter);
		m_pWaitAnyFunction->SetCallVariable(pDecl->GetName(), pDecl->GetStars(), pDecl->GetName(), pContext);
	}

	// search for reply-any-wait-any function
	nOldType = pContext->SetFunctionType(FUNCTION_REPLY_ANY_WAIT_ANY);
	sFuncName = pContext->GetNameFactory()->GetFunctionName(pFEInterface, pContext);
	pContext->SetFunctionType(nOldType);
	m_pReplyAnyWaitAnyFunction = (CBEReplyAnyWaitAnyFunction*)pRoot->FindFunction(sFuncName);
	assert(m_pReplyAnyWaitAnyFunction);
	if (m_pCorbaObject)
	{
	    VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
		CBEDeclarator *pDecl = m_pCorbaObject->GetNextDeclarator(pIter);
		m_pReplyAnyWaitAnyFunction->SetCallVariable(pDecl->GetName(), pDecl->GetStars(), pDecl->GetName(), pContext);
	}
	if (m_pCorbaEnv)
	{
	    VectorElement *pIter = m_pCorbaEnv->GetFirstDeclarator();
		CBEDeclarator *pDecl = m_pCorbaEnv->GetNextDeclarator(pIter);
		m_pReplyAnyWaitAnyFunction->SetCallVariable(pDecl->GetName(), pDecl->GetStars(), pDecl->GetName(), pContext);
	}

	// search for dispatch function
	nOldType = pContext->SetFunctionType(FUNCTION_DISPATCH);
	sFuncName = pContext->GetNameFactory()->GetFunctionName(pFEInterface, pContext);
	pContext->SetFunctionType(nOldType);
	m_pDispatchFunction = (CBEDispatchFunction*)pRoot->FindFunction(sFuncName);
	assert(m_pDispatchFunction);
	if (m_pCorbaObject)
	{
	    VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
		CBEDeclarator *pDecl = m_pCorbaObject->GetNextDeclarator(pIter);
		m_pDispatchFunction->SetCallVariable(pDecl->GetName(), pDecl->GetStars(), pDecl->GetName(), pContext);
	}
	if (m_pCorbaEnv)
	{
	    VectorElement *pIter = m_pCorbaEnv->GetFirstDeclarator();
		CBEDeclarator *pDecl = m_pCorbaEnv->GetNextDeclarator(pIter);
		m_pDispatchFunction->SetCallVariable(pDecl->GetName(), pDecl->GetStars(), pDecl->GetName(), pContext);
	}
	if (m_pMsgBuffer)
	{
	    VectorElement *pIter = m_pMsgBuffer->GetFirstDeclarator();
		CBEDeclarator *pDecl = m_pMsgBuffer->GetNextDeclarator(pIter);
		m_pDispatchFunction->SetCallVariable(pDecl->GetName(), pDecl->GetStars(), pDecl->GetName(), pContext);
	}

    return true;
}

/** \brief write the declaration of the CORBA_Object variable
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBESrvLoopFunction::WriteCorbaObjectDeclaration(CBEFile *pFile, CBEContext *pContext)
{
    if (m_pCorbaObject)
    {
        pFile->PrintIndent("");
        m_pCorbaObject->WriteDeclaration(pFile, pContext);
        pFile->Print("; // is client id\n");
    }
}

/** \brief write the declaration of the CORBA_Environment variable
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBESrvLoopFunction::WriteCorbaEnvironmentDeclaration(CBEFile *pFile, CBEContext *pContext)
{
    // we always have a CORBA_Environment, but sometimes
    // it is set as a cast from the server parameter.
    if (m_pCorbaEnv)
    {
        pFile->PrintIndent("");
        m_pCorbaEnv->WriteDeclaration(pFile, pContext);
        if (!DoUseParameterAsEnv(pContext))
            pFile->Print(" = dice_default_server_environment");
        pFile->Print(";\n");
    }
}

/**	\brief writes the variable declarations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The variable declarations of the call function include the message buffer for send and receive.
 * Variables declared for the server loop include:
 * - the CORBA_Object
 * - the CORBA_Environment
 * - the opcode
 */
void CBESrvLoopFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // write CORBA stuff
	WriteCorbaObjectDeclaration(pFile, pContext);
	WriteCorbaEnvironmentDeclaration(pFile, pContext);
    // clean up

    // write message buffer
    pFile->PrintIndent("");
    assert(m_pMsgBuffer);
    m_pMsgBuffer->WriteDeclaration(pFile, pContext);
    pFile->Print(";\n");

	// write reply code
	CBEReplyCodeType *pReplyType = pContext->GetClassFactory()->GetNewReplyCodeType();
	if (!pReplyType->CreateBackEnd(pContext))
	{
	    delete pReplyType;
		return;
	}
	String sReply = pContext->GetNameFactory()->GetReplyCodeVariable(pContext);
	pFile->PrintIndent("");
	pReplyType->Write(pFile, pContext);
	pFile->Print(" %s;\n", (const char*)sReply);
	delete pReplyType;

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
void CBESrvLoopFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    // do CORBA_ENvironment cast before message buffer init, because it might
    // contain values used to init message buffer
    WriteEnvironmentInitialization(pFile, pContext);
    // init message buffer
    assert(m_pMsgBuffer);
    m_pMsgBuffer->WriteInitialization(pFile, pContext);
}

/**	\brief writes the invocation of the message transfer
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation calls the underlying message trasnfer mechanisms
 */
void CBESrvLoopFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    pFile->PrintIndent("/* invoke */\n");
}

/**	\brief clean up the mess
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation cleans up allocated memory inside this function
 */
void CBESrvLoopFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{
    pFile->PrintIndent("/* clean up */\n");
}

/**	\brief writes the server loop's function body
 *	\param pFile the target file
 *	\param pContext the context of the write operation
 */
void CBESrvLoopFunction::WriteBody(CBEFile * pFile, CBEContext * pContext)
{
    // write variable declaration and initialization
    WriteVariableDeclaration(pFile, pContext);
    WriteVariableInitialization(pFile, pContext);
    // write loop (contains switch)
    WriteLoop(pFile, pContext);
    // write clean up
    WriteCleanup(pFile, pContext);
    // write return
    WriteReturn(pFile, pContext);
}

/**	\brief do not write any additional parameters
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *	\return true if a parameter has been written
 */
bool CBESrvLoopFunction::WriteBeforeParameters(CBEFile * pFile, CBEContext * pContext)
{
    return false;
}

/**	\brief do not write any additional parameters
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *	\param bComma true if we have to write a comma first
 *
 * The server loop has a special parameter, which is a void pointer.
 * If it is NOT NULL, we will cast it to a CORBA_Environment, which
 * is used to set the server loop local CORBA_Environment. If it is
 * NULL, we use a default CORBA_Environment in the server loop.
 */
void CBESrvLoopFunction::WriteAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma)
{
    if (bComma) // should be false, but who knows...
        pFile->Print(", ");
    String sServerParam = pContext->GetNameFactory()->GetServerParameterName();
    pFile->Print("void* %s", (const char*)sServerParam);
}

/**	\brief writes the loop
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 */
void CBESrvLoopFunction::WriteLoop(CBEFile * pFile, CBEContext * pContext)
{
    String sOpcodeVar = pContext->GetNameFactory()->GetOpcodeVariable(pContext);
    pFile->PrintIndent("");
    m_pWaitAnyFunction->WriteCall(pFile, sOpcodeVar, pContext);
    pFile->Print("\n");

    pFile->PrintIndent("while (1)\n");
    pFile->PrintIndent("{\n");
    pFile->IncIndent();

    // write switch
	String sReply = pContext->GetNameFactory()->GetReplyCodeVariable(pContext);
    if (m_pDispatchFunction)
	{
	    pFile->PrintIndent("%s = ", (const char*)sReply);
	    m_pDispatchFunction->WriteCall(pFile, String(), pContext);
		pFile->Print("\n");
	}

	// check if we should reply or not
	pFile->PrintIndent("if (%s == DICE_REPLY)\n", (const char*)sReply);
	pFile->IncIndent();
	pFile->PrintIndent("");
	m_pReplyAnyWaitAnyFunction->WriteCall(pFile, sOpcodeVar, pContext);
	pFile->Print("\n");
	pFile->DecIndent();
    pFile->PrintIndent("else\n");
	pFile->IncIndent();
	pFile->PrintIndent("");
	m_pWaitAnyFunction->WriteCall(pFile, sOpcodeVar, pContext);
    pFile->Print("\n");
	pFile->DecIndent();

    pFile->DecIndent();
    pFile->PrintIndent("}\n");
}

/** \brief test fi this function should be written
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return  true if this function should be written
 *
 * A server loop is only written at the component's side.
 */
bool CBESrvLoopFunction::DoWriteFunction(CBEFile * pFile, CBEContext * pContext)
{
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEHeaderFile)))
		if (!IsTargetFile((CBEHeaderFile*)pFile))
			return false;
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEImplementationFile)))
		if (!IsTargetFile((CBEImplementationFile*)pFile))
			return false;
	return pFile->GetTarget()->IsKindOf(RUNTIME_CLASS(CBEComponent));
}

/** \brief determines the direction, the server loop sends to
 *  \return DIRECTION_OUT
 */
int CBESrvLoopFunction::GetSendDirection()
{
    return DIRECTION_OUT;
}

/** \brief determined the direction the server loop receives from
 *  \return DIRECTION_IN
 */
int CBESrvLoopFunction::GetReceiveDirection()
{
    return DIRECTION_IN;
}

/** \brief adds the message buffer parameter to this function
 *  \param pFEInterface the front-end interface to use as reference
 *  \param pContext the context of the write process
 *  \return true if successful
 */
bool CBESrvLoopFunction::AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext)
{
    // get class's message buffer
    CBEClass *pClass = GetClass();
    assert(pClass);
    // get message buffer type
    CBEMsgBufferType *pMsgBuffer = pClass->GetMessageBuffer();
    assert(pMsgBuffer);
    // msg buffer not initialized yet
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
    // reset the stars of the declarator
    CBEDeclarator *pName = m_pMsgBuffer->GetAlias();
    pName->IncStars(-pName->GetStars());
    return true;
}

/** \brief tests if the server loop parameter should be cast to the CORBA_Environemt
 *  \param pContext the current context
 *  \return true if the parameter should be cast.
 *
 * We test if the Corba Env is present and if the option is set by the user.
 * This indicates that the user wants to provide some information in the
 * environment.
 */
bool CBESrvLoopFunction::DoUseParameterAsEnv(CBEContext *pContext)
{
    return (m_pCorbaEnv &&
        (pContext->IsOptionSet(PROGRAM_SERVER_PARAMETER) ||
         FindAttribute(ATTR_SERVER_PARAMETER)));
}

/** \brief write the initialization code for the CORBA_Environment
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBESrvLoopFunction::WriteEnvironmentInitialization(CBEFile *pFile, CBEContext *pContext)
{
    if (DoUseParameterAsEnv(pContext))
    {
        String sServerParam = pContext->GetNameFactory()->GetServerParameterName();
        VectorElement *pIter = m_pCorbaEnv->GetFirstDeclarator();
        CBEDeclarator *pDecl = m_pCorbaEnv->GetNextDeclarator(pIter);
        // if (server-param)
        //   corba-env = (CORBA_Env*)server-param;
        pFile->PrintIndent("if (%s)\n", (const char*)sServerParam);
        pFile->IncIndent();
        pFile->PrintIndent("%s = (", (const char*)pDecl->GetName());
        m_pCorbaEnv->WriteType(pFile, pContext, true);
        pFile->Print("*)%s;\n", (const char*)sServerParam);
        pFile->DecIndent();
        // should be set to default environment, but if
        // it is a pointer, we cannot, but have to allocate memory first...
        pFile->PrintIndent("else\n");
        pFile->PrintIndent("{\n");
        pFile->IncIndent();
        // corba-env = (CORBA_Env*)malloc(sizeof(CORBA_Env));
        pFile->PrintIndent("%s = ", (const char*)pDecl->GetName());
        m_pCorbaEnv->GetType()->WriteCast(pFile, true, pContext);
        pFile->Print("_dice_alloca(sizeof");
        m_pCorbaEnv->GetType()->WriteCast(pFile, false, pContext);
        pFile->Print(");\n");
        // *corba-env = dice_default_env;
        pFile->PrintIndent("*%s = ", (const char*)pDecl->GetName());
        m_pCorbaEnv->GetType()->WriteCast(pFile, false, pContext);
        pFile->Print("%s;\n", "dice_default_server_environment");
        pFile->DecIndent();
        pFile->PrintIndent("}\n");
    }
}

/** \brief writes the attributes for the function
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This implementation adds the "noreturn" attribute to the declaration
 */
void CBESrvLoopFunction::WriteFunctionAttributes(CBEFile* pFile,  CBEContext* pContext)
{
    pFile->Print(" __attribute__((noreturn))");
}

/** \brief writes the return statement of this function
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * Since this function is "noreturn" it is not allowed to have a return statement.
 */
void CBESrvLoopFunction::WriteReturn(CBEFile* pFile,  CBEContext* pContext)
{
    /* empty */
}
