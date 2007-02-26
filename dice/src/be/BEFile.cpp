/**
 *    \file    dice/src/be/BEFile.cpp
 *    \brief   contains the implementation of the class CBEFile
 *
 *    \date    01/10/2002
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

#include "BEFile.h"
#include "BEContext.h"
#include "BEClass.h"
#include "BENameSpace.h"
#include "BEHeaderFile.h"
#include "BEImplementationFile.h"
#include "BETarget.h"
#include "BEFunction.h"
#include "IncludeStatement.h"

#include "fe/FEOperation.h"
#include "fe/FEInterface.h"
#include "fe/FELibrary.h"
#include "fe/FEFile.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//@{
/** some config variables */
extern const char* dice_version;
//@}

CBEFile::CBEFile()
{
    m_nFileType = 0;
}

CBEFile::CBEFile(CBEFile & src)
: CFile(src)
{
    m_nFileType = src.m_nFileType;

    COPY_VECTOR(CBEClass, m_vClasses, iterC);
    COPY_VECTOR(CIncludeStatement, m_vIncludedFiles, iterI);
    COPY_VECTOR(CBENameSpace, m_vNameSpaces, iterN);
    COPY_VECTOR(CBEFunction, m_vFunctions, iterF);
}

/**    \brief class destructor
 */
CBEFile::~CBEFile()
{
    DEL_VECTOR(m_vIncludedFiles);
    DEL_VECTOR(m_vClasses);
    DEL_VECTOR(m_vNameSpaces);
    DEL_VECTOR(m_vFunctions);
}

/**    \brief writes the header file
 *    \param pContext the context of the write operation
 *
 * This implementation does nothing. It has to be overloaded.
 */
void CBEFile::Write(CBEContext * pContext)
{
    assert(false);
}

/** \brief writes user-defined and helper functions
 *  \param pContext the context of the write operation
 */
void CBEFile::WriteHelperFunctions(CBEContext *pContext)
{
}

/** \brief adds another filename to the list of included files
 *  \param sFileName the new filename
 *  \param bIDLFile true if the file is an IDL file
 *  \param bIsStandardInclude true if the file was included as standard include
 *         (using <>)
 *  \param pRefObj if not NULL, it is used to set source file info
 */
void
CBEFile::AddIncludedFileName(string sFileName,
    bool bIDLFile,
    bool bIsStandardInclude,
    CObject* pRefObj)
{
    if (sFileName.empty())
        return;
    // first check if we have this name already registered
    vector<CIncludeStatement*>::iterator iter;
    for (iter = m_vIncludedFiles.begin(); iter != m_vIncludedFiles.end(); iter++)
    {
        if ((*iter)->GetIncludedFileName() == sFileName)
            return;
    }
    // add new include file
    CIncludeStatement *pNewInc = new CIncludeStatement(bIDLFile, bIsStandardInclude, false, sFileName);
    m_vIncludedFiles.push_back(pNewInc);
    // set source file info
    if (pRefObj)
    {
        pNewInc->SetSourceLine(pRefObj->GetSourceLine());
        if (pRefObj->GetSpecificParent<CFEFile>())
            pNewInc->SetSourceFileName(pRefObj->GetSpecificParent<CFEFile>()->GetSourceFileName());
    }
}

/** \brief returns the first incldue statement
 *  \return an iterator pointng to the first include statement
 */
vector<CIncludeStatement*>::iterator
CBEFile::GetFirstIncludeStatement()
{
    return m_vIncludedFiles.begin();
}

/** \brief return a reference to the next include statement
 *  \param iter the iterator pointing to the next include statement
 *  \return a reference to the next include statement
 */
CIncludeStatement*
CBEFile::GetNextIncludeStatement(vector<CIncludeStatement*>::iterator &iter)
{
    if (iter == m_vIncludedFiles.end())
        return 0;
    return *iter++;
}

