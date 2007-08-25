/**
 *  \file    dice/src/fe/FEFile.cpp
 *  \brief   contains the implementation of the class CFEFile
 *
 *  \date    01/31/2001
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

#include "IncludeStatement.h"
#include "FEFile.h"
#include "FETypedDeclarator.h"
#include "FEConstDeclarator.h"
#include "FEConstructedType.h"
#include "FEStructType.h"
#include "FEUnionType.h"
#include "FEEnumType.h"
#include "FEInterface.h"
#include "FELibrary.h"
#include "Compiler.h"
#include "Visitor.h"
#include <iostream>
#include <cctype>
#include <algorithm>
#include <cassert>

CFEFile::CFEFile(string sFileName,
    string sPath,
    int nIncludedOnLine,
    int nStdInclude)
: m_Constants(0, this),
    m_Typedefs(0, this),
    m_TaggedDeclarators(0, this),
    m_Libraries(0, this),
    m_Interfaces(0, this),
    m_ChildFiles(0, this)
{
    // this procedure is interesting for included files:
    // if the statement was: #include "l4/sys/types.h" and the path where the
    // file was found is "../../include" the following string are created:
    //
    // m_sFilename = "l4/sys/types.h"
    // m_sFilenameWithoutExtension = "types"
    // m_sFileExtension = "h"
    // m_sFileWithPath = "../../include/l4/sys/types.h"
    assert(!sFileName.empty());

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s(%s, %s, %d, %d) called\n",
	__func__, sFileName.c_str(), sPath.c_str(), nIncludedOnLine,
	nStdInclude);

    m_sFilename = sFileName;
    // first strip of extension  (the string after the last '.')
    int iDot = m_sFilename.rfind('.');
    if (iDot < 0)
    {
	m_sFilenameWithoutExtension = m_sFilename;
	m_sFileExtension.clear();
    }
    else
    {
	m_sFilenameWithoutExtension = m_sFilename.substr(0, iDot);
	m_sFileExtension = m_sFilename.substr(iDot + 1);
    }
    // now strip of the path
    int iSlash = m_sFilenameWithoutExtension.rfind('/');
    if (iSlash >= 0)
    {
	m_sFilenameWithoutExtension =
	    m_sFilenameWithoutExtension.substr(iSlash + 1);
    }
    // now generate full filename with path
    SetPath(sPath);
    m_nStdInclude = nStdInclude;
    m_nIncludedOnLine = nIncludedOnLine;

    // make file extension lower case
    transform(m_sFileExtension.begin(), m_sFileExtension.end(),
        m_sFileExtension.begin(), _tolower);

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s has m_sFilename: %s\n", __func__,
	m_sFilename.c_str());
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s has m_sFilenameWithoutExtension: %s\n",
	__func__, m_sFilenameWithoutExtension.c_str());
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s has m_sFileExtension: %s\n",
	__func__, m_sFileExtension.c_str());
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s has m_sFileWithPath: %s\n",
	__func__, m_sFileWithPath.c_str());
}

CFEFile::CFEFile(CFEFile & src)
: CFEBase(src),
    m_Constants(src.m_Constants),
    m_Typedefs(src.m_Typedefs),
    m_TaggedDeclarators(src.m_TaggedDeclarators),
    m_Libraries(src.m_Libraries),
    m_Interfaces(src.m_Interfaces),
    m_ChildFiles(src.m_ChildFiles)
{
    m_sFilename = src.m_sFilename;
    m_sFileExtension = src.m_sFileExtension;
    m_sFilenameWithoutExtension = src.m_sFilenameWithoutExtension;
    m_sFileWithPath = src.m_sFileWithPath;
    m_nStdInclude = src.m_nStdInclude;
    m_nIncludedOnLine = src.m_nIncludedOnLine;
}

/** cleans up the file object */
CFEFile::~CFEFile()
{ }

/** copies the object
 *  \return a reference to the new file object
 */
CObject *CFEFile::Clone()
{
    return new CFEFile(*this);
}

/** returns the file's name
 *  \return the file's name
 */
string CFEFile::GetFileName()
{
    return m_sFilename;
}

/** sets the path of the file
 *  \param sPath the new path
 */
