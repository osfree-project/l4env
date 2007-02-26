/**
 *    \file    dice/src/be/BETestMainFunction.cpp
 *    \brief   contains the implementation of the class CBETestMainFunction
 *
 *    \date    03/11/2002
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

CBETestMainFunction::CBETestMainFunction()
{
}

CBETestMainFunction::CBETestMainFunction(CBETestMainFunction & src)
: CBEFunction(src)
{
    vector<CBETestServerFunction*>::iterator iter;
    for (iter = src.m_vSrvLoops.begin(); iter != src.m_vSrvLoops.end(); iter++)
    {
        CBETestServerFunction *pNew = (CBETestServerFunction*)((*iter)->Clone());
        m_vSrvLoops.push_back(pNew);
        pNew->SetParent(this);
    }
}

/**    \brief destructor of target class */
CBETestMainFunction::~CBETestMainFunction()
{
    while (!m_vSrvLoops.empty())
    {
        delete m_vSrvLoops.back();
        m_vSrvLoops.pop_back();
    }
}

/**    \brief adds a test-function to the server-loop vector
 *    \param pFunction the function to add
 */
void CBETestMainFunction::AddSrvLoop(CBETestServerFunction * pFunction)
{
    if (!pFunction)
        return;
    m_vSrvLoops.push_back(pFunction);
    pFunction->SetParent(this);
}

/**    \brief removes a server-loop test function from the vector
 *    \param pFunction the function to remove
 */
void CBETestMainFunction::RemoveSrvLoop(CBETestServerFunction * pFunction)
{
    if (!pFunction)
        return;
    vector<CBETestServerFunction*>::iterator iter;
    for (iter = m_vSrvLoops.begin(); iter != m_vSrvLoops.end(); iter++)
    {
        if (*iter == pFunction)
        {
            m_vSrvLoops.erase(iter);
            return;
        }
    }
}

/**    \brief retrives a pointer to the first server-loop test function
 *    \return a pointer to the first server-loop test function
 */
vector<CBETestServerFunction*>::iterator CBETestMainFunction::GetFirstSrvLoop()
{
    return m_vSrvLoops.begin();
}

/**    \brief retrieves a reference to the next server-loop test function
 *    \param iter the pointer to the next function
 *    \return a reference to the next server-loop test function
 */
CBETestServerFunction *CBETestMainFunction::GetNextSrvLoop(vector<CBETestServerFunction*>::iterator &iter)
{
    if (iter == m_vSrvLoops.end())
        return 0;
    return *iter++;
}

/**    \brief creates the main function
 *    \param pFEFile the respective front-end file
 *    \param pContext the context of the code generation
 *    \return true if successful
 *
 * Fetch the test-functions and add references here.
 */
bool CBETestMainFunction::CreateBackEnd(CFEFile * pFEFile, CBEContext * pContext)
{
    // call CBEObject's CreateBackEnd method
    if (!CBEObject::CreateBackEnd(pFEFile))
        return false;

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

/**    \brief adds the test-function to the main-function
 *    \param pFEFile the front-end file to search for functions
 *    \param pContext the context of the code generation
 *    \return true if successful
 *
 * This code is taken from CBETarget::AddFunctionToFile.
 */
bool CBETestMainFunction::AddTestFunction(CFEFile * pFEFile, CBEContext * pContext)
{
    if (!pFEFile)
        return true;
    if (!pFEFile->IsIDLFile())
        return true;

    vector<CFEInterface*>::iterator iterI = pFEFile->GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = pFEFile->GetNextInterface(iterI)) != 0)
    {
        if (!AddTestFunction(pInterface, pContext))
            return false;
    }

    vector<CFELibrary*>::iterator iterL = pFEFile->GetFirstLibrary();
    CFELibrary *pLib;
    while ((pLib = pFEFile->GetNextLibrary(iterL)) != 0)
    {
        if (!AddTestFunction(pLib, pContext))
            return false;
    }

    if (pContext->IsOptionSet(PROGRAM_FILE_ALL))
    {
        vector<CFEFile*>::iterator iterF = pFEFile->GetFirstChildFile();
        CFEFile *pIncFile;
        while ((pIncFile = pFEFile->GetNextChildFile(iterF)) != 0)
        {
            if (!AddTestFunction(pIncFile, pContext))
                return false;
        }
    }

    return true;
}

/**    \brief adds the test-functions to the main-function
 *    \param pFELibrary rhe front-end library to search for functions
 *    \param pContext the contet of the code generation
 *    \return true if successful
 */
bool CBETestMainFunction::AddTestFunction(CFELibrary * pFELibrary, CBEContext * pContext)
{
    vector<CFEInterface*>::iterator iterI = pFELibrary->GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = pFELibrary->GetNextInterface(iterI)) != 0)
    {
        if (!AddTestFunction(pInterface, pContext))
            return false;
    }

    vector<CFELibrary*>::iterator iterL = pFELibrary->GetFirstLibrary();
    CFELibrary *pNestedLib;
    while ((pNestedLib = pFELibrary->GetNextLibrary(iterL)) != 0)
    {
        if (!AddTestFunction(pNestedLib, pContext))
            return false;
    }

    return true;
}

