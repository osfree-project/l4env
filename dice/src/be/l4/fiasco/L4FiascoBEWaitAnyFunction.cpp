/**
 *  \file    dice/src/be/l4/fiasco/L4FiascoBEWaitAnyFunction.cpp
 *  \brief   contains the implementation of the class CL4FiascoBEWaitAnyFunction
 *
 *  \date    08/24/2007
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

#include "L4FiascoBEWaitAnyFunction.h"
#include "L4FiascoBENameFactory.h"
#include "be/BEClassFactory.h"
#include "be/BETypedDeclarator.h"
#include "be/BEClassFactory.h"
#include "be/BEFile.h"
#include "be/BEMsgBuffer.h"
#include "TypeSpec-Type.h"
#include <string>
using std::string;

CL4FiascoBEWaitAnyFunction::CL4FiascoBEWaitAnyFunction(bool bOpenWait, bool bReply)
: CL4BEWaitAnyFunction(bOpenWait, bReply)
{ }

/** \brief destructor of target class */
CL4FiascoBEWaitAnyFunction::~CL4FiascoBEWaitAnyFunction()
{ }

void
CL4FiascoBEWaitAnyFunction::AddBeforeParameters()
{
    CL4BEWaitAnyFunction::AddBeforeParameters();

    CBENameFactory *pNF = CBENameFactory::Instance();
	string sTagVar = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);
	string sTagType = pNF->GetTypeName(TYPE_MSGTAG, 0);

    CBEClassFactory *pCF = CBEClassFactory::Instance();
    CBETypedDeclarator *pParameter = pCF->GetNewTypedDeclarator();
    m_Parameters.Add(pParameter);
    pParameter->CreateBackEnd(sTagType, sTagVar, 1);
}

/** \brief writes the unmarshalling code for this function
 *  \param pFile the file to write to
 *
 * Someody could have sent us a short IPC without putting the values into the
 * UTCB. But unmarshalling depends on the values located in the UTCB. Thus
 * copy the first two words from the regular message buffer (where the
 * registers of the short IPC were received to) into the first two values of
 * the UTCB. Do this only if result dope indicates short IPC and received
 * message tag is 0.
 */
void CL4FiascoBEWaitAnyFunction::WriteUnmarshalling(CBEFile& pFile)
{
	CL4BEWaitAnyFunction::WriteUnmarshalling(pFile);

	CBENameFactory *pNF = CBENameFactory::Instance();
	string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
	string sTagVar = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);
	pFile << "\tif (" << sResult << ".msgdope == 2 && " << sTagVar << "->raw == 0)\n";
	pFile << "\t{\n";
	++pFile << "\tl4_umword_t *_dice_utcb_values = ";
	pFile << pNF->GetString(CL4FiascoBENameFactory::STR_UTCB_INITIALIZER, this);
	pFile << ";\n";

	CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
	pFile << "\t_dice_utcb_values[0] = ";
	pMsgBuffer->WriteMemberAccess(pFile, this, CMsgStructType::Generic, TYPE_MWORD, 0);
	pFile << ";\n";
	pFile << "\t_dice_utcb_values[1] = ";
	pMsgBuffer->WriteMemberAccess(pFile, this, CMsgStructType::Generic, TYPE_MWORD, 1);
	pFile << ";\n";
	--pFile << "\t}\n";
}
