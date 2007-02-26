/**
 *    \file    dice/src/be/l4/L4BETestFunction.cpp
 *    \brief   contains the implementation of the class CL4BETestFunction
 *
 *    \date    05/28/2002
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

#include "be/l4/L4BETestFunction.h"
#include "be/BEContext.h"
#include "be/BEComponent.h"
#include "be/BEClient.h"
#include "be/BEDeclarator.h"
#include "be/BETestsuite.h"
#include "be/BEType.h"
#include "TypeSpec-Type.h"

CL4BETestFunction::CL4BETestFunction()
{
}

/** destroys this class */
CL4BETestFunction::~CL4BETestFunction()
{
}

/** \brief writes the error message for the declarators
 *  \param pFile the file to write to
 *  \param pStack the declarator stack
 *  \param pContext the context of this operation
 *
 * For testsuites with server's as task we cannot use the LOG function, because this leads to
 * dealocks. Thus we use the enter_kdebug function instead with a somewhat different output.
 *
 * For threads we use the LOG function, with the same output as the printf function.
 */
void
CL4BETestFunction::WriteErrorMessage(CBEFile * pFile,
    vector<CDeclaratorStackLocation*> * pStack,
    CBEContext * pContext)
{
    if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE_THREAD))
    {
        WriteErrorMessageThread(pFile, pStack, pContext);
    }
    else if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE_TASK))
    {
        WriteErrorMessageTask(pFile, pStack, pContext);
    }
    // otherwise provoke compile error... (I know this is nasty)
}

/** \brief writes the error message for the thread case
 *  \param pFile the file to write to
 *  \param pStack the declarator stack
 *  \param pContext the context of the write operation
 */
void
CL4BETestFunction::WriteErrorMessageThread(CBEFile * pFile,
    vector<CDeclaratorStackLocation*> * pStack,
    CBEContext * pContext)
{
    // get target name
    string sTargetName;
    if (pFile->IsOfFileType(FILETYPE_CLIENT) || pFile->IsOfFileType(FILETYPE_TESTSUITE))
        sTargetName = " at client side";
    else if (pFile->IsOfFileType(FILETYPE_COMPONENT) || pFile->IsOfFileType(FILETYPE_TEMPLATE))
        sTargetName = " at server side";
    else
        sTargetName = " at unknown side";
    // get declarator
    CDeclaratorStackLocation *pHead = pStack->back();
    // print error message
    *pFile << "\t\tprintf(\"WRONG ";
    // if struct add members
    CDeclaratorStackLocation::Write(pFile, pStack, false, false, pContext);
    int nIdxCnt = pHead->GetUsedIndexCount();
    if (nIdxCnt)
    {
        pFile->Print(" (element ");
        for (int i=0; i<nIdxCnt; i++)
        {
            pFile->Print("[");
            if (pHead->nIndex[i] >= 0)
                pFile->Print("%d", pHead->nIndex[i]);
            else if (pHead->nIndex[i] == -2)
                pFile->Print("%%d");
            pFile->Print("]");
        }
        pFile->Print(")");
    }
    pFile->Print("%s\"", sTargetName.c_str());
    if (nIdxCnt)
    {
        for (int i=0; i<nIdxCnt; i++)
        {
            if (pHead->nIndex[i] == -2)
                pFile->Print(", %s", pHead->sIndex[i].c_str());
        }
    }
    pFile->Print(");\n");
}

/** \brief writes the error message for the task case
 *  \param pFile the file to write to
 *  \param pStack the declarator stack
 *  \param pContext the context of the write operation
 *
 * Because we cannot use the LOG server, we enter the kernel debugger.
 */
void
CL4BETestFunction::WriteErrorMessageTask(CBEFile * pFile,
    vector<CDeclaratorStackLocation*> * pStack,
    CBEContext * pContext)
{
    // get declarator
    CDeclaratorStackLocation *pHead = pStack->back();
    CBEDeclarator *pDecl = (CBEDeclarator*)(pHead->pDeclarator);
    CBEFunction *pFunc = pDecl->GetSpecificParent<CBEFunction>();
    pFile->PrintIndent("\tenter_kdebug(\"%s: wrong ",pFunc->GetName().c_str());
    CDeclaratorStackLocation::Write(pFile, pStack, false, false, pContext);
    pFile->Print("\");\n");
}

/** \brief writes the success message if transmittion was correct
 *  \param pFile the file to write to
 *  \param pStack the declarator stack
 *  \param pContext the context of the write operation
 */
void
CL4BETestFunction::WriteSuccessMessage(CBEFile * pFile,
    vector<CDeclaratorStackLocation*> * pStack,
    CBEContext * pContext)
{
    if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE_THREAD))
    {
        WriteSuccessMessageThread(pFile, pStack, pContext);
    }
    else if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE_TASK))
    {
        WriteSuccessMessageTask(pFile, pStack, pContext);
    }
}

/** \brief writes the success message if transmittion was correct
 *  \param pFile the file to write to
 *  \param pStack the declarator stack
 *  \param pContext the context of the write operation
 */
