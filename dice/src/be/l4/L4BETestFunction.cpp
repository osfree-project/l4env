/**
 *	\file	dice/src/be/l4/L4BETestFunction.cpp
 *	\brief	contains the implementation of the class CL4BETestFunction
 *
 *	\date	Tue May 28 2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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
    else if (pFile->IsOfFileType(FILETYPE_COMPONENT) || pFile->IsOfFileType(FILETYPE_TEMPLATE))
        sTargetName = " at server side";
    else
        sTargetName = " at unknown side";
    // get declarator
    CDeclaratorStackLocation *pHead = pStack->GetTop();
    // print error message
    pFile->PrintIndent("\tLOG(\"WRONG ");
    // if struct add members
    pStack->Write(pFile, false, false, pContext);
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
    pFile->Print("%s\"", (const char *) sTargetName);
	if (nIdxCnt)
	{
        for (int i=0; i<nIdxCnt; i++)
		{
			if (pHead->nIndex[i] == -2)
			    pFile->Print(", %s", (const char*)pHead->sIndex[i]);
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
    else if (pFile->IsOfFileType(FILETYPE_COMPONENT) || pFile->IsOfFileType(FILETYPE_TEMPLATE))
        sTargetName = " at server side";
    else
        sTargetName = " at unknown side";
    // get declarator
    CDeclaratorStackLocation *pHead = pStack->GetTop();
    // print error message
    pFile->PrintIndent("\tLOG(\"correct ");
    // if struct add members
    pStack->Write(pFile, false, false, pContext);
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
    pFile->Print("%s\"", (const char *) sTargetName);
	if (nIdxCnt)
	{
        for (int i=0; i<nIdxCnt; i++)
		{
			if (pHead->nIndex[i] == -2)
			    pFile->Print(", %s", (const char*)pHead->sIndex[i]);
		}
    }
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

/** \brief compares two declarators
 *  \param pFile the file to write to
 *  \param pType the type of the declarators
 *  \param pStack the declarator stack
 *  \param pContext the context of the operation
 */
void CL4BETestFunction::CompareDeclarator(CBEFile* pFile,  CBEType* pType,  CDeclaratorStack* pStack,  CBEContext* pContext)
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
void CL4BETestFunction::CompareFlexpages(CBEFile* pFile,  CBEType* pType,  CDeclaratorStack* pStack,  CBEContext* pContext)
{
    CBEDeclarator *pDeclarator = 0;
	if (pType->GetFEType() == TYPE_FLEXPAGE)
	{
	    // compare snd_base (l4_umword_t)
		// create declarators
		pDeclarator = new CBEDeclarator();
		pDeclarator->CreateBackEnd(String("snd_base"), 0, pContext);
		pStack->Push(pDeclarator);
		WriteComparison(pFile, pStack, pContext);
		pStack->Pop();
		// add fpage member (union)
		pDeclarator->CreateBackEnd(String("fpage"), 0, pContext);
		pStack->Push(pDeclarator);
		// fall thru
	}
	// compare fpage (l4_umword_t)
	CBEDeclarator *pDeclarator2 = new CBEDeclarator();
	pDeclarator2->CreateBackEnd(String("fpage"), 0, pContext);
	pStack->Push(pDeclarator2);
	WriteComparison(pFile, pStack, pContext);
	pStack->Pop();
	delete pDeclarator2;
    // cleanup
	if ((pType->GetFEType() == TYPE_FLEXPAGE) &&
	    pDeclarator)
	{
		pStack->Pop();
	    delete pDeclarator;
	}
}
