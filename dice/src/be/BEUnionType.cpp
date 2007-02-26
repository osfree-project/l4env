/**
 *	\file	dice/src/be/BEUnionType.cpp
 *	\brief	contains the implementation of the class CBEUnionType
 *
 *	\date	01/15/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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

IMPLEMENT_DYNAMIC(CBEUnionType);

CBEUnionType::CBEUnionType()
: m_vUnionCases(RUNTIME_CLASS(CBEUnionCase))
{
    m_pSwitchVariable = 0;
    m_bCORBA = false;
    m_bCUnion = false;
    m_pUnionName = 0;
    IMPLEMENT_DYNAMIC_BASE(CBEUnionType, CBEType);
}

CBEUnionType::CBEUnionType(CBEUnionType & src)
: CBEType(src),
  m_vUnionCases(RUNTIME_CLASS(CBEUnionCase))
{
    m_sTag = src.m_sTag;
    m_bCORBA = src.m_bCORBA;
    m_bCUnion = src.m_bCUnion;
    m_pSwitchVariable = src.m_pSwitchVariable;
    if (m_pSwitchVariable)
	m_pSwitchVariable->SetParent(this);
    m_pUnionName = src.m_pUnionName;
    m_vUnionCases.Add(&src.m_vUnionCases);
    m_vUnionCases.SetParentOfElements(this);
    IMPLEMENT_DYNAMIC_BASE(CBEUnionType, CBEType);
}

/**	\brief destructor of this instance */
CBEUnionType::~CBEUnionType()
{
    m_vUnionCases.DeleteAll();
    if (m_pUnionName)
        delete m_pUnionName;
}

/**	\brief creates this class' part of the back-end
 *	\param pFEType the respective type to crete from
 *	\param pContext the context of the write operation
 *	\return true if the code generation was successful
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
    VectorElement *pIter = pFEUnion->GetFirstUnionCase();
    CFEUnionCase *pFEUnionCase;
    while ((pFEUnionCase = pFEUnion->GetNextUnionCase(pIter)) != 0)
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
    // if switch-type == 0 and IsNEUnion() is true, than this is a C Union
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
            String sSwitchName = pFEUnion->GetSwitchVar();
            if (sSwitchName.IsEmpty())
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
            if (!(pFEUnion->GetUnionName()))
            {
                // union name is conforming to CORBA language mapping "_u"
                m_pUnionName = pContext->GetClassFactory()->GetNewDeclarator();
                m_pUnionName->SetParent(this);
                if (!m_pUnionName->CreateBackEnd(String("_u"), 0, pContext))
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
    if (pFEType->IsKindOf(RUNTIME_CLASS(CFETaggedUnionType)))
    {
        // see if we can find the original struct
        String sTag = ((CFETaggedUnionType*)pFEType)->GetTag();
        CFEFile *pFERoot = pFEType->GetRoot();
        ASSERT(pFERoot);
        CFEConstructedType *pFETaggedDecl = pFERoot->FindTaggedDecl(sTag);
        if (pFETaggedDecl)
            m_sTag = pContext->GetNameFactory()->GetTypeName(pFETaggedDecl, sTag, pContext);
        else
            m_sTag = sTag;
    }
    // get union name (DCE specific)
    if (pFEUnion->GetUnionName())
    {
        m_pUnionName = pContext->GetClassFactory()->GetNewDeclarator();
        m_pUnionName->SetParent(this);
        if (!m_pUnionName->CreateBackEnd(pFEUnion->GetUnionName(), 0, pContext))
        {
            delete m_pUnionName;
            m_pUnionName = 0;
            return false;
        }
    }
    // check for CORBA style
    m_bCORBA = pFEUnion->IsCORBA();
    return true;
}

/**	\brief adds a new union case to the vector
 *	\param pUnionCase the new union case
 */
void CBEUnionType::AddUnionCase(CBEUnionCase * pUnionCase)
{
    if (!pUnionCase)
	return;
    m_vUnionCases.Add(pUnionCase);
    pUnionCase->SetParent(this);
}

/**	\brief removes a union case from the union case vector
 *	\param pCase the union case to remove
 */
void CBEUnionType::RemoveUnionCase(CBEUnionCase * pCase)
{
    if (!pCase)
	return;
    m_vUnionCases.Remove(pCase);
}

