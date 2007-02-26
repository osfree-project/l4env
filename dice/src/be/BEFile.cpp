/**
 *	\file	dice/src/be/BEFile.cpp
 *	\brief	contains the implementation of the class CBEFile
 *
 *	\date	01/10/2002
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

#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEClass.h"
#include "be/BENameSpace.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BETarget.h"
#include "be/BEFunction.h"

#include "fe/FEOperation.h"
#include "fe/FEInterface.h"
#include "fe/FELibrary.h"
#include "fe/FEFile.h"

IMPLEMENT_DYNAMIC(IncludeFile);

IncludeFile::IncludeFile()
{
    IMPLEMENT_DYNAMIC_BASE(IncludeFile, CObject);
    bIDLFile = false;
}

/**	destructor */
IncludeFile::~IncludeFile()
{
}

IMPLEMENT_DYNAMIC(CBEFile);

CBEFile::CBEFile()
: m_vIncludedFiles(RUNTIME_CLASS(IncludeFile)),
  m_vClasses(RUNTIME_CLASS(CBEClass)),
  m_vNameSpaces(RUNTIME_CLASS(CBENameSpace)),
  m_vFunctions(RUNTIME_CLASS(CBEFunction))
{
    m_nFileType = 0;
    IMPLEMENT_DYNAMIC_BASE(CBEFile, CFile);
}

CBEFile::CBEFile(CBEFile & src)
: CFile(src),
  m_vIncludedFiles(RUNTIME_CLASS(IncludeFile)),
  m_vClasses(RUNTIME_CLASS(CBEClass)),
  m_vNameSpaces(RUNTIME_CLASS(CBENameSpace)),
  m_vFunctions(RUNTIME_CLASS(CBEFunction))
{
    m_nFileType = src.m_nFileType;
    m_vClasses.Add(&(src.m_vClasses));
    m_vIncludedFiles.Add(&(src.m_vIncludedFiles));
    m_vNameSpaces.Add(&(src.m_vNameSpaces));
    m_vFunctions.Add(&(src.m_vFunctions));
    IMPLEMENT_DYNAMIC_BASE(CBEFile, CFile);
}

/**	\brief class destructor
 */
CBEFile::~CBEFile()
{
}

/**	\brief writes the header file
 *	\param pContext the context of the write operation
 *
 * This implementation does nothing. It has to be overloaded.
 */
void CBEFile::Write(CBEContext * pContext)
{
    ASSERT(false);
}

/**	\brief writes the functions of the file
 *	\param pContext the context of the write opeation
 *
 * This implementation should be overloaded by the header and implementation file to call the
 * appropriate Write Function of the function.
 */
void CBEFile::WriteFunctions(CBEContext * pContext)
{
    ASSERT(false);
}

/** \brief adds another filename to the list of included files
 *  \param sFileName the new filename
 *  \param bIDLFile true if the file is an IDL file
 */
void CBEFile::AddIncludedFileName(String sFileName, bool bIDLFile)
{
    if (sFileName.IsEmpty())
        return;
    IncludeFile *pNew = new IncludeFile();
    pNew->bIDLFile = bIDLFile;
    pNew->sFileName = sFileName;
    m_vIncludedFiles.Add(pNew);
}

/**	\brief retrieves the number of included file names
 *	\return the number of included file names (m_nIncludedFiles)
 */
int CBEFile::GetIncludedFileNameSize()
{
    return m_vIncludedFiles.GetSize();
}

/**	\brief retrieves the included file-name at the given index
 *	\param nIndex the index to search for the file-name
 *	\return a reference to the file name if the index is valid (0 otherwise)
 *
 * This function returns a reference right into its name array. DO NOT change the pointer handed back.
 */
String CBEFile::GetIncludedFileName(int nIndex)
{
    IncludeFile *pElement = (IncludeFile *) m_vIncludedFiles.GetAt(nIndex);
    if (pElement)
        return pElement->sFileName;
    return String();
}

