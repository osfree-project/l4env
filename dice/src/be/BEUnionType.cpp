/**
 *    \file    dice/src/be/BEUnionType.cpp
 *    \brief   contains the implementation of the class CBEUnionType
 *
 *    \date    01/15/2002
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

#include "be/BEUnionType.h"
#include "be/BEContext.h"
#include "be/BEUnionCase.h"
#include "be/BEFile.h"
#include "be/BEDeclarator.h"
#include "be/BETypedef.h"
#include "be/BEDeclarator.h"
#include "be/BEExpression.h"

#include "fe/FETaggedUnionType.h"
#include "fe/FEFile.h"
#include "Attribute-Type.h"

#include "Compiler.h"

CBEUnionType::CBEUnionType()
{
    m_pSwitchVariable = 0;
    m_bCORBA = false;
    m_bCUnion = false;
    m_pUnionName = 0;
}

CBEUnionType::CBEUnionType(CBEUnionType & src)
: CBEType(src)
{
    m_sTag = src.m_sTag;
    m_bCORBA = src.m_bCORBA;
    m_bCUnion = src.m_bCUnion;
    if (src.m_pSwitchVariable)
    {
        m_pSwitchVariable = (CBETypedDeclarator*)src.m_pSwitchVariable->Clone();
        m_pSwitchVariable->SetParent(this);
    }
    else
        m_pSwitchVariable = 0;
    if (src.m_pUnionName)
    {
        m_pUnionName = (CBEDeclarator*)src.m_pUnionName->Clone();
        m_pUnionName->SetParent(this);
    }
    else
        m_pUnionName = 0;
    vector<CBEUnionCase*>::iterator iter = src.m_vUnionCases.begin();
    for (; iter != src.m_vUnionCases.end(); iter++)
    {
        CBEUnionCase *pNew = (CBEUnionCase*)((*iter)->Clone());
        m_vUnionCases.push_back(pNew);
        pNew->SetParent(this);
    }
}

/**    \brief destructor of this instance */
CBEUnionType::~CBEUnionType()
{
    if (m_pUnionName)
        delete m_pUnionName;
    if (m_pSwitchVariable)
        delete m_pSwitchVariable;
    while (!m_vUnionCases.empty())
    {
        delete m_vUnionCases.back();
        m_vUnionCases.pop_back();
    }
}

/**    \brief creates this class' part of the back-end
 *    \param pFEType the respective type to crete from
 *    \param pContext the context of the write operation
 *    \return true if the code generation was successful
 *
 * This implementation calls the base class to perform basic initialization first.
 */
