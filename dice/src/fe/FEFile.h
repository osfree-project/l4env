/**
 *    \file    dice/src/fe/FEFile.h
 *    \brief   contains the declaration of the class CFEFile
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

/** preprocessing symbol to check header file */
#ifndef __DICE_FE_FEFILE_H__
#define __DICE_FE_FEFILE_H__

#include "FEBase.h"
#include <string>
#include <vector>
using namespace std;

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
    CFEFile(string sFileName, string sPath, int nIncludedOnLine = 1, 
	int nStdInclude = 0);
    virtual ~CFEFile();

protected:
    /** \brief copy constructor
     *    \param src the source to copy from
     */
    CFEFile(CFEFile &src);

// Operations
public:
    virtual string GetFullFileName();
    virtual bool IsStdIncludeFile();
    virtual int GetTypedefCount(bool bCountIncludes = true);
    virtual int GetConstantCount(bool bCountIncludes = true);
    virtual bool IsIDLFile();
    virtual void Serialize(CFile *pFile);
    virtual void Dump();
    virtual bool CheckConsistency();
    virtual string GetFileNameWithoutExtension();
    virtual bool HasExtension(string sExtension);
    virtual string GetFileName();

    virtual CFEConstructedType* FindTaggedDecl(string sName);
    virtual CFEConstructedType* GetNextTaggedDecl(
	vector<CFEConstructedType*>::iterator &iter);
    virtual vector<CFEConstructedType*>::iterator GetFirstTaggedDecl();
    virtual void AddTaggedDecl(CFEConstructedType *pTaggedDecl);

    virtual CFEConstDeclarator* FindConstDeclarator(string sName);
    virtual CFEConstDeclarator* GetNextConstant(
	vector<CFEConstDeclarator*>::iterator &iter);
    virtual vector<CFEConstDeclarator*>::iterator GetFirstConstant();
    virtual void AddConstant(CFEConstDeclarator* pConstant);

    virtual CFETypedDeclarator* FindUserDefinedType(string sName);
    virtual CFETypedDeclarator* FindUserDefinedType(const char* sName);
    virtual CFETypedDeclarator* GetNextTypedef(
	vector<CFETypedDeclarator*>::iterator &iter);
    virtual vector<CFETypedDeclarator*>::iterator GetFirstTypedef();
    virtual void AddTypedef(CFETypedDeclarator* pTypedef);

    virtual CFELibrary* FindLibrary(string sName);
    virtual CFELibrary* FindLibrary(const char* sName);
    virtual CFELibrary* GetNextLibrary(vector<CFELibrary*>::iterator &iter);
    virtual vector<CFELibrary*>::iterator GetFirstLibrary();
    virtual void AddLibrary(CFELibrary *pLibrary);

    virtual CFEInterface* FindInterface(string sName);
    virtual CFEInterface* FindInterface(const char* sName);
    virtual CFEInterface* GetNextInterface(
	vector<CFEInterface*>::iterator &iter);
    virtual vector<CFEInterface*>::iterator GetFirstInterface();
    virtual void AddInterface(CFEInterface *pInterface);

    virtual CFEFile* GetNextChildFile(vector<CFEFile*>::iterator &iter);
    virtual vector<CFEFile*>::iterator GetFirstChildFile();
    virtual void AddChild(CFEFile *pNewChild);

    virtual vector<CIncludeStatement*>::iterator GetFirstInclude();
    virtual CIncludeStatement* GetNextInclude(
	vector<CIncludeStatement*>::iterator &iter);
    virtual void AddInclude(CIncludeStatement *pNewInclude);

    virtual CObject* Clone();

    virtual int GetIncludedOnLine();
    virtual int GetSourceLineEnd();

    virtual CFEFile* FindFile(string sFileName);

// Attributes
protected:
    /**    \var vector<CFEConstructedType*> m_vTaggedDecls
     *    \brief the tagged struct and union declarations
     */
    vector<CFEConstructedType*> m_vTaggedDecls;
    /**    \var vector<CFEConstDeclarator*> m_vConstants
     *    \brief the constants of this file
     */
    vector<CFEConstDeclarator*> m_vConstants;
    /**    \var vector<CFETypedDeclarator*> m_vTypedefs
     *    \brief the type definitions in this file
     */
    vector<CFETypedDeclarator*> m_vTypedefs;
    /**    \var vector<CFELibrary*> m_vLibraries
     *    \brief the libraries in this file
     */
    vector<CFELibrary*> m_vLibraries;
    /**    \var vector<CFEInterface*> m_vInterfaces
     *    \brief the interfaces in this file
     */
    vector<CFEInterface*> m_vInterfaces;
    /**    \var vector<CFEFile*> m_vChildFiles
     *    \brief the child files (included files)
     */
    vector<CFEFile*> m_vChildFiles;
    /** \var vector<CIncludeStatement*> m_vIncludes
     *  \brief contains the include statements
     *
     * The preprocessor might swallow some included files,
     * because they have been included elsewhere already.
     * Therefore we keep an extra list of include statements.
     */
    vector<CIncludeStatement*> m_vIncludes;
    /** \var string m_sFileName
     *    \brief contains the file name of the component
     */
    string m_sFileName;
    /**    \var string m_sFilenameWithoutExtension
     *    \brief the file name without the extension
     */
    string m_sFilenameWithoutExtension;
    /**    \var string m_sFileExtension
     *    \brief the extension of the file name
     */
    string m_sFileExtension;
    /**    \var string m_sFileWithPath
     *    \brief the file-name with the complete path
     */
    string m_sFileWithPath;
    /**    \var int m_nStdInclude
     *    \brief set to 1 if this file is a standard include file (#include <...>)
     *
     * A standard include file is a file included by using "<" ">" instead of '"'.
     * This option is used with the notstdinc option.
     */
    int m_nStdInclude;
    /** \var int m_nIncludedOnLine
     *  \brief the line number this file has been included from
     */
    int m_nIncludedOnLine;
};

#endif // __DICE_FE_FEFILE_H__
