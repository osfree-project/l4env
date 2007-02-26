/**
 *	\file	dice/src/be/BEFile.h
 *	\brief	contains the declaration of the class CBEFile
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

/** preprocessing symbol to check header file */
#ifndef __DICE_BEFILE_H__
#define __DICE_BEFILE_H__

#include "File.h"
#include "Vector.h"

class CBEContext;
class CBEFunction;
class CBETarget;
class CBEClass;
class CBENameSpace;

class CFEOperation;
class CFEInterface;
class CFELibrary;
class CFEFile;

/**	\class IncludeFile
 *	\ingroup backend
 *	\brief helper class tomanage included files
 */
class IncludeFile : public CObject
{
DECLARE_DYNAMIC(IncludeFile);
public:
	/** default constructor */
	IncludeFile();
	virtual ~IncludeFile();

public:
	/**	\var bool bIDLFile
	 *	\brief true if this is an IDL file
	 */
	bool bIDLFile;
	/**	\var String sFileName
	 *	\brief the name of the file to include
	 */
	String sFileName;
};

/**	\class CBEFile
 *	\ingroup backend
 *	\brief the output file class for the back-end classes
 *
 * This class unifies the common properties of the class CBEHeaderFile and CBEImplementationFile. These common
 * properties are that both contain functions and include files.
 */
class CBEFile : public CFile
{
DECLARE_DYNAMIC(CBEFile);
// Constructor
public:
	virtual void Write(CBEContext *pContext);
	/**	\brief constructor
	 */
	CBEFile();
	virtual ~CBEFile();

protected:
	/**	\brief copy constructor */
	CBEFile(CBEFile &src);

public:
    virtual CBETarget* GetTarget();

    virtual int Optimize(int nLevel, CBEContext *pContext);
    virtual bool IsIDLFile(int nIndex);

    virtual bool CreateBackEnd(CFEOperation *pFEOperation, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEInterface *pFEInterface, CBEContext *pContext);
    virtual bool CreateBackEnd(CFELibrary *pFELibrary, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEFile *pFEFile, CBEContext *pContext);

    virtual CBEFunction* FindFunction(String sFunctionName);

    virtual CBEClass* FindClass(String sClassName);
    virtual CBEClass* GetNextClass(VectorElement *&pIter);
    virtual VectorElement* GetFirstClass();
    virtual void RemoveClass(CBEClass *pClass);
    virtual void AddClass(CBEClass *pClass);

    virtual CBENameSpace* FindNameSpace(String sNameSpaceName);
    virtual CBENameSpace* GetNextNameSpace(VectorElement *&pIter);
    virtual VectorElement* GetFirstNameSpace();
    virtual void RemoveNameSpace(CBENameSpace *pNameSpace);
    virtual void AddNameSpace(CBENameSpace *pNameSpace);

    virtual String GetIncludedFileName(int nIndex);
    virtual int GetIncludedFileNameSize();
    virtual void AddIncludedFileName(String sFileName, bool bIDLFile);
    virtual CBEFunction* GetNextFunction(VectorElement *&pIter);
    virtual VectorElement* GetFirstFunction();
    virtual void RemoveFunction(CBEFunction *pFunction);
    virtual void AddFunction(CBEFunction *pFunction);
    virtual bool IsOfFileType(int nFileType);
    virtual bool HasFunctionWithUserType(String sTypeName, CBEContext *pContext);

protected:
    virtual void WriteFunctions(CBEContext *pContext);
    virtual void WriteIncludesAfterTypes(CBEContext *pContext);
    virtual void WriteIncludesBeforeTypes(CBEContext *pContext);
    virtual void WriteNameSpaces(CBEContext *pContext);
    virtual void WriteClasses(CBEContext *pContext);

protected:
    /**	\var Vector m_vIncludedFiles
     *	\brief contains the names of the included files
     *
     * This is an array of strings, because a file may not only include CBEFile-represented files, but also other
     * files. For instance C-header files. Therefore it is more convenient to store the names of the files, than to
     * create CBEFiles for the C-header files as well.
     */
    Vector m_vIncludedFiles;
    /**	\var Vector m_vClasses
     *	\brief contains all classes, which belong to this file
     */
    Vector m_vClasses;
    /** \var Vector m_vNameSpaces
     *  \brief contains all namespaces, which belong to this file
     */
    Vector m_vNameSpaces;
    /** \var Vector m_vFunctions
     *  \brief contains the functions, which belong to this file
     */
    Vector m_vFunctions;
    /** \var int m_nFileType
     *  \brief contains the type of the file
     */
    int m_nFileType;
};

#endif // !__DICE_BEFILE_H__
