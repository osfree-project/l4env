/**
 *  \file    dice/src/be/BEFile.cpp
 *  \brief   contains the implementation of the class CBEFile
 *
 *  \date    01/10/2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#include "BEClass.h"
#include "BENameSpace.h"
#include "BEHeaderFile.h"
#include "BEImplementationFile.h"
#include "BETarget.h"
#include "BEFunction.h"
#include "BEContext.h"
#include "BEConstant.h"
#include "BEType.h"
#include "BETypedef.h"
#include "IncludeStatement.h"
#include "Compiler.h"
#include "fe/FEOperation.h"
#include "fe/FEInterface.h"
#include "fe/FELibrary.h"
#include "fe/FEFile.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>

//@{
/** some config variables */
extern const char* dice_version;
extern const char* dice_build;
//@}

CBEFile::CBEFile()
: m_Classes(0, (CObject*)0),
  m_NameSpaces(0, (CObject*)0),
  m_Functions(0, (CObject*)0),
  m_IncludedFiles(0, this)
{
    m_nFileType = FILETYPE_NONE;
}

CBEFile::CBEFile(CBEFile & src)
: CFile(src),
  m_Classes(0,(CObject*)0),
  m_NameSpaces(0, (CObject*)0),
  m_Functions(0, (CObject*)0),
  m_IncludedFiles(src.m_IncludedFiles)
{
    m_nFileType = src.m_nFileType;

    m_IncludedFiles.Adopt(this);
    // only copy references of the classes, namespaces and functions
    vector<CBEClass*>::iterator iC;
    for (iC = src.m_Classes.begin(); 
	 iC != src.m_Classes.end(); 
	 iC++)
	m_Classes.Add(*iC);
    vector<CBENameSpace*>::iterator iN;
    for (iN = src.m_NameSpaces.begin(); 
	 iN != src.m_NameSpaces.end(); 
	 iN++)
	m_NameSpaces.Add(*iN);
    vector<CBEFunction*>::iterator iF;
    for (iF = src.m_Functions.begin(); 
	 iF != src.m_Functions.end(); 
	 iF++)
	m_Functions.Add(*iF);
}

/** \brief class destructor
 */
CBEFile::~CBEFile()
{ }

/** \brief writes user-defined and helper functions
 */
void CBEFile::WriteHelperFunctions()
{ }

/** \brief adds another filename to the list of included files
 *  \param sFileName the new filename
 *  \param bIDLFile true if the file is an IDL file
 *  \param bIsStandardInclude true if the file was included as standard include
 *         (using <>)
 *  \param pRefObj if not 0, it is used to set source file info
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
    if (m_IncludedFiles.Find(sFileName))
	return;
    // add new include file
    CIncludeStatement *pNewInc = new CIncludeStatement(bIDLFile, 
	bIsStandardInclude, false, false, sFileName, string(),
	string(), 0);
    m_IncludedFiles.Add(pNewInc);
    // set source file info
    if (pRefObj)
    {
        pNewInc->SetSourceLine(pRefObj->GetSourceLine());
	CFEFile *pFEFile = pRefObj->GetSpecificParent<CFEFile>();
        if (pFEFile)
            pNewInc->SetSourceFileName(pFEFile->GetSourceFileName());
    }
}

/** \brief tries to find the function with the given name
 *  \param sFunctionName the name to seach for
 *  \param nFunctionType the function type to look for
 *  \return a reference to the function or 0 if not found
 *
 * To find a function, we iterate over the classes and namespaces.
 */
CBEFunction *CBEFile::FindFunction(string sFunctionName,
    FUNCTION_TYPE nFunctionType)
{
    if (sFunctionName.empty())
        return 0;

    // classes
    CBEFunction *pFunction = 0;
    vector<CBEClass*>::iterator iterC;
    for (iterC = m_Classes.begin();
	 iterC != m_Classes.end();
	 iterC++)
    {
        if ((pFunction = (*iterC)->FindFunction(sFunctionName, 
		    nFunctionType)) != 0)
            return pFunction;
    }
    // namespaces
    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_NameSpaces.begin();
	 iterN != m_NameSpaces.end();
	 iterN++)
    {
        if ((pFunction = (*iterN)->FindFunction(sFunctionName, 
		    nFunctionType)) != 0)
            return pFunction;
    }
    // search own functions
    vector<CBEFunction*>::iterator iterF;
    for (iterF = m_Functions.begin();
	 iterF != m_Functions.end();
	 iterF++)
    {
        if ((*iterF)->GetName() == sFunctionName &&
	    (*iterF)->IsFunctionType(nFunctionType))
            return *iterF;
    }
    // no match found -> return 0
    return 0;
}

