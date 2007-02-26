/**
 *    \file    dice/src/fe/FETaggedStructType.cpp
 *    \brief   contains the implementation of the class CFETaggedStructType
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

#include "fe/FETaggedStructType.h"
#include "fe/FEInterface.h"
#include "fe/FELibrary.h"
#include "fe/FEFile.h"

#include "Compiler.h"

CFETaggedStructType::CFETaggedStructType(string sTag,
    vector<CFETypedDeclarator*> * pMembers)
: CFEStructType(pMembers)
{
    m_nType = TYPE_TAGGED_STRUCT;
    m_sTag = sTag;
}

CFETaggedStructType::CFETaggedStructType(CFETaggedStructType & src)
: CFEStructType(src)
{
    m_sTag = src.m_sTag;
}

/** cleans up the tagge struct object */
CFETaggedStructType::~CFETaggedStructType()
{

}

/** \brief retrieves the tag
 *    \return the tag of the struct
 */
string CFETaggedStructType::GetTag()
{
    return m_sTag;
}

/** \brief copys the struct object
 *    \return a referenced to a new tagged struct object
 */
CObject *CFETaggedStructType::Clone()
{
    return new CFETaggedStructType(*this);
}

/** \brief checks consitency
 *  \return false if error occurs, true if everything is fine
 *
 * A tagged struct is consistent if a) it has members and all members
 * are consistent or b) it has no members and the actual type is consitent.
 * (no members cannot mean forward declaration, since it is allowed to
 *  specify an empty struct)
 */
bool CFETaggedStructType::CheckConsistency()
{
    if (!m_bForwardDeclaration)
        return CFEStructType::CheckConsistency();

    /* no members: try to find typedef for this */
    CFETypedDeclarator *pType;
    CFEConstructedType *pTagType = 0;
    CFEInterface *pFEInterface = GetSpecificParent<CFEInterface>();
    if (pFEInterface &&
        ((pType = pFEInterface->FindUserDefinedType(m_sTag)) != 0))
        return pType->CheckConsistency();
    if (pFEInterface &&
        ((pTagType = pFEInterface->FindTaggedDecl(m_sTag)) != 0) &&
        (pTagType != this))
        return pTagType->CheckConsistency();

    CFELibrary *pFELibrary = GetSpecificParent<CFELibrary>();
    while (pFELibrary)
    {
        if ((pType = pFELibrary->FindUserDefinedType(m_sTag)) != 0)
            return pType->CheckConsistency();
        if (((pTagType = pFELibrary->FindTaggedDecl(m_sTag)) != 0) &&
            (pTagType != this))
            return pTagType->CheckConsistency();
        pFELibrary = pFELibrary->GetSpecificParent<CFELibrary>();
    }

    CFEFile *pFEFile = dynamic_cast<CFEFile*>(GetRoot());
    if (pFEFile && ((pType = pFEFile->FindUserDefinedType(m_sTag)) != 0))
        return pType->CheckConsistency();
    if (pFEFile && ((pTagType = pFEFile->FindTaggedDecl(m_sTag)) != 0) &&
        (pTagType != this))
        return pTagType->CheckConsistency();

    CCompiler::GccError(this, 0, "Used type \"%s\" not defined.",
        m_sTag.c_str());
    return false;
}

/** serialize this object
 *    \param pFile the file to serialize to/from
 */
void CFETaggedStructType::SerializeMembers(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        pFile->PrintIndent("<tag>%s</tag>\n", GetTag().c_str());
    }
    CFEStructType::SerializeMembers(pFile);
}
