/**
 *  \file    dice/src/be/l4/v2/L4V2BEReplyFunction.cpp
 *  \brief   contains the implementation of the class CL4V2BEReplyFunction
 *
 *  \date    06/01/2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2004
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

#include "L4V2BEReplyFunction.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEClassFactory.h"
#include "be/l4/v2/L4V2BEIPC.h"
#include "be/l4/L4BEMsgBuffer.h"
#include "be/l4/L4BEMarshaller.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEDeclarator.h"
#include "be/BEType.h"
#include "be/BETypedDeclarator.h"
#include "be/BECommunication.h"
#include "Compiler.h"
#include "TypeSpec-Type.h"
#include "Attribute-Type.h"

CL4V2BEReplyFunction::CL4V2BEReplyFunction()
: CL4BEReplyFunction()
{
}

/** destroys an instance of this class */
CL4V2BEReplyFunction::~CL4V2BEReplyFunction()
{
}

/** \brief Writes the variable declaration
 *  \param pFile the file to write to
 *
 * If we have a short IPC in both direction, we only need the result dope,
 * and two dummy dwords.
 */
void CL4V2BEReplyFunction::WriteVariableDeclaration(CBEFile& pFile)
{
    if (m_pTrace)
	m_pTrace->VariableDeclaration(pFile, this);

    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    CBETypedDeclarator* pException = GetExceptionVariable();
    CL4BEIPC *pComm = dynamic_cast<CL4BEIPC*>(GetCommunication());
    assert(pComm);
    bool bUseAssembler = pComm->UseAssembler(this);
    bool bShortIPC = pMsgBuffer->HasProperty(MSGBUF_PROP_SHORT_IPC,
	GetSendDirection());
    if (bUseAssembler && bShortIPC)
    {
	// write dummys
	CBENameFactory *pNF = CCompiler::GetNameFactory();
	string sDummy = pNF->GetDummyVariable();
	string sMWord = pNF->GetTypeName(TYPE_MWORD, true, 0);
	string sResult = pNF->GetString(STR_RESULT_VAR);

	// test if we need dummies
	pFile << "#if defined(__PIC__)\n";
	// write result variable
	pFile << "\tl4_msgdope_t " << sResult << " = { msgdope: 0 };\n";
	pFile << "\t" << sMWord << " " << sDummy <<
	    " __attribute__((unused));\n",
	if (!FindAttribute(ATTR_NOEXCEPTIONS))
	    // declare local exception variable
	    pException->WriteDeclaration(pFile);

	pFile << "#else // !PIC\n";
	// write result variable
	pFile << "\tl4_msgdope_t " << sResult << " = { msgdope: 0 };\n";
	pFile << "\t" << sMWord << " " << sDummy << " = 0;\n";
	if (!FindAttribute(ATTR_NOEXCEPTIONS))
	    // declare local exception variable
	    pException->WriteDeclaration(pFile);
	pFile << "#endif // !PIC\n";
	// if we have in either direction some bit-stuffing, we need more
	// dummies finished with declaration
    }
    else
    {
	CL4BEReplyFunction::WriteVariableDeclaration(pFile);
	if (bUseAssembler)
	{
	    // need dummies
	    CBENameFactory *pNF = CCompiler::GetNameFactory();
	    string sDummy = pNF->GetDummyVariable();
	    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, 0);
	    pFile << "#if defined(__PIC__)\n";
	    pFile << "\t" << sMWord << " " << sDummy <<
		" __attribute__((unused));\n";
	    pFile << "#endif // !PIC\n";
	}
    }
}
