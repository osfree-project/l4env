/**
 *	\file	dice/src/fe/FETypedDeclarator.h 
 *	\brief	contains the declaration of the class CFETypedDeclarator
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
#ifndef __DICE_FE_FETYPEDDECLARATOR_H__
#define __DICE_FE_FETYPEDDECLARATOR_H__

// Typed declarator types
enum TYPEDDECL_TYPE {
	TYPEDECL_NONE		 = 0x00,	/**< empty typed declarator (invalid) */
	TYPEDECL_VOID		 = 0x01,	/**< void typed declarator (empty member branch, etc.) */
	TYPEDECL_EXCEPTION	 = 0x02,	/**< exception declarator */
	TYPEDECL_PARAM		 = 0x03,	/**< parameter declarator */
	TYPEDECL_FIELD		 = 0x04,	/**< field declarator */
	TYPEDECL_TYPEDEF	 = 0x05,	/**< typedef declarator */
	TYPEDECL_MSGBUF		 = 0x06		/**< is a message buffer type */
};

#include "fe/FEInterfaceComponent.h"
#include "fe/FEAttribute.h"
#include "fe/FEDeclarator.h"

class CFETypeSpec;

/**	\class CFETypedDeclarator
 *	\ingroup frontend
 *	\brief represents a typed declarator
 *
 * A typed declarator is a declared variable with a type and attributes.
 */
class CFETypedDeclarator : public CFEInterfaceComponent
{
DECLARE_DYNAMIC(CFETypedDeclarator);

// standard constructor/destructor
public:
	/** \brief constructor for the typed declarator
	 *  \param nType the type of the declarator (parameter, exception, etc)
	 *  \param pType the type of the declared variables
	 *  \param pDeclarators the variables belonging to this declaration
	 *  \param pTypeAttributes the attributes, associated with the type of this declaration
        */
    CFETypedDeclarator(TYPEDDECL_TYPE nType,
			CFETypeSpec *pType, 
			Vector *pDeclarators,
			Vector *pTypeAttributes = 0);
	virtual ~CFETypedDeclarator();

protected:
	/** \brief copy constructor
	 *	\param src the source to copy from
	 */
	CFETypedDeclarator(CFETypedDeclarator &src);

// operations
public:
	virtual void Serialize(CFile *pFile);
	virtual bool CheckConsistency();
	virtual void AddDeclarator(CFEDeclarator *pDecl);
	virtual void AddAttribute(CFEAttribute *pNewAttr);
	virtual void RemoveAttribute(ATTR_TYPE eAttrType);
	virtual bool RemoveDeclarator(CFEDeclarator *pDeclarator);
	virtual CObject* Clone();
	virtual CFETypeSpec* ReplaceType(CFETypeSpec *pNewType);
	virtual TYPEDDECL_TYPE GetTypedDeclType();
	virtual CFEDeclarator* FindDeclarator(String sName);
	virtual CFEAttribute* FindAttribute(ATTR_TYPE eAttrType);
	virtual CFEAttribute* GetNextAttribute(VectorElement* &iter);
	virtual VectorElement* GetFirstAttribute();
	virtual CFEDeclarator* GetNextDeclarator(VectorElement* &iter);
	virtual VectorElement* GetFirstDeclarator();
	virtual CFETypeSpec* GetType();
	virtual bool IsTypedef();

// attributes
protected:
	/**	\var TYPEDDECL_TYPE m_nType
	 *	\brief the type of the declarator
	 */
	TYPEDDECL_TYPE m_nType;
	/**	\var Vector *m_pTypeAttributes
	 *	\brief the attributes of the declarator
	 */
    Vector *m_pTypeAttributes;
	/**	\var Vector *m_pDeclarators
	 *	\brief the variable names
	 */
    Vector *m_pDeclarators;
	/**	\var CFETypeSpec *m_pType
	 *	\brief the type of the declarator
	 */
    CFETypeSpec *m_pType;
};

#endif /* __DICE_FE_FETYPEDDECLARATOR_H__ */

