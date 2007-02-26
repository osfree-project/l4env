/**
 *	\file	dice/src/fe/FEArrayType.h 
 *	\brief	contains the declaration of the class CFEArrayType
 *
 *	\date	03/23/2001
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
 *
 */

/** preprocessing symbol to check header file */
#ifndef __DICE_FE_FEARRAYTYPE_H__
#define __DICE_FE_FEARRAYTYPE_H__

#include "fe/FETypeSpec.h"

class CFEExpression;

/**	\class CFEArrayType
 *	\ingroup frontend
 *	\brief represents sequenced CORBA types
 */
class CFEArrayType : public CFETypeSpec  
{
DECLARE_DYNAMIC(CFEArrayType);

public:
	/** simple constructor for array type object
	 *	\param pBaseType the base type of the sequence
	 *	\param pBound the maximum elements of the sequence */
	CFEArrayType(CFETypeSpec *pBaseType, CFEExpression *pBound = 0);
	virtual ~CFEArrayType();

protected:
	/** \brief copy constructor
	 *	\param src the source to copy from
	 */
	CFEArrayType(CFEArrayType &src);

// Operations
public:
	virtual void Serialize(CFile *pFile);
	virtual bool CheckConsistency();
	virtual CFEExpression* GetBound();
	virtual CFETypeSpec* GetBaseType();
	virtual CObject* Clone();

protected:
	/**	\var CFEExpression* m_pBound
	 *	\brief the element count of the "elements
	 */
	CFEExpression* m_pBound;
	/**	\var CFETypeSpec* m_pBaseType
	 *	\brief the base type for the array
	 */
	CFETypeSpec* m_pBaseType;
};

#endif // __DICE_FE_FEARRAYTYPE_H__
