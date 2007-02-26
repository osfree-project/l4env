/**
 *    \file    dice/src/fe/FEFile.cpp
 *    \brief   contains the implementation of the class CFEFile
 *
 *    \date    01/31/2001
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

#include "IncludeStatement.h"
#include "File.h"
#include "CPreProcess.h" // needed to get include statements
#include "fe/FEFile.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEConstDeclarator.h"
#include "fe/FEConstructedType.h"
#include "fe/FETaggedStructType.h"
#include "fe/FETaggedUnionType.h"
#include "fe/FETaggedEnumType.h"
#include "fe/FEInterface.h"
#include "fe/FELibrary.h"

#include <ctype.h>
#include <algorithm>
using namespace std;

CFEFile::CFEFile(string sFileName, string sPath, int nIncludedOnLine, int nStdInclude)
{
    // this procedure is interesting for included files:
    // if the statement was: #include "l4/sys/types.h" and the path where the file
    // was found is "../../include" the following string are created:
    //
    // m_sFileName = "l4/sys/types.h"
    // m_sFilenameWithoutExtension = "types"
    // m_sFileExtension = "h"
    // m_sFileWithPath = "../../include/l4/sys/types.h"
    assert(!sFileName.empty());
    m_sFileName = sFileName;
    if (!m_sFileName.empty())
    {
        // first strip of extension  (the string after the last '.')
        int iDot = m_sFileName.rfind('.');
        if (iDot < 0)
        {
            m_sFilenameWithoutExtension = m_sFileName;
            m_sFileExtension.erase(m_sFileExtension.begin(), m_sFileExtension.end());
        }
        else
        {
            m_sFilenameWithoutExtension = m_sFileName.substr(0, iDot);
            m_sFileExtension = m_sFileName.substr(iDot + 1);
        }
        // now strip of the path
        int iSlash = m_sFilenameWithoutExtension.rfind('/');
        if (iSlash >= 0)
        {
            m_sFilenameWithoutExtension =
                m_sFilenameWithoutExtension.substr(iSlash + 1);
        }
        // now generate full filename with path
        if (!sPath.empty())
        {
            bool bLastSlash = sPath[sPath.length()-1] == '/';
            m_sFileWithPath = sPath;
            if (!bLastSlash)
                m_sFileWithPath += "/";
            m_sFileWithPath += m_sFileName;
        }
        else
            m_sFileWithPath = m_sFileName;
    }
    else
    {
        m_sFilenameWithoutExtension.erase(m_sFilenameWithoutExtension.begin(), 
	    m_sFilenameWithoutExtension.end());
        m_sFileExtension.erase(m_sFileExtension.begin(), 
	    m_sFileExtension.end());
        m_sFileWithPath.erase(m_sFileWithPath.begin(), m_sFileWithPath.end());
    }
    m_nStdInclude = nStdInclude;
    m_nIncludedOnLine = nIncludedOnLine;

    // make file extension lower case
    transform(m_sFileExtension.begin(), m_sFileExtension.end(),
        m_sFileExtension.begin(), tolower);

    // get includes from preprocessor
    CPreProcess *pPreprocess = CPreProcess::GetPreProcessor();
    inc_bookmark_t *pCur = pPreprocess->GetFirstIncludeInFile(m_sFileName);
    while (pCur)
    {
        string sFileName = *(pCur->m_pFilename);
        // test if idl file
        bool bIDL = false;
        int iDot = sFileName.rfind('.');
        if (iDot > 0)
        {
            string s = sFileName.substr(sFileName.length() - (iDot + 1));
            transform(s.begin(), s.end(), s.begin(), tolower);
            if (s == "idl")
                bIDL = true;
        }
        // extract include information and add it to m_vIncludes
        CIncludeStatement *pNew = new CIncludeStatement(bIDL,
            pCur->m_bStandard, false, sFileName);
        AddInclude(pNew);
        // set source file info
        pNew->SetSourceLine(pCur->m_nLineNb);
        pNew->SetSourceFileName(m_sFileName);
        // now get next
        pCur = pPreprocess->GetNextIncludeInFile(m_sFileName, pCur);
    }
}

CFEFile::CFEFile(CFEFile & src)
: CFEBase(src)
{
    m_sFileName = src.m_sFileName;
    m_sFileExtension = src.m_sFileExtension;
    m_sFilenameWithoutExtension = src.m_sFilenameWithoutExtension;
    m_sFileWithPath = src.m_sFileWithPath;
    m_nStdInclude = src.m_nStdInclude;
    m_nIncludedOnLine = src.m_nIncludedOnLine;

    COPY_VECTOR(CFEConstructedType, m_vTaggedDecls, iterTD);
    COPY_VECTOR(CFEConstDeclarator, m_vConstants, iterC);
    COPY_VECTOR(CFETypedDeclarator, m_vTypedefs, iterT);
    COPY_VECTOR(CFELibrary, m_vLibraries, iterL);
    COPY_VECTOR(CFEInterface, m_vInterfaces, iterI);
    COPY_VECTOR(CFEFile, m_vChildFiles, iterF);
    COPY_VECTOR(CIncludeStatement, m_vIncludes, iterIF);
}

/** cleans up the file object */
CFEFile::~CFEFile()
{
    DEL_VECTOR(m_vTaggedDecls);
    DEL_VECTOR(m_vConstants);
    DEL_VECTOR(m_vTypedefs);
    DEL_VECTOR(m_vLibraries);
    DEL_VECTOR(m_vInterfaces);
    DEL_VECTOR(m_vChildFiles);
    DEL_VECTOR(m_vIncludes);
}

