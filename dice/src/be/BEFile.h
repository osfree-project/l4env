/**
 *    \file    dice/src/be/BEFile.h
 *    \brief   contains the declaration of the class CBEFile
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

/** preprocessing symbol to check header file */
#ifndef __DICE_BEFILE_H__
#define __DICE_BEFILE_H__

#include "File.h"
#include <vector>
using namespace std;

class CBEContext;
class CBEFunction;
class CBETarget;
class CBEClass;
class CBENameSpace;

class CFEOperation;
class CFEInterface;
class CFELibrary;
class CFEFile;
class CIncludeStatement;

/**    \class CBEFile
 *    \ingroup backend
 *    \brief the output file class for the back-end classes
 *
 * This class unifies the common properties of the class CBEHeaderFile and CBEImplementationFile. These common
 * properties are that both contain functions and include files.
 */
class CBEFile : public CFile
{
// Constructor
public:
    /**    \brief constructor
     */
    CBEFile();
    virtual ~CBEFile();

protected:
    /**    \brief copy constructor */
    CBEFile(CBEFile &src);

public:
    virtual CBETarget* GetTarget();

    virtual void Write(CBEContext *pContext);

    virtual bool CreateBackEnd(CFEOperation *pFEOperation, 
	CBEContext *pContext);
    virtual bool CreateBackEnd(CFEInterface *pFEInterface, 
	CBEContext *pContext);
    virtual bool CreateBackEnd(CFELibrary *pFELibrary, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEFile *pFEFile, CBEContext *pContext);

    virtual CBEFunction* FindFunction(string sFunctionName);

    virtual CBEClass* FindClass(string sClassName);
    virtual CBEClass* GetNextClass(vector<CBEClass*>::iterator &iter);
    virtual vector<CBEClass*>::iterator GetFirstClass();
    virtual void RemoveClass(CBEClass *pClass);
    virtual void AddClass(CBEClass *pClass);

    virtual CBENameSpace* FindNameSpace(string sNameSpaceName);
    virtual CBENameSpace* GetNextNameSpace(
	vector<CBENameSpace*>::iterator &iter);
    virtual vector<CBENameSpace*>::iterator GetFirstNameSpace();
    virtual void RemoveNameSpace(CBENameSpace *pNameSpace);
    virtual void AddNameSpace(CBENameSpace *pNameSpace);

    virtual void AddIncludedFileName(string sFileName, bool bIDLFile, 
	bool bIsStandardInclude, CObject* pRefObj = 0);
    virtual vector<CIncludeStatement*>::iterator GetFirstIncludeStatement();
    virtual CIncludeStatement* GetNextIncludeStatement(
	vector<CIncludeStatement*>::iterator & iter);

    virtual CBEFunction* GetNextFunction(vector<CBEFunction*>::iterator &iter);
    virtual vector<CBEFunction*>::iterator GetFirstFunction();
    virtual void RemoveFunction(CBEFunction *pFunction);
    virtual void AddFunction(CBEFunction *pFunction);

    virtual bool IsOfFileType(int nFileType);
    virtual bool HasFunctionWithUserType(string sTypeName, 
	CBEContext *pContext);

    virtual int GetSourceLineEnd();

protected:
    virtual void WriteFunction(CBEFunction *pFunction, 
	CBEContext *pContext) = 0;
    virtual void WriteDefaultIncludes(CBEContext *pContext);
    virtual void WriteInclude(CIncludeStatement* pInclude, 
	CBEContext *pContext);
    virtual void WriteNameSpace(CBENameSpace *pNameSpace, 
	CBEContext *pContext) = 0;
    virtual void WriteClass(CBEClass *pClass, CBEContext *pContext) = 0;
    virtual void WriteIntro(CBEContext *pContext);
    virtual int GetFunctionCount(void);

    virtual void WriteHelperFunctions(CBEContext *pContext);

    virtual void CreateOrderedElementList(void);
    void InsertOrderedElement(CObject *pObj);

protected:
    /** \var vector<CIncludeStatement*> m_vIncludedFiles
     *  \brief contains the names of the included files
     *
     * This is an array of strings, because a file may not only include
     * CBEFile-represented files, but also other files. For instance C-header
     * files. Therefore it is more convenient to store the names of the files,
     * than to create CBEFiles for the C-header files as well.
     */
    vector<CIncludeStatement*> m_vIncludedFiles;
    /** \var vector<CBEClass*> m_vClasses
     *  \brief contains all classes, which belong to this file
     */
    vector<CBEClass*> m_vClasses;
    /** \var vector<CBENameSpace*> m_vNameSpaces
     *  \brief contains all namespaces, which belong to this file
     */
    vector<CBENameSpace*> m_vNameSpaces;
    /** \var vector<CBEFunction*> m_vFunctions
     *  \brief contains the functions, which belong to this file
     */
    vector<CBEFunction*> m_vFunctions;
    /** \var int m_nFileType
     *  \brief contains the type of the file
     */
    int m_nFileType;
    /** \var vector<CObject*> m_vOrderedElements
     *  \brief contains ordered list of elements
     */
    vector<CObject*> m_vOrderedElements;
};

#endif // !__DICE_BEFILE_H__
