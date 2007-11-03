/**
 *  \file    dice/src/fe/FEUnionCase.h
 *  \brief   contains the declaration of the class CFEUnionCase
 *
 *  \date    01/31/2001
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
#ifndef __DICE_FE_FEUNIONCASE_H__
#define __DICE_FE_FEUNIONCASE_H__

#include "fe/FEBase.h"
#include "fe/FEExpression.h"
#include "template.h"
#include <vector>

class CFETypedDeclarator;

/** \class CFEUnionCase
 *  \ingroup frontend
 *  \brief represents a single case branch of a union
 */
class CFEUnionCase : public CFEBase
{

	// standard constructor/destructor
public:
	/** standard constructor for union case object */
	CFEUnionCase();
	/** constructor for union case object
	 *  \param pUnionArm the corresponding union arm (a type declarator)
	 *  \param pCaseLabels the labels of the bolonging case statement(s) */
	CFEUnionCase(CFETypedDeclarator *pUnionArm,
		vector<CFEExpression*> *pCaseLabels = 0);
	virtual ~CFEUnionCase();

protected:
	/** \brief copy constructor
	 *  \param src the source to copy from
	 */
	CFEUnionCase(CFEUnionCase* src);

	// operations
public:
	virtual CFEUnionCase* Clone();
	virtual void Accept(CVisitor&);

	/** \brief test this union case for default
	 *  \return true if default case, false if not
	 *
	 * Returns the value of m_bDefault, which is set in the constructor. Usually
	 * all, but one union arm have a case label. The C unions (if included from a
	 * header file) have no case labels at all. Thus you can differentiate the
	 * C unions from IDL unions by testing all union case for default.
	 */
	bool IsDefault()
	{ return m_bDefault; }

	/** retrieves the union arm
	 *  \return the typed declarator, which is this union case's arm
	 */
	CFETypedDeclarator* GetUnionArm()
	{ return m_pUnionArm; }

	// attributes
protected:
	/** \var bool m_bDefault
	 *  \brief shows, whether this is the default branch
	 */
	bool m_bDefault;
	/** \var CFETypedDeclarator *m_pUnionArm
	 *  \brief the variable declaration, which hides in this branch
	 */
	CFETypedDeclarator *m_pUnionArm;

public:
	/** \var CCollection<CFEExpression> m_UnionCaseLabelList
	 *  \brief the case labels (if not default) - should be constant values
	 */
	CCollection<CFEExpression> m_UnionCaseLabelList;
};

#endif /* __DICE_FE_FEUNIONCASE_H__ */

