/**
 *	\file	dice/src/be/BETestMainFunction.cpp
 *	\brief	contains the implementation of the class CBETestMainFunction
 *
 *	\date	03/11/2002
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

#include "be/BETestMainFunction.h"
#include "be/BETestServerFunction.h"
#include "be/BEContext.h"
#include "be/BEImplementationFile.h"
#include "be/BEHeaderFile.h"
#include "be/BERoot.h"
#include "be/BETestsuite.h"

#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"

IMPLEMENT_DYNAMIC(CBETestMainFunction);

CBETestMainFunction::CBETestMainFunction()
: m_vSrvLoops(RUNTIME_CLASS(CBETestServerFunction))
{
    IMPLEMENT_DYNAMIC_BASE(CBETestMainFunction, CBEFunction);
}

CBETestMainFunction::CBETestMainFunction(CBETestMainFunction & src)
: CBEFunction(src),
  m_vSrvLoops(RUNTIME_CLASS(CBETestServerFunction))
{
    IMPLEMENT_DYNAMIC_BASE(CBETestMainFunction, CBEFunction);
}

/**	\brief destructor of target class */
CBETestMainFunction::~CBETestMainFunction()
{
    m_vSrvLoops.DeleteAll();
}

/**	\brief adds a test-function to the server-loop vector
 *	\param pFunction the function to add
 */
void CBETestMainFunction::AddSrvLoop(CBETestServerFunction * pFunction)
{
    m_vSrvLoops.Add(pFunction);
    pFunction->SetParent(this);
}

/**	\brief removes a server-loop test function from the vector
 *	\param pFunction the function to remove
 */
void CBETestMainFunction::RemoveSrvLoop(CBETestServerFunction * pFunction)
{
    m_vSrvLoops.Remove(pFunction);
}

/**	\brief retrives a pointer to the first server-loop test function
 *	\return a pointer to the first server-loop test function
 */
VectorElement *CBETestMainFunction::GetFirstSrvLoop()
{
    return m_vSrvLoops.GetFirst();
}

/**	\brief retrieves a reference to the next server-loop test function
 *	\param pIter the pointer to the next function
 *	\return a reference to the next server-loop test function
 */
CBETestServerFunction *CBETestMainFunction::GetNextSrvLoop(VectorElement * &pIter)
{
    if (!pIter)
	return 0;
    CBETestServerFunction *pRet = (CBETestServerFunction *) pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
	return GetNextSrvLoop(pIter);
    return pRet;
}

/**	\brief creates the main function
 *	\param pFEFile the respective front-end file
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * Fetch the test-functions and add references here.
 */
bool CBETestMainFunction::CreateBackEnd(CFEFile * pFEFile, CBEContext * pContext)
{
    if (!AddTestFunction(pFEFile, pContext))
    {
        VERBOSE("CBETestMainFunction::CreateBE failed because test functions could not be added\n");
        return false;
    }
	// set target file name
	SetTargetFileName(pFEFile, pContext);
    // set name
    m_sName = "main";

    return true;
}

/**	\brief adds the test-function to the main-function
 *	\param pFEFile the front-end file to search for functions
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * This code is taken from CBETarget::AddFunctionToFile.
 */
bool CBETestMainFunction::AddTestFunction(CFEFile * pFEFile, CBEContext * pContext)
{
    if (!pFEFile)
	return true;
    if (!pFEFile->IsIDLFile())
	return true;

    VectorElement *pIter = pFEFile->GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = pFEFile->GetNextInterface(pIter)) != 0)
      {
	  if (!AddTestFunction(pInterface, pContext))
	      return false;
      }

    pIter = pFEFile->GetFirstLibrary();
    CFELibrary *pLib;
    while ((pLib = pFEFile->GetNextLibrary(pIter)) != 0)
      {
	  if (!AddTestFunction(pLib, pContext))
	      return false;
      }

    if (pContext->IsOptionSet(PROGRAM_FILE_ALL))
      {
	  pIter = pFEFile->GetFirstIncludeFile();
	  CFEFile *pIncFile;
	  while ((pIncFile = pFEFile->GetNextIncludeFile(pIter)) != 0)
	    {
		if (!AddTestFunction(pIncFile, pContext))
		    return false;
	    }
      }

    return true;
}

/**	\brief adds the test-functions to the main-function
 *	\param pFELibrary rhe front-end library to search for functions
 *	\param pContext the contet of the code generation
 *	\return true if successful
 */
bool CBETestMainFunction::AddTestFunction(CFELibrary * pFELibrary, CBEContext * pContext)
{
    VectorElement *pIter = pFELibrary->GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = pFELibrary->GetNextInterface(pIter)) != 0)
      {
	  if (!AddTestFunction(pInterface, pContext))
	      return false;
      }

    pIter = pFELibrary->GetFirstLibrary();
    CFELibrary *pNestedLib;
    while ((pNestedLib = pFELibrary->GetNextLibrary(pIter)) != 0)
      {
	  if (!AddTestFunction(pNestedLib, pContext))
	      return false;
      }

    return true;
}

/**	\brief adds the test-functions to the main-function
 *	\param pFEInterface the front-end interface to search for functions
 *	\param pContext the context of the code generation
 *	\return true if successful
 */
