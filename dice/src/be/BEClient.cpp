/**
 *	\file	dice/src/be/BEClient.cpp
 *	\brief	contains the implementation of the class CBEClient
 *
 *	\date	01/11/2002
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

#include "be/BEClient.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BESndFunction.h"
#include "be/BERcvFunction.h"
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

IMPLEMENT_DYNAMIC(CBEClient);
IMPLEMENT_DYNAMIC(CPredefinedFunctionID);

CBEClient::CBEClient()
{
    IMPLEMENT_DYNAMIC_BASE(CBEClient, CBETarget);
}

CBEClient::CBEClient(CBEClient & src):CBETarget(src)
{
    IMPLEMENT_DYNAMIC_BASE(CBEClient, CBETarget);
}

/**	\brief destructor
 */
CBEClient::~CBEClient()
{

}

/**	\brief writes the clients output
 *	\param pContext the context of the write operation
 */
void CBEClient::Write(CBEContext * pContext)
{
    WriteHeaderFiles(pContext);
    WriteImplementationFiles(pContext);
}

/**	\brief sets the current file-type in the context
 *	\param pContext the context to manipulate
 *	\param nHeaderOrImplementation a flag to indicate whether we need a header or implementation file
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
    // get root
    CBERoot *pRoot = GetRoot();
    ASSERT(pRoot);
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
        VERBOSE("CBEClient::CreateBackEndFunction failed because file could not be created\n");
        return false;
    }
    // add the functions to the file
    // search the functions
    // if attribute == IN, we need send
    // if attribute == OUT, we need wait, recv, unmarshal
    // if attribute == empty, we need call if test, we need test
    int nOldType;
    String sFuncName;
    CBEFunction *pFunction;
    if (pFEOperation->FindAttribute(ATTR_IN))
    {
        nOldType = pContext->SetFunctionType(FUNCTION_SEND);
        sFuncName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);
        pContext->SetFunctionType(nOldType);
        pFunction = pRoot->FindFunction(sFuncName);
        if (!pFunction)
        {
            VERBOSE("CBEClient::CreateBackEndFunction failed because function %s could not be found\n",
                    (const char*)sFuncName);
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
				VERBOSE("CBEClient::CreateBackEndFunction failed because function %s could not be found\n",
						(const char*)sFuncName);
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
        pFunction = pRoot->FindFunction(sFuncName);
        if (!pFunction)
        {
            VERBOSE("CBEClient::CreateBackEndFunction failed because function %s could not be found\n",
                    (const char*)sFuncName);
            return false;
        }
        pFunction->AddToFile(pImpl, pContext);
        // receive function
        pContext->SetFunctionType(FUNCTION_RECV);
        sFuncName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);
        if (!pFunction)
        {
            VERBOSE("CBEClient::CreateBackEndFunction failed because function %s could not be found\n",
                    (const char*)sFuncName);
            return false;
        }
        pFunction->AddToFile(pImpl, pContext);
        // unmarshal function
        if (pContext->IsOptionSet(PROGRAM_GENERATE_MESSAGE))
        {
            pContext->SetFunctionType(FUNCTION_UNMARSHAL);
            sFuncName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);
            if (!pFunction)
            {
                VERBOSE("CBEClient::CreateBackEndFunction failed because function %s could not be found\n",
                        (const char*)sFuncName);
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
            VERBOSE("CBEClient::CreateBackEndFunction failed because function %s could not be found\n",
                    (const char*)sFuncName);
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
				VERBOSE("CBEClient::CreateBackEndFunction failed because function %s could not be found\n",
						(const char*)sFuncName);
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
 * We could call the base class to create the header file as usual. But we need a
 * reference to it, which we would have to search for. Thus we simply create it here,
 * as the base class would do and use its reference.
 */