/** copies the object
 *    \return a reference to the new file object
 */
CObject *CFEFile::Clone()
{
    return new CFEFile(*this);
}

/** \brief adds another (included) file to this file hierarchy
 *  \param pNewChild the (included) file to add
 */
void CFEFile::AddChild(CFEFile * pNewChild)
{
    if (!pNewChild)
        return;
    m_vChildFiles.push_back(pNewChild);
    pNewChild->SetParent(this);
}

/** returns a pointer to the first included file
 *    \return a pointer to the first included file
 */
vector<CFEFile*>::iterator CFEFile::GetFirstChildFile()
{
    return m_vChildFiles.begin();
}

/** returns a refrence to the next included file
 *    \param iter the pointer to the next included file
 *    \return a refrence to the next included file
 */
CFEFile *CFEFile::GetNextChildFile(vector<CFEFile*>::iterator &iter)
{
    if (iter == m_vChildFiles.end())
        return 0;
    return *iter++;
}

/**    adds an interface to this file
 *    \param pInterface the new interface to add
 */
void CFEFile::AddInterface(CFEInterface * pInterface)
{
    if (!pInterface)
        return;
    m_vInterfaces.push_back(pInterface);
    pInterface->SetParent(this);
}

/** returns a pointer to the first interface
 *    \return a pointer to the first interface
 */
vector<CFEInterface*>::iterator CFEFile::GetFirstInterface()
{
    return m_vInterfaces.begin();
}

/** returns a reference to the next interface
 *    \param iter a pointer to the next interface
 *    \return a reference to the next interface
 */
CFEInterface *CFEFile::GetNextInterface(vector<CFEInterface*>::iterator &iter)
{
    if (iter == m_vInterfaces.end())
        return 0;
    return *iter++;
}

/** adds an library to this file
 *    \param pLibrary the new library to add
 */
void CFEFile::AddLibrary(CFELibrary * pLibrary)
{
    if (!pLibrary)
        return;
    m_vLibraries.push_back(pLibrary);
    pLibrary->SetParent(this);
}

/**    returns a pointer to the first library
 *    \return a pointer to the first library
 */
vector<CFELibrary*>::iterator CFEFile::GetFirstLibrary()
{
    return m_vLibraries.begin();
}

/**    returns a reference to the next library in this file
 *    \param iter a pointer to the next library in this file
 *    \return a reference to the next library in this file
 */
