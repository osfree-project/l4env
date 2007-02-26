/**
 *	\file	dice/src/fe/FELibrary.h 
 *	\brief	contains the declaration of the class CFELibrary
 *
 *	\date	02/22/2001
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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
#ifndef __DICE_FE_FELIBRARY_H__
#define __DICE_FE_FELIBRARY_H__

#include "fe/FEFileComponent.h"
#include "Vector.h"
#include "CString.h"

class CFEIdentifier;
class CFETypedDeclarator;
class CFEConstDeclarator;
class CFETypedDeclarator;
class CFEinterface;
class CFEAttribute;

/**	\class CFELibrary
 *	\ingroup frontend
 *	\brief describes the front-end library
 */
class CFELibrary : public CFEFileComponent
{
DECLARE_DYNAMIC(CFELibrary);

// standard constructor/destructor
public:
	/** constructs a library object
	 *	\param sName the name of the library
	 *	\param pAttributes the attributes of the library
	 *	\param pElements the elements (components) of the library*/
	CFELibrary(String sName, Vector *pAttributes, Vector *pElements);
	virtual ~CFELibrary();

protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CFELibrary(CFELibrary &src);

// Operations
public:
	virtual CFEInterface* FindInterface(String sName);
	virtual CFELibrary* FindLibrary(String sName);
	virtual void AddSameLibrary(CFELibrary *pLibrary);
	virtual void Serialize(CFile *pFile);
	virtual void Dump();
	virtual CFEConstDeclarator* FindConstant(String sName);
	virtual CFELibrary* GetNextLibrary(VectorElement*&iter);
	virtual VectorElement* GetFirstLibrary();
	virtual bool CheckConsistency();
	virtual CFETypedDeclarator* GetNextTypedef(VectorElement*&iter);
	virtual VectorElement* GetFirstTypedef();
	virtual CFEConstDeclarator* GetNextConstant(VectorElement*&iter);
	virtual VectorElement* GetFirstConstant();
	virtual CObject* Clone();
	virtual CFETypedDeclarator* FindUserDefinedType(String sName);
	virtual String GetName();
	virtual CFEAttribute* GetNextAttribute(VectorElement* &iter);
	virtual VectorElement* GetFirstAttribute();
	virtual CFEInterface* GetNextInterface(VectorElement* &iter);
	virtual VectorElement* GetFirstInterface();

	virtual void AddTypedef(CFETypedDeclarator *pFETypedef);
	virtual void AddInterface(CFEInterface *pFEInterface);
	virtual void AddConstant(CFEConstDeclarator *pFEConstant);
	virtual void AddLibrary(CFELibrary *pFELibrary);

	virtual void AddTaggedDecl(CFEConstructedType *pFETaggedDecl);
    virtual CFEConstructedType* GetNextTaggedDecl(VectorElement* &pIter);
    virtual VectorElement* GetFirstTaggedDecl();
    virtual CFEConstructedType* FindTaggedDecl(String sName);

// Attributes
protected:
	/**	\var Vector m_vAttributes
	 *	\brief contains the library's attributes
	 */
	Vector m_vAttributes;
	/**	\var String m_sLibName
	 *	\brief contains the library's name
	 */
	String m_sLibName;
	/**	\var Vector m_vSameLibrary
	 *	\brief the same library in other files
	 */
	Vector m_vSameLibrary;
	/** \var Vector m_vConstants
	 *  \brief constains the constants defined in the library
	 */
	Vector m_vConstants;
	/** \var Vector m_vTypedefs
	 *  \brief contains the typedefs of this library
	 */
	Vector m_vTypedefs;
	/** \var Vector m_vInterfaces
	 *  \brief contains the interfaces of this library
	 */
	Vector m_vInterfaces;
	/** \var Vector m_vLibraries
	 *  \brief contains the nested libraries
	 */
	Vector m_vLibraries;
	/** \var Vector m_vTaggedDeclarators
	 *  \brief contains the tagged constructed types of this library
	 */
	Vector m_vTaggedDeclarators;
};

#endif // __DICE_FE_FELIBRARY_H__
