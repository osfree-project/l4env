/**
 *	\file	dice/src/be/BETarget.cpp
 *	\brief	contains the implementation of the class CBETarget
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

#include "be/BETarget.h"
#include "be/BENameSpace.h"
#include "be/BEContext.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BESndFunction.h"
#include "be/BERcvFunction.h"
#include "be/BEWaitFunction.h"
#include "be/BEReplyRcvFunction.h"
#include "be/BEReplyWaitFunction.h"
#include "be/BERcvAnyFunction.h"
#include "be/BEWaitAnyFunction.h"
#include "be/BEConstant.h"
#include "be/BETypedef.h"
#include "be/BEMsgBufferType.h"
#include "be/BEFunction.h"
#include "be/BERoot.h"
#include "be/BEDeclarator.h"

#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "fe/FEFile.h"
#include "fe/FEConstDeclarator.h"

IMPLEMENT_DYNAMIC(CBETarget);

CBETarget::CBETarget():m_vHeaderFiles(RUNTIME_CLASS(CBEHeaderFile)),
m_vImplementationFiles(RUNTIME_CLASS(CBEImplementationFile))
{
    IMPLEMENT_DYNAMIC_BASE(CBETarget, CBEObject);
}

CBETarget::CBETarget(CBETarget & src):CBEObject(src),
m_vHeaderFiles(RUNTIME_CLASS(CBEHeaderFile)),
m_vImplementationFiles(RUNTIME_CLASS(CBEImplementationFile))
{
    m_vHeaderFiles.Add(&(src.m_vHeaderFiles));
    m_vHeaderFiles.SetParentOfElements(this);
    m_vImplementationFiles.Add(&(src.m_vImplementationFiles));
    m_vImplementationFiles.SetParentOfElements(this);
    IMPLEMENT_DYNAMIC_BASE(CBETarget, CBEObject);
}

/**	\brief destructor of target class */
CBETarget::~CBETarget()
{
    m_vHeaderFiles.DeleteAll();
    m_vImplementationFiles.DeleteAll();
}

/**	\brief optimizes the existing back-end
 *	\param nLevel the level of optimization
 *  \param pContext the context of the optimization
 *	\return success or failure code
 *
 * This implementation iterates over its files and tries to optimize them.
 */
int CBETarget::Optimize(int nLevel, CBEContext *pContext)
{
    int nRet = 0;

    VectorElement *pIter = GetFirstHeaderFile();
    CBEHeaderFile *pHeader;
    while ((pHeader = GetNextHeaderFile(pIter)) != 0)
    {
        if ((nRet = pHeader->Optimize(nLevel, pContext)) != 0)
            return nRet;
    }

    pIter = GetFirstImplementationFile();
    CBEImplementationFile *pImplement;
    while ((pImplement = GetNextImplementationFile(pIter)) != 0)
    {
        if ((nRet = pImplement->Optimize(nLevel, pContext)) != 0)
            return nRet;
    }

    return 0;
}

/**	\brief generates the output files and code
 *	\param pContext the context of the code generation
 */
void CBETarget::Write(CBEContext * pContext)
{
    ASSERTC(false);
}

/**	\brief adds another header file to the respective vector
 *	\param pHeaderFile the header file to add
 */
void CBETarget::AddFile(CBEHeaderFile * pHeaderFile)
{
    if (!pHeaderFile)
	return;
    m_vHeaderFiles.Add(pHeaderFile);
    pHeaderFile->SetParent(this);
}

/**	\brief removes a header file from the respective vector
 *	\param pHeader the header file to remove
 */
void CBETarget::RemoveFile(CBEHeaderFile * pHeader)
{
    if (!pHeader)
	return;
    m_vHeaderFiles.Remove(pHeader);
}

/**	\brief adds another implementation file to the respective vector
 *	\param pImplementationFile the implementation file to add
 */
