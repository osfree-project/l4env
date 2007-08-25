/**
 *    \file    dice/src/fe/FEBase.cpp
 *  \brief   contains the implementation of the class CFEBase
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

#include "fe/FEBase.h"
#include "fe/FEFile.h"
// #include "Compiler.h"

#include <string>
#include <typeinfo>
#include <iostream>

/////////////////////////////////////////////////////////////////////////////
// Base class

CFEBase::CFEBase(CObject * pParent)
: CObject(pParent), m_pParentContext(0)
{
}

CFEBase::CFEBase(CFEBase & src)
: CObject(src)
{
    m_pParentContext = src.m_pParentContext;
}

/** cleans up the base object */
CFEBase::~CFEBase()
{
    // do not delete parent !
}

/** \brief returns the root file object
 *  \return the root object
 *
 * This function climbs the chain of parents up until it found the top level
 * file.  If it is a file itself and has no parent its the top level file
 * itself.
 */
CFEFile *CFEBase::GetRoot()
{
    CObject *pParent = this;
    while (pParent &&
	pParent->GetParent())
//     {
// 	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
// 	    "CFEBase::GetRoot: parent @ %p is %s\n", pParent, typeid(*pParent).name());
	pParent = pParent->GetParent();
//     }
//     CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
// 	"CFEBase::GetRoot: parent is @ %p, return %p\n",
// 	pParent, dynamic_cast<CFEFile*>(pParent));
    return dynamic_cast<CFEFile*>(pParent);
}

/** copies the object
 *  \return a reference to the new base object
 */
CObject *CFEBase::Clone()
{
    return new CFEBase(*this);
}

/** \brief print the object to a string
 *  \return a string with the content of the object
 */
std::string CFEBase::ToString()
{
    // empty ecause this object is nothing
    return std::string();
}
