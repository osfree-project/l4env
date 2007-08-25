/**
 *  \file    dice/src/be/BEFile.h
 *  \brief   contains the declaration of the class CBEFile
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

/** preprocessing symbol to check header file */
#ifndef __DICE_BEFILE_H__
#define __DICE_BEFILE_H__

#include "BEContext.h" // for FILE_TYPE
#include "template.h"
#include <vector>
#include <fstream>

class CBEFunction;
class CBETarget;
class CBEClass;
class CBENameSpace;

class CFEOperation;
class CFEInterface;
class CFELibrary;
class CFEFile;
class CIncludeStatement;

/** \class CBEFile
 *  \ingroup backend
 *  \brief the output file class for the back-end classes
 *
 * This class unifies the common properties of the class CBEHeaderFile and
 * CBEImplementationFile. These common properties are that both contain
 * functions and include files.
 */
class CBEFile : public std::ofstream, public CObject
{
// Constructor
public:
    /** \brief constructor
     */
    CBEFile();
    virtual ~CBEFile();

    /** maimum possible indent */
    const static unsigned int MAX_INDENT;
    /** the standard indentation value */
    const static unsigned int STD_INDENT;

protected:
    /** \brief copy constructor */
    CBEFile(CBEFile &src);

public:
    virtual CBETarget* GetTarget();

    /** \brief write the file */
    virtual void Write(void) = 0;

    /** \brief creating class
     *  \param pFEOperation the operation to use as source
     *  \param nFileType the type of the file
     */
    virtual void CreateBackEnd(CFEOperation *pFEOperation, FILE_TYPE nFileType) = 0;
    /** \brief creating class
     *  \param pFEInterface the interface to use as source
     *  \param nFileType the type of the file
     */
    virtual void CreateBackEnd(CFEInterface *pFEInterface, FILE_TYPE nFileType) = 0;
    /** \brief creating class
     *  \param pFELibrary the library to use as source
     *  \param nFileType the type of the file
     */
    virtual void CreateBackEnd(CFELibrary *pFELibrary, FILE_TYPE nFileType) = 0;
    /** \brief creating class
     *  \param pFEFile the file to use as source
     *  \param nFileType the type of the file
     */
    virtual void CreateBackEnd(CFEFile *pFEFile, FILE_TYPE nFileType) = 0;
    /** \brief return the name of the file
     *  \return the name of the currently open file, 0 if no file is open
     */
    std::string GetFileName() const
    { return m_sFilename; }
    /** \brief return the current indent
     *  \return the current indent
     */
    unsigned int GetIndent() const
    { return m_nIndent; }

    virtual CBEFunction* FindFunction(std::string sFunctionName,
	FUNCTION_TYPE nFunctionType);
    virtual CBEClass* FindClass(std::string sClassName, CBEClass *pPrev = NULL);
    virtual CBENameSpace* FindNameSpace(std::string sNameSpaceName);

    virtual void AddIncludedFileName(std::string sFileName, bool bIDLFile,
	    bool bIsStandardInclude, CObject* pRefObj = 0);

    virtual bool IsOfFileType(FILE_TYPE nFileType);
    virtual bool HasFunctionWithUserType(std::string sTypeName);

    CBEFile& operator++();
    CBEFile& operator--();
    CBEFile& operator+=(int);
    CBEFile& operator-=(int);
    using std::ofstream::operator<<;
    std::ofstream& operator<<(std::string s);
    std::ofstream& operator<<(char const * s);
    std::ofstream& operator<<(char* s);

protected:
    /** \brief write a function
     *  \param pFunction the function to write
     */
    virtual void WriteFunction(CBEFunction *pFunction) = 0;
    virtual void WriteDefaultIncludes(void);
    virtual void WriteInclude(CIncludeStatement* pInclude);
    /** \brief writes a namespace
     *  \param pNameSpace the namespace to write
     */
    virtual void WriteNameSpace(CBENameSpace *pNameSpace) = 0;
    /** \brief writes a class
     *  \param pClass the class to write
     */
    virtual void WriteClass(CBEClass *pClass) = 0;
    virtual void WriteIntro(void);
    virtual int GetFunctionCount(void);

    virtual void WriteHelperFunctions(void);

    virtual void CreateOrderedElementList(void);
    void InsertOrderedElement(CObject *pObj);

    virtual void PrintIndent(void);

protected:
    /** \var FILE_TYPE m_nFileType
     *  \brief contains the type of the file
     */
    FILE_TYPE m_nFileType;
    /** \var vector<CObject*> m_vOrderedElements
     *  \brief contains ordered list of elements
     */
    vector<CObject*> m_vOrderedElements;
    /** \var std::string m_sFilename
     *  \brief the file's name
     */
    std::string m_sFilename;
    /** \var int m_nIndent
     *  \brief the current valid indent, when printing to the file
     */
    unsigned int m_nIndent;
    /** \var m_nLastIndent
     *  \brief remembers last increment
     */
    int m_nLastIndent;


public:
    /** \var CSearchableCollection<CBEClass, std::string> m_Classes
     *  \brief contains all classes, which belong to this file
     */
    CSearchableCollection<CBEClass, std::string> m_Classes;
    /** \var CSearchableCollection<CBENameSpace, std::string> m_NameSpaces
     *  \brief contains all namespaces, which belong to this file
     */
    CSearchableCollection<CBENameSpace, std::string> m_NameSpaces;
    /** \var CCollection<CBEFunction> m_Functions
     *  \brief contains the functions, which belong to this file
     */
    CCollection<CBEFunction> m_Functions;
    /** \var CSearchableCollection<CIncludeStatement, std::string> m_IncludedFiles
     *  \brief contains the names of the included files
     *
     * This is an array of strings, because a file may not only include
     * CBEFile-represented files, but also other files. For instance C-header
     * files. Therefore it is more convenient to store the names of the files,
     * than to create CBEFiles for the C-header files as well.
     */
    CSearchableCollection<CIncludeStatement, std::string> m_IncludedFiles;
};

#endif // !__DICE_BEFILE_H__
