/**
 *  \file    dice/src/fe/FEFile.h
 *  \brief   contains the declaration of the class CFEFile
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

/** preprocessing symbol to check header file */
#ifndef __DICE_FE_FEFILE_H__
#define __DICE_FE_FEFILE_H__

#include "FEBase.h"
#include "template.h"
#include <string>
#include <vector>

class CFETypedDeclarator;
class CFEConstDeclarator;
class CFEConstructedType;
class CFEInterface;
class CFELibrary;
class CIncludeStatement;

/** \class CFEFile
 *  \ingroup frontend
 *  \brief represents an idl file
 */
class CFEFile : public CFEBase
{

// constructor/desctructor
public:
    /** constructs a idl file representation */
    CFEFile(string sFileName, string sPath, int nIncludedOnLine = 1, int nStdInclude = 0);
    virtual ~CFEFile();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFEFile(CFEFile &src);

// Operations
public:
    string GetFullFileName();
    bool IsStdIncludeFile();
    int GetTypedefCount(bool bCountIncludes = true);
    int GetConstantCount(bool bCountIncludes = true);
    bool IsIDLFile();
    virtual void Accept(CVisitor&);
    string GetFileNameWithoutExtension();
    bool HasExtension(string sExtension);
    string GetFileName();
    
    CFEConstDeclarator* FindConstDeclarator(string sName);
    CFEConstructedType* FindTaggedDecl(string sName);
    CFETypedDeclarator* FindUserDefinedType(string sName);
    CFETypedDeclarator* FindUserDefinedType(const char* sName);
    CFELibrary* FindLibrary(string sName);
    CFELibrary* FindLibrary(const char* sName);
    CFEInterface* FindInterface(string sName);
    CFEInterface* FindInterface(const char* sName);

    virtual CObject* Clone();

    int GetIncludedOnLine();
    int GetSourceLineEnd();

    virtual CFEFile* FindFile(string sFileName);

// Attributes
protected:
    /** \var string m_sFilename
     *  \brief contains the file name of the component
     */
    string m_sFilename;
    /** \var string m_sFilenameWithoutExtension
     *  \brief the file name without the extension
     */
    string m_sFilenameWithoutExtension;
    /** \var string m_sFileExtension
     *  \brief the extension of the file name
     */
    string m_sFileExtension;
    /** \var string m_sFileWithPath
     *  \brief the file-name with the complete path
     */
    string m_sFileWithPath;
    /** \var int m_nStdInclude
     *  \brief set to 1 if this file is a standard include file (\#include <...>)
     *
     * A standard include file is a file included by using "<" ">" instead of '"'.
     * This option is used with the notstdinc option.
     */
    int m_nStdInclude;
    /** \var int m_nIncludedOnLine
     *  \brief the line number this file has been included from
     */
    int m_nIncludedOnLine;

public:
    /** \var CSearchableCollection<CFEConstDeclarator, string> m_Constants
     *  \brief the constants of this file
     */
    CSearchableCollection<CFEConstDeclarator, string> m_Constants;
    /** \var CSearchableCollection<CFETypedDeclarator, string> m_Typedefs
     *  \brief the type definitions in this file
     */
    CSearchableCollection<CFETypedDeclarator, string> m_Typedefs;
    /** \var CSearchableCollection<CFEConstructedType, string> m_TaggedDeclarators
     *  \brief the tagged struct and union declarations
     */
    CSearchableCollection<CFEConstructedType, string> m_TaggedDeclarators;
    /** \var CSearchableCollection<CFELibrary, string> m_Libraries
     *  \brief the libraries in this file
     */
    CSearchableCollection<CFELibrary, string> m_Libraries;
    /** \var CSearchableCollection<CFEInterface, string> m_Interfaces
     *  \brief the interfaces in this file
     */
    CSearchableCollection<CFEInterface, string> m_Interfaces;
    /** \var CCollection<CFEFile> m_ChildFiles
     *  \brief the child files (included files)
     */
    CCollection<CFEFile> m_ChildFiles;
    /** \var CCollection<CIncludeStatement> m_Includes
     *  \brief contains the include statements
     *
     * The preprocessor might swallow some included files,
     * because they have been included elsewhere already.
     * Therefore we keep an extra list of include statements.
     */
    CCollection<CIncludeStatement> m_Includes;
};

#endif // __DICE_FE_FEFILE_H__