bool CBETestMainFunction::AddTestFunction(CFEInterface * pFEInterface, CBEContext * pContext)
{
    if (!pFEInterface)
        return true;

    // search for test server loop
    int nOldType = pContext->SetFunctionType(FUNCTION_SRV_LOOP | FUNCTION_TESTFUNCTION);
    String sFuncName = pContext->GetNameFactory()->GetFunctionName(pFEInterface, pContext);
    pContext->SetFunctionType(nOldType);

    CBERoot *pRoot = GetRoot();
    ASSERT(pRoot);
    CBETestServerFunction *pFunction = (CBETestServerFunction*)pRoot->FindFunction(sFuncName);
    if (!pFunction)
    {
        VERBOSE("CBETestMainFunction::AddTestFunction failed because test srv loop could not be found\n");
        return false;
    }
    AddSrvLoop(pFunction);
    return true;
}

/**	\brief writes the implementation of the main function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * Because the main function is different from the other function, we overload the Write function
 * directly. Ths we control the complete behaviour.
 */
void CBETestMainFunction::Write(CBEImplementationFile * pFile, CBEContext * pContext)
{
    if (!pFile->IsOpen())
        return;

    pFile->Print("int %s(int argc, char **argv)\n", (const char *) m_sName);
    pFile->Print("{\n");
    pFile->IncIndent();

    WriteVariableDeclaration(pFile, pContext);
    pFile->Print("\n");

    WriteVariableInitialization(pFile, pContext);
    pFile->Print("\n");

    WriteTestServer(pFile, pContext);

    WriteCleanup(pFile, pContext);

    WriteReturn(pFile, pContext);

    pFile->DecIndent();
    pFile->Print("}\n");
}

/**	\brief writes the variable declaration for the main function
 *	\param pFile the file to write to
 *	\param pContext the context of the operation
 *
 * The local variables contains the CORBA_objects and environment. Need only one set of
 * variables, because each server is tested seperately and can set them individually.
 */
void CBETestMainFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    String sObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
    pFile->PrintIndent("CORBA_Object %s;\n", (const char *) sObj);

    String sEnv = pContext->GetNameFactory()->GetCorbaEnvironmentVariable(pContext);
    pFile->PrintIndent("CORBA_Environment %s;\n", (const char *) sEnv);
}

/**	\brief writes the variable initialization
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * Because the CORBA_object are often set after the server loop starts, the local variables
 * can only be set to default values and have to be set to real values later.
 */
void CBETestMainFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{

}

/**	\brief write the clean up code
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * Free any allocated memory etc.
 */
void CBETestMainFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{

}

/**	\brief writes the servers to test
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 */
void CBETestMainFunction::WriteTestServer(CBEImplementationFile * pFile, CBEContext * pContext)
{
    VectorElement *pIter = GetFirstSrvLoop();
    CBETestServerFunction *pSrvLoop;
    while ((pSrvLoop = GetNextSrvLoop(pIter)) != 0)
    {
		pFile->PrintIndent("");
        pSrvLoop->WriteCall(pFile, String(), pContext);
        pFile->Print("\n");
    }
}

/**	\brief writes the return code
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * Because we defined the main function to return an integer, we have to return something...
 */
void CBETestMainFunction::WriteReturn(CBEFile * pFile, CBEContext * pContext)
{
    pFile->PrintIndent("return 0; // success\n");
}

/** \brief adds this function to the implementation file
 *  \param pImpl the implementation file
 *  \param pContext the context of this operation
 *  \return true if successful
 */
bool CBETestMainFunction::AddToFile(CBEImplementationFile *pImpl, CBEContext * pContext)
{
	if (IsTargetFile(pImpl))
		pImpl->AddFunction(this);
    return true;
}

/** \brief sets the target file name(s) for the main function
 *  \param pFEObject the reference front-end object
 *  \param pContext the context of this operation
 *
 * The implementation file for the main function is the testsuite file.
 */
void CBETestMainFunction::SetTargetFileName(CFEBase * pFEObject, CBEContext * pContext)
{
	CBEFunction::SetTargetFileName(pFEObject, pContext);
	pContext->SetFileType(FILETYPE_TESTSUITE);
	if (pFEObject->IsKindOf(RUNTIME_CLASS(CFEFile)))
		m_sTargetImplementation = pContext->GetNameFactory()->GetFileName(pFEObject, pContext);
	else
		m_sTargetImplementation = pContext->GetNameFactory()->GetFileName(pFEObject->GetFile(), pContext);
}

/** \brief test if the given implementation file is the calculated target file
 *  \param pFile the implementation file to test
 *  \return true if successful
 */
bool CBETestMainFunction::IsTargetFile(CBEImplementationFile * pFile)
{
	if (m_sTargetImplementation.Right(12) != "-testsuite.c")
		return false;
	String sBaseLocal = m_sTargetImplementation.Left(m_sTargetImplementation.GetLength()-12);
	String sBaseTarget = pFile->GetFileName();
	if (!(sBaseTarget.Right(12) == "-testsuite.c"))
		return false;
	sBaseTarget = sBaseTarget.Left(sBaseTarget.GetLength()-12);
	if (sBaseTarget == sBaseLocal)
		return true;
	return false;
}

/** \brief test if this function should be written to the target file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return true if successful
 *
 * A main function is only written for the test-suite's side.
 */
bool CBETestMainFunction::DoWriteFunction(CBEFile * pFile, CBEContext * pContext)
{
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEHeaderFile)))
		if (!CBEFunction::IsTargetFile((CBEHeaderFile*)pFile))
			return false;
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEImplementationFile)))
		if (!IsTargetFile((CBEImplementationFile*)pFile))
			return false;
	return pFile->GetTarget()->IsKindOf(RUNTIME_CLASS(CBETestsuite));
}