/**    \brief adds the test-functions to the main-function
 *    \param pFEInterface the front-end interface to search for functions
 *    \param pContext the context of the code generation
 *    \return true if successful
 */
bool CBETestMainFunction::AddTestFunction(CFEInterface * pFEInterface, CBEContext * pContext)
{
    if (!pFEInterface)
        return true;

    // search for test server loop
    int nOldType = pContext->SetFunctionType(FUNCTION_SRV_LOOP | FUNCTION_TESTFUNCTION);
    string sFuncName = pContext->GetNameFactory()->GetFunctionName(pFEInterface, pContext);
    pContext->SetFunctionType(nOldType);

    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);
    CBETestServerFunction *pFunction = (CBETestServerFunction*)pRoot->FindFunction(sFuncName);
    if (!pFunction)
    {
        VERBOSE("CBETestMainFunction::AddTestFunction failed because test srv loop could not be found\n");
        return false;
    }
    AddSrvLoop(pFunction);
    return true;
}

/**    \brief writes the implementation of the main function
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * Because the main function is different from the other function, we overload
 * the Write function directly. Ths we control the complete behaviour.
 */
void 
CBETestMainFunction::Write(CBEImplementationFile * pFile,
    CBEContext * pContext)
{
    if (!pFile->IsOpen())
        return;

    pFile->Print("int %s(int argc, char **argv)\n", m_sName.c_str());
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

/**    \brief writes the variable declaration for the main function
 *    \param pFile the file to write to
 *    \param pContext the context of the operation
 *
 * The local variables contains the CORBA_objects and environment. Need only
 * one set of variables, because each server is tested seperately and can set
 * them individually.
 */
void
CBETestMainFunction::WriteVariableDeclaration(CBEFile * pFile, 
    CBEContext * pContext)
{
    CBENameFactory *pNF = pContext->GetNameFactory();
    string sObj = pNF->GetCorbaObjectVariable(pContext);
    *pFile << "\tCORBA_Object_base _" << sObj << ";\n";
    *pFile << "\tCORBA_Object " << sObj << " = &_" << sObj << ";\n";

    string sEnv = pNF->GetCorbaEnvironmentVariable(pContext);
    *pFile << "\tCORBA_Environment " << sEnv;
    if (pContext->IsBackEndSet(PROGRAM_BE_C))
	*pFile << " = dice_default_environment";
    *pFile << ";\n";
}

/**    \brief writes the variable initialization
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * Because the CORBA_object are often set after the server loop starts, the local variables
 * can only be set to default values and have to be set to real values later.
 */
void CBETestMainFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{

}

/**    \brief write the clean up code
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * Free any allocated memory etc.
 */
void CBETestMainFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{

}

/**    \brief writes the servers to test
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 */
void CBETestMainFunction::WriteTestServer(CBEImplementationFile * pFile, CBEContext * pContext)
{
    vector<CBETestServerFunction*>::iterator iter = GetFirstSrvLoop();
    CBETestServerFunction *pSrvLoop;
    while ((pSrvLoop = GetNextSrvLoop(iter)) != 0)
    {
        pFile->PrintIndent("");
        pSrvLoop->WriteCall(pFile, string(), pContext);
        pFile->Print("\n");
    }
}

/**    \brief writes the return code
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
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
    if (dynamic_cast<CFEFile*>(pFEObject))
        m_sTargetImplementation = pContext->GetNameFactory()->GetFileName(pFEObject, pContext);
    else
        m_sTargetImplementation = pContext->GetNameFactory()->GetFileName(pFEObject->GetSpecificParent<CFEFile>(0), pContext);
}

/** \brief test if the given implementation file is the calculated target file
 *  \param pFile the implementation file to test
 *  \return true if successful
 */
bool CBETestMainFunction::IsTargetFile(CBEImplementationFile * pFile)
{
    long length = m_sTargetImplementation.length();
    if ((m_sTargetImplementation.substr(length - 12) != "-testsuite.c") &&
	(m_sTargetImplementation.substr(length - 13) != "-testsuite.cc"))
        return false;
    string sBaseLocal = m_sTargetImplementation.substr(0, length-12);
    string sBaseTarget = pFile->GetFileName();
    length = sBaseTarget.length();
    if ((sBaseTarget.substr(length - 12) != "-testsuite.c") &&
	(sBaseTarget.substr(length - 13) != "-testsuite.cc"))
        return false;
    sBaseTarget = sBaseTarget.substr(0, length-12);
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
bool CBETestMainFunction::DoWriteFunction(CBEHeaderFile * pFile, CBEContext * pContext)
{
    if (!CBEFunction::IsTargetFile(pFile))
        return false;
    return dynamic_cast<CBETestsuite*>(pFile->GetTarget());
}

/** \brief test if this function should be written to the target file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return true if successful
 *
 * A main function is only written for the test-suite's side.
 */
bool CBETestMainFunction::DoWriteFunction(CBEImplementationFile * pFile, CBEContext * pContext)
{
    if (!IsTargetFile(pFile))
        return false;
    return dynamic_cast<CBETestsuite*>(pFile->GetTarget());
}
