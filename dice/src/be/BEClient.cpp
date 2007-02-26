/**
 *    \file    dice/src/be/BEClient.cpp
 *    \brief   contains the implementation of the class CBEClient
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

#include "be/BEClient.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BESndFunction.h"
#include "be/BEWaitFunction.h"
#include "be/BECallFunction.h"
#include "be/BEUnmarshalFunction.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BEOpcodeType.h"
#include "be/BEExpression.h"
#include "be/BEConstant.h"
#include "be/BERoot.h"
#include "be/BEClass.h"
#include "be/BENameSpace.h"

#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "fe/FEUnaryExpression.h"
#include "fe/FEIntAttribute.h"


CBEClient::CBEClient()
{
}

CBEClient::CBEClient(CBEClient & src):CBETarget(src)
{
}

/**    \brief destructor
 */
CBEClient::~CBEClient()
{

}

/**    \brief writes the clients output
 *    \param pContext the context of the write operation
 */
void CBEClient::Write(CBEContext * pContext)
{
    WriteHeaderFiles(pContext);
    WriteImplementationFiles(pContext);
}

/**    \brief sets the current file-type in the context
 *    \param pContext the context to manipulate
 *    \param nHeaderOrImplementation a flag to indicate whether we need a header or implementation file
 */
void CBEClient::SetFileType(CBEContext * pContext, int nHeaderOrImplementation)
{
    switch (nHeaderOrImplementation)
    {
    case FILETYPE_HEADER:
        pContext->SetFileType(FILETYPE_CLIENTHEADER);
        break;
    case FILETYPE_IMPLEMENTATION:
        pContext->SetFileType(FILETYPE_CLIENTIMPLEMENTATION);
        break;
    default:
        CBETarget::SetFileType(pContext, nHeaderOrImplementation);
        break;
    }
}

/** \brief creates the back-end files for a function
 *  \param pFEOperation the front-end function to use as reference
 *  \param pContext the context of the create process
 *  \return true if successful
 */
