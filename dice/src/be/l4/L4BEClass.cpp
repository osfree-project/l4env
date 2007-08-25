/**
 *  \file    dice/src/be/l4/L4BEClass.cpp
 *  \brief   contains the implementation of the class CL4BEClass
 *
 *  \date    01/29/2003
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2004
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

#include "L4BEClass.h"
#include "L4BENameFactory.h"
#include "be/BEAttribute.h"
#include "be/BEContext.h"
#include "be/BEHeaderFile.h"

#include "Compiler.h"
#include "Attribute-Type.h"
#include "TypeSpec-Type.h"

CL4BEClass::CL4BEClass()
{
}

/** destroys the object */
CL4BEClass::~CL4BEClass()
{
}

/** \brief write L4 specific helper functions
 *  \param pFile the file to write to
 *
 * We have to write the function declaration for user-defined functions.
 * - default_function: is written by server-loop, since it is used there
 * - init-rcvstring: void name(int, l4_umword_t*, l4_umword_t*, CORBA_Environment)
 * - error-function: void name(l4_msgdope_t, CORBA(_Server)_Environment*)
 * - error-function-client: void name(l4_msgdope_t, CORBA_Environment*)
 * - error-function-server: void name(l4_msgdope_t, CORBA_Server_Environment*)
 */
void
CL4BEClass::WriteHelperFunctions(CBEHeaderFile& pFile)
{
    WriteInitRcvStringFunctions(pFile);
    WriteErrorFunctions(pFile);
    // call base class
    CBEClass::WriteHelperFunctions(pFile);
}

/** \brief write the init-rcv string function declarations
 *  \param pFile the file to write to
 */
void CL4BEClass::WriteInitRcvStringFunctions(CBEHeaderFile& pFile)
{
    if (!m_Attributes.Find(ATTR_INIT_RCVSTRING) &&
        !m_Attributes.Find(ATTR_INIT_RCVSTRING_CLIENT) &&
        !m_Attributes.Find(ATTR_INIT_RCVSTRING_SERVER))
	return;

    string sEnvName;
    if (pFile.IsOfFileType(FILETYPE_COMPONENT))
        sEnvName = "CORBA_Server_Environment";
    else
        sEnvName = "CORBA_Environment";

    CBEAttribute *pAttr = 0;
    string sFuncName;
    if ((pAttr = m_Attributes.Find(ATTR_INIT_RCVSTRING)) != 0)
	sFuncName = pAttr->GetString();
    if (((pAttr = m_Attributes.Find(ATTR_INIT_RCVSTRING_CLIENT)) != 0) &&
	pFile.IsOfFileType(FILETYPE_CLIENT))
	sFuncName = pAttr->GetString();
    if (((pAttr = m_Attributes.Find(ATTR_INIT_RCVSTRING_SERVER)) != 0) &&
	pFile.IsOfFileType(FILETYPE_COMPONENT))
	sFuncName = pAttr->GetString();
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    if (sFuncName.empty())
	sFuncName = pNF->GetString(CL4BENameFactory::STR_INIT_RCVSTRING_FUNC);
    else
	sFuncName = pNF->GetString(CL4BENameFactory::STR_INIT_RCVSTRING_FUNC,
	    (void*)&sFuncName);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true);

    WriteExternCStart(pFile);
    pFile << "\tvoid " << sFuncName << "(int, " << sMWord << "*, "
	<< sMWord << "*, " << sEnvName << "*);\n\n";
    WriteExternCEnd(pFile);
}

/** \brief write the error function declarations
 *  \param pFile the file to write to
 */
void CL4BEClass::WriteErrorFunctions(CBEHeaderFile& pFile)
{
    if (!m_Attributes.Find(ATTR_ERROR_FUNCTION) &&
        !m_Attributes.Find(ATTR_ERROR_FUNCTION_CLIENT) &&
        !m_Attributes.Find(ATTR_ERROR_FUNCTION_SERVER))
	return;

    string sEnvName;
    if (pFile.IsOfFileType(FILETYPE_COMPONENT))
        sEnvName = "CORBA_Server_Environment";
    else
        sEnvName = "CORBA_Environment";

    CBEAttribute *pAttr = 0;
    string sFuncName;
    if ((pAttr = m_Attributes.Find(ATTR_ERROR_FUNCTION)) != 0)
	sFuncName = pAttr->GetString();
    if (((pAttr = m_Attributes.Find(ATTR_ERROR_FUNCTION_CLIENT)) != 0) &&
	pFile.IsOfFileType(FILETYPE_CLIENT))
	sFuncName = pAttr->GetString();
    if (((pAttr = m_Attributes.Find(ATTR_ERROR_FUNCTION_SERVER)) != 0) &&
	pFile.IsOfFileType(FILETYPE_COMPONENT))
	sFuncName = pAttr->GetString();

    if (!sFuncName.empty())
    {
	WriteExternCStart(pFile);
	pFile << "\tvoid " << sFuncName << "(l4_msgdope_t, " << sEnvName
	    << "*);\n\n";
	WriteExternCEnd(pFile);
    }
}
