/**
 *  \file    dice/src/fe/FEOperation.cpp
 *  \brief   contains the implementation of the class CFEOperation
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

#include "FEOperation.h"
#include "FEFile.h"
#include "FEUserDefinedType.h"
#include "FEStructType.h"
#include "FEIsAttribute.h"
#include "FETypedDeclarator.h"
#include "FEDeclarator.h"
#include "FEArrayDeclarator.h"
#include "FESimpleType.h"
#include "FEArrayType.h"
#include "FEInterface.h"
#include "FEAttribute.h"
#include "Compiler.h"
#include "Visitor.h"
#include <iostream>
#include <cassert>

CFEOperation::CFEOperation(CFETypeSpec * pReturnType, std::string sName,
	vector<CFETypedDeclarator*> * pParameters, vector<CFEAttribute*> * pAttributes,
	vector<CFEIdentifier*> * pRaisesDeclarators)
: CFEInterfaceComponent(static_cast<CObject*>(0)),
	m_Attributes(pAttributes, this),
    m_Parameters(pParameters, this),
    m_RaisesDeclarators(pRaisesDeclarators, this)
{
    m_pReturnType = pReturnType;
    m_sOpName = sName;
}

CFEOperation::CFEOperation(CFEOperation* src)
: CFEInterfaceComponent(src),
    m_Attributes(src->m_Attributes),
    m_Parameters(src->m_Parameters),
    m_RaisesDeclarators(src->m_RaisesDeclarators)
{
    m_sOpName = src->m_sOpName;
    CLONE_MEM(CFETypeSpec, m_pReturnType);

    m_Attributes.Adopt(this);
    m_Parameters.Adopt(this);
    m_RaisesDeclarators.Adopt(this);
}

/** the operation's destructor and of all its members */
CFEOperation::~CFEOperation()
{
    if (m_pReturnType)
        delete m_pReturnType;
}

/** \brief create a copy of this object
 *  \return reference to clone
 */
CFEOperation* CFEOperation::Clone()
{
	return new CFEOperation(this);
}

/**
 *  \brief retrieves the operations name
 *  \return the name of the operation
 */
string CFEOperation::GetName()
{
    return m_sOpName;
}

/**
 *  \brief tries to locate a parameter
 *  \param sName the name of the parameter
 *  \return a pointer to the parameter
 *
 * This function searches the operation's parameters for a parameter with the
 * specified name.
 */
CFETypedDeclarator *CFEOperation::FindParameter(std::string sName)
{
    if (m_Parameters.empty())
        return 0;
    if (sName.empty())
        return 0;

    // check for a structural seperator ("." or "->")
    string sBase, sMember;
    string::size_type iDot = sName.find('.');
    string::size_type iPtr = sName.find("->");
    string::size_type iUse;
    if ((iDot == string::npos) && (iPtr == string::npos))
        iUse = string::npos;
    else if ((iDot == string::npos) && (iPtr != string::npos))
        iUse = iPtr;
    else if ((iDot != string::npos) && (iPtr == string::npos))
        iUse = iDot;
    else
        iUse = (iDot < iPtr) ? iDot : iPtr;
    if ((iUse != string::npos) && (iUse > 0))
    {
        sBase = sName.substr(0, iUse);
        if (iUse == iDot)
            sMember = sName.substr(sName.length() - (iDot + 1));
        else
            sMember = sName.substr(sName.length() - (iDot + 2));
    }
    else
        sBase = sName;

    // iterate over parameter
    vector<CFETypedDeclarator*>::iterator iterP;
    for (iterP = m_Parameters.begin();
	 iterP != m_Parameters.end();
	 iterP++)
    {
        // if the parameter is the searched one
        if ((*iterP)->m_Declarators.Find(sBase))
        {
            // the declarator is constructed (it has '.' or '->'
            if ((iUse != string::npos) && (iUse > 0))
            {
                // if the found typed declarator has a constructed type (struct)
                // search for the second part of the name there
		CFEStructType *pStruct =
		    dynamic_cast<CFEStructType*>((*iterP)->GetType());
                if (pStruct)
                {
                    if (!pStruct->FindMember(sMember))
                    {
                        // no nested member with that name found in the member
                        // -> this must be an invalid name
                        return 0;
                    }
                }
            }
            // return the found typed declarator
            return *iterP;
        }
    }

    return 0;
}

/**
 *  \brief returns the return type
 *  \return the return type
 */
CFETypeSpec *CFEOperation::GetReturnType()
{
    return m_pReturnType;
}

/** \brief accepts the iterations of the visitors
 *  \param v reference to the visitor
 */
void CFEOperation::Accept(CVisitor& v)
{
	v.Visit(*this);
}
