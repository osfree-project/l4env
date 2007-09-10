/**
 *  \file    dice/src/be/l4/L4FiascoBESrvLoopFunction.cpp
 *  \brief   contains the implementation of the class CL4FiascoBESrvLoopFunction
 *
 *  \date    02/10/2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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

#include "L4FiascoBESrvLoopFunction.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/TypeSpec-L4Types.h"
#include "be/BETypedDeclarator.h"
#include "be/BEWaitAnyFunction.h"
#include "be/BEDispatchFunction.h"
#include "Compiler.h"

CL4FiascoBESrvLoopFunction::CL4FiascoBESrvLoopFunction()
{ }

/** \brief destructor of target class */
CL4FiascoBESrvLoopFunction::~CL4FiascoBESrvLoopFunction()
{ }

/** \brief create this instance of a server loop function
 *  \param pFEInterface the interface to use as reference
 *  \return true if create was successful
 *
 * Create a message tag variable.
 */
void
CL4FiascoBESrvLoopFunction::CreateBackEnd(CFEInterface * pFEInterface, bool bComponentSide)
{
    CL4BESrvLoopFunction::CreateBackEnd(pFEInterface, bComponentSide);

    CBENameFactory *pNF = CBENameFactory::Instance();
    string sTagVar = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);
    string sTagType = pNF->GetTypeName(TYPE_MSGTAG, 0);
    AddLocalVariable(sTagType, sTagVar, 0, string("l4_msgtag(0,0,0,0)"));

    // reset the call variables
    CBEDeclarator *pDecl = m_LocalVariables.Find(sTagVar)->m_Declarators.First();
    if (m_pWaitAnyFunction)
	m_pWaitAnyFunction->SetCallVariable(pDecl->GetName(),
	    pDecl->GetStars(), pDecl->GetName());
    if (m_pReplyAnyWaitAnyFunction)
	m_pReplyAnyWaitAnyFunction->SetCallVariable(pDecl->GetName(),
	    pDecl->GetStars(), pDecl->GetName());
    if (m_pDispatchFunction)
	m_pDispatchFunction->SetCallVariable(pDecl->GetName(),
	    pDecl->GetStars(), pDecl->GetName());
}

