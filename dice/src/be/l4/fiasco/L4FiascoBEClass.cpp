/**
 *  \file    dice/src/be/l4/fiasco/L4FiascoBEClass.cpp
 *  \brief   contains the implementation of the class CL4FiascoBEClass
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

#include "L4FiascoBEClass.h"
#include "be/l4/L4BENameFactory.h"
#include "TypeSpec-Type.h"
#include "be/BEHeaderFile.h"
#include "be/BEMsgBuffer.h"
#include "Compiler.h"

CL4FiascoBEClass::CL4FiascoBEClass()
{ }

/** destroys the object */
CL4FiascoBEClass::~CL4FiascoBEClass()
{ }

/** \brief write the default function
 *  \param pFile the file to write to
 *
 * In case a default function was defined for the server loop we write its
 * declaration into the header file. The implementation has to be user
 * provided. For the L4.Fiasco backend we have an additional parameter: the
 * message tag.
 */
void CL4FiascoBEClass::WriteDefaultFunction(CBEHeaderFile& pFile)
{
	// check for function prototypes
	if (!pFile.IsOfFileType(FILETYPE_COMPONENT))
		return;

	CBEAttribute *pAttr = m_Attributes.Find(ATTR_DEFAULT_FUNCTION);
	if (!pAttr)
		return;
	string sDefaultFunction = pAttr->GetString();
	if (sDefaultFunction.empty())
		return;

	CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
	string sMsgBuffer = pMsgBuffer->m_Declarators.First()->GetName();
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sTagType = pNF->GetTypeName(TYPE_MSGTAG, false);
	// int \<name\>(\<corba object\>, \<msg buffer type\>*,
	//              \<corba environment\>*)
	WriteExternCStart(pFile);
	pFile << "\tint " << sDefaultFunction << " (CORBA_Object, " << sTagType <<
		"*, " << sMsgBuffer << "*, CORBA_Server_Environment*);\n";
	WriteExternCEnd(pFile);
}