/**	\brief retrieves a pointer to the first union case element
 *	\return a pointer to the first union case
 */
VectorElement *CBEUnionType::GetFirstUnionCase()
{
    return m_vUnionCases.GetFirst();
}

/**	\brief retrieves a reference to the next union case
 *	\param pIter the pointer to the next union case
 *	\return a reference to the next union case
 */
CBEUnionCase *CBEUnionType::GetNextUnionCase(VectorElement * &pIter)
{
    if (!pIter)
	return 0;
    CBEUnionCase *pRet = (CBEUnionCase *) pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
	return GetNextUnionCase(pIter);
    return pRet;
}

/**	\brief writes the union to the target file
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
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
 *	\todo wrap union into struct
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
        if (!m_bCORBA && (!m_sTag.IsEmpty()))
            pFile->Print(" %s_struct", (const char *) m_sTag);
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
    pFile->Print("%s", (const char *) m_sName);	// should be set to "union"
    if (!m_bCORBA && (!m_sTag.IsEmpty()))
        pFile->Print(" %s", (const char*)m_sTag);
    pFile->Print("\n");
    pFile->PrintIndent("{\n");
    pFile->IncIndent();

    // write members
    VectorElement *pIter = GetFirstUnionCase();
    CBEUnionCase *pCase;
    while ((pCase = GetNextUnionCase(pIter)) != 0)
    {
        pFile->PrintIndent("");
        pCase->WriteDeclaration(pFile, pContext);
        pFile->Print(";\n");
    }

    // close union
    pFile->DecIndent();
    if (m_pUnionName)
        pFile->PrintIndent("} %s;\n", (const char *)(m_pUnionName->GetName()));
    else if (m_bCUnion)
        pFile->PrintIndent("} "); // a c style union has the "normal" name(declarator) after this type declaration
//    else
//        pFile->PrintIndent("} _u;\n");
    else
    {
        ASSERT(false);
    }

    // close struct
    if (!m_bCUnion)
    {
        pFile->DecIndent();
        pFile->PrintIndent("}");
        if (m_bCORBA && (!m_sTag.IsEmpty()))
            pFile->Print(" %s", (const char *) m_sTag);
    }
 }


/**	\brief generates an exact copy of this class
 *	\return a reference to the new object
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
    VectorElement *pIter = GetFirstUnionCase();
    CBEUnionCase *pUnionCase;
    while ((pUnionCase = GetNextUnionCase(pIter)) != 0)
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
String CBEUnionType::GetSwitchVariableName()
{
    VectorElement* pIter = m_pSwitchVariable->GetFirstDeclarator();
    CBEDeclarator *pDecl = m_pSwitchVariable->GetNextDeclarator(pIter);
    return pDecl->GetName();
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
    pFile->Print("%s", (const char *)m_sName);	// should be set to "union"
    if (!m_bCORBA && (!m_sTag.IsEmpty()))
        pFile->Print(" %s", (const char*)m_sTag);
    if (m_pUnionName)
        pFile->PrintIndent(" %s", (const char *)(m_pUnionName->GetName()));
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
    return m_vUnionCases.GetSize();
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
	if (m_sTag.IsEmpty())
	{
		// no tag -> we need a typedef to save us
		// the alias can be used for the cast
		CBETypedef *pTypedef = GetTypedef();
		ASSERT(pTypedef);
		// get first declarator (without stars)
		VectorElement *pIter = pTypedef->GetFirstDeclarator();
		CBEDeclarator *pDecl;
		while ((pDecl = pTypedef->GetNextDeclarator(pIter)) != 0)
		{
			if (pDecl->GetStars() <= (bPointer?1:0))
			    break;
		}
		ASSERT(pDecl);
		pFile->Print("%s", (const char*)pDecl->GetName());
		if (bPointer && (pDecl->GetStars() == 0))
			pFile->Print("*");
	}
	else
	{
    	pFile->Print("%s %s", (const char*)m_sName, (const char*)m_sTag);
    	if (bPointer)
    		pFile->Print("*");
	}
	pFile->Print(")");
}

/** \brief tests if this union has the given tag
 *  \param sTag the tag to test for
 *  \return true if the given tag is the same as the member tag
 */
