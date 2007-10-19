/**
 *  \file    dice/src/be/BESrvLoopFunction.cpp
 *  \brief   contains the implementation of the class CBESrvLoopFunction
 *
 *  \date    01/21/2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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

#include "BESrvLoopFunction.h"
#include "BEContext.h"
#include "BEFile.h"
#include "BEType.h"
#include "BEReplyCodeType.h"
#include "BESwitchCase.h"
#include "BEWaitAnyFunction.h"
#include "BEDispatchFunction.h"
#include "BERoot.h"
#include "BEComponent.h"
#include "BEImplementationFile.h"
#include "BEHeaderFile.h"
#include "BEDeclarator.h"
#include "BEMsgBuffer.h"
#include "BEClassFactory.h"
#include "BENameFactory.h"
#include "Trace.h"
#include "TypeSpec-Type.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "fe/FEStringAttribute.h"
#include "Compiler.h"
#include "Error.h"
#include <cassert>

CBESrvLoopFunction::CBESrvLoopFunction()
: CBEInterfaceFunction(FUNCTION_SRV_LOOP)
{
	m_pWaitAnyFunction = 0;
	m_pReplyAnyWaitAnyFunction = 0;
	m_pDispatchFunction = 0;
}

/** \brief destructor of target class */
CBESrvLoopFunction::~CBESrvLoopFunction()
{ }

/** \brief creates the server loop function for the given interface
 *  \param pFEInterface the respective front-end interface
 *  \return true if successful
 *
 * A server loop function does usually not return anything. However, it might
 * be possible to return a status code or something similar. As parameters one
 * might use timeouts or similar.
 *
 * After we created the switch cases, we force them to reset the message
 * buffer type of their functions.  This way, we use in all functions the
 * message buffer type of this server loop.
 */
void CBESrvLoopFunction::CreateBackEnd(CFEInterface * pFEInterface, bool bComponentSide)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);

	// set target file name
	SetTargetFileName(pFEInterface);
	// set name
	SetComponentSide(bComponentSide);
	SetFunctionName(pFEInterface, FUNCTION_SRV_LOOP);

	CBEInterfaceFunction::CreateBackEnd(pFEInterface, bComponentSide);
	// set source line number to last number of interface
	m_sourceLoc.setBeginLine(pFEInterface->m_sourceLoc.getEndLine());

	// set own message buffer
	AddMessageBuffer();
	AddLocalVariable(GetMessageBuffer());

	CreateTrace();
	// CORBA_Object should not have any pointers (its a pointer type itself)
	// set Corba parameters to variables without pointers
	CBETypedDeclarator *pCorbaEnv = GetEnvironment();
	if (pCorbaEnv)
		AddLocalVariable(pCorbaEnv);
	if (GetObject())
		AddLocalVariable(GetObject());

	string exc = string(__func__);
	// create reply code as local variable
	if (!AddReplyVariable())
	{
		exc += " failed, because reply variable could not be added.";
		throw new error::create_error(exc);
	}

	// create opcode as local variable
	if (!AddOpcodeVariable())
	{
		exc += " failed, because opcode variable could not be added.";
		throw new error::create_error(exc);
	}

	// parameters
	AddParameters();

	m_pWaitAnyFunction = static_cast<CBEWaitAnyFunction*>
		(FindGlobalFunction(pFEInterface, FUNCTION_WAIT_ANY));
	assert(m_pWaitAnyFunction);
	SetCallVariables(m_pWaitAnyFunction);

	m_pReplyAnyWaitAnyFunction = static_cast<CBEWaitAnyFunction*>
		(FindGlobalFunction(pFEInterface, FUNCTION_REPLY_WAIT));
	assert(m_pReplyAnyWaitAnyFunction);
	SetCallVariables(m_pReplyAnyWaitAnyFunction);

	m_pDispatchFunction = static_cast<CBEDispatchFunction*>
		(FindGlobalFunction(pFEInterface, FUNCTION_DISPATCH));
	assert(m_pDispatchFunction);
	SetCallVariables(m_pDispatchFunction);

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s returns true\n", __func__);
}

/** \brief looks globally for a function of a specific type
 *  \param pFEInterface the front-end interface used as reference
 *  \param nFunctionType the type of function to look for
 *  \return a reference to the function if found
 */