void CBETarget::AddFile(CBEImplementationFile * pImplementationFile)
{
    if (!pImplementationFile)
	return;
    m_vImplementationFiles.Add(pImplementationFile);
    pImplementationFile->SetParent(this);
}

/**	\brief removes an implementation file from the respective vectpr
 *	\param pImplementation the implementation file to remove
 */
void CBETarget::RemoveFile(CBEImplementationFile * pImplementation)
{
    if (!pImplementation)
	return;
    m_vImplementationFiles.Remove(pImplementation);
}

/**	\brief retrievs a pointer to the first header file
 *	\return a pointer to the first header file
 */
VectorElement *CBETarget::GetFirstHeaderFile()
{
    return m_vHeaderFiles.GetFirst();
}

/**	\brief retrieves the next header file using the given pointer
 *	\param pIter the pointer to the next header file
 *	\return a reference to the next header file
 */
CBEHeaderFile *CBETarget::GetNextHeaderFile(VectorElement * &pIter)
{
    if (!pIter)
	return 0;
    CBEHeaderFile *pRet = (CBEHeaderFile *) pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
	return GetNextHeaderFile(pIter);
    return pRet;
}

/**	\brief retrieves a pointer to the first implementation file
 *	\return a pointer to the first implementation file
 */
VectorElement *CBETarget::GetFirstImplementationFile()
{
    return m_vImplementationFiles.GetFirst();
}

/**	\brief retrieves a reference to the next implementation file
 *	\param pIter the pointer to the next implementation file
 *	\return a reference to the next implementation file
 */
CBEImplementationFile *CBETarget::GetNextImplementationFile(VectorElement * &pIter)
{
    if (!pIter)
        return 0;
    CBEImplementationFile *pRet = (CBEImplementationFile *) pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextImplementationFile(pIter);
    return pRet;
}

/**	\brief writes the header files
 *	\param pContext the context of the write operation
 */
void CBETarget::WriteHeaderFiles(CBEContext * pContext)
{
    VectorElement *pIter = GetFirstHeaderFile();
    CBEHeaderFile *pFile;
    while ((pFile = GetNextHeaderFile(pIter)) != 0)
    {
        pFile->Write(pContext);
    }
}

/**	\brief writes the implementation files
 *	\param pContext the context of the write operation
 */
void CBETarget::WriteImplementationFiles(CBEContext * pContext)
{
    VectorElement *pIter = GetFirstImplementationFile();
    CBEImplementationFile *pFile;
    while ((pFile = GetNextImplementationFile(pIter)) != 0)
    {
        pFile->Write(pContext);
    }
}

/**	\brief adds the constant of the front-end file to the back-end file
 *	\param pFile the back-end file
 *	\param pFEFile the front-end file
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * This implementation iterates over the interfaces and libs of the current file. It
 * also iterates over the included files if the program options allow it. And the
 * implementation also call AddConstantToFile for all constants of the file.
 */
bool CBETarget::AddConstantToFile(CBEFile * pFile, CFEFile * pFEFile, CBEContext * pContext)
{
    if (!pFEFile)
    {
        VERBOSE("CBETarget::AddConstantToFile aborted because front-end file is 0\n");
        return true;
    }
    if (!pFEFile->IsIDLFile())
    {
        VERBOSE("CBETarget::AddConstantToFile aborted because front-end file is not IDL file\n");
        return true;
    }

    VectorElement *pIter = pFEFile->GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = pFEFile->GetNextInterface(pIter)) != 0)
    {
        if (!AddConstantToFile(pFile, pInterface, pContext))
            return false;
    }

    pIter = pFEFile->GetFirstLibrary();
    CFELibrary *pLib;
    while ((pLib = pFEFile->GetNextLibrary(pIter)) != 0)
    {
        if (!AddConstantToFile(pFile, pLib, pContext))
            return false;
    }

    pIter = pFEFile->GetFirstConstant();
    CFEConstDeclarator *pFEConstant;
    while ((pFEConstant = pFEFile->GetNextConstant(pIter)) != 0)
    {
        if (!AddConstantToFile(pFile, pFEConstant, pContext))
            return false;
    }

    if (pContext->IsOptionSet(PROGRAM_FILE_ALL))
    {
        pIter = pFEFile->GetFirstIncludeFile();
        CFEFile *pIncFile;
        while ((pIncFile = pFEFile->GetNextIncludeFile(pIter)) != 0)
        {
            if (!AddConstantToFile(pFile, pIncFile, pContext))
                return false;
        }
    }

    return true;
}

