/**
 *    \file    dice/src/fe/FEUserDefinedType.cpp
 *    \brief   contains the implementation of the class CFEUserDefinedType
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

#include "FEUserDefinedType.h"
#include "FETypedDeclarator.h"
#include "FEConstructedType.h"
#include "FEFile.h"
#include "FELibrary.h"

// needed for Error function
#include "Compiler.h"

CFEUserDefinedType::CFEUserDefinedType(string sName)
: CFETypeSpec(TYPE_USER_DEFINED)
{
    m_sName = sName;
}

CFEUserDefinedType::CFEUserDefinedType(CFEUserDefinedType & src)
: CFETypeSpec(src)
{
    m_sName = src.m_sName;
}

/** cleans up the user defined type */
CFEUserDefinedType::~CFEUserDefinedType()
{

}

/**
 *    \brief creates a copy of this class
 *    \return the copy of this class
 */
CObject *CFEUserDefinedType::Clone()
{
    return new CFEUserDefinedType(*this);
}

/**
 *    \brief returns the name of the type
 *    \return the name of the type
 */
string CFEUserDefinedType::GetName()
{
    return m_sName;
}

/** \brief check consistency
 *  \return false if error occured, true if everything is fine
 *
 * A user-defined type is consistent if it exists somewhere.
 */
bool CFEUserDefinedType::CheckConsistency()
{
    CFEFile *pFile = dynamic_cast<CFEFile*>(GetRoot());
    assert(pFile);
    if (m_sName.empty())
    {
        CCompiler::GccError(this, 0, "A user defined type without a name.");
        return false;
    }
    // the user defined type can also reference an interface
    CFELibrary *pFELibrary = GetSpecificParent<CFELibrary>();
    if ((m_sName.find("::") != string::npos) ||
        (!pFELibrary))
    {
        if (pFile->FindInterface(m_sName))
            return true;
    }
    else
    {
        if (pFELibrary->FindInterface(m_sName))
            return true;
    }

    // test if type has really been defined
    if (!(pFile->FindUserDefinedType(m_sName)))
    {
        CCompiler::GccError(this, 0,
                    "User defined type \"%s\" not defined.",
                    m_sName.c_str());
        return false;
    }
    return true;
}

/** serialize this object
 *    \param pFile the file to serialize from/to
 */
void CFEUserDefinedType::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        pFile->PrintIndent("<type>%s</type>\n", m_sName.c_str());
    }
}

/**    \brief should we ignore to resolve this user defined type
 *    \return true if this user type should be ignored
 *
 * We check the parent typed decl for the ignore attribute. If we have no parent type
 * decl or no attribute is found return false.
 */
bool CFEUserDefinedType::Ignore()
{
    CFEConstructedType *pStruct = GetSpecificParent<CFEConstructedType>();
    if (!pStruct)
    return false;
    CFEBase *pTypedDecl = (CFEBase *) (pStruct->GetParent());
    if (!(dynamic_cast<CFETypedDeclarator*>(pTypedDecl)))
    return false;
    if (((CFETypedDeclarator *) pTypedDecl)->FindAttribute(ATTR_IGNORE))
    return true;
    return false;
}

/** \brief get the type of the original type
 *  \return the type of the type this type is an alias for
 */
TYPESPEC_TYPE CFEUserDefinedType::GetOriginalType()
{
    CFEFile *pFile = dynamic_cast<CFEFile*>(GetRoot());
    assert(pFile);
    assert(!m_sName.empty());
    CFETypedDeclarator *pTypedef = pFile->FindUserDefinedType(m_sName);
    assert(pTypedef);
    CFETypeSpec* pType = pTypedef->GetType();
    assert(pType);
    return pType->GetType();
}