void
CL4BETestFunction::WriteSuccessMessageThread(CBEFile *pFile,
    vector<CDeclaratorStackLocation*> *pStack,
    CBEContext *pContext)
{
    // get target name
    string sTargetName;
    if (pFile->IsOfFileType(FILETYPE_CLIENT) || pFile->IsOfFileType(FILETYPE_TESTSUITE))
        sTargetName = " at client side";
    else if (pFile->IsOfFileType(FILETYPE_COMPONENT) || pFile->IsOfFileType(FILETYPE_TEMPLATE))
        sTargetName = " at server side";
    else
        sTargetName = " at unknown side";
    // get declarator
    CDeclaratorStackLocation *pHead = pStack->back();
    // print error message
    pFile->PrintIndent("\tprintf(\"correct ");
    // if struct add members
    CDeclaratorStackLocation::Write(pFile, pStack, false, false, pContext);
    int nIdxCnt = pHead->GetUsedIndexCount();
    if (nIdxCnt)
    {
        pFile->Print(" (element ");
        for (int i=0; i<nIdxCnt; i++)
        {
            pFile->Print("[");
            if (pHead->nIndex[i] >= 0)
                pFile->Print("%d", pHead->nIndex[i]);
            else if (pHead->nIndex[i] == -2)
                pFile->Print("%%d");
            pFile->Print("]");
        }
        pFile->Print(")");
    }
    pFile->Print("%s\"", sTargetName.c_str());
    if (nIdxCnt)
    {
        for (int i=0; i<nIdxCnt; i++)
        {
            if (pHead->nIndex[i] == -2)
                pFile->Print(", %s", pHead->sIndex[i].c_str());
        }
    }
    pFile->Print(");\n");
}

/** \brief writes the success message if transmittion was correct
 *  \param pFile the file to write to
 *  \param pStack the declarator stack
 *  \param pContext the context of the write operation
 */
void
CL4BETestFunction::WriteSuccessMessageTask(CBEFile *pFile,
    vector<CDeclaratorStackLocation*> *pStack,
    CBEContext *pContext)
{
    // get declarator
    CDeclaratorStackLocation *pHead = pStack->back();
    CBEDeclarator *pDecl = (CBEDeclarator*)(pHead->pDeclarator);
    CBEFunction *pFunc = pDecl->GetSpecificParent<CBEFunction>();
    pFile->PrintIndent("\tenter_kdebug(\"%s: correct ",pFunc->GetName().c_str());
    CDeclaratorStackLocation::Write(pFile, pStack, false, false, pContext);
    pFile->Print("\");\n");
}

/** \brief compares two declarators
 *  \param pFile the file to write to
 *  \param pType the type of the declarators
 *  \param pStack the declarator stack
 *  \param pContext the context of the operation
 */
void
CL4BETestFunction::CompareDeclarator(CBEFile* pFile,
    CBEType* pType,
    vector<CDeclaratorStackLocation*> * pStack,
    CBEContext* pContext)
{
    // test for special types
    if (pType->IsSimpleType())
    {
        switch (pType->GetFEType())
        {
        case TYPE_FLEXPAGE:
        case TYPE_RCV_FLEXPAGE:
            CompareFlexpages(pFile, pType, pStack, pContext);
            return;
            break;
        default:
            break;
        }
    }
    CBETestFunction::CompareDeclarator(pFile, pType, pStack, pContext);
}

/** \brief compares two flexpages
 *  \param pFile the file to write to
 *  \param pType the type of the declarators
 *  \param pStack the declarator stack
 *  \param pContext the context of the operation
 */
void
CL4BETestFunction::CompareFlexpages(CBEFile* pFile,
    CBEType* pType,
    vector<CDeclaratorStackLocation*> * pStack,
    CBEContext* pContext)
{
    CBEDeclarator *pDeclarator = 0;
    CDeclaratorStackLocation *pLoc = 0;
    if (pType->GetFEType() == TYPE_FLEXPAGE)
    {
        // compare snd_base (l4_umword_t)
        // create declarators
        pDeclarator = new CBEDeclarator();
        pDeclarator->CreateBackEnd(string("snd_base"), 0, pContext);
        pLoc = new CDeclaratorStackLocation(pDeclarator);
        pStack->push_back(pLoc);
        WriteComparison(pFile, pStack, pContext);
        pStack->pop_back();
        delete pLoc;
        // add fpage member (union)
        pDeclarator->CreateBackEnd(string("fpage"), 0, pContext);
        pLoc = new CDeclaratorStackLocation(pDeclarator);
        pStack->push_back(pLoc);
        // fall thru
    }
    // compare fpage (l4_umword_t)
    CBEDeclarator *pDeclarator2 = new CBEDeclarator();
    pDeclarator2->CreateBackEnd(string("fpage"), 0, pContext);
    CDeclaratorStackLocation *pLoc2 = new CDeclaratorStackLocation(pDeclarator2);
    pStack->push_back(pLoc2);
    WriteComparison(pFile, pStack, pContext);
    pStack->pop_back();
    delete pLoc2;
    delete pDeclarator2;
    // cleanup
    if ((pType->GetFEType() == TYPE_FLEXPAGE) &&
        pDeclarator)
    {
        pStack->pop_back();
        delete pLoc;
        delete pDeclarator;
    }
}