/**	\brief adds the constants of the front-end library to the back-end file
 *	\param pFile the back-end file
 *	\param pFELibrary the front-end library
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * This implementation iterates over the interfaces and nested libs of the current library.
 * It also calls AddConstantToFile for every constant of the library.
 */
bool CBETarget::AddConstantToFile(CBEFile * pFile, CFELibrary * pFELibrary, CBEContext * pContext)
{
    VectorElement *pIter = pFELibrary->GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = pFELibrary->GetNextInterface(pIter)) != 0)
    {
        if (!AddConstantToFile(pFile, pInterface, pContext))
            return false;
    }

    pIter = pFELibrary->GetFirstLibrary();
    CFELibrary *pNestedLib;
    while ((pNestedLib = pFELibrary->GetNextLibrary(pIter)) != 0)
    {
        if (!AddConstantToFile(pFile, pNestedLib, pContext))
            return false;
    }

    pIter = pFELibrary->GetFirstConstant();
    CFEConstDeclarator *pFEConstant;
    while ((pFEConstant = pFELibrary->GetNextConstant(pIter)) != 0)
    {
        if (!AddConstantToFile(pFile, pFEConstant, pContext))
            return false;
    }

    return true;
}

/**	\brief adds the constants of the front-end interface to the back-end file
 *	\param pFile the back-end file
 *	\param pFEInterface the front-end interface
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * This implementation iterates over the constants of the current interface
 */
bool CBETarget::AddConstantToFile(CBEFile * pFile, CFEInterface * pFEInterface, CBEContext * pContext)
{
    if (!pFEInterface)
    {
        VERBOSE("CBETarget::AddConstantToFile (interface) aborted because front-end interface is 0\n");
        return true;
    }

    VectorElement *pIter = pFEInterface->GetFirstConstant();
    CFEConstDeclarator *pFEConstant;
    while ((pFEConstant = pFEInterface->GetNextConstant(pIter)) != 0)
    {
        if (!AddConstantToFile(pFile, pFEConstant, pContext))
            return false;
    }

    return true;
}

/**	\brief adds the constants of the front-end to the back-end file
 *	\param pFile the file to add the function to
 *	\param pFEConstant the constant to generate the back-end constant from
 *	\param pContext the context of the code generation
 *	\return true if generation went alright
 *
 * This implementation generates a back-end constant and adds it to the header file.
 */
bool CBETarget::AddConstantToFile(CBEFile * pFile, CFEConstDeclarator * pFEConstant, CBEContext * pContext)
{
    if (pFile->IsKindOf(RUNTIME_CLASS(CBEHeaderFile)))
    {
        CBERoot *pRoot = GetRoot();
        ASSERT(pRoot);
        CBEConstant *pConstant = pRoot->FindConstant(pFEConstant->GetName());
        if (!pConstant)
        {
            pConstant = pContext->GetClassFactory()->GetNewConstant();
            ((CBEHeaderFile *) pFile)->AddConstant(pConstant);
            if (!pConstant->CreateBackEnd(pFEConstant, pContext))
            {
                ((CBEHeaderFile *) pFile)->RemoveConstant(pConstant);
                delete pConstant;
                VERBOSE("CBETarget::AddConstantToFile (constant) failed because could not create BE constant\n");
                return false;
            }
        }
        else
            ((CBEHeaderFile *) pFile)->AddConstant(pConstant);
    }

    return true;
}