/** \brief tries to determine the target this file belongs to
 *  \return a reference to the target or 0 if not found
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
 */
void CBEFile::WriteDefaultIncludes()
{ }

/** \brief writes one include statement
 *  \param pInclude the include statement to write
 */
void CBEFile::WriteInclude(CIncludeStatement *pInclude)
{
    string sPrefix = CCompiler::GetIncludePrefix();
    string sFileName = pInclude->m_sFilename;
    if (!sFileName.empty())
    {
        if (pInclude->m_bStandard)
	    m_file << "#include <";
        else
	    m_file << "#include \"";
        if (!pInclude->m_bIDLFile || sPrefix.empty())
        {
	    m_file << sFileName;
        }
        else // bIDLFile && !sPrefix.empty()()
        {
	    m_file << sPrefix << "/" << sFileName;
        }
        if (pInclude->m_bStandard)
	    m_file << ">\n";
        else
	    m_file << "\"\n";
    }
}

/** \brief tries to find a class using its name
 *  \param sClassName the name of the searched class
 *  \return a reference to the found class or 0 if not found
 */
CBEClass* CBEFile::FindClass(string sClassName)
{
    // first search classes
    CBEClass *pClass = m_Classes.Find(sClassName);
    if (pClass)
	return pClass;
    // then search namespaces
    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_NameSpaces.begin();
	 iterN != m_NameSpaces.end();
	 iterN++)
    {
        if ((pClass = (*iterN)->FindClass(sClassName)) != 0)
            return pClass;
    }
    // not found
    return 0;
}

/** \brief tries to find a namespace using a name
 *  \param sNameSpaceName the name of the searched namespace
 *  \return a reference to the found namespace or 0 if none found
 */
CBENameSpace* CBEFile::FindNameSpace(string sNameSpaceName)
{
    // search the namespace
    CBENameSpace *pNameSpace = m_NameSpaces.Find(sNameSpaceName);
    if (pNameSpace)
	return pNameSpace;
    // search nested namespaces
    CBENameSpace *pFoundNameSpace = 0;
    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_NameSpaces.begin();
	 iterN != m_NameSpaces.end();
	 iterN++)
    {
        if ((pFoundNameSpace = (*iterN)->FindNameSpace(sNameSpaceName)) != 0)
            return pFoundNameSpace;
    }
    // nothing found
    return 0;
}

/** \brief returns the number of functions in the function vector
 *  \return the number of functions in the function vector
 */
int CBEFile::GetFunctionCount()
{
    return m_Functions.size();
}

/** \brief test if the target file is of a certain type
 *  \param nFileType the file type to test for
 *  \return true if the file is of this type
 *
 * A special condition is the test for FILETYPE_CLIENT or FILETYPE_COMPONENT,
 * because they have to test for both, header and implementation file.
 */
bool CBEFile::IsOfFileType(FILE_TYPE nFileType)
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
    if ((nFileType == FILETYPE_HEADER) &&
	((m_nFileType == FILETYPE_CLIENTHEADER) ||
	 (m_nFileType == FILETYPE_COMPONENTHEADER) ||
	 (m_nFileType == FILETYPE_OPCODE)))
	return true;
    if ((nFileType == FILETYPE_IMPLEMENTATION) &&
	((m_nFileType == FILETYPE_CLIENTIMPLEMENTATION) ||
	 (m_nFileType == FILETYPE_COMPONENTIMPLEMENTATION)))
	return true;
    return false;
}

/** \brief test if this file contains a function with a given type
 *  \param sTypeName the name of the tye to search for
 *  \return true if a parameter of that type is found
 */
bool CBEFile::HasFunctionWithUserType(string sTypeName)
{
    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_NameSpaces.begin();
	 iterN != m_NameSpaces.end();
	 iterN++)
    {
        if ((*iterN)->HasFunctionWithUserType(sTypeName, this))
            return true;
    }
    vector<CBEClass*>::iterator iterC;
    for (iterC = m_Classes.begin();
	 iterC != m_Classes.end();
	 iterC++)
    {
        if ((*iterC)->HasFunctionWithUserType(sTypeName, this))
           return true;
    }
    vector<CBEFunction*>::iterator iterF;
    for (iterF = m_Functions.begin();
	 iterF != m_Functions.end();
	 iterF++)
    {
        if (dynamic_cast<CBEHeaderFile*>(this) &&
            (*iterF)->DoWriteFunction((CBEHeaderFile*)this) &&
            (*iterF)->FindParameterType(sTypeName))
            return true;
        if (dynamic_cast<CBEImplementationFile*>(this) &&
            (*iterF)->DoWriteFunction((CBEImplementationFile*)this) &&
            (*iterF)->FindParameterType(sTypeName))
            return true;
    }
    return false;
}