CFELibrary *CFEFile::GetNextLibrary(vector<CFELibrary*>::iterator &iter)
{
    if (iter == m_vLibraries.end())
        return 0;
    return *iter++;
}

/**    adds a type definition to this file
 *    \param pTypedef the new type definition
 */
void CFEFile::AddTypedef(CFETypedDeclarator * pTypedef)
{
    if (!pTypedef)
        return;
    m_vTypedefs.push_back(pTypedef);
    pTypedef->SetParent(this);
}

/** returns a pointer to the first type definition
 *    \return a pointer to the first type definition
 */
vector<CFETypedDeclarator*>::iterator CFEFile::GetFirstTypedef()
{
    return m_vTypedefs.begin();
}

/** \brief returns a reference to the next type defintion
 *  \param iter a pointer to the next type defintion
 *  \return a reference to the next type defintion
 */
CFETypedDeclarator *CFEFile::GetNextTypedef(vector<CFETypedDeclarator*>::iterator &iter)
{
    if (iter == m_vTypedefs.end())
        return 0;
    return *iter++;
}

/** adds the declaration of a constant to this file
 *    \param pConstant the new constant to add
 */
void CFEFile::AddConstant(CFEConstDeclarator * pConstant)
{
    if (!pConstant)
        return;
    m_vConstants.push_back(pConstant);
    pConstant->SetParent(this);
}

/** returns a pointer to the first constant of this file
 *    \return a pointer to the first constant of this file
 */
vector<CFEConstDeclarator*>::iterator CFEFile::GetFirstConstant()
{
    return m_vConstants.begin();
}

/** returns a reference to the next constant definition in this file
 *    \param iter the pointer to the next constant definition
 *    \return a reference to the next constant definition in this file
 */
CFEConstDeclarator *CFEFile::GetNextConstant(vector<CFEConstDeclarator*>::iterator &iter)
{
    if (iter == m_vConstants.end())
        return 0;
    return *iter++;
}

/** \brief adds the definition of a tagged struct or union to this file
 *    \param pTaggedDecl the new declaration of this file
 */
void CFEFile::AddTaggedDecl(CFEConstructedType * pTaggedDecl)
{
    if (!pTaggedDecl)
        return;
    m_vTaggedDecls.push_back(pTaggedDecl);
    pTaggedDecl->SetParent(this);
}

/** returns a pointer to the first tagged declarator
 *    \return a pointer to the first tagged declarator
 */
vector<CFEConstructedType*>::iterator CFEFile::GetFirstTaggedDecl()
{
    return m_vTaggedDecls.begin();
}

/** returns a reference to the next tagged declarator
 *    \param iter the pointer to the next tagged declarator
 *    \return a reference to the next tagged declarator
 */
CFEConstructedType *CFEFile::GetNextTaggedDecl(vector<CFEConstructedType*>::iterator &iter)
{
    if (iter == m_vTaggedDecls.end())
        return 0;
    return *iter++;
}

/** returns the file's name
 *    \return the file's name
 */
string CFEFile::GetFileName()
{
    return m_sFileName;
}

/** returns a reference to the user defined type
 *    \param sName the name of the type to search for
 *    \return a reference to the user defined type, or 0 if it does not exist
 */
CFETypedDeclarator *CFEFile::FindUserDefinedType(string sName)
{
    if (sName.empty())
        return 0;
    // first search own typedefs
    vector<CFETypedDeclarator*>::iterator iterT = GetFirstTypedef();
    CFETypedDeclarator *pTypedDecl;
    while ((pTypedDecl = GetNextTypedef(iterT)) != 0)
    {
        if (pTypedDecl->FindDeclarator(sName))
            return pTypedDecl;
    }
    // then search interfaces
    vector<CFEInterface*>::iterator iterI = GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextInterface(iterI)) != 0)
    {
        if ((pTypedDecl = pInterface->FindUserDefinedType(sName)))
            return pTypedDecl;
    }
    // then search libraries
    vector<CFELibrary*>::iterator iterL = GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = GetNextLibrary(iterL)) != 0)
    {
        if ((pTypedDecl = pLibrary->FindUserDefinedType(sName)))
            return pTypedDecl;
    }
    // next search included files
    vector<CFEFile*>::iterator iterF = GetFirstChildFile();
    CFEFile *pFile;
    while ((pFile = GetNextChildFile(iterF)) != 0)
    {
        if ((pTypedDecl = pFile->FindUserDefinedType(sName)))
            return pTypedDecl;
    }
    // none found
    return 0;
}