/**	\brief adds the type definition of the front-end file to the back-end file
 *	\param pFile the back-end file
 *	\param pFEFile the front-end file
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * This implementation iterates over the interfaces and libs of the current file. It
 * also iterates over the included files if the program options allow it. And it calls
 * AddTypedefToFile for the type definitions of the file
 */
bool CBETarget::AddTypedefToFile(CBEFile * pFile, CFEFile * pFEFile, CBEContext * pContext)
{
    if (!pFEFile)
        return true;
    if (!pFEFile->IsIDLFile())
        return true;

    VectorElement *pIter = pFEFile->GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = pFEFile->GetNextInterface(pIter)) != 0)
    {
        if (!AddTypedefToFile(pFile, pInterface, pContext))
            return false;
    }

    pIter = pFEFile->GetFirstLibrary();
    CFELibrary *pLib;
    while ((pLib = pFEFile->GetNextLibrary(pIter)) != 0)
    {
        if (!AddTypedefToFile(pFile, pLib, pContext))
            return false;
    }

    pIter = pFEFile->GetFirstTypedef();
    CFETypedDeclarator *pFETypedef;
    while ((pFETypedef = pFEFile->GetNextTypedef(pIter)) != 0)
    {
        if (!AddTypedefToFile(pFile, pFETypedef, pContext))
            return false;
    }

    if (pContext->IsOptionSet(PROGRAM_FILE_ALL))
    {
        pIter = pFEFile->GetFirstIncludeFile();
        CFEFile *pIncFile;
        while ((pIncFile = pFEFile->GetNextIncludeFile(pIter)) != 0)
        {
            if (!AddTypedefToFile(pFile, pIncFile, pContext))
                return false;
        }
    }

    return true;
}

/**	\brief adds the type definitions of the front-end library to the back-end file
 *	\param pFile the back-end file
 *	\param pFELibrary the front-end library
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * This implementation iterates over the interfaces and nested libs of the current library.
 * And it calls AddTypedefToFile for the type definitions in the library.
 */
bool CBETarget::AddTypedefToFile(CBEFile * pFile, CFELibrary * pFELibrary, CBEContext * pContext)
{
    VectorElement *pIter = pFELibrary->GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = pFELibrary->GetNextInterface(pIter)) != 0)
    {
        if (!AddTypedefToFile(pFile, pInterface, pContext))
            return false;
    }

    pIter = pFELibrary->GetFirstLibrary();
    CFELibrary *pNestedLib;
    while ((pNestedLib = pFELibrary->GetNextLibrary(pIter)) != 0)
    {
        if (!AddTypedefToFile(pFile, pNestedLib, pContext))
            return false;
    }

    pIter = pFELibrary->GetFirstTypedef();
    CFETypedDeclarator *pFETypedef;
    while ((pFETypedef = pFELibrary->GetNextTypedef(pIter)) != 0)
    {
        if (!AddTypedefToFile(pFile, pFETypedef, pContext))
            return false;
    }

    return true;
}

/**	\brief adds the type definitions of the front-end interface to the back-end file
 *	\param pFile the back-end file
 *	\param pFEInterface the front-end interface
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * This implementation iterates over the type definitions of the current interface.
 * It also adds one message buffer per interface to the header file.
 */
bool CBETarget::AddTypedefToFile(CBEFile * pFile, CFEInterface * pFEInterface, CBEContext * pContext)
{
    VectorElement *pIter = pFEInterface->GetFirstTypeDef();
    CFETypedDeclarator *pFETypedef;
    while ((pFETypedef = pFEInterface->GetNextTypeDef(pIter)) != 0)
    {
        if (!AddTypedefToFile(pFile, pFETypedef, pContext))
            return false;
    }

//    CBEMsgBufferType *pMsgBuf = pContext->GetClassFactory()->GetNewMessageBufferType();
//    ((CBEHeaderFile *) pFile)->AddTypedef(pMsgBuf);
//    pMsgBuf->SetParent(this);
//    if (!pMsgBuf->CreateBackEnd(pFEInterface, pContext))
//    {
//        ((CBEHeaderFile *) pFile)->RemoveTypedef(pMsgBuf);
//        delete pMsgBuf;
//        return false;
//    }

    return true;
}