/**    \brief creates the file name of this file
 *    \param pFEFile the front-end file to use for the file name
 *    \param pContext the context of the code generation
 *    \return true if successful
 *
 * This function is not supposed to extract anything else from the front-end
 * file than the file name and included files.  the other stuff - constants,
 * types, functions, etc. - is extracted somewhere else and added using the
 * AddXXX functions.
 */
bool CBEFile::CreateBackEnd(CFEFile * pFEFile, CBEContext * pContext)
{
    assert(false);
    return true;
}

/**    \brief creates the file name of this file
 *    \param pFELibrary the front-end lib to use for the file name
 *    \param pContext the context of the code generation
 *    \return true if successful
 *
 * This function is not supposed to extract anything else from the front-end
 * lib than the file name and included files.  the other stuff - constants,
 * types, functions, etc. - is extracted somewhere else and added using the
 * AddXXX functions.
 */
bool CBEFile::CreateBackEnd(CFELibrary * pFELibrary, CBEContext * pContext)
{
    assert(false);
    return true;
}

/**    \brief creates the file name of this file
 *    \param pFEInterface the front-end interface to use for the file name
 *    \param pContext the context of the code generation
 *    \return true if successful
 *
 * This function is not supposed to extract anything else from the front-end
 * interface than the file name and included files.  the other stuff -
 * constants, types, functions, etc. - is extracted somewhere else and added
 * using the AddXXX functions.
 */
bool CBEFile::CreateBackEnd(CFEInterface * pFEInterface, CBEContext * pContext)
{
    assert(false);
    return true;
}

/**    \brief creates the file name of this file
 *    \param pFEOperation the front-end operation to use for the file name
 *    \param pContext the context of the code generation
 *    \return true if successful
 *
 * This function is not supposed to extract anything else from the front-end
 * operation than the file name and included files.  the other stuff -
 * constants, types, functions, etc. - is extracted somewhere else and added
 * using the AddXXX functions.
 */
bool CBEFile::CreateBackEnd(CFEOperation * pFEOperation, CBEContext * pContext)
{
    assert(false);
    return true;
}

/**    \brief tries to find the function with the given name
 *    \param sFunctionName the name to seach for
 *    \return a reference to the function or 0 if not found
 *
 * To find a function, we iterate over the classes and namespaces.
 */
CBEFunction *CBEFile::FindFunction(string sFunctionName)
{
    if (sFunctionName.empty())
        return 0;

    // classes
    vector<CBEClass*>::iterator iterC = GetFirstClass();
    CBEClass *pClass;
    CBEFunction *pFunction = 0;
    while ((pClass = GetNextClass(iterC)) != 0)
    {
        if ((pFunction = pClass->FindFunction(sFunctionName)) != 0)
            return pFunction;
    }
    // namespaces
    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(iterN)) != 0)
    {
        if ((pFunction = pNameSpace->FindFunction(sFunctionName)) != 0)
            return pFunction;
    }
    // search own functions
    vector<CBEFunction*>::iterator iterF = GetFirstFunction();
    while ((pFunction = GetNextFunction(iterF)) != 0)
    {
        if (pFunction->GetName() == sFunctionName)
            return pFunction;
    }
    // no match found -> return 0
    return 0;
}

/**    \brief tries to determine the target this file belongs to
 *    \return a reference to the target or 0 if not found
 */
CBETarget *CBEFile::GetTarget()
{
    CObject *pCur;
    for (pCur = GetParent(); pCur; pCur = pCur->GetParent())
    {
        if (dynamic_cast<CBETarget*>(pCur))
            return (CBETarget *) pCur;
    }
    return 0;
}

/** \brief writes includes, which always have to be there
 *  \param pContext the context of the write operation
 */
void CBEFile::WriteDefaultIncludes(CBEContext *pContext)
{
}

/** \brief writes one include statement
 *  \param pInclude the include statement to write
 *  \param pCOntext the context of the write operation
 */
