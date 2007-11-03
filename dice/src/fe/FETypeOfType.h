/**
 *  \file   dice/src/fe/FETypeOfType.h
 *  \brief  contains the declaration of the class CFETypeOfType
 *
 *  \date   08/07/2007
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
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
#ifndef __DICE_FE_FETYPEOFTYPE_H__
#define __DICE_FE_FETYPEOFTYPE_H__

#include "fe/FEConstructedType.h"

class CFEExpression;

/** \class CFETypeOfType
 *  \ingroup frontend
 *  \brief represents the type of statement in the IDL
 */
class CFETypeOfType : public CFEConstructedType
{

// standard constructor/destructor
public:
    /** constructs a pipe type object
     *  \param pExpression the expression to get the type from */
    CFETypeOfType(CFEExpression *pExpression);
    virtual ~CFETypeOfType();

protected:
    /** \brief copy constrcutor
     *  \param src the source to copy from
     */
    CFETypeOfType(CFETypeOfType* src);

public:
	virtual CFETypeOfType* Clone();

// attributes
protected:
    /** \var CFEExpression * m_pExpression
     *  \brief the expression to get the type from
     */
    CFEExpression *m_pExpression;
};

#endif /* __DICE_FE_FETYPEOFTYPE_H__ */

