/**
 *	\file	dice/src/be/BESrvLoopFunction.cpp
 *	\brief	contains the implementation of the class CBESrvLoopFunction
 *
 *	\date	01/21/2002
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

#include "be/BESrvLoopFunction.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEOpcodeType.h"
#include "be/BESwitchCase.h"
#include "be/BEWaitAnyFunction.h"
#include "be/BEReplyAnyWaitAnyFunction.h"
#include "be/BERoot.h"
#include "be/BEComponent.h"
#include "be/BEImplementationFile.h"
#include "be/BEHeaderFile.h"
#include "be/BEMsgBufferType.h"
#include "be/BEDeclarator.h"

#include "fe/FETypeSpec.h"
#include "fe/FEInterface.h"
#include "fe/FEStringAttribute.h"

IMPLEMENT_DYNAMIC(CBESrvLoopFunction);

CBESrvLoopFunction::CBESrvLoopFunction()
:m_vSwitchCases(RUNTIME_CLASS(CBESwitchCase))
{
    m_pWaitAnyFunction = 0;
    m_pReplyAnyWaitAnyFunction = 0;
    IMPLEMENT_DYNAMIC_BASE(CBESrvLoopFunction, CBEInterfaceFunction);
}

CBESrvLoopFunction::CBESrvLoopFunction(CBESrvLoopFunction & src)
:CBEInterfaceFunction(src),
 m_vSwitchCases(RUNTIME_CLASS(CBESwitchCase))
{
    m_pWaitAnyFunction = src.m_pWaitAnyFunction;
    m_pReplyAnyWaitAnyFunction = src.m_pReplyAnyWaitAnyFunction;
    m_vSwitchCases.Add(&(src.m_vSwitchCases));
    m_vSwitchCases.SetParentOfElements(this);
    m_sDefaultFunction = src.m_sDefaultFunction;
    IMPLEMENT_DYNAMIC_BASE(CBESrvLoopFunction, CBEInterfaceFunction);
}

/**	\brief destructor of target class */
CBESrvLoopFunction::~CBESrvLoopFunction()
{
    m_vSwitchCases.DeleteAll();
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

    // add functions
    if (!AddSwitchCases(pFEInterface, pContext))
        return false;

    // set own message buffer
    if (!AddMessageBuffer(pFEInterface, pContext))
        return false;

    // set Corba parameters to variables without pointers
    if (m_pCorbaObject)
    {
        VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
        CBEDeclarator *pDecl = m_pCorbaObject->GetNextDeclarator(pIter);
        pDecl->IncStars(-pDecl->GetStars());
        SetCallVariable(m_pCorbaObject, pContext);
    }
    if (m_pCorbaEnv)
    {
        if (!DoUseParameterAsEnv(pContext))
        {
            VectorElement *pIter = m_pCorbaEnv->GetFirstDeclarator();
            CBEDeclarator *pDecl = m_pCorbaEnv->GetNextDeclarator(pIter);
            pDecl->IncStars(-pDecl->GetStars());
        }
        SetCallVariable(m_pCorbaEnv, pContext);
    }

    // no parameters added

    // set message buffer type
    VectorElement *pIter = GetFirstSwitchCase();
    CBESwitchCase *pSwitchCase;
    while ((pSwitchCase = GetNextSwitchCase(pIter)) != 0)
    {
        pSwitchCase->SetMessageBufferType(pContext);
    }

    CBERoot *pRoot = GetRoot();
    ASSERT(pRoot);
    // search for wait any function
    int nOldType = pContext->SetFunctionType(FUNCTION_WAIT_ANY);
    String sFuncName = pContext->GetNameFactory()->GetFunctionName(pFEInterface, pContext);
    pContext->SetFunctionType(nOldType);
    m_pWaitAnyFunction = (CBEWaitAnyFunction*)pRoot->FindFunction(sFuncName);
    ASSERT(m_pWaitAnyFunction);
    if (m_pCorbaObject)
        SetCallVariable(m_pWaitAnyFunction, m_pCorbaObject, pContext);
    if (m_pCorbaEnv)
        SetCallVariable(m_pWaitAnyFunction, m_pCorbaEnv, pContext);

    // check if interface has default function and add its name if available
    if (pFEInterface->FindAttribute(ATTR_DEFAULT_FUNCTION))
    {
        CFEStringAttribute *pDefaultFunc = ((CFEStringAttribute*)(pFEInterface->FindAttribute(ATTR_DEFAULT_FUNCTION)));
        m_sDefaultFunction = pDefaultFunc->GetString();
        if (!m_sDefaultFunction.IsEmpty())
        {
            // search for reply-any-wait-any function
            nOldType = pContext->SetFunctionType(FUNCTION_REPLY_ANY_WAIT_ANY);
            sFuncName = pContext->GetNameFactory()->GetFunctionName(pFEInterface, pContext);
            pContext->SetFunctionType(nOldType);
            m_pReplyAnyWaitAnyFunction = (CBEReplyAnyWaitAnyFunction*)pRoot->FindFunction(sFuncName);
            ASSERT(m_pReplyAnyWaitAnyFunction);
            if (m_pCorbaObject)
                SetCallVariable(m_pReplyAnyWaitAnyFunction, m_pCorbaObject, pContext);
            if (m_pCorbaEnv)
                SetCallVariable(m_pReplyAnyWaitAnyFunction, m_pCorbaEnv, pContext);
        }
    }

    return true;
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

    // write CORBA stuff
    // write variables
    if (m_pCorbaObject)
    {
        pFile->PrintIndent("");
        m_pCorbaObject->WriteDeclaration(pFile, pContext);
        pFile->Print("; // is client id\n");
    }
    // we always have a CORBA_Environment, but sometimes
    // it is set as a cast from the server parameter.
    if (m_pCorbaEnv)
    {
        pFile->PrintIndent("");
        m_pCorbaEnv->WriteDeclaration(pFile, pContext);
        if (!DoUseParameterAsEnv(pContext))
            pFile->Print(" = dice_default_environment");
        pFile->Print(";\n");
    }
    // clean up

    // write message buffer
    pFile->PrintIndent("");
    ASSERT(m_pMsgBuffer);
    m_pMsgBuffer->WriteDeclaration(pFile, pContext);
    pFile->Print(";\n");
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
    ASSERT(m_pMsgBuffer);
    m_pMsgBuffer->WriteInitialization(pFile, pContext);
    // init opcode
    CBEOpcodeType *pOpcodeType = pContext->GetClassFactory()->GetNewOpcodeType();
    pOpcodeType->SetParent(this);
    if (!pOpcodeType->CreateBackEnd(pContext))
    {
        delete pOpcodeType;
        return;
    }
    String sOpcodeVar = pContext->GetNameFactory()->GetOpcodeVariable(pContext);
    pFile->PrintIndent("%s = ", (const char *) sOpcodeVar);
    pOpcodeType->WriteZeroInit(pFile, pContext);
    pFile->Print(";\n");
    delete pOpcodeType;
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
    WriteSwitch(pFile, pContext);

    pFile->DecIndent();
    pFile->PrintIndent("}\n");
}

