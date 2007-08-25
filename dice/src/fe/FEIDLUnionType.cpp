/**
 *    \file    dice/src/fe/FEIDLUnionType.cpp
 *    \brief   contains the implementation of the class CFEIDLUnionType
 *
 *    \date    03/14/2006
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006
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

#include "FEIDLUnionType.h"
#include "FEUnionCase.h"
#include "FEFile.h"
#include "Compiler.h"
#include <iostream>

CFEIDLUnionType::CFEIDLUnionType(std::string sTag,
    vector<CFEUnionCase*> *pUnionBody,
    CFETypeSpec * pSwitchType,
    std::string sSwitchVar,
    std::string sUnionName)
: CFEUnionType(sTag, pUnionBody)
{
    m_nType = TYPE_IDL_UNION;
    m_pSwitchType = pSwitchType;
    m_sSwitchVar = sSwitchVar;
    m_sUnionName = sUnionName;
}

CFEIDLUnionType::CFEIDLUnionType(CFEIDLUnionType & src)
: CFEUnionType(src)
{
    m_nType = TYPE_IDL_UNION;
    m_sSwitchVar = src.m_sSwitchVar;
    m_sUnionName = src.m_sUnionName;
    CLONE_MEM(CFETypeSpec, m_pSwitchType);
}

/** cleans up a union type object */
CFEIDLUnionType::~CFEIDLUnionType()
{
    if (m_pSwitchType)
        delete m_pSwitchType;
}

/** creates a copy of this object
 *  \return a reference to a new union type object
 */
CObject *CFEIDLUnionType::Clone()
{
    return new CFEIDLUnionType(*this);
}
