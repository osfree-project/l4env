/**
 *    \file    dice/src/fe/FEInterface.cpp
 *  \brief   contains the implementation of the class CFEInterface
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

#include "FEInterface.h"
#include "FEIdentifier.h"
#include "FEConstDeclarator.h"
#include "FETypedDeclarator.h"
#include "FEOperation.h"
#include "FEFile.h"
#include "FESimpleType.h"
#include "FEVersionAttribute.h"
#include "FEIntAttribute.h"
#include "FEConstructedType.h"
#include "FEStructType.h"
#include "FEEnumType.h"
#include "FEUnionType.h"
#include "FEAttributeDeclarator.h"
#include "FELibrary.h"
#include "Compiler.h"
#include "File.h"
#include "Visitor.h"
#include <string>
#include <iostream>
#include <stdexcept>
#include <cassert>

/////////////////////////////////////////////////////////////////////
// Interface stuff
CFEInterface::CFEInterface(vector<CFEAttribute*> * pIAttributes,
    string sIName,
    vector<CFEIdentifier*> *pIBaseNames,
    CFEBase *pParent)
: CFEFileComponent(pParent),
    m_Attributes(pIAttributes, this),
    m_Constants(0, this),
    m_AttributeDeclarators(0, this),
    m_TaggedDeclarators(0, this),
    m_Operations(0, this),
    m_Typedefs(0, this),
    m_BaseInterfaceNames(pIBaseNames, this),
    m_DerivedInterfaces(0, (CObject*)0),
    m_BaseInterfaces(0, (CObject*)0)
{
    m_sInterfaceName = sIName;
}

CFEInterface::CFEInterface(CFEInterface & src)
: CFEFileComponent(src),
    m_Attributes(src.m_Attributes),
    m_Constants(src.m_Constants),
    m_AttributeDeclarators(src.m_AttributeDeclarators),
    m_TaggedDeclarators(src.m_TaggedDeclarators),
    m_Operations(src.m_Operations),
    m_Typedefs(src.m_Typedefs),
    m_BaseInterfaceNames(src.m_BaseInterfaceNames),
    m_DerivedInterfaces(src.m_DerivedInterfaces),
    m_BaseInterfaces(src.m_BaseInterfaces)
{
    m_sInterfaceName = src.m_sInterfaceName;
    m_Attributes.Adopt(this);
    m_Constants.Adopt(this);
    m_AttributeDeclarators.Adopt(this);
    m_TaggedDeclarators.Adopt(this);
    m_Operations.Adopt(this);
    m_Typedefs.Adopt(this);
    m_BaseInterfaceNames.Adopt(this);
    // not for m_DerivedInterfaces, m_BaseInterfaces
}

/** \brief add components to this interface
 *  \param pComponents vector of components to add
 */
void
CFEInterface::AddComponents(vector<CFEInterfaceComponent*> *pComponents)
{
    if (pComponents)
    {
	vector<CFEInterfaceComponent*>::iterator iter;
	for (iter = pComponents->begin(); iter != pComponents->end(); iter++)
	{
	    if (!*iter)
		continue;
	    // parent is set in Add* functions
	    if (dynamic_cast<CFEConstDeclarator*>(*iter))
		m_Constants.Add((CFEConstDeclarator*)*iter);
	    else if (dynamic_cast<CFETypedDeclarator*>(*iter))
		m_Typedefs.Add((CFETypedDeclarator*) *iter);
	    else if (dynamic_cast<CFEOperation*>(*iter))
		m_Operations.Add((CFEOperation*)*iter);
	    else if (dynamic_cast<CFEConstructedType*>(*iter))
		m_TaggedDeclarators.Add((CFEConstructedType*)*iter);
	    else if (dynamic_cast<CFEAttributeDeclarator*>(*iter))
		m_AttributeDeclarators.Add((CFEAttributeDeclarator*)*iter);
	    else
		throw new std::invalid_argument("Unknown interface component");
	}
    }
}

/** destructs the interface and all its members */
CFEInterface::~CFEInterface()
{ }

/**
 *  \brief returns the name of the interface
 *  \return the interface's name
 *
 * This function redirects the request to the interface's header, which
 * contains all names and attributes of the interface.
 */
