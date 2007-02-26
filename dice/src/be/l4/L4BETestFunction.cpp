/**
 *	\file	dice/src/be/l4/L4BETestFunction.cpp
 *	\brief	contains the implementation of the class CL4BETestFunction
 *
 *	\date	Tue May 28 2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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

IMPLEMENT_DYNAMIC(CL4BETestFunction);

CL4BETestFunction::CL4BETestFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4BETestFunction, CBETestFunction);
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
void CL4BETestFunction::WriteErrorMessage(CBEFile * pFile, CDeclaratorStack * pStack, CBEContext * pContext)
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
void CL4BETestFunction::WriteErrorMessageThread(CBEFile * pFile, CDeclaratorStack * pStack, CBEContext * pContext)
{
    // get target name
    String sTargetName;
    if (pFile->IsOfFileType(FILETYPE_CLIENT) || pFile->IsOfFileType(FILETYPE_TESTSUITE))
        sTargetName = " at client side";
    else if (pFile->IsOfFileType(FILETYPE_COMPONENT) || pFile->IsOfFileType(FILETYPE_SKELETON))
        sTargetName = " at server side";
    else
        sTargetName = " at unknown side";
    // get declarator
    CDeclaratorStackLocation *pHead = pStack->GetTop();
    // print error message
    pFile->PrintIndent("\tLOG(\"WRONG ");
    // if struct add members
    pStack->Write(pFile, false, false, pContext);
    if (pHead->nIndex >= 0)
        pFile->Print(" (element %d)", pHead->nIndex);
    else if (pHead->nIndex == -2)
        pFile->Print(" (element %%d)");
    pFile->Print("%s\\n\"", (const char *) sTargetName);
    if (pHead->nIndex == -2)
    {
        pFile->Print(", %s", (const char*)pHead->sIndex);
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
void CL4BETestFunction::WriteErrorMessageTask(CBEFile * pFile, CDeclaratorStack * pStack, CBEContext * pContext)
{
    // get declarator
    CDeclaratorStackLocation *pHead = pStack->GetTop();
    CBEDeclarator *pDecl = (CBEDeclarator*)(pHead->pDeclarator);
    pFile->PrintIndent("\tenter_kdebug(\"%s: wrong ",(const char*)pDecl->GetFunction()->GetName());
    pStack->Write(pFile, false, false, pContext);
    pFile->Print("\");\n");
}

/** \brief writes the success message if transmittion was correct
 *  \param pFile the file to write to
 *  \param pStack the declarator stack
 *  \param pContext the context of the write operation
 */
void CL4BETestFunction::WriteSuccessMessage(CBEFile * pFile, CDeclaratorStack * pStack, CBEContext * pContext)
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
void CL4BETestFunction::WriteSuccessMessageThread(CBEFile *pFile, CDeclaratorStack *pStack, CBEContext *pContext)
{
    // get target name
    String sTargetName;
    if (pFile->IsOfFileType(FILETYPE_CLIENT) || pFile->IsOfFileType(FILETYPE_TESTSUITE))
        sTargetName = " at client side";
    else if (pFile->IsOfFileType(FILETYPE_COMPONENT) || pFile->IsOfFileType(FILETYPE_SKELETON))
        sTargetName = " at server side";
    else
        sTargetName = " at unknown side";
    // get declarator
    CDeclaratorStackLocation *pHead = pStack->GetTop();
    // print error message
    pFile->PrintIndent("\tLOG(\"correct ");
    // if struct add members
    pStack->Write(pFile, false, false, pContext);
    if (pHead->nIndex >= 0)
        pFile->Print(" (element %d)", pHead->nIndex);
    else if (pHead->nIndex == -2)
        pFile->Print(" (element %%d)");
    pFile->Print("%s\\n\"", (const char *) sTargetName);
    if (pHead->nIndex == -2)
        pFile->Print(", %s", (const char*)pHead->sIndex);
    pFile->Print(");\n");
}

/** \brief writes the success message if transmittion was correct
 *  \param pFile the file to write to
 *  \param pStack the declarator stack
 *  \param pContext the context of the write operation
 */
void CL4BETestFunction::WriteSuccessMessageTask(CBEFile *pFile, CDeclaratorStack *pStack, CBEContext *pContext)
{
    // get declarator
    CDeclaratorStackLocation *pHead = pStack->GetTop();
    CBEDeclarator *pDecl = (CBEDeclarator*)(pHead->pDeclarator);
    pFile->PrintIndent("\tenter_kdebug(\"%s: correct ",(const char*)pDecl->GetFunction()->GetName());
    pStack->Write(pFile, false, false, pContext);
    pFile->Print("\");\n");
}