/**	\brief returns whether the file with the given index is an IDL file
 *	\param nIndex the index to test
 *	\return true if it is an IDL file
 */
bool CBEFile::IsIDLFile(int nIndex)
{
    IncludeFile *pElement = (IncludeFile *) m_vIncludedFiles.GetAt(nIndex);
    if (pElement)
        return pElement->bIDLFile;
    return false;
}

/**	\brief creates the file name of this file
 *	\param pFEFile the front-end file to use for the file name
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * This function is not supposed to extract anything else from the front-end file than the file name and included files.
 * the other stuff - constants, types, functions, etc. - is extracted somewhere else and added using the AddXXX functions.
 */
bool CBEFile::CreateBackEnd(CFEFile * pFEFile, CBEContext * pContext)
{
    ASSERT(false);
    return true;
}

/**	\brief creates the file name of this file
 *	\param pFELibrary the front-end lib to use for the file name
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * This function is not supposed to extract anything else from the front-end lib than the file name and included files.
 * the other stuff - constants, types, functions, etc. - is extracted somewhere else and added using the AddXXX functions.
 */
bool CBEFile::CreateBackEnd(CFELibrary * pFELibrary, CBEContext * pContext)
{
    ASSERT(false);
    return true;
}

/**	\brief creates the file name of this file
 *	\param pFEInterface the front-end interface to use for the file name
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * This function is not supposed to extract anything else from the front-end interface than the file name and included files.
 * the other stuff - constants, types, functions, etc. - is extracted somewhere else and added using the AddXXX functions.
 */
bool CBEFile::CreateBackEnd(CFEInterface * pFEInterface, CBEContext * pContext)
{
    ASSERT(false);
    return true;
}

/**	\brief creates the file name of this file
 *	\param pFEOperation the front-end operation to use for the file name
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * This function is not supposed to extract anything else from the front-end operation than the file name and included files.
 * the other stuff - constants, types, functions, etc. - is extracted somewhere else and added using the AddXXX functions.
 */
bool CBEFile::CreateBackEnd(CFEOperation * pFEOperation, CBEContext * pContext)
{
    ASSERT(false);
    return true;
}

/**	\brief optimizes this file
 *	\param nLevel the optimization level
 *  \param pContext the context of the optimization
 *	\return the success or failure code
 *
 * This implementation iterates over classes and namespace and calls their
 * Optimize functions.
 */
int CBEFile::Optimize(int nLevel, CBEContext *pContext)
{
    VectorElement *pIter = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        int nRet;
        if ((nRet = pClass->Optimize(nLevel, pContext)) != 0)
            return nRet;
    }

    pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
    {
        int nRet;
        if ((nRet = pNameSpace->Optimize(nLevel, pContext)) != 0)
            return nRet;
    }

    pIter = GetFirstFunction();
    CBEFunction *pFunction;
    while ((pFunction = GetNextFunction(pIter)) != 0)
    {
        int nRet;
        if ((nRet = pFunction->Optimize(nLevel, pContext)) != 0)
            return nRet;
    }

    return 0;
}

/**	\brief tries to find the function with the given name
 *	\param sFunctionName the name to seach for
 *	\return a reference to the function or 0 if not found
 *
 * To find a function, we iterate over the classes and namespaces.
 */
CBEFunction *CBEFile::FindFunction(String sFunctionName)
{
    if (sFunctionName.IsEmpty())
        return 0;

    // classes
    VectorElement *pIter = GetFirstClass();
    CBEClass *pClass;
    CBEFunction *pFunction = 0;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        if ((pFunction = pClass->FindFunction(sFunctionName)) != 0)
            return pFunction;
    }
    // namespaces
    pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
    {
        if ((pFunction = pNameSpace->FindFunction(sFunctionName)) != 0)
            return pFunction;
    }
    // search own functions
    pIter = GetFirstFunction();
    while ((pFunction = GetNextFunction(pIter)) != 0)
    {
        if (pFunction->GetName() == sFunctionName)
            return pFunction;
    }
    // no match found -> return 0
    return 0;
}

