/**
 *    \file    dice/src/fe/FETypeSpec.cpp
 *    \brief   contains the implementation of the class CFETypeSpec
 *
 *    \date    01/31/2001
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "fe/FETypeSpec.h"
#include "fe/FESimpleType.h"
#include "fe/FEUserDefinedType.h"
#include "fe/FEStructType.h"
#include "fe/FEUnionType.h"
#include "fe/FEFile.h"
#include "fe/FEDeclarator.h"

// needed for Error function
#include "Compiler.h"
#include "Messages.h"
#include <cassert>

CFETypeSpec::CFETypeSpec(unsigned int nType)
    : m_Attributes(NULL, NULL)
{
    m_nType = nType;
}

CFETypeSpec::CFETypeSpec(CFETypeSpec & src)
    : CFEInterfaceComponent(src),
    m_Attributes(NULL, NULL)
{
    m_nType = src.m_nType;
}

/** cleans up the type spec object */
CFETypeSpec::~CFETypeSpec()
{
    // nothing to clean up
}

/** \brief test a type whether it is a constructed type or not
 *  \return true if it is a constructed type, false if not
 */
bool CFETypeSpec::IsConstructedType()
{
    // not a constructed type -> return false
    return false;
}

/** \brief test if a type is a pointered type
 *  \return true if it is a pointered type, false if not
 */
bool CFETypeSpec::IsPointerType()
{
    // not a pointered type -> return false
    return false;
}

/** \brief add attributes to a type
 *  \param pAttributes
 */
void CFETypeSpec::AddAttributes(vector<CFEAttribute*> *pAttributes)
{
    m_Attributes.Add(pAttributes);
}