void CBEFile::WriteInclude(CIncludeStatement *pInclude, CBEContext *pContext)
{
    string sPrefix = pContext->GetIncludePrefix();
    string sFileName = pInclude->GetIncludedFileName();
    if (!sFileName.empty())
    {
        if (pInclude->IsStdInclude())
            Print("#include <");
        else
            Print("#include \"");
        if (!pInclude->IsIDLFile() || sPrefix.empty())
        {
            Print("%s", sFileName.c_str());
        }
        else // bIDLFile && !sPrefix.empty()()
        {
            Print("%s/%s", sPrefix.c_str(), sFileName.c_str());
        }
        if (pInclude->IsStdInclude())
            Print(">\n");
        else
            Print("\"\n");
    }
}

/** \brief adds a class to this file's collection
 *  \param pClass the class to add
 */
void CBEFile::AddClass(CBEClass *pClass)
{
    if (!pClass)
        return;
    m_vClasses.push_back(pClass);
}

/** \brief removes a class from this file's collection
 *  \param pClass the class to remove
 */
void CBEFile::RemoveClass(CBEClass *pClass)
{
    vector<CBEClass*>::iterator iter;
    for (iter = m_vClasses.begin(); iter != m_vClasses.end(); iter++)
    {
        if (*iter == pClass)
        {
            m_vClasses.erase(iter);
            return;
        }
    }
}

/** \brief retrieves a pointer to the first class
 *  \return a pointer to the first class
 */
vector<CBEClass*>::iterator CBEFile::GetFirstClass()
{
    return m_vClasses.begin();
}

/** \brief returns a reference to the next class
 *  \param iter the pointer to the next class
 *  \return a reference to the next class
 */
CBEClass* CBEFile::GetNextClass(vector<CBEClass*>::iterator &iter)
{
    if (iter == m_vClasses.end())
        return 0;
    return *iter++;
}

/** \brief tries to find a class using its name
 *  \param sClassName the name of the searched class
 *  \return a reference to the found class or 0 if not found
 */
CBEClass* CBEFile::FindClass(string sClassName)
{
    // first search classes
    vector<CBEClass*>::iterator iterC = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(iterC)) != 0)
    {
        if (pClass->GetName() == sClassName)
            return pClass;
    }
    // then search namespaces
    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(iterN)) != 0)
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
    if (!pNameSpace)
        return;
    m_vNameSpaces.push_back(pNameSpace);
}

/** \brief removes a namespace
 *  \param pNameSpace the namespace to remove
 */
void CBEFile::RemoveNameSpace(CBENameSpace *pNameSpace)
{
    vector<CBENameSpace*>::iterator iter;
    for (iter = m_vNameSpaces.begin(); iter != m_vNameSpaces.end(); iter++)
    {
        if (*iter == pNameSpace)
        {
            m_vNameSpaces.erase(iter);
            return;
        }
    }
}

/** \brief retrieves a pointer to the first namespace
 *  \return a pointer to the first namespace
 */
vector<CBENameSpace*>::iterator CBEFile::GetFirstNameSpace()
{
    return m_vNameSpaces.begin();
}

/** \brief retrieves a reference to the next namespace
 *  \param iter the pointer to the next namespace
 *  \return a reference to the next namespace
 */
CBENameSpace* CBEFile::GetNextNameSpace(vector<CBENameSpace*>::iterator &iter)
{
    if (iter == m_vNameSpaces.end())
        return 0;
    return *iter++;
}

/** \brief tries to find a namespace using a name
 *  \param sNameSpaceName the name of the searched namespace
 *  \return a reference to the found namespace or 0 if none found
 */
CBENameSpace* CBEFile::FindNameSpace(string sNameSpaceName)
{
    // search the namespace
    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(iterN)) != 0)
    {
        if (pNameSpace->GetName() == sNameSpaceName)
            return pNameSpace;
    }
    // search nested namespaces
    CBENameSpace *pFoundNameSpace = 0;
    iterN = GetFirstNameSpace();
    while ((pNameSpace = GetNextNameSpace(iterN)) != 0)
    {
        if ((pFoundNameSpace = pNameSpace->FindNameSpace(sNameSpaceName)) != 0)
            return pFoundNameSpace;
    }
    // nothing found
    return 0;
}