/**	\brief tries to determine the target this file belongs to
 *	\return a reference to the target or 0 if not found
 */
CBETarget *CBEFile::GetTarget()
{
    CObject *pCur;
    for (pCur = GetParent(); pCur; pCur = pCur->GetParent())
    {
        if (pCur->IsKindOf(RUNTIME_CLASS(CBETarget)))
            return (CBETarget *) pCur;
    }
    return 0;
}

/** \brief writes includes, which have to appear before any type definition
 *  \param pContext the context of the write operation
 */
void CBEFile::WriteIncludesBeforeTypes(CBEContext *pContext)
{
}

/** \brief writes includes, which can appear after type definitions
 *  \param pContext the context of the write operation
 */
void CBEFile::WriteIncludesAfterTypes(CBEContext *pContext)
{
    int nIncludeMax = GetIncludedFileNameSize();
    String sPrefix = pContext->GetIncludePrefix();
    for (int i = 0; i < nIncludeMax; i++)
    {
        String sFileName = GetIncludedFileName(i);
        bool bIDLFile = IsIDLFile(i);
        if (!sFileName.IsEmpty())
        {
            if (!bIDLFile || sPrefix.IsEmpty())
            {
                Print("#include \"%s\"\n", (const char *) sFileName);
            }
            else // bIDLFile && !sPrefix.IsEmpty()
            {
                Print("#include \"%s/%s\"\n",(const char *) sPrefix, (const char *) sFileName);
            }
        }
    }
    // pretty print: newline
    if (nIncludeMax > 0)
        Print("\n");
}

/** \brief adds a class to this file's collection
 *  \param pClass the class to add
 */
void CBEFile::AddClass(CBEClass *pClass)
{
    m_vClasses.Add(pClass);
}

/** \brief removes a class from this file's collection
 *  \param pClass the class to remove
 */
void CBEFile::RemoveClass(CBEClass *pClass)
{
    m_vClasses.Remove(pClass);
}

/** \brief retrieves a pointer to the first class
 *  \return a pointer to the first class
 */
VectorElement* CBEFile::GetFirstClass()
{
    return m_vClasses.GetFirst();
}

/** \brief returns a reference to the next class
 *  \param pIter the pointer to the next class
 *  \return a reference to the next class
 */
CBEClass* CBEFile::GetNextClass(VectorElement *&pIter)
{
    if (!pIter)
        return 0;
    CBEClass *pRet = (CBEClass*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextClass(pIter);
    return pRet;
}

/** \brief tries to find a class using its name
 *  \param sClassName the name of the searched class
 *  \return a reference to the found class or 0 if not found
 */
CBEClass* CBEFile::FindClass(String sClassName)
{
    // first search classes
    VectorElement *pIter = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        if (pClass->GetName() == sClassName)
            return pClass;
    }
    // then search namespaces
    pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
    {
        if ((pClass = pNameSpace->FindClass(sClassName)) != 0)
            return pClass;
    }
    // not found
    return 0;
}

/** \brief adds a new namespace to the collection of this file
 *  \param pNameSpace the namespace to add
 */
void CBEFile::AddNameSpace(CBENameSpace *pNameSpace)
{
    m_vNameSpaces.Add(pNameSpace);
}

/** \brief removes a namespace
 *  \param pNameSpace the namespace to remove
 */
void CBEFile::RemoveNameSpace(CBENameSpace *pNameSpace)
{
    m_vNameSpaces.Remove(pNameSpace);
}

/** \brief retrieves a pointer to the first namespace
 *  \return a pointer to the first namespace
 */
VectorElement* CBEFile::GetFirstNameSpace()
{
    return m_vNameSpaces.GetFirst();
}

/** \brief retrieves a reference to the next namespace
 *  \param pIter the pointer to the next namespace
 *  \return a reference to the next namespace
 */