/**	\brief adds the type definitions of the front-end to the back-end file
 *	\param pFile the file to add the function to
 *	\param pFETypedDeclarator the typedef to generate the back-end typedef from
 *	\param pContext the context of the code generation
 *	\return true if generation went alright
 *
 * This implementation adds the type definition to the header file, but skips the implementation file.
 * It searches for the typedef at the root and then adds a reference to its own collection.
 */
bool CBETarget::AddTypedefToFile(CBEFile * pFile,
				 CFETypedDeclarator * pFETypedDeclarator,
				 CBEContext * pContext)
{
    if (pFile->IsKindOf(RUNTIME_CLASS(CBEHeaderFile)))
    {
        CBERoot *pRoot = GetRoot();
        ASSERT(pRoot);
        VectorElement *pIter = pFETypedDeclarator->GetFirstDeclarator();
        CFEDeclarator *pDecl = pFETypedDeclarator->GetNextDeclarator(pIter);
        CBETypedef *pTypedef = pRoot->FindTypedef(pDecl->GetName());
        ASSERT(pTypedef);
        ((CBEHeaderFile *) pFile)->AddTypedef(pTypedef);
    }
    return true;
}

/**	\brief sets the current file-type in the context
 *	\param pContext the context to manipulate
 *	\param nHeaderOrImplementation a flag to indicate whether we need a header or implementation file
 *
 * This implementation is empty, because this function should be overloaded by client and component
 */
void CBETarget::SetFileType(CBEContext * pContext, int nHeaderOrImplementation)
{
    pContext->SetFileType(nHeaderOrImplementation);
}

/**	\brief finds a header file which belongs to a certain front-end file
 *	\param pFEFile the front-end file
 *	\param pContext the context of the file's creation
 *	\return a reference to the respective file; 0 if not found
 *
 * The search uses the file name to find a file. The name-factory generates a file-name based on
 * the front-end file and this name is used to find the file.
 */
CBEHeaderFile *CBETarget::FindHeaderFile(CFEFile * pFEFile, CBEContext * pContext)
{
    // get file name
    int nFileType = pContext->GetFileType();
    SetFileType(pContext, FILETYPE_HEADER);
    String sFileName = pContext->GetNameFactory()->GetFileName(pFEFile, pContext);
    pContext->SetFileType(nFileType);
    // search file
    return FindHeaderFile(sFileName, pContext);
}

/**	\brief finds a header file which belongs to a certain front-end library
 *	\param pFELibrary the front-end library
 *	\param pContext the context of the file's creation
 *	\return a reference to the respective file; 0 if not found
 *
 * The search uses the file name to find a file. The name-factory generates a file-name based on
 * the front-end library and this name is used to find the file.
 */
CBEHeaderFile *CBETarget::FindHeaderFile(CFELibrary * pFELibrary, CBEContext * pContext)
{
    // get file name
    int nFileType = pContext->GetFileType();
    SetFileType(pContext, FILETYPE_HEADER);
    String sFileName = pContext->GetNameFactory()->GetFileName(pFELibrary->GetFile(), pContext);
    pContext->SetFileType(nFileType);
    // search file
    return FindHeaderFile(sFileName, pContext);
}

/**	\brief finds a header file which belongs to a certain front-end interface
 *	\param pFEInterface the front-end interface
 *	\param pContext the context of the file's creation
 *	\return a reference to the respective file; 0 if not found
 *
 * The search uses the file name to find a file. The name-factory generates a file-name based on
 * the front-end interface and this name is used to find the file.
 */