bool CBEClient::CreateBackEndFunction(CFEOperation *pFEOperation, CBEContext *pContext)
{
    VERBOSE("%s for %s called\n", __PRETTY_FUNCTION__,
        pFEOperation->GetName().c_str());
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
    // if attribute == IN, we need send
    // if attribute == OUT, we need wait, recv, unmarshal
    // if attribute == empty, we need call if test, we need test
    int nOldType;
    string sFuncName;
    CBEFunction *pFunction;
    if (pFEOperation->FindAttribute(ATTR_IN))
    {
        nOldType = pContext->SetFunctionType(FUNCTION_SEND);
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
        // test suite's test function
        if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
        {
            nOldType = pContext->SetFunctionType(FUNCTION_SEND | FUNCTION_TESTFUNCTION);
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
    }
    else if (pFEOperation->FindAttribute(ATTR_OUT))
    {
        // wait function
        nOldType = pContext->SetFunctionType(FUNCTION_WAIT);
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
        // receive function
        pContext->SetFunctionType(FUNCTION_RECV);
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
        // unmarshal function
        if (pContext->IsOptionSet(PROGRAM_GENERATE_MESSAGE))
        {
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
        pContext->SetFunctionType(nOldType);
    }
    else
    {
        nOldType = pContext->SetFunctionType(FUNCTION_CALL);
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
        // test suite's test function
        if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
        {
            nOldType = pContext->SetFunctionType(FUNCTION_CALL | FUNCTION_TESTFUNCTION);
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
    }
    return true;
}

/** \brief creates the header files of the client
 *  \param pFEFile the front end file to use as reference
 *  \param pContext the context of this operation
 *  \return true if successful
 *
 * We could call the base class to create the header file as usual. But we
 * need a reference to it, which we would have to search for. Thus we simply
 * create it here, as the base class would do and use its reference.
 */
bool CBEClient::CreateBackEndHeader(CFEFile * pFEFile, CBEContext * pContext)
{
    VERBOSE("CBEClient::CreateBackEndHeader(file: %s) called\n",
        pFEFile->GetFileName().c_str());
    
    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);
    CBEClassFactory *pCF = pContext->GetClassFactory();
    // the header files are created on a per IDL file basis, no matter
    // which option is set
    CBEHeaderFile *pHeader = pCF->GetNewHeaderFile();
    AddFile(pHeader);
    pContext->SetFileType(FILETYPE_CLIENTHEADER);
    if (!pHeader->CreateBackEnd(pFEFile, pContext))
    {
        RemoveFile(pHeader);
        delete pHeader;
        VERBOSE("%s failed because header file could not be created\n",
	    __PRETTY_FUNCTION__);
        return false;
    }
    pRoot->AddToFile(pHeader, pContext);
    // create opcode files per IDL file
    if (!pContext->IsOptionSet(PROGRAM_NO_OPCODES))
    {
        CBEHeaderFile *pOpcodes = pCF->GetNewHeaderFile();
        AddFile(pOpcodes);
        pContext->SetFileType(FILETYPE_OPCODE);
        if (!pOpcodes->CreateBackEnd(pFEFile, pContext))
        {
            RemoveFile(pOpcodes);
            delete pOpcodes;
            VERBOSE("%s failed because opcode file could not be created\n",
		__PRETTY_FUNCTION__);
            return false;
        }
        pRoot->AddOpcodesToFile(pOpcodes, pFEFile, pContext);
        // include opcode file to included files
        // do not use include file name, since the opcode file is
        // assumed to be in the same directory
        pHeader->AddIncludedFileName(pOpcodes->GetFileName(), true, false, 
	    pFEFile);
    }
    VERBOSE("CBEClient::CreateBackEndHeader(file: %s) return true\n",
        pFEFile->GetFileName().c_str());
    return true;
}

/** \brief create the back-end implementation files
 *  \param pFEFile the respective front-end file
 *  \param pContext the context of this operation
 *  \return true if successful
 */
bool CBEClient::CreateBackEndImplementation(CFEFile * pFEFile, CBEContext * pContext)
{
    VERBOSE("CBEClient::CreateBackEndImplementation(file: %s) called\n",
        pFEFile->GetFileName().c_str());
    // depending on options call respective functions
    if (pContext->IsOptionSet(PROGRAM_FILE_ALL) || pContext->IsOptionSet(PROGRAM_FILE_IDLFILE))
    {
        if (!CreateBackEndFile(pFEFile, pContext))
            return false;
    }
    else if (pContext->IsOptionSet(PROGRAM_FILE_MODULE))
    {
        if (!CreateBackEndModule(pFEFile, pContext))
            return false;
    }
    else if (pContext->IsOptionSet(PROGRAM_FILE_INTERFACE))
    {
        if (!CreateBackEndInterface(pFEFile, pContext))
            return false;
    }
    else if (pContext->IsOptionSet(PROGRAM_FILE_FUNCTION))
    {
        if (!CreateBackEndFunction(pFEFile, pContext))
            return false;
    }
    return true;
}

/** \brief internal function to create a file-pair per front-end file
 *  \param pFEFile the respective front-end file
 *  \param pContext the context of the creation
 *  \return true if successful
 *
 * the client generates one implementation file per IDL file
 * or for all IDL files (depending on the options).
 */
bool CBEClient::CreateBackEndFile(CFEFile *pFEFile, CBEContext *pContext)
{
    if (!pFEFile->IsIDLFile())
        return true;

    VERBOSE("CBEClient::CreateBackEndFile(file: %s) called\n",
        pFEFile->GetFileName().c_str());

    // find appropriate header file
    CBEHeaderFile *pHeader = FindHeaderFile(pFEFile, pContext);
    if (!pHeader)
        return false;

    // create file
    CBEImplementationFile *pImpl = pContext->GetClassFactory()->GetNewImplementationFile();
    AddFile(pImpl);
    pContext->SetFileType(FILETYPE_CLIENTIMPLEMENTATION);
    pImpl->SetHeaderFile(pHeader);
    if (!pImpl->CreateBackEnd(pFEFile, pContext))
    {
        RemoveFile(pImpl);
        delete pImpl;
        VERBOSE("CBEClient::CreateBackEndFile failed because file could not be created\n");
        return false;
    }
    // add interfaces and functions
    if (!CreateBackEndFile(pFEFile, pContext, pImpl))
        return false;
    return true;
}

/** \brief internal functions, which adds all members of a file to a file
 *  \param pFEFile the front-end file to search
 *  \param pContext the context of the creation
 *  \param pImpl the implementation file to add the members to
 *  \return true if successful
 */
bool CBEClient::CreateBackEndFile(CFEFile * pFEFile, CBEContext * pContext, CBEImplementationFile *pImpl)
{
    VERBOSE("CBEClient::CreateBackEndFile(file: %s, impl: %s) called\n",
        pFEFile->GetFileName().c_str(), pImpl->GetFileName().c_str());

    // get root
    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);
    // iterate over interfaces and add them
    vector<CFEInterface*>::iterator iterI = pFEFile->GetFirstInterface();
    CFEInterface *pFEInterface;
    while ((pFEInterface = pFEFile->GetNextInterface(iterI)) != 0)
    {
        CBEClass *pClass = pRoot->FindClass(pFEInterface->GetName());
        if (!pClass)
        {
            VERBOSE("CBEClient::CreateBackEndFile failed because interface %s could not be found\n",
                    pFEInterface->GetName().c_str());
            return false;
        }
        // if class has been added already, then skip it
        if (pImpl->FindClass(pClass->GetName()) != pClass)
            pClass->AddToFile(pImpl, pContext);
    }
    // iterate over libraries and add them
    vector<CFELibrary*>::iterator iterL = pFEFile->GetFirstLibrary();
    CFELibrary *pFELibrary;
    while ((pFELibrary = pFEFile->GetNextLibrary(iterL)) != 0)
    {
        CBENameSpace *pNameSpace = pRoot->FindNameSpace(pFELibrary->GetName());
        if (!pNameSpace)
        {
            VERBOSE("CBEClient::CreateBackEndFile failed because library %s could not be found\n",
                    pFELibrary->GetName().c_str());
            return false;
        }
        // if this namespace is already added, skip it
        if (pImpl->FindNameSpace(pNameSpace->GetName()) != pNameSpace)
            pNameSpace->AddToFile(pImpl, pContext);
    }
    // if FILE_ALL: iterate over included files and call this function using them
    if (pContext->IsOptionSet(PROGRAM_FILE_ALL))
    {
        vector<CFEFile*>::iterator iterF = pFEFile->GetFirstChildFile();
        CFEFile *pIncFile;
        while ((pIncFile = pFEFile->GetNextChildFile(iterF)) != 0)
        {
            if (!CreateBackEndFile(pIncFile, pContext, pImpl))
                return false;
        }
    }
    return true;
}

/** \brief creates the back-end files for the FILE_MODULE option
 *  \param pFEFile the respective front-end file
 *  \param pContext the context of the file creation
 *  \return true if successful
 *
 * Because a file may also contain interfaces, we have to create a file for
 * them as well (if there are any). We assume that interfaces without a module
 * belong to a "default" module (or namespace). The name of the file for these
 * interfaces is derived from the IDL file directly. We do not add typedefs and
 * constants, because we do not add them to implementation files.
 */
bool CBEClient::CreateBackEndModule(CFEFile *pFEFile, CBEContext *pContext)
{
    if (!pFEFile->IsIDLFile())
        return true; // do not abort creation

    VERBOSE("CBEClient::CreateBackEndModule(file: %s) called\n",
        pFEFile->GetFileName().c_str());
    // find appropriate header file
    CBEHeaderFile *pHeader = FindHeaderFile(pFEFile, pContext);
    if (!pHeader)
        return false;

    // get root
    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);
    // check if we have interfaces
    vector<CFEInterface*>::iterator iterI = pFEFile->GetFirstInterface();
    CFEInterface *pFEInterface = pFEFile->GetNextInterface(iterI);
    if (pFEInterface)
    {
        // we do have interfaces
        // create file
        CBEImplementationFile *pImpl = pContext->GetClassFactory()->GetNewImplementationFile();
        AddFile(pImpl);
        pContext->SetFileType(FILETYPE_CLIENTIMPLEMENTATION);
        pImpl->SetHeaderFile(pHeader);
        if (!pImpl->CreateBackEnd(pFEFile, pContext))
        {
            RemoveFile(pImpl);
            delete pImpl;
            VERBOSE("CBEClient::CreateBackEndModule failed because impl. file could not be created\n");
            return false;
        }
        // add interfaces to this file
        iterI = pFEFile->GetFirstInterface();
        while ((pFEInterface = pFEFile->GetNextInterface(iterI)) != 0)
        {
            // find interface
            CBEClass *pBEClass = pRoot->FindClass(pFEInterface->GetName());
            if (!pBEClass)
            {
                RemoveFile(pImpl);
                delete pImpl;
                VERBOSE("CBEClient::CreateBackEndModule failed because function %s is not created\n",
                        pFEInterface->GetName().c_str());
                return false;
            }
            // add interface to file
            pBEClass->AddToFile(pImpl, pContext);
        }
    }
    // iterate over libraries and create files for them
    vector<CFELibrary*>::iterator iterL = pFEFile->GetFirstLibrary();
    CFELibrary *pFELibrary;
    while ((pFELibrary = pFEFile->GetNextLibrary(iterL)) != 0)
    {
        if (!CreateBackEndModule(pFELibrary, pContext))
            return false;
    }
    // success
    return true;
}

