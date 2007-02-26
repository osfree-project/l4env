/**
 *    \file    dice/src/be/BETestsuite.cpp
 *    \brief   contains the implementation of the class CBETestsuite
 *
 *    \date    01/11/2002
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

#include "be/BETestsuite.h"
#include "be/BETestFunction.h"
#include "be/BETestMainFunction.h"
#include "be/BETestServerFunction.h"
#include "be/BEContext.h"
#include "be/BEImplementationFile.h"
#include "be/BENameFactory.h"
#include "be/BERoot.h"

#include "fe/FEFile.h"
#include "Attribute-Type.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"

CBETestsuite::CBETestsuite()
{
}

CBETestsuite::CBETestsuite(CBETestsuite & src):CBETarget(src)
{
}

/**    \brief destructor
 */
CBETestsuite::~CBETestsuite()
{

}

/**    \brief writes the testsuite to the target files
 *    \param pContext the context to write to
 *
 * Because there are no header files we only call WriteImplementationFiles.
 */
void CBETestsuite::Write(CBEContext * pContext)
{
    WriteImplementationFiles(pContext);
}

/**    \brief sets the current file-type in the context
 *    \param pContext the context to manipulate
 *    \param nHeaderOrImplementation a flag to indicate whether we need a header or implementation file
 */
void CBETestsuite::SetFileType(CBEContext * pContext, int nHeaderOrImplementation)
{
    switch (nHeaderOrImplementation)
    {
    case FILETYPE_HEADER:
        pContext->SetFileType(FILETYPE_CLIENTHEADER);
        break;
    case FILETYPE_IMPLEMENTATION:
        pContext->SetFileType(FILETYPE_TESTSUITE);
        break;
    default:
        CBETarget::SetFileType(pContext, nHeaderOrImplementation);
        break;
    }
}

/** \brief creates the header file(s) for the testsuite
 *  \param pFEFile the respective front-end file
 *  \param pContext the context of the operation
 *    \return true if successful
 *
 * The testsuite does not need any header files, the implementation file simply
 * includes the header files of the client and server.
 */
bool CBETestsuite::CreateBackEndHeader(CFEFile * pFEFile, CBEContext * pContext)
{
    return true;
}

/** \brief creates the implementation files for the testsuite
 *  \param pFEFile the front-end file to use as reference
 *  \param pContext the context of this operation
 *  \return true if successful
 *
 * The implementation file of the test-suite contains the main function and the test-functions.
 */
bool CBETestsuite::CreateBackEndImplementation(CFEFile * pFEFile, CBEContext * pContext)
{
    CBEImplementationFile *pImplementation = pContext->GetClassFactory()->GetNewImplementationFile();
    pContext->SetFileType(FILETYPE_TESTSUITE);
    AddFile(pImplementation);
    if (!pImplementation->CreateBackEnd(pFEFile, pContext))
    {
        RemoveFile(pImplementation);
        delete pImplementation;
        VERBOSE("CBETestsuite::CreateBackEnd failed because testsuite file could not be creates\n");
        return false;
    }
    // add include files to implementation file
    // do not use include text file names, since these files are in same directory
    pContext->SetFileType(FILETYPE_CLIENTHEADER);
    string sHeader = pContext->GetNameFactory()->GetFileName(pFEFile, pContext);
    pImplementation->AddIncludedFileName(sHeader, true, false);
    pContext->SetFileType(FILETYPE_COMPONENTHEADER);
    sHeader = pContext->GetNameFactory()->GetFileName(pFEFile, pContext);
    pImplementation->AddIncludedFileName(sHeader, true, false);

    // add dice testsuite header which includes functions used in testsuite
    pImplementation->AddIncludedFileName(string("dice/dice-testsuite.h"), false, false);

    // add functions
    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);
    if (!pRoot->AddToFile(pImplementation, pContext))
    {
        RemoveFile(pImplementation);
        delete pImplementation;
        VERBOSE("CBETestsuite::CreateBackEnd failed because functions could not be added\n");
        return false;
    }

    return true;
}