/** \brief writes an introductionary notice
 *
 * This method should always be called first when writing into
 * a file.
 */
void CBEFile::WriteIntro()
{
    m_file <<
	"/*\n" <<
	" * THIS FILE IS MACHINE GENERATED!";
    if (m_nFileType == FILETYPE_TEMPLATE)
    {
	m_file << "\n" <<
	    " *\n" <<
	    " * Implement the server templates here.\n" <<
	    " * This file is regenerated with every run of 'dice -t ...'.\n" <<
	    " * Move it to another location after changing to\n" <<
	    " * keep your changes!\n";
    }
    else
	m_file << " DO NOT EDIT!\n";
    m_file << " *\n";
    m_file << " * " << m_sFilename << "\n";
    m_file << " * created ";

    char * user = getlogin();
    if (user)
    {
	m_file << "by " << user;
	
	char host[255];
	if (!gethostname(host, sizeof(host)))
	    m_file << "@" << host;
	
	m_file << " ";
    }

    time_t t = time(NULL);
    m_file << "on " << ctime(&t);
    m_file << " * with Dice version " << dice_version << " (compiled on " << 
	dice_build << ")\n";
    m_file << " * send bug reports to <dice@os.inf.tu-dresden.de>\n";
    m_file << " */\n\n";
}

/** \brief creates a list of ordered elements
 *
 * This method iterates each member vector and inserts their
 * elements into the ordered element list using bubble sort.
 * Sort criteria is the source line number.
 */
void CBEFile::CreateOrderedElementList(void)
{
    // clear vector
    m_vOrderedElements.clear();
    // Includes
    vector<CIncludeStatement*>::iterator iterI;
    for (iterI = m_IncludedFiles.begin();
	 iterI != m_IncludedFiles.end();
	 iterI++)
    {
        InsertOrderedElement(*iterI);
    }
    // namespaces
    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_NameSpaces.begin();
	 iterN != m_NameSpaces.end();
	 iterN++)
    {
        InsertOrderedElement(*iterN);
    }
    // classes
    vector<CBEClass*>::iterator iterC;
    for (iterC = m_Classes.begin();
	 iterC != m_Classes.end();
	 iterC++)
    {
        InsertOrderedElement(*iterC);
    }
    // functions
    vector<CBEFunction*>::iterator iterF;
    for (iterF = m_Functions.begin();
	 iterF != m_Functions.end();
	 iterF++)
    {
        InsertOrderedElement(*iterF);
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
            // insert before that element
            m_vOrderedElements.insert(iter, pObj);
            return;
        }
    }
    // new object is bigger that all existing
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
    vector<CIncludeStatement*>::iterator iterI;
    for (iterI = m_IncludedFiles.begin();
	 iterI != m_IncludedFiles.end();
	 iterI++)
    {
       int sLine = (*iterI)->GetSourceLine();
       int eLine = (*iterI)->GetSourceLineEnd();
       m_nSourceLineNbEnd = std::max(sLine, std::max(eLine, m_nSourceLineNbEnd));
    }
    // namespaces
    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_NameSpaces.begin();
	 iterN != m_NameSpaces.end();
	 iterN++)
    {
       int sLine = (*iterN)->GetSourceLine();
       int eLine = (*iterN)->GetSourceLineEnd();
       m_nSourceLineNbEnd = std::max(sLine, std::max(eLine, m_nSourceLineNbEnd));
    }
    // classes
    vector<CBEClass*>::iterator iterC;
    for (iterC = m_Classes.begin();
	 iterC != m_Classes.end();
	 iterC++)
    {
       int sLine = (*iterC)->GetSourceLine();
       int eLine = (*iterC)->GetSourceLineEnd();
       m_nSourceLineNbEnd = std::max(sLine, std::max(eLine, m_nSourceLineNbEnd));
    }
    // functions
    vector<CBEFunction*>::iterator iterF;
    for (iterF = m_Functions.begin();
	 iterF != m_Functions.end();
	 iterF++)
    {
       int sLine = (*iterF)->GetSourceLine();
       int eLine = (*iterF)->GetSourceLineEnd();
       m_nSourceLineNbEnd = std::max(sLine, std::max(eLine, m_nSourceLineNbEnd));
    }

    return m_nSourceLineNbEnd;
}