CBEFunction* CBESrvLoopFunction::FindGlobalFunction(CFEInterface *pFEInterface, FUNCTION_TYPE nFunctionType)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sFuncName = pNF->GetFunctionName(pFEInterface, nFunctionType, IsComponentSide());
	CBERoot *pRoot = GetSpecificParent<CBERoot>();
	assert(pRoot);
	CBEFunction *pFunction = pRoot->FindFunction(sFuncName, nFunctionType);
	return pFunction;
}

/** \brief sets the call variables for a function
 *  \param pFunction the function to set the call vars in
 */
void CBESrvLoopFunction::SetCallVariables(CBEFunction *pFunction)
{
	assert(pFunction);
	if (GetObject())
	{
		CBEDeclarator *pDecl = GetObject()->m_Declarators.First();
		pFunction->SetCallVariable(pDecl->GetName(), pDecl->GetStars(),
			pDecl->GetName());
	}
	if (GetEnvironment())
	{
		CBEDeclarator *pDecl = GetEnvironment()->m_Declarators.First();
		pFunction->SetCallVariable(pDecl->GetName(), pDecl->GetStars(),
			pDecl->GetName());
	}
	if (GetMessageBuffer())
	{
		CBEDeclarator *pDecl = GetMessageBuffer()->m_Declarators.First();
		pFunction->SetCallVariable(pDecl->GetName(), pDecl->GetStars(),
			pDecl->GetName());
	}
}

/** \brief adds the reply code variable locally
 *  \return true if successful
 */
bool
CBESrvLoopFunction::AddReplyVariable()
{
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBEReplyCodeType *pReplyType = pCF->GetNewReplyCodeType();
	CBETypedDeclarator *pVariable = pCF->GetNewTypedDeclarator();
	pReplyType->SetParent(pVariable);
	AddLocalVariable(pVariable);
	pReplyType->CreateBackEnd();
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sReply = pNF->GetReplyCodeVariable();
	pVariable->CreateBackEnd(pReplyType, sReply);
	delete pReplyType;
	return true;
}

/** \brief adds the opcode variable locally
 *  \return true if successful
 */
bool
CBESrvLoopFunction::AddOpcodeVariable()
{
	CBETypedDeclarator *pOpcode = CreateOpcodeVariable();
	AddLocalVariable(pOpcode);
	return true;
}

/** \brief creates the corba object variable
 *  \return true if successful
 *
 * Since the server loop has to maintain a copy of the sender of a request and
 * CORBA_Object is only a reference to the sender, we have to create a
 * hard-copy and init the original corba object with a reference to the
 * hard-copy.
 */
void
CBESrvLoopFunction::CreateObject()
{
	CBEInterfaceFunction::CreateObject();

	CBENameFactory *pNF = CBENameFactory::Instance();
	CBEClassFactory *pCF = CBEClassFactory::Instance();

	// create CORBA_Object_base type
	string sTypeName("CORBA_Object_base");
	string sName = string("_") + pNF->GetCorbaObjectVariable();
	CBETypedDeclarator *pBaseObject = pCF->GetNewTypedDeclarator();
	pBaseObject->SetParent(this);
	pBaseObject->CreateBackEnd(sTypeName, sName, 0);
	// add as local variable
	AddLocalVariable(pBaseObject);
	// set init string to invalid id
	pBaseObject->SetDefaultInitString(string("INVALID_CORBA_OBJECT_BASE"));
	// do not set init string for corba object, since we don't know in which
	// order they are declared -> use WriteVariableInitialization instead
}

/** \brief writes the invocation of the message transfer
 *  \param pFile the file to write to
 *
 * This implementation calls the underlying message trasnfer mechanisms
 */
void
CBESrvLoopFunction::WriteInvocation(CBEFile& pFile)
{
	pFile << "\t/* invoke */\n";
}

/** \brief writes the server loop's function body
 *  \param pFile the target file
 */
void CBESrvLoopFunction::WriteBody(CBEFile& pFile)
{
	// write variable declaration and initialization
	WriteVariableDeclaration(pFile);
	WriteVariableInitialization(pFile);
	// write loop (contains switch)
	WriteLoop(pFile);
	// write return
	WriteReturn(pFile);
}

/** \brief writes the declaration of the variables
 *  \param pFile the file to write to
 *
 * For C++ do not declare object and environment (they are class members);
 */
