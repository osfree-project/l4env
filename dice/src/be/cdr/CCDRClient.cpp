/**
 *    \file    dice/src/be/cdr/CCDRClient.cpp
 *  \brief   contains the implementation of the class CCDRClient
 *
 *    \date    10/28/2003
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "be/cdr/CCDRClient.h"
#include "be/BEContext.h"
#include "be/BEFunction.h"
#include "be/BEImplementationFile.h"
#include "be/BERoot.h"
#include "be/BEClassFactory.h"
#include "Compiler.h"
#include "Attribute-Type.h"
#include "fe/FEOperation.h"
#include <cassert>

CCDRClient::CCDRClient()
 : CBEClient()
{
}

/** destroys this object */
CCDRClient::~CCDRClient()
{
}

/** \brief creates the back-end files for a function
 *  \param pFEOperation the front-end function to use as reference
 *  \return true if successful
 *
 * This implementation only generates the marshal and unmarshal
 * functions.
 */
void
CCDRClient::CreateBackEndFunction(CFEOperation* pFEOperation)
{
    // get root
    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);

    string exc = string(__func__);
    // find appropriate header file
    CBEHeaderFile *pHeader = FindHeaderFile(pFEOperation, FILETYPE_CLIENTHEADER);
    if (!pHeader)
    {
	exc += " failed, because the header file could not be found";
	throw new CBECreateException(exc);
    }

    // create the file
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBEImplementationFile *pImpl = pCF->GetNewImplementationFile();
    m_ImplementationFiles.Add(pImpl);
    pImpl->SetHeaderFile(pHeader);
    try
    {
	pImpl->CreateBackEnd(pFEOperation, FILETYPE_CLIENTIMPLEMENTATION);
    }
    catch (CBECreateException *e)
    {
	m_ImplementationFiles.Remove(pImpl);
        delete pImpl;
	throw;
    }
    // add the functions to the file
    // search the functions
    // if attribute == IN, we need marshal
    // if attribute == OUT, we need unmarshal
    // if attribute == empty, we need marshal and unmarshal
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sFuncName;
    CBEFunction *pFunction;
    if (pFEOperation->m_Attributes.Find(ATTR_IN))
    {
        sFuncName = pNF->GetFunctionName(pFEOperation, FUNCTION_MARSHAL);
        pFunction = pRoot->FindFunction(sFuncName, FUNCTION_MARSHAL);
        if (!pFunction)
        {
	    exc += " failed because function " + sFuncName + 
		" could not be found\n";
	    throw new CBECreateException(exc);
        }
        pFunction->AddToFile(pImpl);
    }
    else if (pFEOperation->m_Attributes.Find(ATTR_OUT))
    {
        // wait function
        sFuncName = pNF->GetFunctionName(pFEOperation, FUNCTION_UNMARSHAL);
        pFunction = pRoot->FindFunction(sFuncName, FUNCTION_UNMARSHAL);
        if (!pFunction)
        {
	    exc += " failed because function " + sFuncName + 
		" could not be found\n";
	    throw new CBECreateException(exc);
        }
        pFunction->AddToFile(pImpl);
    }
    else
    {
        sFuncName = pNF->GetFunctionName(pFEOperation, FUNCTION_MARSHAL);
        pFunction = pRoot->FindFunction(sFuncName, FUNCTION_MARSHAL);
        if (!pFunction)
        {
	    exc += " failed because function " + sFuncName + 
		" could not be found\n";
	    throw new CBECreateException(exc);
        }
        pFunction->AddToFile(pImpl);

        sFuncName = pNF->GetFunctionName(pFEOperation, FUNCTION_UNMARSHAL);
        pFunction = pRoot->FindFunction(sFuncName, FUNCTION_UNMARSHAL);
        if (!pFunction)
        {
	    exc += " failed because function " + sFuncName + 
		" could not be found\n";
	    throw new CBECreateException(exc);
        }
        pFunction->AddToFile(pImpl);
    }
}