/** \brief searches for a tagged declarator
 *  \param sName the tag (name) of the tagged decl
 *  \return a reference to the tagged declarator or NULL if none found
 */
CFEConstructedType* CFEFile::FindTaggedDecl(string sName)
{
    // own tagged decls
    vector<CFEConstructedType*>::iterator iterC = GetFirstTaggedDecl();
    CFEConstructedType* pTaggedDecl;
    while ((pTaggedDecl = GetNextTaggedDecl(iterC)) != 0)
    {
        if (dynamic_cast<CFETaggedStructType*>(pTaggedDecl))
            if (((CFETaggedStructType*)pTaggedDecl)->GetTag() == sName)
                return pTaggedDecl;
        if (dynamic_cast<CFETaggedUnionType*>(pTaggedDecl))
            if (((CFETaggedUnionType*)pTaggedDecl)->GetTag() == sName)
                return pTaggedDecl;
        if (dynamic_cast<CFETaggedEnumType*>(pTaggedDecl))
            if (((CFETaggedEnumType*)pTaggedDecl)->GetTag() == sName)
                return pTaggedDecl;
    }
    // search interfaces
    vector<CFEInterface*>::iterator iterI = GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextInterface(iterI)) != 0)
    {
        if ((pTaggedDecl = pInterface->FindTaggedDecl(sName)) != 0)
            return pTaggedDecl;
    }
    // search libs
    vector<CFELibrary*>::iterator iterL = GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = GetNextLibrary(iterL)) != 0)
    {
        if ((pTaggedDecl = pLibrary->FindTaggedDecl(sName)) != 0)
            return pTaggedDecl;
    }
    // search included files
    vector<CFEFile*>::iterator iterF = GetFirstChildFile();
    CFEFile *pFile;
    while ((pFile = GetNextChildFile(iterF)) != 0)
    {
        if ((pTaggedDecl = pFile->FindTaggedDecl(sName)) != 0)
            return pTaggedDecl;
    }
    // nothing found:
    return 0;
}

/** returns a reference to the user defined type
 *    \param sName the name of the type to search for
 *    \return a reference to the user defined type, or 0 if it does not exist
 */
CFETypedDeclarator* CFEFile::FindUserDefinedType(const char *sName)
{
    return FindUserDefinedType(string(sName));
}

/**    \brief returns a reference to the interface
 *    \param sName the name of the interface to search for
 *    \return a reference to the interface, or 0 if not found
 *
 * If the name is a scoped name, we have to get the scope name and
 * use it to find the library for it. And then use the library to find
 * the rest of the name.
 */
CFEInterface *CFEFile::FindInterface(string sName)
{
    if (sName.empty())
        return 0;

    // if scoped name
    string::size_type nScopePos;
    if ((nScopePos = sName.find("::")) != string::npos)
    {
        string sRest = sName.substr(nScopePos+2);
        string sScope = sName.substr(0, nScopePos);
        if (sScope.empty())
        {
            // has been a "::<name>"
            return FindInterface(sRest);
        }
        else
        {
            CFELibrary *pFELibrary = FindLibrary(sScope);
            if (pFELibrary == 0)
                return 0;
            return pFELibrary->FindInterface(sRest);
        }
    }
    // first search the interfaces in this file
    vector<CFEInterface*>::iterator iterI = GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextInterface(iterI)) != 0)
    {
        if (pInterface->GetName() == sName)
            return pInterface;
    }
    // search libraries
