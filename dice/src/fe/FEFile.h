/**
 *	\file	dice/src/fe/FEFile.h 
 *	\brief	contains the declaration of the class CFEFile
 *
 *	\date	01/31/2001
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
#ifndef __DICE_FE_FEFILE_H__
#define __DICE_FE_FEFILE_H__

#include "FEBase.h"

class CFETypedDeclarator;
class CFEConstDeclarator;
class CFEConstructedType;
class CFEInterface;
class CFELibrary;

/** \class CFEFile
 *	\ingroup frontend
 *	\brief represents an idl file
 */
class CFEFile : public CFEBase  
{
DECLARE_DYNAMIC(CFEFile);

// constructor/desctructor
public:
	/** constructs a idl file representation */
	CFEFile(String sFileName, String sPath, int nIncludeLevel, int nStdInclude = 0);
	virtual ~CFEFile();

protected:
	/** \brief copy constructor
	 *	\param src the source to copy from
	 */
	CFEFile(CFEFile &src);

// Operations
public:
	virtual String GetFullFileName();
	virtual bool IsStdIncludeFile();
	virtual int GetTypedefCount(bool bCountIncludes = true);
	virtual int GetConstantCount(bool bCountIncludes = true);
	virtual bool IsIDLFile();
	virtual void Serialize(CFile *pFile);
	virtual void Dump();
	virtual bool CheckConsistency();
	virtual String GetFileNameWithoutExtension();
	virtual bool HasExtension(String sExtension);
	virtual String GetFileName();

    virtual CFEConstructedType* FindTaggedDecl(String sName);
	virtual CFEConstructedType* GetNextTaggedDecl(VectorElement* &iter);
	virtual VectorElement* GetFirstTaggedDecl();
	virtual void AddTaggedDecl(CFEConstructedType *pTaggedDecl);

	virtual CFEConstDeclarator* FindConstDeclarator(String sName);
	virtual CFEConstDeclarator* GetNextConstant(VectorElement* &iter);
	virtual VectorElement* GetFirstConstant();
	virtual void AddConstant(CFEConstDeclarator* pConstant);

	virtual CFETypedDeclarator* FindUserDefinedType(String sName);
    virtual CFETypedDeclarator* FindUserDefinedType(const char* sName);
    virtual CFETypedDeclarator* GetNextTypedef(VectorElement* &iter);
	virtual VectorElement* GetFirstTypedef();
	virtual void AddTypedef(CFETypedDeclarator* pTypedef);

	virtual CFELibrary* FindLibrary(String sName);
    virtual CFELibrary* FindLibrary(const char* sName);
	virtual CFELibrary* GetNextLibrary(VectorElement* &iter);
	virtual VectorElement* GetFirstLibrary();
	virtual void AddLibrary(CFELibrary *pLibrary);

	virtual CFEInterface* FindInterface(String sName);
    virtual CFEInterface* FindInterface(const char* sName);
	virtual CFEInterface* GetNextInterface(VectorElement* &iter);
	virtual VectorElement* GetFirstInterface();
	virtual void AddInterface(CFEInterface *pInterface);

	virtual CFEFile* GetNextIncludeFile(VectorElement* &iter);
	virtual VectorElement* GetFirstIncludeFile();
	virtual void AddChild(CFEFile *pNewChild);

	virtual int GetIncludeLevel();
	virtual bool IsIncluded();
	virtual CObject* Clone();

// Attributes
protected:
	/**	\var Vector m_vTaggedDecls
	 *	\brief the tagged struct and union declarations
	 */
	Vector m_vTaggedDecls;
	/**	\var Vector m_vConstants
	 *	\brief the constants of this file
	 */
	Vector m_vConstants;
	/**	\var Vector m_vTypedefs
	 *	\brief the type definitions in this file
	 */
	Vector m_vTypedefs;
	/**	\var Vector m_vLibraries
	 *	\brief the libraries in this file
	 */
	Vector m_vLibraries;
	/**	\var Vector m_vInterfaces
	 *	\brief the interfaces in this file
	 */
	Vector m_vInterfaces;
	/**	\var Vector m_vChildFiles
	 *	\brief the child files (included files)
	 */
	Vector m_vChildFiles;
	/** \var String m_sFileName
	 *	\brief contains the file name of the component
	 */
	String m_sFileName;
	/**	\var String m_sFilenameWithoutExtension
	 *	\brief the file name without the extension
	 */
	String m_sFilenameWithoutExtension;
	/**	\var String m_sFileExtension
	 *	\brief the extension of the file name
	 */
	String m_sFileExtension;
	/**	\var String m_sFileWithPath
	 *	\brief the file-name with the complete path
	 */
	String m_sFileWithPath;
	/**	\var int m_nIncludeLevel
	 *	\brief contains the include level of the component
	 *
	 * This value is situated in the interface component class, because most
	 * of the components of an interface may occure in the top level of a file,
	 * thus they need to know, which include level they are in, because they have
	 * no parent interface to ask.
	 */
	int m_nIncludeLevel;
	/**	\var int m_nStdInclude
	 *	\brief set to 1 if this file is a standard include file (#include <...>)
	 *
	 * A standard include file is a file included by using "<" ">" instead of '"'.
	 * This option is used with the notstdinc option.
	 */
	int m_nStdInclude;
};

#endif // __DICE_FE_FEFILE_H__