void CFEFile::SetPath(std::string sPath)
{
    if (!sPath.empty())
    {
	bool bLastSlash = sPath[sPath.length()-1] == '/';
	m_sFileWithPath = sPath;
	if (!bLastSlash)
	    m_sFileWithPath += "/";
	m_sFileWithPath += m_sFilename;
    }
    else
	m_sFileWithPath = m_sFilename;
}

/** returns a reference to the user defined type
 *  \param sName the name of the type to search for
 *  \return a reference to the user defined type, or 0 if it does not exist
 */
CFETypedDeclarator *CFEFile::FindUserDefinedType(string sName)
{
    if (sName.empty())
        return 0;
    // first search own typedefs
    CFETypedDeclarator *pTypedDecl = m_Typedefs.Find(sName);
    if (pTypedDecl)
	return pTypedDecl;
    // then search interfaces
    vector<CFEInterface*>::iterator iterI;
    for (iterI = m_Interfaces.begin();
	 iterI != m_Interfaces.end();
	 iterI++)
    {
        if ((pTypedDecl = (*iterI)->m_Typedefs.Find(sName)))
            return pTypedDecl;
    }
    // then search libraries
    vector<CFELibrary*>::iterator iterL;
    for (iterL = m_Libraries.begin();
	 iterL != m_Libraries.end();
	 iterL++)
    {
        if ((pTypedDecl = (*iterL)->FindUserDefinedType(sName)))
            return pTypedDecl;
    }
    // next search included files
    vector<CFEFile*>::iterator iterF;
    for (iterF = m_ChildFiles.begin();
	 iterF != m_ChildFiles.end();
	 iterF++)
    {
        if ((pTypedDecl = (*iterF)->FindUserDefinedType(sName)))
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
    CFEConstructedType* pTaggedDecl = m_TaggedDeclarators.Find(sName);
    if (pTaggedDecl)
	return pTaggedDecl;
    // search interfaces
    vector<CFEInterface*>::iterator iterI;
    for (iterI = m_Interfaces.begin();
	 iterI != m_Interfaces.end();
	 iterI++)
    {
        if ((pTaggedDecl = (*iterI)->m_TaggedDeclarators.Find(sName)))
            return pTaggedDecl;
    }
    // search libs
    vector<CFELibrary*>::iterator iterL;
    for (iterL = m_Libraries.begin();
	 iterL != m_Libraries.end();
	 iterL++)
    {
        if ((pTaggedDecl = (*iterL)->FindTaggedDecl(sName)))
            return pTaggedDecl;
    }
    // search included files
    vector<CFEFile*>::iterator iterF;
    for (iterF = m_ChildFiles.begin();
	 iterF != m_ChildFiles.end();
	 iterF++)
    {
        if ((pTaggedDecl = (*iterF)->FindTaggedDecl(sName)))
            return pTaggedDecl;
    }
    // nothing found:
    return 0;
}

/** returns a reference to the user defined type
 *  \param sName the name of the type to search for
 *  \return a reference to the user defined type, or 0 if it does not exist
 */
CFETypedDeclarator* CFEFile::FindUserDefinedType(const char *sName)
{
    return FindUserDefinedType(string(sName));
}

/** \brief returns a reference to the interface
 *  \param sName the name of the interface to search for
 *  \return a reference to the interface, or 0 if not found
 *
 * If the name is a scoped name, we have to get the scope name and
 * use it to find the library for it. And then use the library to find
 * the rest of the name.
 */
CFEInterface *CFEFile::FindInterface(string sName)
{
    if (sName.empty())
        return 0;

    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"%s(%s) called\n", __func__, sName.c_str());

    // if scoped name
    string::size_type nScopePos;
    if ((nScopePos = sName.find("::")) != string::npos)
    {
        string sRest = sName.substr(nScopePos+2);
        string sScope = sName.substr(0, nScopePos);
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	    "%s: bisected name into \"%s\"::\"%s\"\n", __func__,
	    sScope.c_str(), sRest.c_str());
        if (sScope.empty())
        {
            // has been a "::<name>"
            return FindInterface(sRest);
        }
        else
        {
            CFELibrary *pFELibrary = FindLibrary(sScope);
	    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"%s: search for interface in lib (%p)\n", __func__,
		pFELibrary);
            if (pFELibrary == 0)
                return 0;
	    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"%s: search for interface \"%s\" in lib \"%s\"\n", __func__,
		sRest.c_str(), sScope.c_str());
            return pFELibrary->FindInterface(sRest);
        }
    }
    // first search the interfaces in this file
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"%s: search in interface of file %s\n", __func__,
	GetFileName().c_str());
    CFEInterface *pInterface = m_Interfaces.Find(sName);
    if (pInterface)
	return pInterface;
