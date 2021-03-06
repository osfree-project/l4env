/**
 *  \file    dice/src/be/BEDispatchFunction.cpp
 *  \brief   contains the implementation of the class CBEDispatchFunction
 *
 *  \date    10/10/2003
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
#include "BEDispatchFunction.h"
#include "BESwitchCase.h"
#include "BEContext.h"
#include "BEDeclarator.h"
#include "BEReplyCodeType.h"
#include "BEMarshaller.h"
#include "BEHeaderFile.h"
#include "BEImplementationFile.h"
#include "BEComponent.h"
#include "BEUserDefinedType.h"
#include "BEMsgBuffer.h"
#include "BEAttribute.h"
#include "BENameFactory.h"
#include "BEClassFactory.h"
#include "TypeSpec-Type.h"
#include "Compiler.h"
#include "Error.h"
#include "fe/FEInterface.h"
#include "fe/FEStringAttribute.h"
#include "fe/FEOperation.h"
#include <cassert>

CBEDispatchFunction::CBEDispatchFunction()
: CBEInterfaceFunction(FUNCTION_DISPATCH),
	m_SwitchCases(0, this),
	m_RangeCases(0, this)
{ }

/** \brief destructor of target class */
CBEDispatchFunction::~CBEDispatchFunction()
{ }

/** \brief creates the dispatch function for the given interface
 *  \param pFEInterface the respective front-end interface
 *  \param bComponentSide true if this function was constructed at component side
 *  \return true if successful
 *
 * The dispatch function return the reply code which determines if the server
 * loop should return a reply to the client or not.
 *
 * After we created the switch cases, we force them to reset the message buffer
 * type of their functions. This way, we use in all functions the message
 * buffer type of this server loop.
 *
 * The dispatch function has four parameters:
 * -# CORBA_Object pointing to sender of request
 * -# opcode of request
 * -# pointer to message buffer
 * -# pointer to CORBA environment
 *
 * The opcode and the message buffer are added using AddParameter at the end
 * of this method, so we have them constructed and can add them in order.  The
 * other two parameters are added using AddBeforeParameters and
 * AddAfterParameters respectively.
 */
void CBEDispatchFunction::CreateBackEnd(CFEInterface * pFEInterface, bool bComponentSide)
{
	// set target file name
	SetTargetFileName(pFEInterface);
	// set name
	SetComponentSide(bComponentSide);
	SetFunctionName(pFEInterface, FUNCTION_DISPATCH);

	CBEInterfaceFunction::CreateBackEnd(pFEInterface, bComponentSide);
	// set source line number to last number of interface
	m_sourceLoc.setBeginLine(pFEInterface->m_sourceLoc.getEndLine());

	// add functions
	AddSwitchCases(pFEInterface);
	// set own message buffer
	AddMessageBuffer();
	// add marshaller and communication class
	CreateMarshaller();
	CreateCommunication();

	// return type -> set to IPC reply code
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBEReplyCodeType *pReplyType = pCF->GetNewReplyCodeType();
	pReplyType->CreateBackEnd();
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sReply = pNF->GetReplyCodeVariable();
	SetReturnVar(pReplyType, sReply);
	// set initializer of reply variable
	CBETypedDeclarator *pReturn = GetReturnVariable();
	pReturn->SetDefaultInitString(string("DICE_REPLY"));

	// set message buffer type
	vector<CBESwitchCase*>::iterator iterS;
	for (iterS = m_SwitchCases.begin();
		iterS != m_SwitchCases.end();
		iterS++)
	{
		(*iterS)->SetMessageBufferType();
		// also set the call variable value of our return variable, which is
		// the reply code. The switch case' SetCallVariable method propagates
		// the call to its respective nested functions.
		CBEDeclarator *pDecl = pReturn->m_Declarators.First();
		(*iterS)->SetCallVariable(pDecl->GetName(), pDecl->GetStars(),
			pDecl->GetName());
	}

	// check if interface has default function and add its name if available
	if (pFEInterface->m_Attributes.Find(ATTR_DEFAULT_FUNCTION))
	{
		CFEStringAttribute *pDefaultFunc = dynamic_cast<CFEStringAttribute*>
			(pFEInterface->m_Attributes.Find(ATTR_DEFAULT_FUNCTION));
		assert(pDefaultFunc);
		m_sDefaultFunction = pDefaultFunc->GetString();
	}
	// create exception word local variable
	if (m_sDefaultFunction.empty())
		AddExceptionVariable();

	// add parameters (eventually adds opcode and message buffer)
	AddParameters();
}