bool CBEUnionType::CreateBackEnd(CFETypeSpec * pFEType, CBEContext * pContext)
{
    // sets m_sName to "union"
    if (!CBEType::CreateBackEnd(pFEType, pContext))
    {
        VERBOSE("CBEUnionType::CreateBE failed because base type could not be created\n");
        return false;
    }

    CFEUnionType *pFEUnion = (CFEUnionType *) pFEType;
    // iterate over members
    vector<CFEUnionCase*>::iterator iterUC = pFEUnion->GetFirstUnionCase();
    CFEUnionCase *pFEUnionCase;
    while ((pFEUnionCase = pFEUnion->GetNextUnionCase(iterUC)) != 0)
    {
        CBEUnionCase *pUnionCase = pContext->GetClassFactory()->GetNewUnionCase();
        AddUnionCase(pUnionCase);
        if (!pUnionCase->CreateBackEnd(pFEUnionCase, pContext))
        {
            RemoveUnionCase(pUnionCase);
            delete pUnionCase;
            VERBOSE("CBEUnionType::CreateBE failed because union case could not be added\n");
            return false;
        }
    }
    // set switch type and variable
    // if switch-type == 0 and IsNEUnion() is true, then this is a C Union
    m_bCUnion =  pFEUnion->IsNEUnion();
    if (!(pFEUnion->GetSwitchType()) && !m_bCUnion)
    {
        CCompiler::GccError(pFEUnion, 0, "Union is neither C nor IDL union!");
        return false;
    }
    else
    {
        if (pFEUnion->GetSwitchType())
        {
            m_pSwitchVariable = pContext->GetClassFactory()->GetNewTypedDeclarator();
            m_pSwitchVariable->SetParent(this);
            // first create type
            CBEType *pSwitchType =  pContext->GetClassFactory()->GetNewType(pFEUnion->GetSwitchType()->GetType());
            pSwitchType->SetParent(m_pSwitchVariable);
            if (!pSwitchType->CreateBackEnd(pFEUnion->GetSwitchType(), pContext))
            {
                delete pSwitchType;
                delete m_pSwitchVariable;
                m_pSwitchVariable = 0;
                VERBOSE("CBEUnionType::CreateBE failed because switch type could not be created\n");
                return false;
            }
            // then create name
            string sSwitchName = pFEUnion->GetSwitchVar();
            if (sSwitchName.empty())
                sSwitchName = pContext->GetNameFactory()->GetSwitchVariable();
            // and finally the variable
            if (!m_pSwitchVariable->CreateBackEnd(pSwitchType, sSwitchName, pContext))
            {
                delete pSwitchType;
                delete m_pSwitchVariable;
                m_pSwitchVariable = 0;
                VERBOSE("CBEUnionType:: CreateBE failed bacause switch could not be added\n");
                return false;
            }
            if (pFEUnion->GetUnionName().empty())
            {
                // union name is conforming to CORBA language mapping "_u"
                m_pUnionName = pContext->GetClassFactory()->GetNewDeclarator();
                m_pUnionName->SetParent(this);
                if (!m_pUnionName->CreateBackEnd(string("_u"), 0, pContext))
                {
                    delete m_pUnionName;
                    m_pUnionName = 0;
                    delete m_pSwitchVariable;
                    m_pSwitchVariable = 0;
                    VERBOSE("CBEUnionType::CreateBE failed because union name could not be created\n");
                    return false;
                }
            }
        }
    }
    // set tag
    if (dynamic_cast<CFETaggedUnionType*>(pFEType))
    {
        // see if we can find the original struct
        string sTag = ((CFETaggedUnionType*)pFEType)->GetTag();
        CFEFile *pFERoot = dynamic_cast<CFEFile*>(pFEType->GetRoot());
        assert(pFERoot);
        CFEConstructedType *pFETaggedDecl = pFERoot->FindTaggedDecl(sTag);
        if (pFETaggedDecl)
            m_sTag = pContext->GetNameFactory()->GetTypeName(pFETaggedDecl, sTag, pContext);
        else
            m_sTag = sTag;
    }
    // get union name (DCE specific)
    if (!pFEUnion->GetUnionName().empty())
    {
        m_pUnionName = pContext->GetClassFactory()->GetNewDeclarator();
        m_pUnionName->SetParent(this);
        if (!m_pUnionName->CreateBackEnd(pFEUnion->GetUnionName(), 0, pContext))
        {
            delete m_pUnionName;
            m_pUnionName = 0;
            VERBOSE("%s failed because union name (DCE) could not be created\n", __PRETTY_FUNCTION__);
            return false;
        }
    }
    // check for CORBA style
    m_bCORBA = pFEUnion->IsCORBA();
    return true;
}

/** \brief creates a union
 *  \param sTag the tag of the union
 *  \param pSwitchType the type of the switch statement
 *  \param sSwitchName the name of the switch variable
 *  \param bCUnion true if this is a C style union (without switch)
 *  \param sUnionName DCE specific union name
 *  \param bCORBA true if CORBA style union
 *  \param pContext the context of the create process
 *  \return true if succcessful
 */
