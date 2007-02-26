/**
 *    \file    dice/src/be/cdr/CCDRClient.cpp
 *    \brief   contains the implementation of the class CCDRClient
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

#include "Attribute-Type.h"
#include "fe/FEOperation.h"

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
 *  \param pContext the context of the create process
 *  \return true if successful
 */
bool CCDRClient::CreateBackEndFunction(CFEOperation* pFEOperation,  CBEContext* pContext)
{
    // get root
    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);
    // find appropriate header file
    CBEHeaderFile *pHeader = FindHeaderFile(pFEOperation, pContext);
    if (!pHeader)
        return false;
    // create the file
    CBEImplementationFile *pImpl = pContext->GetClassFactory()->GetNewImplementationFile();
    AddFile(pImpl);
    pContext->SetFileType(FILETYPE_CLIENTIMPLEMENTATION);
    pImpl->SetHeaderFile(pHeader);
    if (!pImpl->CreateBackEnd(pFEOperation, pContext))
    {
        RemoveFile(pImpl);
        delete pImpl;
        VERBOSE("%s failed because file could not be created\n",
            __PRETTY_FUNCTION__);
        return false;
    }
    // add the functions to the file
    // search the functions
    // if attribute == IN, we need marshal
    // if attribute == OUT, we need unmarshal
    // if attribute == empty, we need marshal and unmarshal
    int nOldType;
    string sFuncName;
    CBEFunction *pFunction;
    if (pFEOperation->FindAttribute(ATTR_IN))
    {
        nOldType = pContext->SetFunctionType(FUNCTION_MARSHAL);
        sFuncName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);
        pContext->SetFunctionType(nOldType);
        pFunction = pRoot->FindFunction(sFuncName);
        if (!pFunction)
        {
            VERBOSE("%s failed because function %s could not be found\n",
                    __PRETTY_FUNCTION__, sFuncName.c_str());
            return false;
        }
        pFunction->AddToFile(pImpl, pContext);
    }
    else if (pFEOperation->FindAttribute(ATTR_OUT))
    {
        // wait function
        nOldType = pContext->SetFunctionType(FUNCTION_UNMARSHAL);
        sFuncName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);
        pFunction = pRoot->FindFunction(sFuncName);
        if (!pFunction)
        {
            VERBOSE("%s failed because function %s could not be found\n",
                    __PRETTY_FUNCTION__, sFuncName.c_str());
            return false;
        }
        pFunction->AddToFile(pImpl, pContext);
    }
    else
    {
        nOldType = pContext->SetFunctionType(FUNCTION_MARSHAL);
        sFuncName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);
        pContext->SetFunctionType(nOldType);
        pFunction = pRoot->FindFunction(sFuncName);
        if (!pFunction)
        {
            VERBOSE("%s failed because function %s could not be found\n",
                    __PRETTY_FUNCTION__, sFuncName.c_str());
            return false;
        }
        pFunction->AddToFile(pImpl, pContext);

        nOldType = pContext->SetFunctionType(FUNCTION_UNMARSHAL);
        sFuncName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);
        pContext->SetFunctionType(nOldType);
        pFunction = pRoot->FindFunction(sFuncName);
        if (!pFunction)
        {
            VERBOSE("%s failed because function %s could not be found\n",
                    __PRETTY_FUNCTION__, sFuncName.c_str());
            return false;
        }
        pFunction->AddToFile(pImpl, pContext);
    }
    return true;
}
