/**
 *	\file	dice/src/be/BEUnionCase.h
 *	\brief	contains the declaration of the class CBEUnionCase
 *
 *	\date	01/15/2002
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
#ifndef __DICE_BEUNIONCASE_H__
#define __DICE_BEUNIONCASE_H__

#include "be/BETypedDeclarator.h"
#include "Vector.h"

class CFEUnionCase;
class CBEContext;
class CBEExpression;
class CBETypedDeclarator;

/**	\class CBEUnionCase
 *	\ingroup backend
 *	\brief the back-end union case (part of the union type)
 */
class CBEUnionCase : public CBETypedDeclarator
{
DECLARE_DYNAMIC(CBEUnionCase);
// Constructor
  public:
	/**	\brief constructor
	 */
    CBEUnionCase();
    virtual ~ CBEUnionCase();

  protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
    CBEUnionCase(CBEUnionCase & src);

  public:
    virtual void RemoveLabel(CBEExpression * pLabel);
    virtual void WriteDeclaration(CBEFile * pFile, CBEContext * pContext);
    virtual CBEExpression *GetNextLabel(VectorElement * &pIter);
    virtual VectorElement *GetFirstLabel();
    virtual void AddLabel(CBEExpression * pLabel);
    virtual bool CreateBackEnd(CFEUnionCase * pFEUnionCase, CBEContext * pContext);
    virtual bool IsDefault();

  protected:
	/**	\var bool m_bDefault
	 *	\brief true is default branch
	 */
     bool m_bDefault;
	/**	\var Vector m_vCaseLabels
	 *	\brief contains a list of expressions, or is empoty if default
	 */
    Vector m_vCaseLabels;
};

#endif	// !__DICE_BEUNIONCASE_H__