// do not search interfaces in libs (breaks scope)
//     vector<CFELibrary*>::iterator iterL = GetFirstLibrary();
//     CFELibrary *pLibrary;
//     while ((pLibrary = GetNextLibrary(iterL)) != 0)
//     {
//         if ((pInterface = pLibrary->FindInterface(sName)))
//             return pInterface;
//     }
    // the search the included files
    vector<CFEFile*>::iterator iterF = GetFirstChildFile();
    CFEFile *pFile;
    while ((pFile = GetNextChildFile(iterF)) != 0)
    {
        if ((pInterface = pFile->FindInterface(sName)))
            return pInterface;
    }
    // none found
    return 0;
}

/**    \brief returns a reference to the interface
 *    \param sName the name of the interface to search for
 *    \return a reference to the interface, or 0 if not found
 */
CFEInterface* CFEFile::FindInterface(const char* sName)
{
    return FindInterface(string(sName));
}

/**    \brief searches for a library
 *    \param sName the name to search for
 *    \return a reference to the found lib or 0 if not found
 */
CFELibrary *CFEFile::FindLibrary(string sName)
{
    if (sName.empty())
        return 0;

    vector<CFELibrary*>::iterator iterL = GetFirstLibrary();
    CFELibrary *pLib;
    while ((pLib = GetNextLibrary(iterL)) != 0)
    {
        if (pLib->GetName() == sName)
            return pLib;
         // no matter if the name if 0, test nested libs
// Do not test libs if they contain this lib (breaks scopes)
//          if ((pLib2 = pLib->FindLibrary(sName)) != 0)
//              return pLib2;
    }

    // search included/imported files
    vector<CFEFile*>::iterator iterF = GetFirstChildFile();
    CFEFile *pFEFile;
    while ((pFEFile = GetNextChildFile(iterF)) != 0)
    {
        if ((pLib = pFEFile->FindLibrary(sName)) != 0)
            return pLib;
    }

    return 0;
}

/**    \brief searches for a library
 *    \param sName the name to search for
 *    \return a reference to the found lib or 0 if not found
 */
CFELibrary* CFEFile::FindLibrary(const char* sName)
{
    return FindLibrary(string(sName));
}

/** returns a reference to the found constant declarator
 *    \param sName the name of the constant declarator to search for
 *    \return a reference to the found constant declarator, or 0 if not found
 */
CFEConstDeclarator *CFEFile::FindConstDeclarator(string sName)
{
    if (sName.empty())
        return 0;
    // first search this file's constants
    vector<CFEConstDeclarator*>::iterator iterC = GetFirstConstant();
    CFEConstDeclarator *pConst;
    while ((pConst = GetNextConstant(iterC)) != 0)
    {
        if (pConst->GetName() == sName)
            return pConst;
    }
    // then search included files
    vector<CFEFile*>::iterator iterF = GetFirstChildFile();
    CFEFile *pFile;
    while ((pFile = GetNextChildFile(iterF)) != 0)
    {
        if ((pConst = pFile->FindConstDeclarator(sName)))
            return pConst;
    }
    // then search the interfaces
    vector<CFEInterface*>::iterator iterI = GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextInterface(iterI)) != 0)
    {
        if ((pConst = pInterface->FindConstant(sName)))
            return pConst;
    }
    // then search the libraries
    vector<CFELibrary*>::iterator iterL = GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = GetNextLibrary(iterL)) != 0)
    {
        if ((pConst = pLibrary->FindConstant(sName)))
            return pConst;
    }
    // if none found, return 0
    return 0;
}

/** checks the file-name for a given extension (has to be last)
 *    \param sExtension the extension to search for
 *    \return true if found, false if not
 *
 * This function is case insensitive.
 */
bool CFEFile::HasExtension(string sExtension)
{
    // make sExtension lower case
    transform(sExtension.begin(), sExtension.end(), sExtension.begin(), tolower);
    // if no extension, this file cannot be of this extension
    return m_sFileExtension == sExtension;
}

/**    \brief checks if this file is an IDL file
 *    \return true if this file is an IDL file
 *
 * The check is done using the extension of the input file. This should be
 * "idl" (case-insensitive) or it also might be &lt;stdin&gt; if read from
 * the standard input. Or empty if this is the top level file.
 */