bool CBEUnionType::HasTag(String sTag)
{
    return (m_sTag == sTag);
}

/** \brief write the zero init code for a union
 *  \param pFile the file to write to
 *  \param pContext the context of this write operation
 */
void CBEUnionType::WriteZeroInit(CBEFile * pFile, CBEContext * pContext)
{
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
void CBEUnionType::WriteGetSize(CBEFile * pFile, CDeclaratorStack *pStack, CBEContext * pContext)
{
    int nFixedSize = GetFixedSize();
    // first add union name
    if (GetUnionName())
    {
        pStack->Push(GetUnionName());
        pStack->GetTop()->SetIndex(-3);
    }
    // build vector with var sized members
    Vector vVarSizedUnionCase(RUNTIME_CLASS(CBEUnionCase));
    VectorElement *pIter = GetFirstUnionCase();
    CBEUnionCase *pCase;
    while ((pCase = GetNextUnionCase(pIter)) != 0)
    {
        if (pCase->IsVariableSized())
            vVarSizedUnionCase.Add(pCase);
    }
    // get first of var-sized members
    pIter = vVarSizedUnionCase.GetFirst();
    // call recursive write
    if (pIter != 0)
    {
        pFile->Print("( ");
        WriteGetMaxSize(pFile, &vVarSizedUnionCase, pIter, pStack, pContext);
        pFile->Print(" : %d)", nFixedSize);
    }
    else
        // should never get here, becaus WriteGetSize is only called if Var-Sized
        pFile->Print("%d", nFixedSize);
    // remove the union's name
    if (GetUnionName())
        pStack->Pop();
}

/** \brief calculates the maximum size of the fixed sized mebers
 *  \return the maximum fixed size in bytes
 */
int CBEUnionType::GetFixedSize()
{
    int nSize = 0;
    VectorElement *pIter = GetFirstUnionCase();
    CBEUnionCase *pUnionCase;
    while ((pUnionCase = GetNextUnionCase(pIter)) != 0)
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
 *  \param pIter the current position in the vector
 *  \param pStack contains the declarator stack for variables sized parameters
 *  \param pContext the context of the write operation
 */
void CBEUnionType::WriteGetMaxSize(CBEFile *pFile, Vector *pMembers, VectorElement *pIter, CDeclaratorStack *pStack, CBEContext *pContext)
{
    ASSERT(pIter);
    CBEUnionCase *pMember = (CBEUnionCase*)pIter->GetElement();
    pIter = pIter->GetNext();
    // always write '(<switch> == <case>)?(<member size>)'
    if (m_pSwitchVariable)
    {
        // pop union name, because switch belongs above union
        int nIndex = pStack->GetTop()->nIndex;
        CBEDeclarator *pUnionName = pStack->Pop();
        // write cases
        pFile->Print("(");
        VectorElement *pILabel = pMember->GetFirstLabel();
        CBEExpression *pLabel;
        bool bFirst = true;
        while ((pLabel = pMember->GetNextLabel(pILabel)) != 0)
        {
            if (pILabel || !bFirst)
                pFile->Print("(");
            // write switch
            VectorElement *pIDecl = m_pSwitchVariable->GetFirstDeclarator();
            CBEDeclarator *pDecl = m_pSwitchVariable->GetNextDeclarator(pIDecl);
            pStack->Push(pDecl);
            pStack->Write(pFile, false, false, pContext);
            pStack->Pop();
            pFile->Print(" == %d", pLabel->GetIntValue());
            if (pILabel || !bFirst)
                pFile->Print(")");
            if (pILabel)
                pFile->Print(" || ");
            bFirst = false;
        }
        pFile->Print(")?");
        // add union name
        pStack->Push(pUnionName);
        pStack->GetTop()->SetIndex(nIndex);
    }
    pFile->Print("(");
    WriteGetMemberSize(pFile, pMember, pStack, pContext);
    pFile->Print(")");
    // if we have successor, add ':(<max(next)>)'
    // otherwise nothing
    if (pIter)
    {
        pFile->Print(" : (");
        WriteGetMaxSize(pFile, pMembers, pIter, pStack, pContext);
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
void CBEUnionType::WriteGetMemberSize(CBEFile *pFile, CBEUnionCase *pMember, CDeclaratorStack *pStack, CBEContext *pContext)
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