bool CBEUnionType::CreateBackEnd(string sTag, CBEType *pSwitchType,
    string sSwitchName, bool bCUnion, string sUnionName, bool bCORBA,
    CBEContext *pContext)
{
    m_sName = pContext->GetNameFactory()->GetTypeName(TYPE_UNION, false, pContext);
    if (m_sName.empty())
    {
        // user defined type overloads this function -> m_sName.c_str()
        // should always be set
        VERBOSE("%s failed, because no type name could be assigned for (%d)\n",
            __PRETTY_FUNCTION__, TYPE_UNION);
        return false;
    }
    m_nFEType = TYPE_UNION;

    // set switch type and variable
    // if switch-type == 0 and IsNEUnion() is true, then this is a C Union
    m_bCUnion =  bCUnion;
    if (!pSwitchType && !m_bCUnion)
    {
        CCompiler::GccError(0, 0, "Union is neither C nor IDL union!");
        return false;
    }
    else
    {
        if (pSwitchType)
        {
            m_pSwitchVariable = pContext->GetClassFactory()->GetNewTypedDeclarator();
            m_pSwitchVariable->SetParent(this);
            // first create type
            pSwitchType->SetParent(m_pSwitchVariable);
            // then create name
            if (sSwitchName.empty())
                sSwitchName = pContext->GetNameFactory()->GetSwitchVariable();
            // and finally the variable
            if (!m_pSwitchVariable->CreateBackEnd(pSwitchType, sSwitchName, pContext))
            {
                delete m_pSwitchVariable;
                m_pSwitchVariable = 0;
                VERBOSE("%s failed, bacause switch could not be added\n",
                    __PRETTY_FUNCTION__);
                return false;
            }
            if (sUnionName.empty())
                // union name is conforming to CORBA language mapping "_u"
                sUnionName = "_u";
        }
    }
    // set tag
    m_sTag = sTag;
    // get union name (DCE specific)
    if (!sUnionName.empty())
    {
        m_pUnionName = pContext->GetClassFactory()->GetNewDeclarator();
        m_pUnionName->SetParent(this);
        if (!m_pUnionName->CreateBackEnd(sUnionName, 0, pContext))
        {
            delete m_pUnionName;
            m_pUnionName = 0;
            VERBOSE("%s failed because union name (DCE) could not be created\n",
                __PRETTY_FUNCTION__);
            return false;
        }
    }
    // check for CORBA style
    m_bCORBA = bCORBA;
    return true;
}

/**    \brief adds a new union case to the vector
 *    \param pUnionCase the new union case
 */
void CBEUnionType::AddUnionCase(CBEUnionCase * pUnionCase)
{
    if (!pUnionCase)
        return;
    m_vUnionCases.push_back(pUnionCase);
    pUnionCase->SetParent(this);
}

/**    \brief removes a union case from the union case vector
 *    \param pCase the union case to remove
 */
void CBEUnionType::RemoveUnionCase(CBEUnionCase * pCase)
{
    if (!pCase)
        return;
    vector<CBEUnionCase*>::iterator iter;
    for (iter = m_vUnionCases.begin(); iter != m_vUnionCases.end(); iter++)
    {
        if (*iter == pCase)
        {
            m_vUnionCases.erase(iter);
            return;
        }
    }
}

/**    \brief retrieves a pointer to the first union case element
 *    \return a pointer to the first union case
 */
vector<CBEUnionCase*>::iterator CBEUnionType::GetFirstUnionCase()
{
    return m_vUnionCases.begin();
}

/**    \brief retrieves a reference to the next union case
 *    \param iter the pointer to the next union case
 *    \return a reference to the next union case
 */
CBEUnionCase *CBEUnionType::GetNextUnionCase(vector<CBEUnionCase*>::iterator &iter)
{
    if (iter == m_vUnionCases.end())
        return 0;
    return *iter++;
}

