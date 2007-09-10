/**
 *    \file    dice/src/be/BECppCallWrapperFunction.cpp
 *    \brief   contains the implementation of the class CBECppCallWrapperFunction
 *
 *    \date    04/23/2007
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "BECppCallWrapperFunction.h"
#include "BETypedDeclarator.h"
#include "BENameFactory.h"
#include "BEClassFactory.h"
#include "Compiler.h"
#include "BEFile.h"
#include "fe/FEOperation.h"

CBECppCallWrapperFunction::CBECppCallWrapperFunction()
    : CBECallFunction()
{
  m_nSkipParameter = 0;
}

/** \brief destructor of target class */
CBECppCallWrapperFunction::~CBECppCallWrapperFunction()
{ }

/** \brief creates the call function
 *  \param pFEOperation the front-end operation used as reference
 *  \return true if successful
 *
 * This implementation only sets the name of the function.
 */
void
CBECppCallWrapperFunction::CreateBackEnd(CFEOperation * pFEOperation, bool bComponentSide,
    int nSkipParameter)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s for operation %s called\n", __func__,
        pFEOperation->GetName().c_str());

    CBECallFunction::CreateBackEnd(pFEOperation, bComponentSide);
    m_nSkipParameter = nSkipParameter;

    // FIXME iterate parameters and prefix them
    CBENameFactory *pNF = CBENameFactory::Instance();
    string sPrefix = pNF->GetWrapperVariablePrefix();
    vector<CBETypedDeclarator*>::iterator iter = m_Parameters.begin();
    for (; iter != m_Parameters.end(); iter++)
    {
	if (*iter == GetObject())
	    continue;
	if (*iter == GetEnvironment())
	    continue;
	// if name is not already prefixed, prefix it with special string
	CBEDeclarator *pDeclarator = (*iter)->m_Declarators.First();
	string sName = pDeclarator->GetName();
	if (sName.find(sPrefix) != 0)
	    pDeclarator->CreateBackEnd(sPrefix + sName, pDeclarator->GetStars());
    }

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s returns true\n", __func__);
}

/** \brief writes the body of the function to the target file
 *  \param pFile the file to write to
 */
void
CBECppCallWrapperFunction::WriteBody(CBEFile& pFile)
{
    if (m_nSkipParameter == 1)
    {
	// use the _dice_server member to call one of the other functions
	CBEDeclarator *pObj = GetObject()->m_Declarators.First();
	string sObj = string("&_dice_server");
	SetCallVariable(pObj->GetName(), 0, sObj);

	CBETypedDeclarator *pReturn = GetReturnVariable();
	string sReturn;
	if (!pReturn->GetType()->IsVoid())
	{
	    pReturn->WriteInitDeclaration(pFile, string());
	    sReturn = pReturn->m_Declarators.First()->GetName();
	}

	m_nSkipParameter = 0;
	CBECallFunction::WriteCall(pFile, sReturn, true);
	m_nSkipParameter = 1;

	if (!pReturn->GetType()->IsVoid())
	    WriteReturn(pFile);

	RemoveCallVariable(sObj);
	return;
    }

    if (m_nSkipParameter == 3)
    {
	// construct a default environment and call the next function
	pFile << "\tCORBA_Environment _env;\n";
	CBEDeclarator *pEnv = GetEnvironment()->m_Declarators.First();
	string sEnv = string("_env");
	SetCallVariable(pEnv->GetName(), 0, sEnv);

	CBETypedDeclarator *pReturn = GetReturnVariable();
	string sReturn;
	if (!pReturn->GetType()->IsVoid())
	{
	    pReturn->WriteInitDeclaration(pFile, string());
	    sReturn = pReturn->m_Declarators.First()->GetName();
	}

	m_nSkipParameter = 1;
	CBECallFunction::WriteCall(pFile, sReturn, true);
	m_nSkipParameter = 3;

	if (!pReturn->GetType()->IsVoid())
	    WriteReturn(pFile);

	RemoveCallVariable(sEnv);
    }
}

/** \brief check if parameter should be written
 *  \param pParam the parameter to test
 *  \return true if writing param, false if not
 *
 * Do not write CORBA_Object and CORBA_Env depending on m_nSkipParameter map.
 */
bool
CBECppCallWrapperFunction::DoWriteParameter(CBETypedDeclarator *pParam)
{
    if ((m_nSkipParameter & 1) &&
	pParam == GetObject())
	return false;
    if ((m_nSkipParameter & 2) &&
	pParam == GetEnvironment())
	return false;
    return CBECallFunction::DoWriteParameter(pParam);
}
