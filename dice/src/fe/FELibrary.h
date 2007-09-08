/**
 *  \file    dice/src/fe/FELibrary.h
 *  \brief   contains the declaration of the class CFELibrary
 *
 *  \date    02/22/2001
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
#ifndef __DICE_FE_FELIBRARY_H__
#define __DICE_FE_FELIBRARY_H__

#include "FEFileComponent.h"
#include "Attribute-Type.h"
#include "template.h"
#include <string>
#include <vector>

class CFEIdentifier;
class CFETypedDeclarator;
class CFEConstDeclarator;
class CFETypedDeclarator;
class CFEinterface;
class CFEAttribute;

/**    \class CFELibrary
 *    \ingroup frontend
 *  \brief describes the front-end library
 */
class CFELibrary : public CFEFileComponent
{
	// standard constructor/destructor
public:
	/** constructs a library object
	 *  \param sName the name of the library
	 *  \param pAttributes the attributes of the library
	 *  \param pParent the parent of this lib
	 */
	CFELibrary(std::string sName, vector<CFEAttribute*> *pAttributes, CFEBase *pParent);
	virtual ~CFELibrary();

protected:
	/** \brief copy constructor
	 *  \param src the source to copy from
	 */
	CFELibrary(CFELibrary* src);

	// Operations
public:
	CFEInterface* FindInterface(std::string sName, CFELibrary *pStart = 0);
	CFETypedDeclarator* FindUserDefinedType(std::string sName);
	CFEConstDeclarator* FindConstant(std::string sName);
	CFELibrary* FindLibrary(std::string sName);
	CFEConstructedType* FindTaggedDecl(std::string sName);

	void AddSameLibrary(CFELibrary *pLibrary);
	void AddComponents(vector<CFEFileComponent*> *pComponents);

	virtual CObject* Clone();
	virtual void Accept(CVisitor&);
	virtual std::string GetName();
	bool Match(std::string sName);


	// Attributes
protected:
	/** \var std::string m_sLibName
	 *  \brief contains the library's name
	 */
	std::string m_sLibName;
	/** \var CFELibrary* m_pSameLibraryNext
	 *  \brief the same library in other files
	 */
	CFELibrary* m_pSameLibraryNext;
	/** \var CFELibrary* m_pSameLibraryPrev
	 *  \brief the same library in other files
	 */
	CFELibrary* m_pSameLibraryPrev;

public:
	/** \var CSearchableCollection<CFEAttribute> m_Attributes
	 *  \brief contains the library's attributes
	 */
	CSearchableCollection<CFEAttribute, ATTR_TYPE> m_Attributes;
	/** \var CSearchableCollection<CFEConstDeclarator> m_Constants
	 *  \brief constains the constants defined in the library
	 */
	CSearchableCollection<CFEConstDeclarator, std::string> m_Constants;
	/** \var CSearchableCollection<CFETypedDeclarator> m_Typedefs
	 *  \brief contains the typedefs of this library
	 */
	CSearchableCollection<CFETypedDeclarator, std::string> m_Typedefs;
	/** \var CSearchableCollection<CFEInterface> m_Interfaces
	 *  \brief contains the interfaces of this library
	 */
	CSearchableCollection<CFEInterface, std::string> m_Interfaces;
	/** \var CSearchableCollection<CFELibrary> m_Libraries
	 *  \brief contains the nested libraries
	 */
	CSearchableCollection<CFELibrary, std::string> m_Libraries;
	/** \var CSearchableCollection<CFEConstructedType> m_TaggedDeclarators
	 *  \brief contains the tagged constructed types of this library
	 */
	CSearchableCollection<CFEConstructedType, std::string> m_TaggedDeclarators;
};

#endif // __DICE_FE_FELIBRARY_H__