// do not search interfaces in libs (breaks scope)
    // the search the included files
    vector<CFEFile*>::iterator iterF;
    for (iterF = m_ChildFiles.begin();
	 iterF != m_ChildFiles.end();
	 iterF++)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	    "%s: search in included file %s\n", __func__,
	    (*iterF)->GetFileName().c_str());
        if ((pInterface = (*iterF)->FindInterface(sName)))
            return pInterface;
    }
    // none found
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"%s: returns NULL\n", __func__);
    return 0;
}

/** \brief returns a reference to the interface
 *  \param sName the name of the interface to search for
 *  \return a reference to the interface, or 0 if not found
 */
CFEInterface* CFEFile::FindInterface(const char* sName)
{
    return FindInterface(string(sName));
}

/** \brief searches for a library
 *  \param sName the name to search for
 *  \return a reference to the found lib or 0 if not found
 */
CFELibrary *CFEFile::FindLibrary(string sName)
{
    if (sName.empty())
        return 0;

    CFELibrary *pLib = m_Libraries.Find(sName);
    if (pLib)
	return pLib;
// Do not test libs if they contain this lib (breaks scopes)

    // search included/imported files
    vector<CFEFile*>::iterator iterF;
    for (iterF = m_ChildFiles.begin();
	 iterF != m_ChildFiles.end();
	 iterF++)
    {
        if ((pLib = (*iterF)->FindLibrary(sName)))
            return pLib;
    }

    return 0;
}

/** \brief searches for a library
 *  \param sName the name to search for
 *  \return a reference to the found lib or 0 if not found
 */
CFELibrary* CFEFile::FindLibrary(const char* sName)
{
    return FindLibrary(string(sName));
}

/** returns a reference to the found constant declarator
 *  \param sName the name of the constant declarator to search for
 *  \return a reference to the found constant declarator, or 0 if not found
 */
CFEConstDeclarator *CFEFile::FindConstDeclarator(string sName)
{
    if (sName.empty())
        return 0;
    // first search this file's constants
    CFEConstDeclarator *pConst = m_Constants.Find(sName);
    if (pConst)
	return pConst;
    // then search included files
    vector<CFEFile*>::iterator iterF;
    for (iterF = m_ChildFiles.begin();
	 iterF != m_ChildFiles.end();
	 iterF++)
    {
        if ((pConst = (*iterF)->FindConstDeclarator(sName)))
            return pConst;
    }
    // then search the interfaces
    vector<CFEInterface*>::iterator iterI;
    for (iterI = m_Interfaces.begin();
	 iterI != m_Interfaces.end();
	 iterI++)
    {
        if ((pConst = (*iterI)->m_Constants.Find(sName)))
            return pConst;
    }
    // then search the libraries
    vector<CFELibrary*>::iterator iterL;
    for (iterL = m_Libraries.begin();
	 iterL != m_Libraries.end();
	 iterL++)
    {
        if ((pConst = (*iterL)->FindConstant(sName)))
            return pConst;
    }
    // if none found, return 0
    return 0;
}

/** checks the file-name for a given extension (has to be last)
 *  \param sExtension the extension to search for
 *  \return true if found, false if not
 *
 * This function is case insensitive.
 */
bool CFEFile::HasExtension(string sExtension)
{
    // make sExtension lower case
    transform(sExtension.begin(), sExtension.end(), sExtension.begin(), _tolower);
    // if no extension, this file cannot be of this extension
    return m_sFileExtension == sExtension;
}

/** \brief checks if this file is an IDL file
 *  \return true if this file is an IDL file
 *
 * The check is done using the extension of the input file. This should be
 * "idl" (case-insensitive) or it also might be \<stdin\> if read from
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
 *  \return the filename without the extension
 */
string CFEFile::GetFileNameWithoutExtension()
{
    return m_sFilenameWithoutExtension;
}

/** \brief iterate the elements to call the visitor with them
 *  \param v the visitor
 */
void CFEFile::Accept(CVisitor &v)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CFEFile::%s called for file %s\n",
	__func__, GetFileName().c_str());
    // only check consistency if this is an IDL file!
    if (!IsIDLFile())
	return;
    // call visitor
    v.Visit(*this);
    // included files