CBENameSpace* CBEFile::GetNextNameSpace(VectorElement *&pIter)
{
    if (!pIter)
        return 0;
    CBENameSpace *pRet = (CBENameSpace*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextNameSpace(pIter);
    return pRet;
}

/** \brief tries to find a namespace using a name
 *  \param sNameSpaceName the name of the searched namespace
 *  \return a reference to the found namespace or 0 if none found
 */
CBENameSpace* CBEFile::FindNameSpace(String sNameSpaceName)
{
    // search the namespace
    VectorElement *pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
    {
        if (pNameSpace->GetName() == sNameSpaceName)
            return pNameSpace;
    }
    // search nested namespaces
    CBENameSpace *pFoundNameSpace = 0;
    pIter = GetFirstNameSpace();
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
    {
        if ((pFoundNameSpace = pNameSpace->FindNameSpace(sNameSpaceName)) != 0)
            return pFoundNameSpace;
    }
    // nothing found
    return 0;
}

/** \brief writes the classes belonging to this file
 *  \param pContext the context of this write operation
 */
void CBEFile::WriteClasses(CBEContext *pContext)
{
    ASSERTC(false);
}

/** \brief writes the namespaces of this file
 *  \param pContext the context of the write operation
 */
void CBEFile::WriteNameSpaces(CBEContext *pContext)
{
    ASSERTC(false);
}

/** \brief adds a function to the file
 *  \param pFunction the function to add
 */
void CBEFile::AddFunction(CBEFunction *pFunction)
{
    m_vFunctions.Add(pFunction);
}

/** \brief removes a function from the file
 *  \param pFunction the function to remove
 */
void CBEFile::RemoveFunction(CBEFunction *pFunction)
{
    m_vFunctions.Remove(pFunction);
}

/** \brief retrieves pointer to first function
 *  \return a pointer to the first function
 */
VectorElement* CBEFile::GetFirstFunction()
{
    return m_vFunctions.GetFirst();
}

/** \brief retrieves a reference to the next function
 *  \param pIter the iterator pointing to the next function
 *  \return a reference to the next function
 */
CBEFunction* CBEFile::GetNextFunction(VectorElement *&pIter)
{
    if (!pIter)
        return 0;
    CBEFunction *pRet = (CBEFunction*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextFunction(pIter);
    return pRet;
}

/** \brief test if the target file is of a certain type
 *  \param nFileType the file type to test for
 *  \return true if the file is of this type
 *
 * A special condition is the test for FILETYPE_CLIENT or FILETYPE_COMPONENT, because
 * they have to test for both, header and implementation file.
 */
bool CBEFile::IsOfFileType(int nFileType)
{
    if (m_nFileType == nFileType)
        return true;
    if ((nFileType == FILETYPE_CLIENT) &&
        ((m_nFileType == FILETYPE_CLIENTHEADER) ||
         (m_nFileType == FILETYPE_CLIENTIMPLEMENTATION)))
        return true;
    if ((nFileType == FILETYPE_COMPONENT) &&
        ((m_nFileType == FILETYPE_COMPONENTHEADER) ||
         (m_nFileType == FILETYPE_COMPONENTIMPLEMENTATION)))
        return true;
    return false;
}

/** \brief test if this file contains a function with a given type
 *  \param sTypeName the name of the tye to search for
 *  \param pContext the context of this operation
 *  \return true if a parameter of that type is found
 */
bool CBEFile::HasFunctionWithUserType(String sTypeName, CBEContext *pContext)
{
    VectorElement *pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
    {
        if (pNameSpace->HasFunctionWithUserType(sTypeName, this, pContext))
            return true;
    }
    pIter = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        if (pClass->HasFunctionWithUserType(sTypeName, this, pContext))
           return true;
    }
    pIter = GetFirstFunction();
    CBEFunction *pFunction;
    while ((pFunction = GetNextFunction(pIter)) != 0)
    {
        if (pFunction->DoWriteFunction(this, pContext))
        {
            if (pFunction->FindParameterType(sTypeName))
                return true;
        }
    }
    return false;
}