CBEHeaderFile *CBETarget::FindHeaderFile(CFEInterface * pFEInterface, CBEContext * pContext)
{
    // get file name
    int nFileType = pContext->GetFileType();
    SetFileType(pContext, FILETYPE_HEADER);
    String sFileName = pContext->GetNameFactory()->GetFileName(pFEInterface->GetFile(), pContext);
    pContext->SetFileType(nFileType);
    // search file
    return FindHeaderFile(sFileName, pContext);
}

/**	\brief finds a header file which belongs to a certain front-end operation
 *	\param pFEOperation the front-end operation
 *	\param pContext the context of the file's creation
 *	\return a reference to the respective file; 0 if not found
 *
 * The search uses the file name to find a file. The name-factory generates a file-name based on
 * the front-end operation and this name is used to find the file.
 */
CBEHeaderFile *CBETarget::FindHeaderFile(CFEOperation * pFEOperation, CBEContext * pContext)
{
    // get file name
    int nFileType = pContext->GetFileType();
    SetFileType(pContext, FILETYPE_HEADER);
    String sFileName = pContext->GetNameFactory()->GetFileName(pFEOperation->GetFile(), pContext);
    pContext->SetFileType(nFileType);
    // search file
    return FindHeaderFile(sFileName, pContext);
}

/**	\brief finds a header file which belongs to a certain name
 *	\param sFileName the file name to search for
 *	\param pContext the context of the file's creation
 *	\return a reference to the respective file; 0 if not found
 */
CBEHeaderFile *CBETarget::FindHeaderFile(String sFileName, CBEContext * pContext)
{
    VectorElement *pIter = GetFirstHeaderFile();
    CBEHeaderFile *pHeader;
    while ((pHeader = GetNextHeaderFile(pIter)) != 0)
    {
        if (pHeader->GetFileName() == sFileName)
            return pHeader;
    }
    return 0;
}

/**	\brief tries to find the typedef with a type of the given name
 *	\param sTypeName the name to search for
 *	\return a reference to the searched typedef or 0
 *
 * To find a typedef, we iterate over the header files and check them. (Implementation
 * file cannot contain typedefs).
 */
CBETypedef *CBETarget::FindTypedef(String sTypeName)
{
    CBETypedef *pRet = 0;
    VectorElement *pIter = GetFirstHeaderFile();
    CBEHeaderFile *pHeader;
    while (((pHeader = GetNextHeaderFile(pIter)) != 0) && (!pRet))
    {
        pRet = pHeader->FindTypedef(sTypeName);
    }
    return pRet;
}

/**	\brief tries to find the function with the given function name
 *	\param sFunctionName the name to search for
 *	\return a reference to the found function or 0 if not found
 *
 * To find a function, we iterate over the header files and the implementation files.
 */
CBEFunction *CBETarget::FindFunction(String sFunctionName)
{
    CBEFunction *pRet = 0;
    // search header files
    VectorElement *pIter = GetFirstHeaderFile();
    CBEHeaderFile *pHeader;
    while (((pHeader = GetNextHeaderFile(pIter)) != 0) && (!pRet))
    {
        pRet = pHeader->FindFunction(sFunctionName);
    }
    // search implementation files
    pIter = GetFirstImplementationFile();
    CBEImplementationFile *pImplementation;
    while (((pImplementation = GetNextImplementationFile(pIter)) != 0) && (!pRet))
    {
        pRet = pImplementation->FindFunction(sFunctionName);
    }
    return pRet;
}

/** \brief creates the back-end classes for this target
 *  \param pFEFile is the respective front-end file to use as reference
 *  \param pContext the context of this create process
 *  \return true if successful
 */
