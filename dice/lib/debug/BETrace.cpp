/**
 *    \file    dice/src/be/BETrace.cpp
 *    \brief   contains the implementation of the class CBETrace
 *
 *    \date    12/05/2005
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2005-2007
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

#include "BETrace.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEFunction.h"
#include "be/BEMsgBuffer.h"
#include "be/BETypedDeclarator.h"
#include "be/BENameFactory.h"
#include "Compiler.h"
#include "TypeSpec-Type.h"
#include <cassert>

CBETrace::CBETrace()
{ }

CBETrace::~CBETrace()
{ }

/** \brief writes necessary variable declarations
 *  \param pFunction the function to write for
 */
void CBETrace::AddLocalVariable(CBEFunction *pFunction)
{
	// write loop variable for msg buffer dump
	if (!CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF))
		return;

	pFunction->AddLocalVariable(TYPE_INTEGER, false, 4, string("_i"), 0);
}

/** \brief write varible declarations
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *
 * This function can be used to make a more straight forward write of
 * variables. The preferred way sould be to use \a AddLocalVariable instead.
 */
void CBETrace::VariableDeclaration(CBEFile& /*pFile*/, CBEFunction* /*pFunction*/)
{ }

/** \brief write the code required to setup at server
 *  \param pFile the file to write to
 *  \param pFunction the server loop function calling this hook
 */
void CBETrace::InitServer(CBEFile& /*pFile*/, CBEFunction* /*pFunction*/)
{ }

/** \brief write the code required just before the loop starts
 *  \param pFile the file to write to
 *  \param pFunction the server loop function calling this hook
 *
 * This is different from InitServer, because InitServer is called before all
 * the variables are initialized. BeforeLoop is called after all the variables
 * are initialized and just before the loop actually starts doing something.
 */
void CBETrace::BeforeLoop(CBEFile& /*pFile*/, CBEFunction* /*pFunction*/)
{ }

/** \brief writes default includes required for tracing
 *  \param pFile the file to write to
 */
void CBETrace::DefaultIncludes(CBEFile& pFile)
{
	if (!pFile.IsOfFileType(FILETYPE_HEADER))
		return;

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBETrace::%s called\n", __func__);

	if (CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
	{
		pFile << "#ifndef DICE_TRACE_SERVER\n";
		pFile << "#define DICE_TRACE_SERVER 1\n";
		pFile << "#endif /* DICE_TRACE_SERVER */\n";
	}
	if (CCompiler::IsOptionSet(PROGRAM_TRACE_CLIENT))
	{
		pFile << "#ifndef DICE_TRACE_CLIENT\n";
		pFile << "#define DICE_TRACE_CLIENT 1\n";
		pFile << "#endif /* DICE_TRACE_CLIENT */\n";
	}
	if (CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF))
	{
		pFile << "#ifndef DICE_TRACE_MSGBUF\n";
		pFile << "#define DICE_TRACE_MSGBUF 1\n";
		pFile << "#endif /* DICE_TRACE_MSGBUF */\n";
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBETrace::%s returns\n", __func__);
}

/** \brief prints the tracing message before a call
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBETrace::BeforeCall(CBEFile& pFile, CBEFunction *pFunction)
{
	if (!CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF))
		return;

	CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
	assert(pMsgBuffer);
	// get tracing function
	string sFunc;
	CCompiler::GetBackEndOption("trace-client-func", sFunc);

	pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
		": before call\\n\");\n";
	pMsgBuffer->WriteDump(pFile);
}

/** \brief write the tracing code after the call
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBETrace::AfterCall(CBEFile& pFile, CBEFunction *pFunction)
{
	if (!CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF))
		return;

	CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
	assert(pMsgBuffer);
	// get tracing function
	string sFunc;
	CCompiler::GetBackEndOption("trace-client-func", sFunc);

	pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
		": after call\\n\");\n";
	pMsgBuffer->WriteDump(pFile);
}

/** \brief write the tracing code shortly before switch statement
 *  \param pFile the file to write to
 *  \param pFunction the dispatch function to write for
 */
void CBETrace::BeforeDispatch(CBEFile& pFile, CBEFunction *pFunction)
{
	if (!pFunction->IsComponentSide())
		return;
	if (!CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
		return;

	CBENameFactory *pNF = CBENameFactory::Instance();
	string sOpcodeVar = pNF->GetOpcodeVariable();
	string sFunc;
	CCompiler::GetBackEndOption("trace-server-func", sFunc);

	pFile << "\t" << sFunc << " (\"opcode %x received\\n\", "
		<< sOpcodeVar << ");\n";
}

/** \brief write the tracing code shortly after the switch statement
 *  \param pFile the file to write to
 *  \param pFunction the dispatch function to write for
 */
void CBETrace::AfterDispatch(CBEFile& pFile, CBEFunction *pFunction)
{
	if (!pFunction->IsComponentSide())
		return;
	if (!CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
		return;

	CBENameFactory *pNF = CBENameFactory::Instance();
	string sFunc;
	CCompiler::GetBackEndOption("trace-server-func", sFunc);
	string sReply = pNF->GetReplyCodeVariable();

	pFile << "\t" << sFunc << " (\"reply %s\\n\", (" <<
		sReply << "==DICE_REPLY)?\"DICE_REPLY\":" <<
		"((" << sReply << "==DICE_DEFERRED_REPLY)?\"DICE_DEFERRED_REPLY\":" <<
		"\"DICE_NEVER_REPLY\"));\n";
}

/** \brief write the tracing code before the reply IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBETrace::BeforeReplyOnly(CBEFile& pFile, CBEFunction *pFunction)
{
	BeforeCall(pFile, pFunction);
}

/** \brief write the tracing code after the reply IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBETrace::AfterReplyOnly(CBEFile& pFile, CBEFunction *pFunction)
{
	AfterCall(pFile, pFunction);
}

/** \brief write the tracing code before reply IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBETrace::BeforeReplyWait(CBEFile& /*pFile*/, CBEFunction* /*pFunction*/)
{ }

/** \brief writes the tracing code after the server waited for a message
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBETrace::AfterReplyWait(CBEFile& pFile, CBEFunction *pFunction)
{
	if (!CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF))
		return;

	CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
	assert(pMsgBuffer);
	// get tracing function
	string sFunc;
	CCompiler::GetBackEndOption("trace-client-func", sFunc);

	pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
		": after wait\\n\");\n";
	pMsgBuffer->WriteDump(pFile);
}

/** \brief write the tracing code before calling the component function
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBETrace::BeforeComponent(CBEFile& /*pFile*/, CBEFunction* /*pFunction*/)
{ }

/** \brief write the tracing code after calling the component function
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBETrace::AfterComponent(CBEFile& /*pFile*/, CBEFunction* /*pFunction*/)
{ }

/** \brief write the tracing code before marshalling
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBETrace::BeforeMarshalling(CBEFile& /*pFile*/, CBEFunction* /*pFunction*/)
{ }

/** \brief write the tracing code after marshalling
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBETrace::AfterMarshalling(CBEFile& /*pFile*/, CBEFunction* /*pFunction*/)
{ }

/** \brief write the tracing code before unmarshalling
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBETrace::BeforeUnmarshalling(CBEFile& /*pFile*/, CBEFunction* /*pFunction*/)
{ }

/** \brief write the tracing code after unmarshalling
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBETrace::AfterUnmarshalling(CBEFile& /*pFile*/, CBEFunction* /*pFunction*/)
{ }