string CFEInterface::GetName()
{
    // if we got an identifier get it's name
    return m_sInterfaceName;
}

/** \brief try to match the given name with the own name
 *  \param sName the name to match
 *  \return true if matches, false otherwise
 */
bool CFEInterface::Match(string sName)
{
    return GetName() == sName;
}

/**
 *  \brief adds a reference to a base interface
 *  \param pBaseInterface the reference to the base interface
 *
 * Creates a new array for the references if none exists or adds the given reference
 * to the existing array (m_pBaseInterfaces).
 */
void CFEInterface::AddBaseInterface(CFEInterface * pBaseInterface)
{
    if (!pBaseInterface)
        return;
    m_BaseInterfaces.Add(pBaseInterface);
    pBaseInterface->m_DerivedInterfaces.Add(this);
}

/**
 *  \brief calculates the number of operations in this interface
 *  \param bCountBase true if the base interfaces should be counted too
 *  \return the number of operations (functions) in this interface
 *
 * This function is used for the enumeration of the operation identifiers, used
 * to identify, which function is called.
 */
int CFEInterface::GetOperationCount(bool bCountBase)
{
    int count = 0;
    if (bCountBase)
    {
        vector<CFEInterface*>::iterator iterI;
	for (iterI = m_BaseInterfaces.begin();
	     iterI != m_BaseInterfaces.end();
	     iterI++)
        {
            count += (*iterI)->GetOperationCount();
        }
    }
    // now count functions
    count += m_Operations.size();
    return count;
}

/**
 *  \brief tries to find a base interface
 *  \param sName the name of the base interface
 *  \return a reference to the searched interface, 0 if not found
 */
CFEInterface *CFEInterface::FindBaseInterface(string sName)
{
    if (sName.empty())
        return 0;
    vector<CFEInterface*>::iterator iter;
    for (iter = m_BaseInterfaces.begin();
	 iter != m_BaseInterfaces.end();
	 iter++)
    {
        if ((*iter)->GetName() == sName)
            return *iter;
    }
    return 0;
}

/** \brief the accept method for the visitors
 *  \param v reference to the current visitor
 *
 * Iterate the members of the interface and call the respective accept methods
 * there.
 */
void CFEInterface::Accept(CVisitor &v)
{
    v.Visit(*this);
    // iterate attribues
    vector<CFEAttribute*>::iterator iA;
    for (iA = m_Attributes.begin();
	 iA != m_Attributes.end();
	 iA++)
    {
	(*iA)->Accept(v);
    }
    // check constants
    vector<CFEConstDeclarator*>::iterator iC;
    for (iC = m_Constants.begin();
	 iC != m_Constants.end();
	 iC++)
            {
	(*iC)->Accept(v);
    }
    // iterate attribute declarators
    vector<CFEAttributeDeclarator*>::iterator iAD;
    for (iAD = m_AttributeDeclarators.begin();
	 iAD != m_AttributeDeclarators.end();
	 iAD++)
	{
	(*iAD)->Accept(v);
    }
    // iterate tagged declarators
    vector<CFEConstructedType*>::iterator iTD;
    for (iTD = m_TaggedDeclarators.begin();
	 iTD != m_TaggedDeclarators.end();
	 iTD++)
    {
	(*iTD)->Accept(v);
    }
    // iterate operations
    vector<CFEOperation*>::iterator iO;
    for (iO = m_Operations.begin();
	 iO != m_Operations.end();
	 iO++)
	{
	(*iO)->Accept(v);
    }
    // check typedefs
    vector<CFETypedDeclarator*>::iterator iT;
    for (iT = m_Typedefs.begin();
	 iT != m_Typedefs.end();
	 iT++)
    {
	(*iT)->Accept(v);
    }
}

/** \brief tests if this is a foward declaration
 *  \return true if it is
 *
 * A forward declaration contains no elements
 */
bool CFEInterface::IsForward()
{
    return m_Attributes.empty() &&
           m_Constants.empty() &&
	   m_Operations.empty() &&
	   m_TaggedDeclarators.empty() &&
	   m_Typedefs.empty();
}