bool CBETarget::CreateBackEnd(CFEFile *pFEFile, CBEContext *pContext)
{
    // if argument is 0, we assume a mistaken include file
    if (!pFEFile)
    {
        VERBOSE("CBETarget::CreateBackEnd aborted because front-end file is 0\n");
        return true;
    }
    // if file is not IDL file we simply return "no error", because C files might also be included files
    if (!pFEFile->IsIDLFile())
    {
        VERBOSE("CBETarget::CreateBackEnd aborted because front-end file is not IDL file\n");
        return true;
    }

    // create included files first, because they usually contain base interfaces
    if (!pContext->IsOptionSet(PROGRAM_FILE_ALL))
    {
        VectorElement *pIter = pFEFile->GetFirstIncludeFile();
        CFEFile *pIncFile;
        while ((pIncFile = pFEFile->GetNextIncludeFile(pIter)) != 0)
        {
            if (!CreateBackEnd(pIncFile, pContext))
                return false;
        }
    }

    // create header file(s)
    if (!CreateBackEndHeader(pFEFile, pContext))
        return false;

	// create implementation file(s)
	if (!CreateBackEndImplementation(pFEFile, pContext))
		return false;

	return true;
}

/** \brief creates the default header file for the IDL file
 *  \param pFEFile the front-end file to use as reference
 *  \param pContext the context of the create process
 *  \return true if successful
 */
bool CBETarget::CreateBackEndHeader(CFEFile *pFEFile, CBEContext *pContext)
{
    ASSERTC(false);
    return false;
}

/** \brief creates the back-end implementation file(s)
 *  \param pFEFile the front-end reference file
 *  \param pContext the context of the create process
 *  \return true if successful
 *
 * This function has to be overloaded by the derived classes (client, component, testsuite).
 */
bool CBETarget::CreateBackEndImplementation(CFEFile *pFEFile, CBEContext *pContext)
{
    ASSERTC(false);
    return false;
}

/** \brief prints all generated target file name to the given output
 *  \param output the output stream to write to
 *  \param nCurCol the current column where to start output (indent)
 *  \param nMaxCol the maximum number of columns
 */
void CBETarget::PrintTargetFiles(FILE *output, int &nCurCol, int nMaxCol)
{
	// iterate over implementation files
	VectorElement *pIter = GetFirstHeaderFile();
	CBEHeaderFile *pHeader;
	while ((pHeader = GetNextHeaderFile(pIter)) != 0)
	{
		PrintTargetFileName(output, pHeader->GetFileName(), nCurCol, nMaxCol);
	}
	// iterate over header files
	pIter = GetFirstImplementationFile();
	CBEImplementationFile *pImpl;
	while ((pImpl = GetNextImplementationFile(pIter)) != 0)
	{
		PrintTargetFileName(output, pImpl->GetFileName(), nCurCol, nMaxCol);
	}
}

/** \brief prints the current filename
 *  \param output the output stream to print to
 *  \param sFilename the filename to print
 *  \param nCurCol the current output column
 *  \param nMaxCol the maximum output column
 */
void CBETarget::PrintTargetFileName(FILE *output, String sFilename, int &nCurCol, int nMaxCol)
{
	nCurCol += sFilename.GetLength();
	if (nCurCol > nMaxCol)
	{
		fprintf(output, "\\\n ");
		nCurCol = sFilename.GetLength()+1;
	}
	fprintf(output, "%s ", (const char*)sFilename);
}

/** \brief tests if this target has a function with a given type
 *  \param sTypeName the name of the user defined type
 *  \param pContext the context of the test operation
 *  \return true if the function has this type
 *
 * The methods searches for a function, which has a parameter of the given
 * user defined type. Since all functions are declared in the header files,
 * we only need to search those.
 */
bool CBETarget::HasFunctionWithUserType(String sTypeName, CBEContext *pContext)
{
    VectorElement *pIter = GetFirstHeaderFile();
    CBEHeaderFile *pFile;
    while ((pFile = GetNextHeaderFile(pIter)) != 0)
    {
        if (pFile->HasFunctionWithUserType(sTypeName, pContext))
            return true;
    }
    return false;
}