/** \brief creates the file for the module
 *  \param pFELibrary the respective front-end library
 *  \param pContext the context of the creation
 *  \return true if successful
 */
bool CBEClient::CreateBackEndModule(CFELibrary *pFELibrary, CBEContext *pContext)
{
    VERBOSE("CBEClient::CreateBackEndModule(lib: %s) called\n",
        pFELibrary->GetName().c_str());
    // get the root
    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);

    // find appropriate header file
    CBEHeaderFile *pHeader = FindHeaderFile(pFELibrary, pContext);
    if (!pHeader)
        return false;

    // search for the library
    CBENameSpace *pBENameSpace = pRoot->FindNameSpace(pFELibrary->GetName());
    if (!pBENameSpace)
    {
        VERBOSE("CBEClient::CreateBackEndModule failed because namespace %s could not be found\n",
                pFELibrary->GetName().c_str());
        return true;
    }
    // create the file
    CBEImplementationFile *pImpl = pContext->GetClassFactory()->GetNewImplementationFile();
    AddFile(pImpl);
    pContext->SetFileType(FILETYPE_CLIENTIMPLEMENTATION);
    pImpl->SetHeaderFile(pHeader);
    if (!pImpl->CreateBackEnd(pFELibrary, pContext))
    {
        RemoveFile(pImpl);
        delete pImpl;
        VERBOSE("CBEClient::CreateBackEndModule failed because implem. file  could not be created\n");
        return false;
    }
    // add it to the file
    pBENameSpace->AddToFile(pImpl, pContext);
    // iterate over nested libs and call this function for them as well
    vector<CFELibrary*>::iterator iterL = pFELibrary->GetFirstLibrary();
    CFELibrary *pFENested;
    while ((pFENested = pFELibrary->GetNextLibrary(iterL)) != 0)
    {
        if (!CreateBackEndModule(pFENested, pContext))
            return false;
    }
    return true;
}