bool CBEClient::CreateBackEndHeader(CFEFile * pFEFile, CBEContext * pContext)
{
    // the header files are created on a per IDL file basis, no matter
    // which option is set
    CBEHeaderFile *pHeader = pContext->GetClassFactory()->GetNewHeaderFile();
    AddFile(pHeader);
    pContext->SetFileType(FILETYPE_CLIENTHEADER);
    if (!pHeader->CreateBackEnd(pFEFile, pContext))
    {
        RemoveFile(pHeader);
        delete pHeader;
        VERBOSE("CBEClient::CreateBackEndHeader failed because header file could not be created\n");
        return false;
    }
    GetRoot()->AddToFile(pHeader, pContext);
    // create opcode files per IDL file
    if (!pContext->IsOptionSet(PROGRAM_NO_OPCODES))
    {
        CBEHeaderFile *pOpcodes = pContext->GetClassFactory()->GetNewHeaderFile();
        AddFile(pOpcodes);
        pContext->SetFileType(FILETYPE_OPCODE);
        if (!pOpcodes->CreateBackEnd(pFEFile, pContext))
        {
            RemoveFile(pOpcodes);
            delete pOpcodes;
            VERBOSE("CBEClient::CreateBackEndHeader failed because opcode file could not be created\n");
            return false;
        }
        GetRoot()->AddOpcodesToFile(pOpcodes, pFEFile, pContext);
        // include opcode fie to included files
        // do not use include file name, since the opcode file is
        // assumed to be in the same directory
        pHeader->AddIncludedFileName(pOpcodes->GetFileName(), true);
    }
    return true;
}

/** \brief create the back-end implementation files
 *  \param pFEFile the respective front-end file
 *  \param pContext the context of this operation
 *  \return true if successful
 */