/**	\brief writes the switch statement
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 */
void CBESrvLoopFunction::WriteSwitch(CBEFile * pFile, CBEContext * pContext)
{
    String sOpcodeVar = pContext->GetNameFactory()->GetOpcodeVariable(pContext);

    pFile->PrintIndent("switch (%s)\n", (const char *) sOpcodeVar);
    pFile->PrintIndent("{\n");

    // iterate over functions
    ASSERT(m_pClass);
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


/**	\brief adds the functions for the given front-end interface
 *	\param pFEInterface the interface to add the functions for
 *	\param pContext the context of the code generation
 */
bool CBESrvLoopFunction::AddSwitchCases(CFEInterface * pFEInterface, CBEContext * pContext)
{
    if (!pFEInterface)
        return true;

    VectorElement *pIter = pFEInterface->GetFirstOperation();
    CFEOperation *pFEOperation;
    while ((pFEOperation = pFEInterface->GetNextOperation(pIter)) != 0)
    {
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
void CBESrvLoopFunction::AddSwitchCase(CBESwitchCase * pFunction)
{
    if (!pFunction)
        return;
    m_vSwitchCases.Add(pFunction);
    pFunction->SetParent(this);
}

/**	\brief removes a function from the functions vector
 *	\param pFunction the function to remove
 */
void CBESrvLoopFunction::RemoveSwitchCase(CBESwitchCase * pFunction)
{
    if (!pFunction)
        return;
    m_vSwitchCases.Remove(pFunction);
}

/**	\brief retrieves a pointer to the first function
 *	\return a pointer to the first function
 */
VectorElement *CBESrvLoopFunction::GetFirstSwitchCase()
{
    return m_vSwitchCases.GetFirst();
}

/**	\brief retrieves reference to the next function
 *	\param pIter the pointer to the next function
 *	\return a reference to the next function
 */
CBESwitchCase *CBESrvLoopFunction::GetNextSwitchCase(VectorElement * &pIter)
{
    if (!pIter)
	    return 0;
    CBESwitchCase *pRet = (CBESwitchCase *) pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
	    return GetNextSwitchCase(pIter);
    return pRet;
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
void CBESrvLoopFunction::WriteDefaultCase(CBEFile *pFile, CBEContext *pContext)
{
    String sOpcode = pContext->GetNameFactory()->GetOpcodeVariable(pContext);
    pFile->PrintIndent("default:\n");
    pFile->IncIndent();
    if (m_sDefaultFunction.IsEmpty())
    {
        pFile->PrintIndent("/* unknown opcode */\n");
        pFile->PrintIndent("");
        m_pWaitAnyFunction->WriteCall(pFile, sOpcode, pContext);
        pFile->Print("\n");
    }
    else
    {
        String sReturn = pContext->GetNameFactory()->GetReturnVariable(pContext);
        String sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
        String sObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
        String sEnv = pContext->GetNameFactory()->GetCorbaEnvironmentVariable(pContext);

        pFile->PrintIndent("if (%s(&%s, &%s, &%s) == REPLY)\n", (const char*)m_sDefaultFunction,
                           (const char*)sObj, (const char*)sMsgBuffer, (const char*)sEnv);
        pFile->IncIndent();
        pFile->PrintIndent("");
        m_pReplyAnyWaitAnyFunction->WriteCall(pFile, sOpcode, pContext);
        pFile->Print("\n");
        pFile->DecIndent();
        pFile->PrintIndent("else\n");
        pFile->IncIndent();
        pFile->PrintIndent("");
        m_pWaitAnyFunction->WriteCall(pFile, sOpcode, pContext);
        pFile->Print("\n");
        pFile->DecIndent();
    }
    pFile->PrintIndent("break;\n");
    pFile->DecIndent();
}

/** \brief writes function declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This implementation adds the decalartion of the default function if defined and used. And
 * it has to declare the reply-any-wait-any function
 */
void CBESrvLoopFunction::WriteFunctionDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // call base
    CBEInterfaceFunction::WriteFunctionDeclaration(pFile, pContext);
    // add declaration of default function
    pFile->Print("\n");
    if (!m_sDefaultFunction.IsEmpty())
    {
        CBEMsgBufferType *pMsgBuffer = m_pClass->GetMessageBuffer();
        String sMsgBufferType = pMsgBuffer->GetAlias()->GetName();
        // int &lt;name&gt;(&lt;corba object&gt;*, &lt;msg buffer type&gt;*, &lt;corba environment&gt;*)
        pFile->Print("CORBA_int %s(CORBA_Object*, %s*, CORBA_Environment*);\n", (const char*)m_sDefaultFunction,
                     (const char*)sMsgBufferType);
    }
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

/** \brief sets the call varaiable for a specific function
 *  \param pFunction the function to set this for
 *  \param pParameter the parameter to be changed
 *  \param pContext the context of this operation
 *
 * This implementation should make it easier to set the call variable for
 * the corba object and environment variables. Since they keep their name
 * and onyl change the number of stars, we can use their name as original
 * and call name.
 */
void CBESrvLoopFunction::SetCallVariable(CBEFunction *pFunction, CBETypedDeclarator *pParameter, CBEContext *pContext)
{
    // get name
    VectorElement *pIter = pParameter->GetFirstDeclarator();
    CBEDeclarator *pDecl = pParameter->GetNextDeclarator(pIter);
    // call function's SetCallVariable method
    pFunction->SetCallVariable(pDecl->GetName(), pDecl->GetStars(), pDecl->GetName(), pContext);
}

/** \brief delegates the set-call-variable call to the switch cases
 *  \param pParameter the parameter to be changed
 *  \param pContext the context of this operation
 */
void CBESrvLoopFunction::SetCallVariable(CBETypedDeclarator *pParameter, CBEContext *pContext)
{
    VectorElement *pIter = GetFirstSwitchCase();
    CBESwitchCase *pSwitchCase;
    while ((pSwitchCase = GetNextSwitchCase(pIter)) != 0)
    {
        SetCallVariable(pSwitchCase, pParameter, pContext);
    }
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
    ASSERT(pClass);
    // get message buffer type
    CBEMsgBufferType *pMsgBuffer = pClass->GetMessageBuffer();
    ASSERT(pMsgBuffer);
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
        pFile->Print("alloca(sizeof");
        m_pCorbaEnv->GetType()->WriteCast(pFile, false, pContext);
        pFile->Print(");\n");
        // *corba-env = dice_default_env;
        pFile->PrintIndent("*%s = ", (const char*)pDecl->GetName());
        m_pCorbaEnv->GetType()->WriteCast(pFile, false, pContext);
        pFile->Print("%s;\n", "dice_default_environment");
        pFile->DecIndent();
        pFile->PrintIndent("}\n");
    }
}
