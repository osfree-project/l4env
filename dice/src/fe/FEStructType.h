/**
 *	\file	dice/src/fe/FEStructType.h 
 *	\brief	contains the declaration of the class CFEStructType
 *
 *	\date	01/31/2001
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
#ifndef __DICE_FE_FESTRUCTTYPE_H__
#define __DICE_FE_FESTRUCTTYPE_H__

#include "fe/FEConstructedType.h"
#include "fe/FETypedDeclarator.h"

/**	\class CFEStructType
 *	\ingroup frontend
 *	\brief represent the struct type
 */
class CFEStructType : public CFEConstructedType
{
DECLARE_DYNAMIC(CFEStructType);

// standard constructor/destructor
public:
	/** constructs a struct object
	 *	\param pMembers the members of the struct object
	 */
	CFEStructType(Vector *pMembers);
	virtual ~CFEStructType();

protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CFEStructType(CFEStructType &src);

// Operations
public:
	virtual void Serialize(CFile *pFile);
	virtual bool CheckConsistency();
	virtual CFETypedDeclarator* FindMember(String sName);
	virtual CObject* Clone();
	virtual CFETypedDeclarator* GetNextMember(VectorElement* &iter);
	virtual VectorElement* GetFirstMember();

// Attributes
protected:
	/**	\var Vector *m_pMembers
	 *	\brief the members of the structure
	 */
	Vector *m_pMembers;
};

#endif /* __DICE_FE_FESTRUCTTYPE_H__ */