/** \brief adds parameters before all other parameters
 *
 * We use this function to add the opcode, because it should come right after
 * the CORBA_Object before the message buffer, which is added right after the
 * CORBA_Object.
 */
void
CBEDispatchFunction::AddBeforeParameters()
{
	CBEInterfaceFunction::AddBeforeParameters();

	CBETypedDeclarator *pParameter = CreateOpcodeVariable();
	m_Parameters.Add(pParameter);
}

/** \brief adds the functions for the given front-end interface
 *  \param pFEInterface the interface to add the functions for
 */
void CBEDispatchFunction::AddSwitchCases(CFEInterface * pFEInterface)
{
	assert(pFEInterface);

	CBEClassFactory *pCF = CBEClassFactory::Instance();
	vector<CFEOperation*>::iterator iterO;
	for (iterO = pFEInterface->m_Operations.begin();
		iterO != pFEInterface->m_Operations.end();
		iterO++)
	{
		// skip OUT functions
		if ((*iterO)->m_Attributes.Find(ATTR_OUT))
			continue;
		CBESwitchCase *pFunction = pCF->GetNewSwitchCase();
		if ((*iterO)->m_Attributes.Find(ATTR_UUID_RANGE))
			m_RangeCases.Add(pFunction);
		else
			m_SwitchCases.Add(pFunction);
		pFunction->CreateBackEnd(*iterO, true);
	}

	vector<CFEInterface*>::iterator iterI;
	for (iterI = pFEInterface->m_BaseInterfaces.begin();
		iterI != pFEInterface->m_BaseInterfaces.end();
		iterI++)
	{
		AddSwitchCases(*iterI);
	}
}

/** \brief writes the variable initializations of this function
 *  \param pFile the file to write to
 *
 * don't do anything (no variables to initialize)
 */
void CBEDispatchFunction::WriteVariableInitialization(CBEFile& /*pFile*/)
{}

/** \brief writes the switch cases with uuid ranges
 *  \param pFile the file to write to
 */
void CBEDispatchFunction::WriteRanges(CBEFile& pFile)
{
	vector<CBESwitchCase*>::iterator i;
	for (i = m_RangeCases.begin(); i != m_RangeCases.end(); i++)
	{
		(*i)->Write(pFile);
		pFile << "\telse\n";
	}
}

/** \brief writes the switch statement
 *  \param pFile the file to write to
 */
void CBEDispatchFunction::WriteSwitch(CBEFile& pFile)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sOpcodeVar = pNF->GetOpcodeVariable();

	WriteRanges(pFile);

	pFile << "\tswitch (" << sOpcodeVar << ")\n";
	pFile << "\t{\n";

	// iterate over functions
	vector<CBESwitchCase*>::iterator i;
	for (i = m_SwitchCases.begin(); i != m_SwitchCases.end(); i++)
		(*i)->Write(pFile);

	// writes default case
	WriteDefaultCase(pFile);

	pFile << "\t}\n";
}

/** \brief writes the default case of the switch statetment
 *  \param pFile the file to write to
 *
 * The default case is usually empty. It is only used if the opcode does not
 * match any of the defined opcodes. Because the switch statement expects a
 * valid opcode after the function returns, it hastohave the format:
 * \<opcode type\> \<name\>(\<corba object\>*, \<msgbuffer* type\>*,
 *                          \<corba environment\>*)
 *
 * An alternative is to call the wait-any function.
 *
 * If you want to check for the ranges after checking for specific opcodes, do
 * call WriteRanges here.
 */
void
CBEDispatchFunction::WriteDefaultCase(CBEFile& pFile)
{
	pFile << "\tdefault:\n";
	++pFile;
	if (m_sDefaultFunction.empty())
		WriteDefaultCaseWithoutDefaultFunc(pFile);
	else
		WriteDefaultCaseWithDefaultFunc(pFile);
	pFile << "\tbreak;\n";
	--pFile;
}

/** \brief writes the code in the default case if the default function is not \
 *         available
 *  \param pFile the file to write to
 */
void
CBEDispatchFunction::WriteDefaultCaseWithoutDefaultFunc(CBEFile& pFile)
{
	pFile << "\t/* unknown opcode */\n";
	WriteSetWrongOpcodeException(pFile);
	// send reply
	string sReply = CBENameFactory::Instance()->GetReplyCodeVariable();
	pFile << "\t" << sReply << " = DICE_REPLY;\n";
}

