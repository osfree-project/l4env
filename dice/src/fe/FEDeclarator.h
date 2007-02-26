/**
 *	\file	dice/src/fe/FEDeclarator.h 
 *	\brief	contains the declaration of the class CFEDeclarator
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
#ifndef __DICE_FE_FEDECLARATOR_H__
#define __DICE_FE_FEDECLARATOR_H__

// Declarator types
enum DECL_TYPE {
	DECL_NONE		= 0,	// for void constructor
	DECL_ATTR_VAR	= 1,
	DECL_VOID		= 2,	// for no Declarator in list
	DECL_ARRAY		= 3,	// CFEArrayDeclarator
	DECL_IDENTIFIER	= 4,	// simple declarator
	DECL_ENUM		= 5,	// enumeration's decl
	DECL_DECLARATOR	= 6	
};

#include "defines.h"
#include "fe/FEIdentifier.h"

/** \class CFEDeclarator
 *	\ingroup frontend
 *	\brief describes an declarator
 *
 * This class is used to describe a declarator. A declarator can be, for instance,
 * a variable declaration, or a parameter.
 */
class CFEDeclarator : public CFEIdentifier
{
DECLARE_DYNAMIC(CFEDeclarator);
// copy constructor
protected:
	/** \brief constructs a declarator object (copy constructor)
	 *  \param src the source for this object */
	CFEDeclarator(CFEDeclarator &src);

// standard constructor/destructor
public:
	/** \brief default constructor for a declarator object
	 *  \param nType the type of this declarator
	 */
    CFEDeclarator(DECL_TYPE nType);
	/** \brief constructs a declarator object
	 *  \param nType the type of this declarator
	 *  \param sName the name of this declarator
	 *  \param nNumStars the number of starisks in front of the declarator
	 *  \param nBitfields the bits this declarator should use
	 */
    CFEDeclarator(DECL_TYPE nType, String sName, int nNumStars = 0, int nBitfields = 0);
    virtual ~CFEDeclarator();

// operations
public:
	virtual void Serialize(CFile *pFile);
	virtual void SetBitfields(int nBitfields);
	virtual int GetBitfields();
	virtual CObject* Clone();
	virtual bool IsReference();
	virtual DECL_TYPE GetType();
	virtual int GetStars();
    virtual void SetStars(int nNumStars);

protected:
    virtual void SetType(DECL_TYPE nNewType);

// attributes
protected:
	/** \var int m_nBitfields
	 *	\brief the bit-fields of a struct member
	 */
	int m_nBitfields;
	/** \var int m_nNumStars
	 *	\brief how many asterisks appear in declaration?
	 */
    int m_nNumStars;
	/**	\var DECL_TYPE m_nType
	 *	\brief the type of the declaration (e.g. array declaration)
	 */
    DECL_TYPE m_nType;

    friend class CFETypedDeclarator;
};

#endif /* __DICE_FE_FEDECLARATOR_H__ */

