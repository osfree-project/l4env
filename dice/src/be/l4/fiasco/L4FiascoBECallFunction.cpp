/**
 *  \file    dice/src/be/l4/fiasco/L4FiascoBECallFunction.cpp
 *  \brief   contains the implementation of the class CL4FiascoBECallFunction
 *
 *  \date    10/26/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
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

#include "L4FiascoBECallFunction.h"
#include "L4FiascoBENameFactory.h"
#include "be/l4/L4BEMsgBuffer.h"
#include <string>
using std::string;

CL4FiascoBECallFunction::CL4FiascoBECallFunction()
{ }

/** \brief destructor of target class */
CL4FiascoBECallFunction::~CL4FiascoBECallFunction()
{ }

/** \brief creates the call function
 *  \param pFEOperation the front-end operation to use as reference
 *  \param bComponentSide true if this function is created at component side
 *  \return true if successful
 *
 * The L4.Fiasco implementation makes a pointer out of the message buffer
 * variable and initializes it to the address of the utcb.
 */
void CL4FiascoBECallFunction::CreateBackEnd(CFEOperation *pFEOperation, bool bComponentSide)
{
	CL4BECallFunction::CreateBackEnd(pFEOperation, bComponentSide);

	CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
	if (pMsgBuffer->HasProperty(CL4BEMsgBuffer::MSGBUF_PROP_UTCB_IPC, CMsgStructType::Generic))
	{
		pMsgBuffer->m_Declarators.First()->SetStars(1);
		string sInit;
		pMsgBuffer->GetType(this)->WriteCastToStr(sInit, true);
		CBENameFactory *pNF = CBENameFactory::Instance();
		sInit += pNF->GetString(CL4FiascoBENameFactory::STR_UTCB_INITIALIZER, this);
		pMsgBuffer->SetDefaultInitString(sInit);
	}
}

