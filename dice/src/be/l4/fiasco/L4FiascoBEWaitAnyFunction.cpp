/**
 *  \file    dice/src/be/l4/fiasco/L4FiascoBEWaitAnyFunction.cpp
 *  \brief   contains the implementation of the class CL4FiascoBEWaitAnyFunction
 *
 *  \date    08/24/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "L4FiascoBEWaitAnyFunction.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/TypeSpec-L4Types.h"
#include "be/BEClassFactory.h"
#include "be/BETypedDeclarator.h"
#include "Compiler.h"

CL4FiascoBEWaitAnyFunction::CL4FiascoBEWaitAnyFunction(bool bOpenWait, bool bReply)
: CL4BEWaitAnyFunction(bOpenWait, bReply)
{
}

/** \brief destructor of target class */
CL4FiascoBEWaitAnyFunction::~CL4FiascoBEWaitAnyFunction()
{
}

void
CL4FiascoBEWaitAnyFunction::AddBeforeParameters()
{
    CL4BEWaitAnyFunction::AddBeforeParameters();

    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sTagVar = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);
    string sTagType = pNF->GetTypeName(TYPE_MSGTAG, 0);

    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBETypedDeclarator *pParameter = pCF->GetNewTypedDeclarator();
    m_Parameters.Add(pParameter);
    pParameter->CreateBackEnd(sTagType, sTagVar, 1);
}