void
CBESrvLoopFunction::WriteVariableDeclaration(CBEFile& pFile)
{
	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_C))
	{
		CBEInterfaceFunction::WriteVariableDeclaration(pFile);
		return;
	}

	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
	{
		vector<CBETypedDeclarator*>::iterator iter;
		for (iter = m_LocalVariables.begin();
			iter != m_LocalVariables.end();
			iter++)
		{
			if (*iter == GetObject())
				continue;
			if (*iter == GetEnvironment())
				continue;
			(*iter)->WriteInitDeclaration(pFile);
		}

		if (m_pTrace)
			m_pTrace->VariableDeclaration(pFile, this);
	}
}

/** \brief writes the variable initializations of this function
 *  \param pFile the file to write to
 *
 * This implementation should initialize the message buffer and the pointers
 * of the out variables.  The CROBA stuff does not need to be set, because it
 * is set by the first wait function.  This function is also used to cast the
 * server loop parameter to the CORBA_Environment if it is used.
 */
void
CBESrvLoopFunction::WriteVariableInitialization(CBEFile& pFile)
{
	if (m_pTrace)
		m_pTrace->InitServer(pFile, this);

	WriteObjectInitialization(pFile);
	// do CORBA_ENvironment cast before message buffer init, because it might
	// contain values used to init message buffer
	WriteEnvironmentInitialization(pFile);
	// init message buffer
	CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
	pMsgBuffer->WriteInitialization(pFile, this, 0 , CMsgStructType::Generic);
}

/** \brief writes the loop
 *  \param pFile the file to write to
 */
void CBESrvLoopFunction::WriteLoop(CBEFile& pFile)
{
	if (m_pTrace)
		m_pTrace->BeforeLoop(pFile, this);

	CBENameFactory *pNF = CBENameFactory::Instance();
	string sOpcodeVar = pNF->GetOpcodeVariable();
	m_pWaitAnyFunction->WriteCall(pFile, sOpcodeVar, true);

	pFile << "\twhile (1)\n";
	pFile << "\t{\n";
	++pFile;

	// write switch
	WriteDispatchInvocation(pFile);

	string sReply = pNF->GetReplyCodeVariable();
	// check if we should reply or not
	pFile << "\tif (" << sReply << " == DICE_REPLY)\n";
	++pFile;
	m_pReplyAnyWaitAnyFunction->WriteCall(pFile, sOpcodeVar, true);
	--pFile << "\telse\n";
	++pFile;
	m_pWaitAnyFunction->WriteCall(pFile, sOpcodeVar, true);
	--(--pFile) << "\t}\n";
}

/** \brief writes the dispatcher invocation
 *  \param pFile the file to write to
 */
void
CBESrvLoopFunction::WriteDispatchInvocation(CBEFile& pFile)
{
	if (m_pDispatchFunction)
	{
		if (m_pTrace)
			m_pTrace->BeforeDispatch(pFile, this);

		CBENameFactory *pNF = CBENameFactory::Instance();
		string sReply = pNF->GetReplyCodeVariable();
		m_pDispatchFunction->WriteCall(pFile, sReply, true);

		if (m_pTrace)
			m_pTrace->AfterDispatch(pFile, this);
	}
}

/** \brief test if this function should be written inline
 *  \param pFile the file to write to
 *
 * Never write this function inline.
 */
bool
CBESrvLoopFunction::DoWriteFunctionInline(CBEFile& /*pFile*/)
{
	return false;
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \return  true if this function should be written
 *
 * A server loop is only written at the component's side.
 */
bool CBESrvLoopFunction::DoWriteFunction(CBEFile* pFile)
{
	if (!IsTargetFile(pFile))
		return false;
	return pFile->IsOfFileType(FILETYPE_COMPONENT);
}

/** \brief determines the direction, the server loop sends to
 *  \return DIRECTION_OUT
 */
CMsgStructType CBESrvLoopFunction::GetSendDirection()
{
	return CMsgStructType::Out;
}

/** \brief determined the direction the server loop receives from
 *  \return DIRECTION_IN
 */
CMsgStructType CBESrvLoopFunction::GetReceiveDirection()
{
	return CMsgStructType::In;
}

/** \brief write the initialization code for the CORBA_Environment
 *  \param pFile the file to write to
 */
void
CBESrvLoopFunction::WriteEnvironmentInitialization(CBEFile& pFile)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sServerParam = pNF->GetServerParameterName();
	CBETypedDeclarator *pEnv = GetEnvironment();
	CBEDeclarator *pDecl = pEnv->m_Declarators.First();
	// if (server-param)
	//   corba-env = (CORBA_Env*)server-param;
	pFile << "\tif (" << sServerParam << ")\n";
	++pFile << "\t" << pDecl->GetName() << " = (";
	pEnv->WriteType(pFile);
	pFile << "*)" << sServerParam << ";\n";

	// should be set to default environment, but if
	// it is a pointer, we cannot, but have to allocate memory first...
	--pFile << "\telse\n";
	pFile << "\t{\n";
	// corba-env = (CORBA_Env*)malloc(sizeof(CORBA_Env));
	++pFile << "\t" << pDecl->GetName() << " = ";
	pEnv->GetType()->WriteCast(pFile, true);
	pFile << "_dice_alloca(sizeof";
	pEnv->GetType()->WriteCast(pFile, false);
	pFile << ");\n";

	WriteDefaultEnvAssignment(pFile);

	--pFile << "\t}\n";
}

