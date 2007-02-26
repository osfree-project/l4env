/**
 *    \file    dice/src/fe/FEUnionType.cpp
 *    \brief   contains the implementation of the class CFEUnionType
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

#include "fe/FEUnionType.h"
#include "fe/FETaggedUnionType.h"
#include "fe/FEUnionCase.h"
#include "Compiler.h"
#include "File.h"

CFEUnionType::CFEUnionType(CFETypeSpec * pSwitchType,
    string sSwitchVar,
    vector<CFEUnionCase*> *pUnionBody,
    string sUnionName)
: CFEConstructedType(TYPE_UNION)
{
    m_bNE = false;
    m_bCORBA = false;
    m_pSwitchType = pSwitchType;
    if (pUnionBody)
        m_vUnionBody.swap(*pUnionBody);
    else
        m_bForwardDeclaration = false;
    vector<CFEUnionCase*>::iterator iter;
    for (iter = m_vUnionBody.begin(); iter != m_vUnionBody.end(); iter++)
    {
        (*iter)->SetParent(this);
    }
}

CFEUnionType::CFEUnionType(vector<CFEUnionCase*> * pUnionBody)
: CFEConstructedType(TYPE_UNION)
{
    m_bNE = true;
    m_bCORBA = false;
    m_pSwitchType = 0;
    if (pUnionBody)
        m_vUnionBody.swap(*pUnionBody);
    else
        m_bForwardDeclaration = true;
    vector<CFEUnionCase*>::iterator iter;
    for (iter = m_vUnionBody.begin(); iter != m_vUnionBody.end(); iter++)
    {
        (*iter)->SetParent(this);
    }
}

CFEUnionType::CFEUnionType(CFEUnionType & src)
: CFEConstructedType(src)
{
    m_bNE = src.m_bNE;
    m_bCORBA = src.m_bCORBA;
    m_sSwitchVar = src.m_sSwitchVar;
    m_sUnionName = src.m_sUnionName;
    if (src.m_pSwitchType != 0)
    {
        m_pSwitchType = (CFETypeSpec *) (src.m_pSwitchType->Clone());
        m_pSwitchType->SetParent(this);
    }
    else
        m_pSwitchType = 0;
    vector<CFEUnionCase*>::iterator iter = src.m_vUnionBody.begin();
    for (; iter != src.m_vUnionBody.end(); iter++)
    {
        CFEUnionCase *pNew = (CFEUnionCase*)((*iter)->Clone());
        m_vUnionBody.push_back(pNew);
        pNew->SetParent(this);
    }
}

/** cleans up a union type object */
CFEUnionType::~CFEUnionType()
{
    if (m_pSwitchType)
        delete m_pSwitchType;
    while (!m_vUnionBody.empty())
    {
        delete m_vUnionBody.back();
        m_vUnionBody.pop_back();
    }
}

/** retrieves the type of the switch variable
 *    \return the type of the switch variable
 */
CFETypeSpec *CFEUnionType::GetSwitchType()
{
    return m_pSwitchType;
}

/** retrieves the name of the switch variable
 *    \return the name of the switch variable
 */
string CFEUnionType::GetSwitchVar()
{
    return m_sSwitchVar;
}

/** retrieves the name of the union
 *    \return an identifier containing the name of the union
 */
string CFEUnionType::GetUnionName()
{
    return m_sUnionName;
}

/** retrives a pointer to the first union case
 *    \return an iterator which points to the first union case object
 */
vector<CFEUnionCase*>::iterator CFEUnionType::GetFirstUnionCase()
{
    return m_vUnionBody.begin();
}

/** \brief retrieves the next union case object
 *  \param iter the iterator, which points to the next union case object
 *  \return the next union case object
 */
CFEUnionCase *CFEUnionType::GetNextUnionCase(vector<CFEUnionCase*>::iterator &iter)
{
    if (iter == m_vUnionBody.end())
        return 0;
    return *iter++;
}

/** checks if this is a non-encapsulated union
 *    \return true if this object is a non-encapsulated union
 */
bool CFEUnionType::IsNEUnion()
{
    return m_bNE;
}

/** creates a copy of this object
 *    \return a reference to a new union type object
 */
CObject *CFEUnionType::Clone()
{
    return new CFEUnionType(*this);
}

/** \brief checks the integrity of an union
 *  \return true if everything is fine
 *
 * A union is consistent if it has a union body and the elements of this body are
 * consistent.
 */
bool CFEUnionType::CheckConsistency()
{
    if (m_vUnionBody.empty())
    {
        CCompiler::GccError(this, 0, "A union without members is not allowed.");
        return false;
    }
    vector<CFEUnionCase*>::iterator iter = GetFirstUnionCase();
    CFEUnionCase *pUnionCase;
    while ((pUnionCase = GetNextUnionCase(iter)) != 0)
    {
        if (!(pUnionCase->CheckConsistency()))
            return false;
    }
    return true;
}

/** serialize this object
 *    \param pFile the file to serialize to/from
 */
void CFEUnionType::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        pFile->PrintIndent("<union_type>\n");
        pFile->IncIndent();
        if (!GetUnionName().empty())
            pFile->PrintIndent("<name>%s</name>\n", GetUnionName().c_str());
        if (GetSwitchType() != 0)
        {
            pFile->PrintIndent("<switch_type>\n");
            pFile->IncIndent();
            GetSwitchType()->Serialize(pFile);
            pFile->DecIndent();
            pFile->PrintIndent("</switch_type>\n");
        }
        if (!GetSwitchVar().empty())
        {
            pFile->PrintIndent("<switch_var>%s</switch_var>\n",
                    GetSwitchVar().c_str());
        }
        SerializeMembers(pFile);
        pFile->DecIndent();
        pFile->PrintIndent("</union_type>\n");
    }
}

/** serialize members of this object
 *    \param pFile the file to serialize to/from
 */
void CFEUnionType::SerializeMembers(CFile *pFile)
{
    vector<CFEUnionCase*>::iterator iter = GetFirstUnionCase();
    CFEBase *pElement;
    while ((pElement = GetNextUnionCase(iter)) != 0)
    {
        pElement->Serialize(pFile);
    }
}

/**    \brief allows to differentiate between CORBA IDL and other IDLs
 */
void CFEUnionType::SetCORBA()
{
    m_bCORBA = true;
}

/**    \brief test if this was part of CORBA IDL
 *    \return true if this was part of CORBA IDL
 */
bool CFEUnionType::IsCORBA()
{
    return m_bCORBA;
}
