/**
 *	\file	dice/src/fe/FEUnionType.h
 *	\brief	contains the declaration of the class CFEUnionType
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
#ifndef __DICE_FE_FEUNIONTYPE_H__
#define __DICE_FE_FEUNIONTYPE_H__

#include "fe/FEConstructedType.h"
#include "CString.h"

class Vector;
class VectorElement;
class CFEUnionCase;

/**	\class CFEUnionType
 *	\ingroup frontend
 *	\brief represents a union
 */
class CFEUnionType : public CFEConstructedType
{
DECLARE_DYNAMIC(CFEUnionType);

// standard constructor/destructor
public:
	/** \brief constructor for object of union type
	 *  \param pSwitchType the tyoe of the switch variable
	 *  \param sSwitchVar the name of the switch variable
	 *  \param pUnionBody the "real" union
	 *  \param sUnionName the name of the union
	 */
	CFEUnionType(CFETypeSpec *pSwitchType,
		String sSwitchVar,
		Vector *pUnionBody,
		String sUnionName = "");
	/** \brief constructor for object of union type
	 *  \param pUnionBody the elements of the union
        *
	 * This is a "non-encapsulated" union, which does not have a switch argument in
	 * the union statement, but receives the switch type and variable via attributes.
	 */
	CFEUnionType(Vector *pUnionBody); // n_e Type
	virtual ~CFEUnionType();

// Operations
public:
	bool IsCORBA();
	void SetCORBA();
	virtual void Serialize(CFile *pFile);
	virtual bool CheckConsistency();
	virtual CObject* Clone();
	virtual bool IsNEUnion();
	virtual CFEUnionCase* GetNextUnionCase(VectorElement* &iter);
	virtual VectorElement* GetFirstUnionCase();
	virtual String GetUnionName();
	virtual String GetSwitchVar();
	virtual CFETypeSpec* GetSwitchType();

protected:
	/** a copy construtor used for the tagged union class */
	CFEUnionType(CFEUnionType& src); // copy constructor for tagged union

// attribute
protected:
	/**	\var bool m_bNE
	 *	\brief shows if this is a NE union (slightly different syntax)
	 */
	bool m_bNE;
	/**	\var bool m_bCORBA
	 *	\brief true if the class was part of CORBA IDL
	 */
	bool m_bCORBA;
	/**	\var CFETypeSpec *m_pSwitchType
	 *	\brief the type of the switch argument
	 */
	CFETypeSpec *m_pSwitchType;
	/**	\var String m_sSwitchVar
	 *	\brief the name of the switch variable
	 */
	String m_sSwitchVar;
	/**	\var String m_sUnionName
	 *	\brief the name of the union
	 */
	String m_sUnionName;
	/**	\var Vector *m_pUnionBody
	 *	\brief the elements of the union (it's body)
	 */
	Vector *m_pUnionBody;
};

#endif /* __DICE_FE_FEUNIONTYPE_H__ */