/** \brief writes the assignment of the default environment to the env var
 *  \param pFile the file to write to
 */
void
CBESrvLoopFunction::WriteDefaultEnvAssignment(CBEFile& pFile)
{
	string sName = GetEnvironment()->m_Declarators.First()->GetName();

	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_C))
	{
		// *corba-env = dice_default_env;
		pFile << "\t*" << sName << " = ";
		GetEnvironment()->GetType()->WriteCast(pFile, false);
		pFile << "dice_default_server_environment;\n";
	}
	else if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
	{
		pFile << "\tDICE_EXCEPTION_MAJOR(" << sName <<
			") = CORBA_NO_EXCEPTION;\n";
		pFile << "\tDICE_EXCEPTION_MINOR(" << sName <<
			") = CORBA_DICE_EXCEPTION_NONE;\n";
		pFile << "\t" << sName << "->_p.param = 0;\n";
		pFile << "\t" << sName << "->user_data = 0;\n";
		pFile << "\tfor (int i=0; i<DICE_PTRS_MAX; i++)\n";
		++pFile << "\t" << sName << "->ptrs[i] = 0;\n";
		--pFile << "\t" << sName << "->ptrs_cur = 0;\n";
	}
}

/** \brief write the initialization code for the CORBA_Environment
 *  \param pFile the file to write to
 */
void
CBESrvLoopFunction::WriteObjectInitialization(CBEFile& pFile)
{
	// set init string: "<corba obj> = &<corba obj base>;"
	CBETypedDeclarator *pObj = GetObject();
	assert(pObj);
	string sObj = pObj->m_Declarators.First()->GetName();
	pFile << "\t" << sObj << " = &_" << sObj << ";\n";
}

/** \brief writes the attributes for the function
 *  \param pFile the file to write to
 *
 * This implementation adds the "noreturn" attribute to the declaration
 */
void
CBESrvLoopFunction::WriteFunctionAttributes(CBEFile& pFile)
{
	pFile << " __attribute__((noreturn))";
}

/** \brief writes the return statement of this function
 *  \param pFile the file to write to
 *
 * Since this function is "noreturn" it is not allowed to have a return
 * statement.
 */
void
CBESrvLoopFunction::WriteReturn(CBEFile& /*pFile*/)
{}

/** \brief manipulates the message buffer
 *  \param pMsgBuffer the message buffer to initialize
 *  \return true if successful
 */
void
CBESrvLoopFunction::MsgBufferInitialization(CBEMsgBuffer *pMsgBuffer)
{
	CBEInterfaceFunction::MsgBufferInitialization(pMsgBuffer);
	// add as local variable and remove pointer
	CBEDeclarator *pDecl = pMsgBuffer->m_Declarators.First();
	pDecl->SetStars(0);
}

/** \brief add parameters after all other parameters
 *
 * The server loop has a special parameter, which is a void pointer.
 * If it is NOT 0, we will cast it to a CORBA_Environment, which
 * is used to set the server loop local CORBA_Environment. If it is
 * 0, we use a default CORBA_Environment in the server loop.
 */
void
CBESrvLoopFunction::AddParameters()
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);

	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBETypedDeclarator *pParameter = pCF->GetNewTypedDeclarator();
	pParameter->SetParent(this);

	CBEType *pType = pCF->GetNewType(TYPE_VOID);
	pType->SetParent(pParameter);
	pType->CreateBackEnd(false, 0, TYPE_VOID);

	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName = pNF->GetServerParameterName();
	pParameter->CreateBackEnd(pType, sName);
	delete pType; // cloned in CBETypedDeclarator::CreateBackEnd
	pParameter->m_Declarators.First()->IncStars(1);

	m_Parameters.Add(pParameter);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "%s returns true.\n", __func__);
}

