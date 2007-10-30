/**
 *  \file     dice/src/be/l4/v4/L4V4BEMarshalFunction.cpp
 *  \brief    contains the implementation of the class CL4V4BEMarshalFunction
 *
 *  \date     Mon Jul 5 2004
 *  \author   Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#include "L4V4BEMarshalFunction.h"
#include "L4V4BENameFactory.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BESizes.h"
#include "be/Trace.h"
#include "be/BEMsgBuffer.h"
#include "be/BEClass.h"
#include "Attribute-Type.h"
#include "TypeSpec-Type.h"
#include "Compiler.h"
#include <cassert>

CL4V4BEMarshalFunction::CL4V4BEMarshalFunction()
: CL4BEMarshalFunction()
{ }

/** destroys the instance of this class */
CL4V4BEMarshalFunction::~CL4V4BEMarshalFunction()
{ }

/** \brief write the L4 specific marshalling code
 *  \param pFile the file to write to
 */
void
CL4V4BEMarshalFunction::WriteMarshalling(CBEFile& pFile)
{
	assert (m_pTrace);
	bool bLocalTrace = false;
	if (!m_bTraceOn)
	{
		m_pTrace->BeforeMarshalling(pFile, this);
		m_bTraceOn = bLocalTrace = true;
	}

	// FIXME: get message buffer var and use its declarator
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sMsgBuffer = pNF->GetMessageBufferVariable();
	string sType = pNF->GetTypeName(TYPE_MSGTAG, false);
	// clear message
	pFile << "\tL4_MsgClear ( (" << sType << "*) " << sMsgBuffer << " );\n";
	// set exception in msgbuffer and return if there was an exception.
	WriteMarshalException(pFile, true, true);
	// call base class
	// we skip L4 specific implementation, because it marshals flexpages
	// or exceptions (we don't need this) and it sets the send dope, which
	// is set by convenience functions automatically.
	// we also skip basic backend marshalling, because it starts marshalling
	// after the opcode, which is in the tag as well
	CBEOperationFunction::WriteMarshalling(pFile);
	// set dopes
	CBEMsgBuffer *pMsgBuffer = m_pClass->GetMessageBuffer();
	assert(pMsgBuffer);
	pMsgBuffer->WriteInitialization(pFile, this, TYPE_MSGDOPE_SEND, GetSendDirection());

	if (bLocalTrace)
	{
		m_pTrace->AfterMarshalling(pFile, this);
		m_bTraceOn = false;
	}
}