/**    \brief writes the union to the target file
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * A union is rather simple:
 * union &lt;tag&gt;
 * {
 *  &lt;member list&gt;
 * }
 *
 * But since the IDL prescribes a switch variable to determine which of the union members to send.
 *
 * CORBA C mapping defines the translation of IDL union into C struct with nested union (or struct):
 *
 * typedef struct
 * {
 *  &lt;switch type&gt; "_d";
 *  union
 *  {
 *    &lt;member list&gt;
 *  } _u;
 * } &lt;tag&gt;;
 *
 * Because the DCE IDL has a differnt understanding of the tag than the CORBA IDL we might also write:
 *
 * typedef struct &lt;tag&gt;
 * {
 *  &lt;switch type&gt; &lt;switch name&gt;;
 *  union
 *  {
 *   &lt;member list&gt;
 *  } &lt;union name&gt;;
 * }
 *
 * To distinguish the two there exists a boolean variable, which is set during creation.
 * we set the switch name if we have one; same with the union name.
 *
 * If we scanned a C style union, we do not construct a struct, but leave it as it is. If we would
 * change this, no one could use C unions...
 *
 *    \todo wrap union into struct
 */
void CBEUnionType::Write(CBEFile * pFile, CBEContext * pContext)
{
    // writes struct
    if (!m_bCUnion)
    {
        // since we cannot use the tag as struct name and
        // lates as union name, we have to add something to the tag
        // here. This is "_struct" for now.
        // XXX FIXME: make "_struct" name-factory string
        pFile->Print("struct");
        if (!m_bCORBA && (!m_sTag.empty()))
            pFile->Print(" %s_struct", m_sTag.c_str());
        pFile->Print("\n");
        pFile->PrintIndent("{\n");
        pFile->IncIndent();

        // write switch statement
        if (m_pSwitchVariable)
        {
            pFile->PrintIndent("");
            m_pSwitchVariable->WriteDeclaration(pFile, pContext);
            pFile->Print(";\n");
        }

        pFile->PrintIndent(""); // to avoid double indent if this branch is skipped
    }

    // write union
    pFile->Print("%s", m_sName.c_str());    // should be set to "union"
    if (!m_bCORBA && (!m_sTag.empty()))
        pFile->Print(" %s", m_sTag.c_str());
    if (m_vUnionCases.size() > 0)
    {
        pFile->Print("\n");
        pFile->PrintIndent("{\n");
        pFile->IncIndent();

        // write members
        vector<CBEUnionCase*>::iterator iterU = GetFirstUnionCase();
        CBEUnionCase *pCase;
        while ((pCase = GetNextUnionCase(iterU)) != 0)
        {
            pFile->PrintIndent("");
            pCase->WriteDeclaration(pFile, pContext);
            pFile->Print(";\n");
        }

        // close union
        pFile->DecIndent();
        if (m_pUnionName)
            pFile->PrintIndent("} %s;\n", m_pUnionName->GetName().c_str());
        else if (m_bCUnion)
            pFile->PrintIndent("} "); // a c style union has the "normal" name(declarator) after this type declaration
    //    else
    //        pFile->PrintIndent("} _u;\n");
        else
        {
            assert(false);
        }
    }

    // close struct
    if (!m_bCUnion)
    {
        pFile->DecIndent();
        pFile->PrintIndent("}");
        if (m_bCORBA && (!m_sTag.empty()))
            pFile->Print(" %s", m_sTag.c_str());
    }
 }


/**    \brief generates an exact copy of this class
 *    \return a reference to the new object
 */
CObject *CBEUnionType::Clone()
{
    return new CBEUnionType(*this);
}

/** \brief calculates the size of this union type
 *  \return the size in bytes
 *
 * The union type's size is the size of the biggest member and the switch type.
 */
int CBEUnionType::GetSize()
{
    int nSize = 0;
    vector<CBEUnionCase*>::iterator iter = GetFirstUnionCase();
    CBEUnionCase *pUnionCase;
    while ((pUnionCase = GetNextUnionCase(iter)) != 0)
    {
        int nUnionSize = pUnionCase->GetSize();
        // if we have one variable sized union member, the
        // whole union is variable sized, because we cannot pinpoint
        // its exact size to determine a message buffer size
        if (nUnionSize < 0)
            return nUnionSize;
        if (nSize < nUnionSize)
            nSize = nUnionSize;
    }
    if (m_pSwitchVariable)
        nSize += m_pSwitchVariable->GetType()->GetSize();
    return nSize;
}