bool CFEFile::IsIDLFile()
{
    if (!HasExtension(string("idl")))
    {
        if (m_sFilenameWithoutExtension.empty())
            return true;
        if (m_sFilenameWithoutExtension == "<stdin>")
            return true;
        return false;
    }
    return true;
}

/** returns the filename without the extension
 *    \return the filename without the extension
 */
string CFEFile::GetFileNameWithoutExtension()
{
    return m_sFilenameWithoutExtension;
}

/** \brief checks the consitency of this file
 *  \return true if everything is fine, false otherwise
 *
 * A file is ok, when all it's included files are ok in the first place. Then it
 * checks if the defined types do already exists. Then the constants are checked
 * and finally all contained interfaces and libraries are checked by starting a
 * self-test.
 */
bool CFEFile::CheckConsistency()
{
    // included files
    vector<CFEFile*>::iterator iterF = GetFirstChildFile();
    CFEFile *pFile;
    while ((pFile = GetNextChildFile(iterF)) != 0)
    {
        // only check consistency of IDL files
        if (!(pFile->HasExtension("idl")))
            continue;
        if (!(pFile->CheckConsistency()))
            return false;
    }
    // check types
    vector<CFETypedDeclarator*>::iterator iterT = GetFirstTypedef();
    CFETypedDeclarator *pTypedef;
    while ((pTypedef = GetNextTypedef(iterT)) != 0)
    {
        if (!(pTypedef->CheckConsistency()))
            return false;
    }
    // check constants
    vector<CFEConstDeclarator*>::iterator iterC = GetFirstConstant();
    CFEConstDeclarator *pConst;
    while ((pConst = GetNextConstant(iterC)) != 0)
    {
        if (!(pConst->CheckConsistency()))
            return false;
    }
    // check interfaces
    vector<CFEInterface*>::iterator iterI = GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextInterface(iterI)) != 0)
    {
        if (!(pInterface->CheckConsistency()))
            return false;
    }
    // check libraries
    vector<CFELibrary*>::iterator iterL = GetFirstLibrary();
    CFELibrary *pLib;
    while ((pLib = GetNextLibrary(iterL)) != 0)
    {
        if (!(pLib->CheckConsistency()))
            return false;
    }
    // everything ran through, so we might consider this file clean
    return true;
}

/** for debugging purposes only */
void CFEFile::Dump()
{
    printf("Dump: CFEFile (%s)\n", GetFileName().c_str());
    printf("Dump: CFEFile (%s): included files:\n", GetFileName().c_str());
    vector<CFEFile*>::iterator iterF = GetFirstChildFile();
    CFEBase *pElement;
    while ((pElement = GetNextChildFile(iterF)) != 0)
    {
        pElement->Dump();
    }
    printf("Dump: CFEFile (%s): typedefs:\n", GetFileName().c_str());
    vector<CFETypedDeclarator*>::iterator iterT = GetFirstTypedef();
    while ((pElement = GetNextTypedef(iterT)) != 0)
    {
        pElement->Dump();
    }
    printf("Dump: CFEFile (%s): constants:\n", GetFileName().c_str());
    vector<CFEConstDeclarator*>::iterator iterC = GetFirstConstant();
    while ((pElement = GetNextConstant(iterC)) != 0)
    {
        pElement->Dump();
    }
    printf("Dump: CFEFile (%s): interfaces:\n", GetFileName().c_str());
    vector<CFEInterface*>::iterator iterI = GetFirstInterface();
    while ((pElement = GetNextInterface(iterI)) != 0)
    {
        pElement->Dump();
    }
    printf("Dump: CFEFile (%s): libraries:\n", GetFileName().c_str());
    vector<CFELibrary*>::iterator iterL = GetFirstLibrary();
    while ((pElement = GetNextLibrary(iterL)) != 0)
    {
        pElement->Dump();
    }
}

/** write this object to a file
 *    \param pFile the file to serialize from/to
 */
