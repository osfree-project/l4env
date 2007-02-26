/**
 *	\file	dice/src/be/BETestServerFunction.cpp
 *	\brief	contains the implementation of the class CBETestServerFunction
 *
 *	\date	04/04/2002
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

#include "be/BETestServerFunction.h"
#include "be/BEContext.h"
#include "be/BETestFunction.h"
#include "be/BERoot.h"
#include "be/BEImplementationFile.h"
#include "be/BEHeaderFile.h"
#include "be/BETestsuite.h"
#include "be/BEType.h"
#include "be/BETypedDeclarator.h"

#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "fe/FEFile.h"

IMPLEMENT_DYNAMIC(CBETestServerFunction);

CBETestServerFunction::CBETestServerFunction()
: m_vFunctions(RUNTIME_CLASS(CBETestFunction))
{
    m_pFunction = 0;
    IMPLEMENT_DYNAMIC_BASE(CBETestServerFunction, CBEInterfaceFunction);
}

CBETestServerFunction::CBETestServerFunction(CBETestServerFunction & src)
: CBEInterfaceFunction(src),
  m_vFunctions(RUNTIME_CLASS(CBETestFunction))
{
    m_pFunction = 0;
    IMPLEMENT_DYNAMIC_BASE(CBETestServerFunction, CBEInterfaceFunction);
}

/**	\brief destructor of target class */
CBETestServerFunction::~CBETestServerFunction()
{
    m_vFunctions.DeleteAll();
}

/**	\brief creates the test function for the server loop
 *	\param pFEInterface the corresponding front-end interface
 *	\param pContext the context of the code generation
 */
bool CBETestServerFunction::CreateBackEnd(CFEInterface * pFEInterface, CBEContext * pContext)
{
    CBERoot *pRoot = GetRoot();
    assert(pRoot);

    // find server loop to test
    int nOldType = pContext->SetFunctionType(FUNCTION_SRV_LOOP);
    String sFunctionName = pContext->GetNameFactory()->GetFunctionName(pFEInterface, pContext);
    m_pFunction = pRoot->FindFunction(sFunctionName);
    assert(m_pFunction);

    pContext->SetFunctionType(FUNCTION_SRV_LOOP | FUNCTION_TESTFUNCTION);
	// set target file name
	SetTargetFileName(pFEInterface, pContext);
    // get own name
    m_sName = pContext->GetNameFactory()->GetFunctionName(pFEInterface, pContext);
    // reset function type
    pContext->SetFunctionType(nOldType);

    // add test functions
    if (!AddTestFunction(pFEInterface, pContext))
	{
        VERBOSE("%s failed because test function could not be added\n", __PRETTY_FUNCTION__);
        return false;
	}

    return true;
}

/** \brief adds the test function for the given interface and its base interfaces
 *  \param pFEInterface the given interface
 *  \param pContext the context of the create process
 *  \return true if successful
 *
 * We add the functions of the base interface as well, because we want to know if the
 * derived server loop can cope with them.
 */
bool CBETestServerFunction::AddTestFunction(CFEInterface *pFEInterface, CBEContext *pContext)
{
    // add base interface's test functions
    VectorElement *pIter = pFEInterface->GetFirstBaseInterface();
    CFEInterface *pFEBaseInterface;
    while ((pFEBaseInterface = pFEInterface->GetNextBaseInterface(pIter)) != 0)
    {
        if (!AddTestFunction(pFEBaseInterface, pContext))
            return false;
    }
    // add own functions
    pIter = pFEInterface->GetFirstOperation();
    CFEOperation *pOperation;
    while ((pOperation = pFEInterface->GetNextOperation(pIter)) != 0)
    {
        if (!AddTestFunction(pOperation, pContext))
            return false;
    }
    return true;
}

/**	\brief adds the test-function to the main-function
 *	\param pFEOperation the front-end operation to use as reference
 *	\param pContext the context of the operation
 *	\return true if successful
 */
bool CBETestServerFunction::AddTestFunction(CFEOperation * pFEOperation, CBEContext * pContext)
{
    // skip pure OUT functions
    if ((pFEOperation->FindAttribute(ATTR_OUT)) &&
        !(pFEOperation->FindAttribute(ATTR_IN)))
        return true;

    // create test function
    CBETestFunction *pFunction = pContext->GetClassFactory()->GetNewTestFunction();
    AddFunction(pFunction);
    if (!pFunction->CreateBackEnd(pFEOperation, pContext))
    {
        RemoveFunction(pFunction);
        delete pFunction;
        return false;
    }

    return true;
}

/**	\brief adds a new test-function to the main-function
 *	\param pFunction the new function
 */
void CBETestServerFunction::AddFunction(CBETestFunction * pFunction)
{
    m_vFunctions.Add(pFunction);
    pFunction->SetParent(this);
}

/**	\brief removes a function from the function's vector
 *	\param pFunction the function to remove
 */
void CBETestServerFunction::RemoveFunction(CBETestFunction * pFunction)
{
    m_vFunctions.Remove(pFunction);
}

/**	\brief retrieves a pointer to the first test-function
 *	\return a pointer to the first test-function
 */