/** \brief writes the code in the default case if the default function is \
 *         available
 *  \param pFile the file to write to
 */
void
CBEDispatchFunction::WriteDefaultCaseWithDefaultFunc(CBEFile& pFile)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sMsgBuffer = pNF->GetMessageBufferVariable();
	string sObj = pNF->GetCorbaObjectVariable();
	string sEnv = pNF->GetCorbaEnvironmentVariable();

	pFile << "\treturn " << m_sDefaultFunction << " (" << sObj <<
		", " << sMsgBuffer << ", " << sEnv << ");\n";
}

/** \brief writes function declaration
 *  \param pFile the file to write to
 *
 * This implementation adds the decalartion of the default function if defined
 * and used. And it has to declare the reply-any-wait-any function
 */
void
CBEDispatchFunction::WriteFunctionDeclaration(CBEFile& pFile)
{
	// call base
	CBEInterfaceFunction::WriteFunctionDeclaration(pFile);
	WriteDefaultFunctionDeclaration(pFile);
}

/** \brief writes the decalaration of the default function
 *  \param pFile the file to write to
 */
void
CBEDispatchFunction::WriteDefaultFunctionDeclaration(CBEFile& pFile)
{
	// add declaration of default function
	if (m_sDefaultFunction.empty())
		return;

	pFile << "\n/* CBEDispatchFunction::WriteDefaultFunctionDeclaration */\n";
	// get the class' message buffer to get the correct type
	CBEClass *pClass = GetSpecificParent<CBEClass>();
	assert(pClass);
	CBEMsgBuffer *pMsgBuffer = pClass->GetMessageBuffer();
	assert(pMsgBuffer);
	string sMsgBufferType = pMsgBuffer->m_Declarators.First()->GetName();
	// int \<name\>(\<corba object\>, \<msg buffer type\>*,
	//              \<corba environment\>*)
	pFile << "int " << m_sDefaultFunction << " (CORBA_Object, " <<
		sMsgBufferType << "*, CORBA_Server_Environment*);\n";
}

/** \brief writes the code to set and marshal the exception codes for a wrong
 *          opcode
 *  \param pFile the file to write to
 */
void
CBEDispatchFunction::WriteSetWrongOpcodeException(CBEFile& pFile)
{
	// set the exception in the environment
	string sSetFunc;
	CBETypedDeclarator *pEnv = GetEnvironment();
	if (((CBEUserDefinedType*)pEnv->GetType())->GetName() ==
		"CORBA_Server_Environment")
		sSetFunc = "CORBA_server_exception_set";
	else
		sSetFunc = "CORBA_exception_set";
	CBEDeclarator *pDecl = pEnv->m_Declarators.First();
	pFile << "\t" << sSetFunc << "(";
	if (pDecl->GetStars() == 0)
		pFile << "&";
	pDecl->WriteName(pFile);
	pFile << ",\n";
	++pFile << "\tCORBA_SYSTEM_EXCEPTION,\n";
	pFile << "\tCORBA_DICE_EXCEPTION_WRONG_OPCODE,\n";
	pFile << "\t0);\n";
	--pFile;
	// copy from environment to local exception variable
	WriteExceptionWordInitialization(pFile);
	// marshal exception variable
	WriteMarshalException(pFile, true, false);
}

/** \brief test fi this function should be written
 *  \param pFile the file to write to
 *  \return  true if this function should be written
 *
 * A server loop is only written at the component's side.
 */
bool CBEDispatchFunction::DoWriteFunction(CBEFile* pFile)
{
	if (!IsTargetFile(pFile))
		return false;
	// do not write this function implementation if
	// option NO_DISPATCHER is set
	if (CCompiler::IsOptionSet(PROGRAM_NO_DISPATCHER) &&
		pFile->IsOfFileType(FILETYPE_IMPLEMENTATION))
		return false;
	return pFile->IsOfFileType(FILETYPE_COMPONENT);
}

/** \brief writes the invocation of the message transfer
 *  \param pFile the file to write to
 */
void
CBEDispatchFunction::WriteInvocation(CBEFile& /*pFile*/)
{}

/** \brief writes the server loop's function body
 *  \param pFile the target file
 */
void CBEDispatchFunction::WriteBody(CBEFile& pFile)
{
	// write variable declaration and initialization
	WriteVariableDeclaration(pFile);
	WriteVariableInitialization(pFile);
	// write loop (contains switch)
	WriteSwitch(pFile);
	// write clean up
	WriteCleanup(pFile);
	// write return
	WriteReturn(pFile);
}