void CFEFile::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        pFile->PrintIndent("<idl>\n");
        pFile->IncIndent();
        pFile->PrintIndent("<name>%s</name>\n", GetFileName().c_str());
        // write included files
        vector<CFEFile*>::iterator iterF = GetFirstChildFile();
        CFEBase *pElement;
        while ((pElement = GetNextChildFile(iterF)) != 0)
        {
            pFile->PrintIndent("<include>%s</include>\n", (*iterF)->GetFileName().c_str());
        }
        // write constants
        vector<CFEConstDeclarator*>::iterator iterC = GetFirstConstant();
        while ((pElement = GetNextConstant(iterC)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // write typedefs
        vector<CFETypedDeclarator*>::iterator iterT = GetFirstTypedef();
        while ((pElement = GetNextTypedef(iterT)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // write tagged decls
        vector<CFEConstructedType*>::iterator iterTD = GetFirstTaggedDecl();
        while ((pElement = GetNextTaggedDecl(iterTD)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // write libraries
        vector<CFELibrary*>::iterator iterL = GetFirstLibrary();
        while ((pElement = GetNextLibrary(iterL)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // write interfaces
        vector<CFEInterface*>::iterator iterI = GetFirstInterface();
        while ((pElement = GetNextInterface(iterI)) != 0)
        {
            pElement->Serialize(pFile);
        }
        pFile->DecIndent();
        pFile->PrintIndent("</idl>\n");
    }
}

/**    \brief counts the constants of the file
 *    \param bCountIncludes true if included files should be countedt as well
 *    \return number of constants in this file
 */
int CFEFile::GetConstantCount(bool bCountIncludes)
{
     if (!IsIDLFile())
         return 0;

     int nCount = 0;
     vector<CFEConstDeclarator*>::iterator iterC = GetFirstConstant();
     CFEConstDeclarator *pDecl;
     while ((pDecl = GetNextConstant(iterC)) != 0)
     {
         nCount++;
     }

     if (!bCountIncludes)
         return nCount;

     vector<CFEFile*>::iterator iterF = GetFirstChildFile();
     CFEFile *pFile;
     while ((pFile = GetNextChildFile(iterF)) != 0)
     {
         nCount += pFile->GetConstantCount();
     }

     return nCount;
}

/**    \brief count the typedefs of the file
 *    \param bCountIncludes true if included files should be counted as well
 *    \return number of typedefs in file
 */
int CFEFile::GetTypedefCount(bool bCountIncludes)
{
     if (!IsIDLFile())
         return 0;

     int nCount = 0;
     vector<CFETypedDeclarator*>::iterator iterT = GetFirstTypedef();
     CFETypedDeclarator *pDecl;
     while ((pDecl = GetNextTypedef(iterT)) != 0)
     {
         nCount++;
     }

     if (!bCountIncludes)
         return nCount;

     vector<CFEFile*>::iterator iterF = GetFirstChildFile();
     CFEFile *pFile;
     while ((pFile = GetNextChildFile(iterF)) != 0)
     {
         nCount += pFile->GetTypedefCount();
     }

     return nCount;
}

/**    \brief checks if this file is a standard include file
 *    \return true if this is a standard include file
 */
bool CFEFile::IsStdIncludeFile()
{
    return (m_nStdInclude == 1);
}

/**    \brief returns the filename including the path
 *    \return a reference to m_sFileWithPath
 */
string CFEFile::GetFullFileName()
{
    return m_sFileWithPath;
}

/** \brief returns the line number on which this file has been included in its parent file
 *  \return line number
 */
int CFEFile::GetIncludedOnLine()
{
    return m_nIncludedOnLine;
}

/** \brief retrieves a pointer to the first include statement
 *  \return a iterator
 */
vector<CIncludeStatement*>::iterator CFEFile::GetFirstInclude()
{
    return m_vIncludes.begin();
}

/** \brief get the next include statement
 *  \param iter the pointer to the next include statement
 *  \return the next include statement
 */
CIncludeStatement* CFEFile::GetNextInclude(vector<CIncludeStatement*>::iterator &iter)
{
    if (iter == m_vIncludes.end())
        return 0;
    return *iter++;
}

/** \brief adds a new include statement
 *  \param pNewInclude the new include statement
 */
void CFEFile::AddInclude(CIncludeStatement *pNewInclude)
{
    if (!pNewInclude)
        return;
    m_vIncludes.push_back(pNewInclude);
    pNewInclude->SetParent(this);
}

/** \brief tries to find a file with the given filename
 *  \param sFileName the name of the file to find
 *  \return a reference to the found file
 */
CFEFile* CFEFile::FindFile(string sFileName)
{
    if (m_sFileName == sFileName)
        return this;
    vector<CFEFile*>::iterator iterF = GetFirstChildFile();
    CFEFile *pFile, *pFoundFile;
    while ((pFile = GetNextChildFile(iterF)) != 0)
    {
        if ((pFoundFile = pFile->FindFile(sFileName)) != 0)
            return pFoundFile;
    }
    return 0;
}

/** \brief retrieves the maximum line number in the file
 *  \return the maximum line number in this file
 *
 * If line number is not that, i.e., is zero, then we iterate the elements and
 * check their end line number. The maximum is out desired maximum line
 * number.
 */
int
CFEFile::GetSourceLineEnd()
{
    if (m_nSourceLineNbEnd != 0)
	return m_nSourceLineNbEnd;

    // Includes
    vector<CIncludeStatement*>::iterator iterI = m_vIncludes.begin();
    for (; iterI != m_vIncludes.end(); iterI++)
    {
	int sLine = (*iterI)->GetSourceLine();
	int eLine = (*iterI)->GetSourceLineEnd();
	m_nSourceLineNbEnd = MAX(sLine, MAX(eLine, m_nSourceLineNbEnd));
    }
    // libraries
    vector<CFELibrary*>::iterator iterL = GetFirstLibrary();
    CFELibrary *pL;
    while ((pL = GetNextLibrary(iterL)) != NULL)
    {
	int sLine = pL->GetSourceLine();
	int eLine = pL->GetSourceLineEnd();
	m_nSourceLineNbEnd = MAX(sLine, MAX(eLine, m_nSourceLineNbEnd));
    }
    // interfaces
    vector<CFEInterface*>::iterator iterIF = GetFirstInterface();
    CFEInterface *pI;
    while ((pI = GetNextInterface(iterIF)) != NULL)
    {
	int sLine = pI->GetSourceLine();
	int eLine = pI->GetSourceLineEnd();
	m_nSourceLineNbEnd = MAX(sLine, MAX(eLine, m_nSourceLineNbEnd));
    }
    // typedefs
    vector<CFETypedDeclarator*>::iterator iterTD = GetFirstTypedef();
    CFETypedDeclarator *pTD;
    while ((pTD = GetNextTypedef(iterTD)) != NULL)
    {
	int sLine = pTD->GetSourceLine();
	int eLine = pTD->GetSourceLineEnd();
	m_nSourceLineNbEnd = MAX(sLine, MAX(eLine, m_nSourceLineNbEnd));
    }
    // tagged types
    vector<CFEConstructedType*>::iterator iterCT = GetFirstTaggedDecl();
    CFEConstructedType *pCT;
    while ((pCT = GetNextTaggedDecl(iterCT)) != NULL)
    {
	int sLine = pCT->GetSourceLine();
	int eLine = pCT->GetSourceLineEnd();
	m_nSourceLineNbEnd = MAX(sLine, MAX(eLine, m_nSourceLineNbEnd));
    }
    // tagged types
    vector<CFEConstDeclarator*>::iterator iterC = GetFirstConstant();
    CFEConstDeclarator *pC;
    while ((pC = GetNextConstant(iterC)) != NULL)
    {
	int sLine = pC->GetSourceLine();
	int eLine = pC->GetSourceLineEnd();
	m_nSourceLineNbEnd = MAX(sLine, MAX(eLine, m_nSourceLineNbEnd));
    }

    return m_nSourceLineNbEnd;
}