/** \brief returns true if this is a C style union (without switch type)
 *  \return true if this is a C style union
 */
bool CBEUnionType::IsCStyleUnion()
{
    return m_bCUnion;
}

/** \brief returns the name of the switch variable
 *  \return the name of the switch variable
 *
 * The name of the switch variable is it's first declarator
 */
string CBEUnionType::GetSwitchVariableName()
{
    vector<CBEDeclarator*>::iterator iter = m_pSwitchVariable->GetFirstDeclarator();
    return (*iter)->GetName();
}
/** \brief returns the switch variable
 *  \return a reference to a typed declarator containing the switch variable
 */
CBETypedDeclarator* CBEUnionType::GetSwitchVariable()
{
    return m_pSwitchVariable;
}

/** \brief writes the name of the union
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This function is used to e.g. cast the message buffer. So we don't need the
 * whole type declaration, but the simple type name.
 * 'union &lt;tag&gt; { ... }' should be printed as 'union &lt;tag&gt;'   and
 * 'union { ... } &lt;union name&gt;;' as 'union &lt;union name&gt;'
 */
void CBEUnionType::WriteUnionName(CBEFile *pFile, CBEContext *pContext)
{
    pFile->Print("%s", m_sName.c_str());    // should be set to "union"
    if (!m_bCORBA && (!m_sTag.empty()))
        pFile->Print(" %s", m_sTag.c_str());
    if (m_pUnionName)
        pFile->PrintIndent(" %s", m_pUnionName->GetName().c_str());
}

/** \brief retrieves a reference to the union name
 *  \return reference to the union name declarator
 */
CBEDeclarator* CBEUnionType::GetUnionName()
{
    return m_pUnionName;
}

/** \brief counts the union cases
 *  \return the number of union cases
 */
int CBEUnionType::GetUnionCaseCount()
{
    return m_vUnionCases.size();
}

/** \brief checks if this is a constructed type
 *  \return true, because a union is usually regarded a constructed type
 */
bool CBEUnionType::IsConstructedType()
{
    return true;
}

/** \brief writes the cast for this type
 *  \param pFile the file to write to
 *  \param bPointer true if the cast should produce a pointer
 *  \param pContext the context of the write operation
 *
 * The cast of a union type is '(union tag)'.
 */
void CBEUnionType::WriteCast(CBEFile * pFile, bool bPointer, CBEContext * pContext)
{
    pFile->Print("(");
    if (m_sTag.empty())
    {
        // no tag -> we need a typedef to save us
        // the alias can be used for the cast
        CBETypedef *pTypedef = GetTypedef();
        assert(pTypedef);
        // get first declarator (without stars)
        vector<CBEDeclarator*>::iterator iterD = pTypedef->GetFirstDeclarator();
        CBEDeclarator *pDecl;
        while ((pDecl = pTypedef->GetNextDeclarator(iterD)) != 0)
        {
            if (pDecl->GetStars() <= (bPointer?1:0))
                break;
        }
        assert(pDecl);
        pFile->Print("%s", pDecl->GetName().c_str());
        if (bPointer && (pDecl->GetStars() == 0))
            pFile->Print("*");
    }
    else
    {
        pFile->Print("%s %s", m_sName.c_str(), m_sTag.c_str());
        if (bPointer)
            pFile->Print("*");
    }
    pFile->Print(")");
}

/** \brief tests if this union has the given tag
 *  \param sTag the tag to test for
 *  \return true if the given tag is the same as the member tag
 */
bool CBEUnionType::HasTag(string sTag)
{
    return (m_sTag == sTag);
}

/** \brief return the tag
 *  \return the tag
 */
string CBEUnionType::GetTag()
{
    return m_sTag;
}

/** \brief write the zero init code for a union
 *  \param pFile the file to write to
 *  \param pContext the context of this write operation
 *
 * init union similar to struct, but use only first member.
 */
