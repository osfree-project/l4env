/**
 *    \file    dice/src/fe/FEUnionCase.cpp
 *  \brief   contains the implementation of the class CFEUnionCase
 *
 *    \date    01/31/2001
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "FEUnionCase.h"
#include "FETypedDeclarator.h"
#include "FEExpression.h"
#include "Compiler.h"
#include "Visitor.h"
#include <iostream>

CFEUnionCase::CFEUnionCase()
: m_UnionCaseLabelList(0, this)
{
    m_bDefault = false;
    m_pUnionArm = 0;
}

CFEUnionCase::CFEUnionCase(CFETypedDeclarator * pUnionArm,
    vector<CFEExpression*>* pCaseLabels)
: m_UnionCaseLabelList(pCaseLabels, this)
{
    m_bDefault = (!pCaseLabels) ? true : false;
    m_pUnionArm = pUnionArm;
}

CFEUnionCase::CFEUnionCase(CFEUnionCase & src)
: CFEBase(src),
    m_UnionCaseLabelList(src.m_UnionCaseLabelList)
{
    m_bDefault = src.m_bDefault;
    CLONE_MEM(CFETypedDeclarator, m_pUnionArm);
    m_UnionCaseLabelList.Adopt(this);
}

/** cleans up the union case object */
CFEUnionCase::~CFEUnionCase()
{
    if (m_pUnionArm)
        delete m_pUnionArm;
}

/** creates a copy of this object
 *  \return a copy of this object
 */
CObject* CFEUnionCase::Clone()
{
    return new CFEUnionCase(*this);
}

/** \brief accept the iterations of the visitors
 *  \param v reference to the current visitor
 */
void
CFEUnionCase::Accept(CVisitor& v)
{
    v.Visit(*this);
}