//     for_each(m_ChildFiles.begin(), m_ChildFiles.end(),
// 	std::bind2nd(std::mem_fun(&CFEFile::Accept), v));
    vector<CFEFile*>::iterator iterF;
    for (iterF = m_ChildFiles.begin();
	 iterF != m_ChildFiles.end();
	 iterF++)
    {
	(*iterF)->Accept(v);
    }
    // check constants
    vector<CFEConstDeclarator*>::iterator iterC;
    for (iterC = m_Constants.begin();
	 iterC != m_Constants.end();
	 iterC++)
    {
	(*iterC)->Accept(v);
    }
    // check types
    vector<CFETypedDeclarator*>::iterator iterT;
    for (iterT = m_Typedefs.begin();
	 iterT != m_Typedefs.end();
	 iterT++)
    {
	(*iterT)->Accept(v);
    }
    // check type declarations
    vector<CFEConstructedType*>::iterator iterCT;
    for (iterCT = m_TaggedDeclarators.begin();
	 iterCT != m_TaggedDeclarators.end();
	 iterCT++)
    {
	(*iterCT)->Accept(v);
    }
    // check interfaces
    vector<CFEInterface*>::iterator iterI;
    for (iterI = m_Interfaces.begin();
	 iterI != m_Interfaces.end();
	 iterI++)
    {
	(*iterI)->Accept(v);
    }
    // check libraries
    vector<CFELibrary*>::iterator iterL;
    for (iterL = m_Libraries.begin();
	 iterL != m_Libraries.end();
	 iterL++)
    {
	(*iterL)->Accept(v);
    }
}

class ConstantSum {
    int res;
    std::mem_fun1_t<int, CFEFile, bool> fun;
public:
    ConstantSum(int (CFEFile::*__pf)(bool), int i = 0) : res(i), fun(__pf) { }
    void operator () (CFEFile *f) { res += fun(f, true); }
    int result() const { return res; }
};

/** \brief counts the constants of the file
 *  \param bCountIncludes true if included files should be countedt as well
 *  \return number of constants in this file
 */
int CFEFile::GetConstantCount(bool bCountIncludes)
{
     if (!IsIDLFile())
         return 0;

     int nCount = m_Constants.size();

     if (!bCountIncludes)
         return nCount;

     ConstantSum s(&CFEFile::GetConstantCount);
     s = for_each(m_ChildFiles.begin(), m_ChildFiles.end(), s);
     return nCount + s.result();
}

/** \brief count the typedefs of the file
 *  \param bCountIncludes true if included files should be counted as well
 *  \return number of typedefs in file
 */
int CFEFile::GetTypedefCount(bool bCountIncludes)
{
     if (!IsIDLFile())
         return 0;

     int nCount = m_Typedefs.size();

     if (!bCountIncludes)
         return nCount;

     ConstantSum s(&CFEFile::GetTypedefCount);
     s = for_each(m_ChildFiles.begin(), m_ChildFiles.end(), s);
     return nCount + s.result();
}

/** \brief checks if this file is a standard include file
 *  \return true if this is a standard include file
 */
bool CFEFile::IsStdIncludeFile()
{
    return (m_nStdInclude == 1);
}

/** \brief returns the filename including the path
 *  \return a reference to m_sFileWithPath
 */
string CFEFile::GetFullFileName()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s() called, return %s\n", __func__,
	m_sFileWithPath.c_str());
    return m_sFileWithPath;
}

/** \brief returns the line number on which this file has been included in its parent file
 *  \return line number
 */
int CFEFile::GetIncludedOnLine()
{
    return m_nIncludedOnLine;
}

/** \brief tries to find a file with the given filename
 *  \param sFileName the name of the file to find
 *  \return a reference to the found file
 */
CFEFile* CFEFile::FindFile(string sFileName)
{
    if (m_sFilename == sFileName)
        return this;
    CFEFile *pFoundFile;
    vector<CFEFile*>::iterator iterF;
    for (iterF = m_ChildFiles.begin();
	 iterF != m_ChildFiles.end();
	 iterF++)
    {
        if ((pFoundFile = (*iterF)->FindFile(sFileName)) != 0)
            return pFoundFile;
    }
    return 0;
}