VectorElement *CBETestServerFunction::GetFirstFunction()
{
    return m_vFunctions.GetFirst();
}

/**	\brief retrieves a reference to the next test-function
 *	\param pIter the pointer to the next function
 *	\return a reference to the next function or 0 if at end of vector
 */
CBETestFunction *CBETestServerFunction::GetNextFunction(VectorElement * &pIter)
{
    if (!pIter)
		return 0;
    CBETestFunction *pRet = (CBETestFunction *) pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
		return GetNextFunction(pIter);
    return pRet;
}

/**	\brief writes the test of a server
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation first starts the server and sets the parameters of the server loop. It then
 * calls the functions testing the server loop and finally terminates the server loop.
 */
void CBETestServerFunction::WriteBody(CBEFile * pFile, CBEContext * pContext)
{
	// variable declaration and initialization
	WriteVariableDeclaration(pFile, pContext);
	WriteVariableInitialization(pFile, pContext);
	// test server loop
    WriteStartServerLoop(pFile, pContext);
    WriteTestFunctions(pFile, pContext);
    WriteStopServerLoop(pFile, pContext);
    // finish
    WriteCleanup(pFile, pContext);
    WriteReturn(pFile, pContext);
}

/**	\brief writes the start code for one server loop
 *	\param pFile the file to write to
 *	\param pContext the context of the writes operation
 */
void CBETestServerFunction::WriteStartServerLoop(CBEFile * pFile, CBEContext * pContext)
{

}

/**	\brief writes the test functions
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * Calls each test-function, which in turn initializes and calls the server loop.
 * If the configuration file is set, we have to check the number and order of the function calls.
 */
void CBETestServerFunction::WriteTestFunctions(CBEFile * pFile, CBEContext * pContext)
{
    VectorElement *pIter = GetFirstFunction();
    CBETestFunction *pFunction;
    while ((pFunction = GetNextFunction(pIter)) != 0)
    {
        WriteTestFunction(pFile, pFunction, pContext);
    }
}

/**	\brief writes the code to call a test-function
 *	\param pFile the file to write to
 *	\param pFunction the function to call
 *	\param pContext the context of the write operation
 */
void CBETestServerFunction::WriteTestFunction(CBEFile * pFile,
					      CBETestFunction * pFunction,
					      CBEContext * pContext)
{
    pFile->PrintIndent("");
    pFunction->WriteCall(pFile, String(), pContext);
    pFile->Print("\n");
}

/**	\brief writes the code to stop a server loop
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 */
void CBETestServerFunction::WriteStopServerLoop(CBEFile * pFile, CBEContext * pContext)
{

}

/** \brief test if this function should be written to the target file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return true if it should be written
 *
 * A server-test function is only written at the test-suite's side if the
 * PROGRAM_GENERATE_TESTSUITE option is set.
 */
bool CBETestServerFunction::DoWriteFunction(CBEFile * pFile, CBEContext * pContext)
{
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEHeaderFile)))
		if (!CBEInterfaceFunction::IsTargetFile((CBEHeaderFile*)pFile))
			return false;
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEImplementationFile)))
		if (!IsTargetFile((CBEImplementationFile*)pFile))
			return false;
	return pFile->GetTarget()->IsKindOf(RUNTIME_CLASS(CBETestsuite)) &&
		   pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE);
}

/** \brief writes the parameters of the server test function
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * no parameters.
 */
void CBETestServerFunction::WriteParameterList(CBEFile * pFile, CBEContext * pContext)
{
	pFile->Print("void");
}

/** \brief writes the parameter list when calling this function
 *  \param pFile the file to write to
 *  \param pContext the context of the write
 *
 * no parameters.
 */
void CBETestServerFunction::WriteCallParameterList(CBEFile * pFile, CBEContext * pContext)
{
}

/** \brief writes this function to the implementation file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * overloaded to declare global variables.
 */
void CBETestServerFunction::Write(CBEImplementationFile * pFile, CBEContext * pContext)
{
	WriteGlobalVariableDeclaration(pFile, pContext);
	pFile->Print("static\n");
	CBEInterfaceFunction::Write(pFile, pContext);
}

/** \brief declares globale variables
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBETestServerFunction::WriteGlobalVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
}

/** \brief test if the given file is the target file for the test-server function
 *  \param pFile the file to test
 *  \return true if test-server function belongs to the file
 */
bool CBETestServerFunction::IsTargetFile(CBEImplementationFile * pFile)
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

/** \brief set name of target file
 *  \param pFEObject the front-end object to use as reference
 *  \param pContext the context of this operation
 *
 * A test-function's implementation file is the test-suite.
 */
void CBETestServerFunction::SetTargetFileName(CFEBase * pFEObject, CBEContext * pContext)
{
	CBEInterfaceFunction::SetTargetFileName(pFEObject, pContext);
	pContext->SetFileType(FILETYPE_TESTSUITE);
	if (pFEObject->IsKindOf(RUNTIME_CLASS(CFEFile)))
		m_sTargetImplementation = pContext->GetNameFactory()->GetFileName(pFEObject, pContext);
	else
		m_sTargetImplementation = pContext->GetNameFactory()->GetFileName(pFEObject->GetFile(), pContext);
}