bool CBEClient::CreateBackEndImplementation(CFEFile * pFEFile, CBEContext * pContext)
{
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
    // get root
    CBERoot *pRoot = GetRoot();
    ASSERT(pRoot);
    // iterate over interfaces and add them
    VectorElement *pIter = pFEFile->GetFirstInterface();
    CFEInterface *pFEInterface;
    while ((pFEInterface = pFEFile->GetNextInterface(pIter)) != 0)
    {
        CBEClass *pClass = pRoot->FindClass(pFEInterface->GetName());
        if (!pClass)
        {
            VERBOSE("CBEClient::CreateBackEndFile failed because interface %s could not be found\n",
                    (const char*)pFEInterface->GetName());
            return false;
        }
        pClass->AddToFile(pImpl, pContext);
    }
    // iterate over libraries and add them
    pIter = pFEFile->GetFirstLibrary();
    CFELibrary *pFELibrary;
    while ((pFELibrary = pFEFile->GetNextLibrary(pIter)) != 0)
    {
        CBENameSpace *pNameSpace = pRoot->FindNameSpace(pFELibrary->GetName());
        if (!pNameSpace)
        {
            VERBOSE("CBEClient::CreateBackEndFile failed because library %s could not be found\n",
                    (const char*)pFELibrary->GetName());
            return false;
        }
        pNameSpace->AddToFile(pImpl, pContext);
    }
    // if FILE_ALL: iterate over included files and call this function using them
    if (pContext->IsOptionSet(PROGRAM_FILE_ALL))
    {
        pIter = pFEFile->GetFirstIncludeFile();
        CFEFile *pIncFile;
        while ((pIncFile = pFEFile->GetNextIncludeFile(pIter)) != 0)
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

    // find appropriate header file
    CBEHeaderFile *pHeader = FindHeaderFile(pFEFile, pContext);
    if (!pHeader)
        return false;

    // get root
    CBERoot *pRoot = GetRoot();
    ASSERT(pRoot);
    // check if we have interfaces
	VectorElement *pIter = pFEFile->GetFirstInterface();
    if (pIter)
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
        CFEInterface *pFEInterface;
        while ((pFEInterface = pFEFile->GetNextInterface(pIter)) != 0)
        {
            // find interface
            CBEClass *pBEClass = pRoot->FindClass(pFEInterface->GetName());
            if (!pBEClass)
            {
                RemoveFile(pImpl);
                delete pImpl;
                VERBOSE("CBEClient::CreateBackEndModule failed because function %s is not created\n",
                        (const char*)pFEInterface->GetName());
                return false;
            }
            // add interface to file
            pBEClass->AddToFile(pImpl, pContext);
        }
    }
    // iterate over libraries and create files for them
    pIter = pFEFile->GetFirstLibrary();
    CFELibrary *pFELibrary;
    while ((pFELibrary = pFEFile->GetNextLibrary(pIter)) != 0)
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
    // get the root
    CBERoot *pRoot = GetRoot();
    ASSERT(pRoot);

    // find appropriate header file
    CBEHeaderFile *pHeader = FindHeaderFile(pFELibrary, pContext);
    if (!pHeader)
        return false;

    // search for the library
    CBENameSpace *pBENameSpace = pRoot->FindNameSpace(pFELibrary->GetName());
    if (!pBENameSpace)
    {
        VERBOSE("CBEClient::CreateBackEndModule failed because namespace %s could not be found\n",
                (const char*)pFELibrary->GetName());
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
    VectorElement *pIter = pFELibrary->GetFirstLibrary();
    CFELibrary *pFENested;
    while ((pFENested = pFELibrary->GetNextLibrary(pIter)) != 0)
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
    // search for top-level interfaces
    VectorElement *pIter = pFEFile->GetFirstInterface();
    CFEInterface *pFEInterface;
    while ((pFEInterface = pFEFile->GetNextInterface(pIter)) != 0)
    {
        if (!CreateBackEndInterface(pFEInterface, pContext))
            return false;
    }
    // search for libraries
    pIter = pFEFile->GetFirstLibrary();
    CFELibrary *pFELibrary;
    while ((pFELibrary = pFEFile->GetNextLibrary(pIter)) != 0)
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
    // search for interfaces
    VectorElement *pIter = pFELibrary->GetFirstInterface();
    CFEInterface *pFEInterface;
    while ((pFEInterface = pFELibrary->GetNextInterface(pIter)) != 0)
    {
        if (!CreateBackEndInterface(pFEInterface, pContext))
            return false;
    }
    // search for nested libs
    pIter = pFELibrary->GetFirstLibrary();
    CFELibrary *pFENested;
    while ((pFENested = pFELibrary->GetNextLibrary(pIter)) != 0)
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
    // get root
    CBERoot *pRoot = GetRoot();
    ASSERT(pRoot);

    // find appropriate header file
    CBEHeaderFile *pHeader = FindHeaderFile(pFEInterface, pContext);
    if (!pHeader)
        return false;

    // find the interface
    CBEClass *pBEClass = pRoot->FindClass(pFEInterface->GetName());
    if (!pBEClass)
    {
        VERBOSE("CBEClient::CreateBackEndInterface failed because interface %s could not be found\n",
                (const char*)pFEInterface->GetName());
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
	// if there are any top level type definitions and  constants
    // iterate over interfaces
    VectorElement *pIter = pFEFile->GetFirstInterface();
    CFEInterface *pFEInterface;
    while ((pFEInterface = pFEFile->GetNextInterface(pIter)) != 0)
    {
        if (!CreateBackEndFunction(pFEInterface, pContext))
            return false;
    }
    // iterate over libraries
    pIter = pFEFile->GetFirstLibrary();
    CFELibrary *pFELibrary;
    while ((pFELibrary = pFEFile->GetNextLibrary(pIter)) != 0)
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
    // search for interface
    VectorElement *pIter = pFELibrary->GetFirstInterface();
    CFEInterface *pFEInterface;
    while ((pFEInterface = pFELibrary->GetNextInterface(pIter)) != 0)
    {
        if (!CreateBackEndFunction(pFEInterface, pContext))
            return false;
    }
    // search for nested libs
    pIter = pFELibrary->GetFirstLibrary();
    CFELibrary *pFENested;
    while ((pFENested = pFELibrary->GetNextLibrary(pIter)) != 0)
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
    // search the interface
    VectorElement *pIter = pFEInterface->GetFirstOperation();
    CFEOperation *pFEOperation;
    while ((pFEOperation = pFEInterface->GetNextOperation(pIter)) != 0)
    {
        if (!CreateBackEndFunction(pFEOperation, pContext))
            return false;
    }
    return true;
}