/** \brief adds a function to the file
 *  \param pFunction the function to add
 */
void CBEFile::AddFunction(CBEFunction *pFunction)
{
    if (!pFunction)
        return;
    m_vFunctions.push_back(pFunction);
}

/** \brief removes a function from the file
 *  \param pFunction the function to remove
 */
void CBEFile::RemoveFunction(CBEFunction *pFunction)
{
    vector<CBEFunction*>::iterator iter;
    for (iter = m_vFunctions.begin(); iter != m_vFunctions.end(); iter++)
    {
        if (*iter == pFunction)
        {
            m_vFunctions.erase(iter);
            return;
        }
    }
}

/** \brief retrieves pointer to first function
 *  \return a pointer to the first function
 */
vector<CBEFunction*>::iterator CBEFile::GetFirstFunction()
{
    return m_vFunctions.begin();
}

/** \brief retrieves a reference to the next function
 *  \param iter the iterator pointing to the next function
 *  \return a reference to the next function
 */
CBEFunction* CBEFile::GetNextFunction(vector<CBEFunction*>::iterator &iter)
{
    if (iter == m_vFunctions.end())
        return 0;
    return *iter++;
}

/**    \brief returns the number of functions in the function vector
 *    \return the number of functions in the function vector
 */
int CBEFile::GetFunctionCount()
{
    return m_vFunctions.size();
}

/** \brief test if the target file is of a certain type
 *  \param nFileType the file type to test for
 *  \return true if the file is of this type
 *
 * A special condition is the test for FILETYPE_CLIENT or FILETYPE_COMPONENT,
 * because they have to test for both, header and implementation file.
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
bool CBEFile::HasFunctionWithUserType(string sTypeName, CBEContext *pContext)
{
    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(iterN)) != 0)
    {
        if (pNameSpace->HasFunctionWithUserType(sTypeName, this, pContext))
            return true;
    }
    vector<CBEClass*>::iterator iterC = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(iterC)) != 0)
    {
        if (pClass->HasFunctionWithUserType(sTypeName, this, pContext))
           return true;
    }
    vector<CBEFunction*>::iterator iterF = GetFirstFunction();
    CBEFunction *pFunction;
    while ((pFunction = GetNextFunction(iterF)) != 0)
    {
        if (dynamic_cast<CBEHeaderFile*>(this) &&
            pFunction->DoWriteFunction((CBEHeaderFile*)this, pContext) &&
            pFunction->FindParameterType(sTypeName))
            return true;
        if (dynamic_cast<CBEImplementationFile*>(this) &&
            pFunction->DoWriteFunction((CBEImplementationFile*)this, pContext) &&
            pFunction->FindParameterType(sTypeName))
            return true;
    }
    return false;
}

/** \brief writes an introductionary notice
 *  \param pContext the context of the write
 *
 * This method should always be called first when writing into a file.
 */
void CBEFile::WriteIntro(CBEContext *pContext)
{
    Print("/*\n");
    Print(" * This file is auto generated by Dice-%s", dice_version);
    if (m_nFileType == FILETYPE_TEMPLATE)
    {
        Print(".\n");
        Print(" *\n");
        Print(" * Implement the server templates here.\n");
        Print(" * This file is regenerated with every run of 'dice -t ...'.\n");
        Print(" * Move it to another location after changing to\n");
        Print(" * keep your changes!\n");
    }
    else
        Print(",  DO NOT EDIT!\n");
    Print(" */\n\n");
}

/** \brief creates a list of ordered elements
 *
 * This method iterates each member vector and inserts their elements into the
 * ordered element list using bubble sort.  Sort criteria is the source line
 * number.
 */