void CBEUnionType::WriteZeroInit(CBEFile * pFile, CBEContext * pContext)
{
    if (IsCStyleUnion())
    {
        pFile->Print("{ ");
        pFile->IncIndent();
        vector<CBEUnionCase*>::iterator iter = GetFirstUnionCase();
        CBEUnionCase *pMember = GetNextUnionCase(iter);
        if (pMember)
        {
            // get type
            CBEType *pType = pMember->GetType();
            // get declarator
            vector<CBEDeclarator*>::iterator iterD = pMember->GetFirstDeclarator();
            CBEDeclarator *pDecl;
            while ((pDecl = pMember->GetNextDeclarator(iterD)) != 0)
            {
                // be C99 compliant:
                *pFile << pDecl->GetName() << " : ";
                if (pDecl->IsArray())
                    WriteZeroInitArray(pFile, pType, pDecl, pDecl->GetFirstArrayBound(), pContext);
                else if (pType)
                    pType->WriteZeroInit(pFile, pContext);
            }
        }
        pFile->DecIndent();
        pFile->Print(" }");
    }
}

/** \brief used to determine if this type writes a zero init string
 *  \return false
 */
bool CBEUnionType::DoWriteZeroInit()
{
    return false;
}

/** \brief if struct is variable size, it has to write the size
 *  \param pFile the file to write to
 *  \param pStack the declarator stack for constructed types with variable sized members
 *  \param pContext the context of teh write operation
 *
 * This is not the maximum of all cases, because this would mean that all cases
 * are of equal type (to determine their size), but they are not. Therefore we have to
 * write code which test the switch variable.
 * We can write it as recursive function.
 *
 * \todo what if default case is variable sized?
 */
void CBEUnionType::WriteGetSize(CBEFile * pFile,
    vector<CDeclaratorStackLocation*> *pStack,
    CBEContext * pContext)
{
    int nFixedSize = GetFixedSize();
    // first add union name
    CDeclaratorStackLocation *pLoc = 0;
    if (GetUnionName())
    {
        pLoc = new CDeclaratorStackLocation(GetUnionName());
        pLoc->SetIndex(-3);
        pStack->push_back(pLoc);
    }
    // build vector with var sized members
    vector<CBEUnionCase*> vVarSizedUnionCase;
    vector<CBEUnionCase*>::iterator iter = GetFirstUnionCase();
    CBEUnionCase *pCase;
    while ((pCase = GetNextUnionCase(iter)) != 0)
    {
        if (pCase->IsVariableSized())
            vVarSizedUnionCase.push_back(pCase);
    }
    // get first of var-sized members
    iter = vVarSizedUnionCase.begin();
    // call recursive write
    if (iter != vVarSizedUnionCase.end())
    {
        pFile->Print("( ");
        WriteGetMaxSize(pFile, &vVarSizedUnionCase, iter, pStack, pContext);
        pFile->Print(" : %d)", nFixedSize);
    }
    else
        // should never get here, becaus WriteGetSize is only called if Var-Sized
        pFile->Print("%d", nFixedSize);
    // remove the union's name
    if (GetUnionName())
    {
        pStack->pop_back();
        delete pLoc;
    }
}

/** \brief calculates the maximum size of the fixed sized mebers
 *  \return the maximum fixed size in bytes
 */
int CBEUnionType::GetFixedSize()
{
    int nSize = 0;
    vector<CBEUnionCase*>::iterator iter = GetFirstUnionCase();
    CBEUnionCase *pUnionCase;
    while ((pUnionCase = GetNextUnionCase(iter)) != 0)
    {
        int nUnionSize = pUnionCase->GetSize();
        // if this is negative size, then its variable sized
        // and we skip it here (the max operation will simply ignore it)
        if (nSize < nUnionSize)
            nSize = nUnionSize;
    }
    if (m_pSwitchVariable)
        nSize += m_pSwitchVariable->GetType()->GetSize();
    return nSize;
}

/** \brief writes the maximum of the variable sized members
 *  \param pFile the file to write to
 *  \param pMembers the vector containing the variable sized members
 *  \param iter the current position in the vector
 *  \param pStack contains the declarator stack for variables sized parameters
 *  \param pContext the context of the write operation
 */