/** \brief creates the files for the FILE_INTERFACE option
 *  \param pFEFile the file to search for interfaces
 *  \param pContext the context of the creation process
 *  \return true if successful
 */
bool CBEClient::CreateBackEndInterface(CFEFile *pFEFile, CBEContext *pContext)
{
    VERBOSE("CBEClient::CreateBackEndInterface(file: %s) called\n",
        pFEFile->GetFileName().c_str());
    // search for top-level interfaces
    vector<CFEInterface*>::iterator iterI = pFEFile->GetFirstInterface();
    CFEInterface *pFEInterface;
    while ((pFEInterface = pFEFile->GetNextInterface(iterI)) != 0)
    {
        if (!CreateBackEndInterface(pFEInterface, pContext))
            return false;
    }
    // search for libraries
    vector<CFELibrary*>::iterator iterL = pFEFile->GetFirstLibrary();
    CFELibrary *pFELibrary;
    while ((pFELibrary = pFEFile->GetNextLibrary(iterL)) != 0)
    {
        if (!CreateBackEndInterface(pFELibrary, pContext))
            return false;
    }
    return true;
}

/** \brief creates the file for the FILE_INTERFACE option
 *  \param pFELibrary the module to search for interfaces
 *  \param pContext the context of the creation
 *  \return true if successful
 */
bool CBEClient::CreateBackEndInterface(CFELibrary *pFELibrary, CBEContext *pContext)
{
    VERBOSE("CBEClient::CreateBackEndInterface(lib: %s) called\n",
        pFELibrary->GetName().c_str());
    // search for interfaces
    vector<CFEInterface*>::iterator iterI = pFELibrary->GetFirstInterface();
    CFEInterface *pFEInterface;
    while ((pFEInterface = pFELibrary->GetNextInterface(iterI)) != 0)
    {
        if (!CreateBackEndInterface(pFEInterface, pContext))
            return false;
    }
    // search for nested libs
    vector<CFELibrary*>::iterator iterL = pFELibrary->GetFirstLibrary();
    CFELibrary *pFENested;
    while ((pFENested = pFELibrary->GetNextLibrary(iterL)) != 0)
    {
        if (!CreateBackEndInterface(pFENested, pContext))
            return false;
    }
    return true;
}

/** \brief create the back-end file for an interface
 *  \param pFEInterface the front-end interface
 *  \param pContext the context of the creation
 *  \return true if successful
 */