void CBEFile::CreateOrderedElementList(void)
{
    // clear vector
    m_vOrderedElements.erase(m_vOrderedElements.begin(), 
	m_vOrderedElements.end());
    // Includes
    vector<CIncludeStatement*>::iterator iterI = m_vIncludedFiles.begin();
    for (; iterI != m_vIncludedFiles.end(); iterI++)
    {
        InsertOrderedElement(*iterI);
    }
    // namespaces
    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNS;
    while ((pNS = GetNextNameSpace(iterN)) != NULL)
    {
        InsertOrderedElement(pNS);
    }
    // classes
    vector<CBEClass*>::iterator iterC = GetFirstClass();
    CBEClass *pC;
    while ((pC = GetNextClass(iterC)) != NULL)
    {
        InsertOrderedElement(pC);
    }
    // functions
    vector<CBEFunction*>::iterator iterF = GetFirstFunction();
    CBEFunction *pF;
    while ((pF = GetNextFunction(iterF)) != NULL)
    {
        InsertOrderedElement(pF);
    }
}

/** \brief insert one element into the ordered list
 *  \param pObj the new element
 *
 * This is the insert implementation
 */
void CBEFile::InsertOrderedElement(CObject *pObj)
{
    // get source line number
    int nLine = pObj->GetSourceLine();
    // search for element with larger number
    vector<CObject*>::iterator iter = m_vOrderedElements.begin();
    for (; iter != m_vOrderedElements.end(); iter++)
    {
        if ((*iter)->GetSourceLine() > nLine)
        {
//             TRACE("Insert element from %s:%d before element from %s:%d\n",
//                 pObj->GetSourceFileName().c_str(), pObj->GetSourceLine(),
//                 (*iter)->GetSourceFileName().c_str(),
//                 (*iter)->GetSourceLine());
            // insert before that element
            m_vOrderedElements.insert(iter, pObj);
            return;
        }
    }
    // new object is bigger that all existing
//     TRACE("Insert element from %s:%d at end\n",
//         pObj->GetSourceFileName().c_str(), pObj->GetSourceLine());
    m_vOrderedElements.push_back(pObj);
}

/** \brief retrieves the maximum line number in the file
 *  \return the maximum line number in this file
 *
 * If line number is not that, i.e., is zero, then we iterate the elements and
 * check their end line number. The maximum is out desired maximum line
 * number.
 */
int
CBEFile::GetSourceLineEnd()
{
    if (m_nSourceLineNbEnd != 0)
	return m_nSourceLineNbEnd;

    // Includes
    vector<CIncludeStatement*>::iterator iterI = m_vIncludedFiles.begin();
    for (; iterI != m_vIncludedFiles.end(); iterI++)
    {
	int sLine = (*iterI)->GetSourceLine();
	int eLine = (*iterI)->GetSourceLineEnd();
	m_nSourceLineNbEnd = MAX(sLine, MAX(eLine, m_nSourceLineNbEnd));
    }
    // namespaces
    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNS;
    while ((pNS = GetNextNameSpace(iterN)) != NULL)
    {
	int sLine = pNS->GetSourceLine();
	int eLine = pNS->GetSourceLineEnd();
	m_nSourceLineNbEnd = MAX(sLine, MAX(eLine, m_nSourceLineNbEnd));
    }
    // classes
    vector<CBEClass*>::iterator iterC = GetFirstClass();
    CBEClass *pC;
    while ((pC = GetNextClass(iterC)) != NULL)
    {
	int sLine = pC->GetSourceLine();
	int eLine = pC->GetSourceLineEnd();
	m_nSourceLineNbEnd = MAX(sLine, MAX(eLine, m_nSourceLineNbEnd));
    }
    // functions
    vector<CBEFunction*>::iterator iterF = GetFirstFunction();
    CBEFunction *pF;
    while ((pF = GetNextFunction(iterF)) != NULL)
    {
	int sLine = pF->GetSourceLine();
	int eLine = pF->GetSourceLineEnd();
	m_nSourceLineNbEnd = MAX(sLine, MAX(eLine, m_nSourceLineNbEnd));
    }

    return m_nSourceLineNbEnd;
}
