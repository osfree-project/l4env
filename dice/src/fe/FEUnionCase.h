/**
 *	\file	dice/src/fe/FEUnionCase.h 
 *	\brief	contains the declaration of the class CFEUnionCase
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
#ifndef __DICE_FE_FEUNIONCASE_H__
#define __DICE_FE_FEUNIONCASE_H__

#include "defines.h"
#include "fe/FEBase.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEExpression.h"

/**	\class CFEUnionCase
 *	\ingroup frontend
 *	\brief represents a single case branch of a union
 */
class CFEUnionCase : public CFEBase
{
DECLARE_DYNAMIC(CFEUnionCase);

// standard constructor/destructor
public:
	/** standard constructor for union case object */
	CFEUnionCase();
	/** constructor for union case object
	 *	\param pUnionArm the corresponding union arm (a type declarator)
	 *	\param pCaseLabels the labels of the bolonging case statement(s) */
    CFEUnionCase(CFETypedDeclarator *pUnionArm, Vector *pCaseLabels = 0);
    virtual ~CFEUnionCase();

protected:
	/** \brief copy constructor
	 *	\param src the source to copy from
	 */
	CFEUnionCase(CFEUnionCase &src);

// operations
public:
	virtual void Serialize(CFile *pFile);
	virtual bool CheckConsistency();
	virtual bool IsDefault();
	virtual CObject* Clone();
	virtual CFEExpression* GetNextUnionCaseLabel(VectorElement* &iter);
	virtual VectorElement* GetFirstUnionCaseLabel();
	virtual CFETypedDeclarator* GetUnionArm();

// attributes
protected:
	/**	\var bool m_bDefault
	 *	\brief shows, whether this is the default branch
	 */
    bool m_bDefault;
	/**	\var CFETypedDeclarator *m_pUnionArm
	 *	\brief the variable declaration, which hides in this branch
	 */
    CFETypedDeclarator *m_pUnionArm;
	/**	\var Vector *m_pUnionCaseLabelList
	 *	\brief the case labels (if not default) - should be constant values
	 */
    Vector *m_pUnionCaseLabelList; // of type const expression
};

#endif /* __DICE_FE_FEUNIONCASE_H__ */