void CBEUnionType::WriteGetMaxSize(CBEFile *pFile,
    const vector<CBEUnionCase*> *pMembers,
    vector<CBEUnionCase*>::iterator iter,
    vector<CDeclaratorStackLocation*> *pStack,
    CBEContext *pContext)
{
    assert(pMembers);
    assert(iter != pMembers->end());
    CBEUnionCase *pMember = *iter++;
    // always write '(<switch> == <case>)?(<member size>)'
    if (m_pSwitchVariable)
    {
        // pop union name, because switch belongs above union
        CDeclaratorStackLocation *topSave = pStack->back();
        pStack->pop_back();
        // write cases
        pFile->Print("(");
        vector<CBEExpression*>::iterator iterL = pMember->GetFirstLabel();
        CBEExpression *pLabel;
        bool bFirst = true;
        while ((pLabel = pMember->GetNextLabel(iterL)) != 0)
        {
            vector<CBEExpression*>::iterator iTemp = iterL;
            CBEExpression *pNextLabel = pMember->GetNextLabel(iTemp);
            if (pNextLabel || !bFirst)
                pFile->Print("(");
            // write switch
            vector<CBEDeclarator*>::iterator iterS = m_pSwitchVariable->GetFirstDeclarator();
            CBEDeclarator *pDecl = *iterS;
            CDeclaratorStackLocation *pLoc = new CDeclaratorStackLocation(pDecl);
            pStack->push_back(pLoc);
            CDeclaratorStackLocation::Write(pFile, pStack, false, false, pContext);
            pStack->pop_back();
            delete pLoc;
            pFile->Print(" == %d", pLabel->GetIntValue());
            if (pNextLabel || !bFirst)
                pFile->Print(")");
            if (pNextLabel)
                pFile->Print(" || ");
            bFirst = false;
        }
        pFile->Print(")?");
        // add union name
        pStack->push_back(topSave);
    }
    pFile->Print("(");
    WriteGetMemberSize(pFile, pMember, pStack, pContext);
    pFile->Print(")");
    // if we have successor, add ':(<max(next)>)'
    // otherwise nothing
    if (iter != pMembers->end())
    {
        pFile->Print(" : (");
        WriteGetMaxSize(pFile, pMembers, iter, pStack, pContext);
        pFile->Print(")");
    }
}

/** \brief writes the whole size string for a member
 *  \param pFile the file to write to
 *  \param pMember the member to write for
 *  \param pStack contains the declarator stack for variable sized parameters
 *  \param pContext the context of the write operation
 *
 * This code is taken from CBEMsgBufferType::WriteInitializationVarSizedParameters
 * if something is not working, check if something changed there as well.
 */
void CBEUnionType::WriteGetMemberSize(CBEFile *pFile,
    CBEUnionCase *pMember,
    vector<CDeclaratorStackLocation*> *pStack,
    CBEContext *pContext)
{
    // get size of switch variable
    if (m_pSwitchVariable)
    {
        int nSwitchSize = m_pSwitchVariable->GetType()->GetSize();
        pFile->Print("%d+", nSwitchSize);
    }
    pMember->WriteGetSize(pFile, pStack, pContext);
    if ((pMember->GetType()->GetSize() > 1) && !(pMember->IsString()))
    {
        pFile->Print("*sizeof");
        pMember->GetType()->WriteCast(pFile, false, pContext);
    }
    else if (pMember->IsString())
    {
        // add terminating zero
        pFile->Print("+1");
        bool bHasSizeAttr = pMember->HasSizeAttr(ATTR_SIZE_IS) ||
                pMember->HasSizeAttr(ATTR_LENGTH_IS) ||
                pMember->HasSizeAttr(ATTR_MAX_IS);
        if (!bHasSizeAttr)
            pFile->Print("+%d", pContext->GetSizes()->GetSizeOfType(TYPE_INTEGER));
    }
}

/** \brief test if this is a simple type
 *  \return false
 */
bool CBEUnionType::IsSimpleType()
{
    return false;
}