bool CBEClient::CreateBackEndInterface(CFEInterface *pFEInterface, CBEContext *pContext)
{
    VERBOSE("CBEClient::CreateBackEndInterface(interface: %s) called\n",
        pFEInterface->GetName().c_str());
    // get root
    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);

    // find appropriate header file
    CBEHeaderFile *pHeader = FindHeaderFile(pFEInterface, pContext);
    if (!pHeader)
        return false;

    // find the interface
    CBEClass *pBEClass = pRoot->FindClass(pFEInterface->GetName());
    if (!pBEClass)
    {
        VERBOSE("CBEClient::CreateBackEndInterface failed because interface %s could not be found\n",
                pFEInterface->GetName().c_str());
        return false;
    }
    // create the file
    CBEImplementationFile *pImpl = pContext->GetClassFactory()->GetNewImplementationFile();
    AddFile(pImpl);
    pContext->SetFileType(FILETYPE_CLIENTIMPLEMENTATION);
    pImpl->SetHeaderFile(pHeader);
    if (!pImpl->CreateBackEnd(pFEInterface, pContext))
    {
        RemoveFile(pImpl);
        delete pImpl;
        VERBOSE("CBEClient::CreateBackEndInterface failed because implementation file couldnot be created\n");
        return false;
    }
    // add the interface
    pBEClass->AddToFile(pImpl, pContext);
    return true;
}

/** \brief creates the files for the FILE_FUNCTION option
 *  \param pFEFile the file to search for functions
 *  \param pContext the context of the create process
 *  \return true if successful
 */
bool CBEClient::CreateBackEndFunction(CFEFile *pFEFile, CBEContext *pContext)
{
    VERBOSE("%s for %s called\n", __PRETTY_FUNCTION__,
        pFEFile->GetFileName().c_str());
    // if there are any top level type definitions and  constants
    // iterate over interfaces
    vector<CFEInterface*>::iterator iterI = pFEFile->GetFirstInterface();
    CFEInterface *pFEInterface;
    while ((pFEInterface = pFEFile->GetNextInterface(iterI)) != 0)
    {
        if (!CreateBackEndFunction(pFEInterface, pContext))
            return false;
    }
    // iterate over libraries
    vector<CFELibrary*>::iterator iterL = pFEFile->GetFirstLibrary();
    CFELibrary *pFELibrary;
    while ((pFELibrary = pFEFile->GetNextLibrary(iterL)) != 0)
    {
        if (!CreateBackEndFunction(pFELibrary, pContext))
            return true;
    }
    return true;
}

/** \brief creates the back-end files for the FILE_FUNCTION option
 *  \param pFELibrary the library to search for functions
 *  \param pContext the context of the create process
 *  \return true if successful
 */
bool CBEClient::CreateBackEndFunction(CFELibrary *pFELibrary, CBEContext *pContext)
{
    VERBOSE("%s for %s called\n", __PRETTY_FUNCTION__,
        pFELibrary->GetName().c_str());
    // search for interface
    vector<CFEInterface*>::iterator iterI = pFELibrary->GetFirstInterface();
    CFEInterface *pFEInterface;
    while ((pFEInterface = pFELibrary->GetNextInterface(iterI)) != 0)
    {
        if (!CreateBackEndFunction(pFEInterface, pContext))
            return false;
    }
    // search for nested libs
    vector<CFELibrary*>::iterator iterL = pFELibrary->GetFirstLibrary();
    CFELibrary *pFENested;
    while ((pFENested = pFELibrary->GetNextLibrary(iterL)) != 0)
    {
        if (!CreateBackEndFunction(pFENested, pContext))
            return false;
    }
    return true;
}

/** \brief creates the back-end file for the FILE_FUNCTION option
 *  \param pFEInterface the interface to search for the functions
 *  \param pContext the context of the creation
 *  \return true if successful
 */
bool CBEClient::CreateBackEndFunction(CFEInterface *pFEInterface, CBEContext *pContext)
{
    VERBOSE("%s for %s called\n", __PRETTY_FUNCTION__,
        pFEInterface->GetName().c_str());
    // search the interface
    vector<CFEOperation*>::iterator iter = pFEInterface->GetFirstOperation();
    CFEOperation *pFEOperation;
    while ((pFEOperation = pFEInterface->GetNextOperation(iter)) != 0)
    {
        if (!CreateBackEndFunction(pFEOperation, pContext))
            return false;
    }
    return true;
}
