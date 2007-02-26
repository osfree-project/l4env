/**
 * \file    dice/src/be/BEUnionType.cpp
 * \brief   contains the implementation of the class CBEUnionType
 *
 * \date    01/15/2002
 * \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "BEUnionType.h"
#include "BEContext.h"
#include "BEUnionCase.h"
#include "BEFile.h"
#include "BEFunction.h"
#include "BEDeclarator.h"
#include "BETypedef.h"
#include "BEDeclarator.h"
#include "BEExpression.h"
#include "BERoot.h"
#include "BESizes.h"
#include "BEStructType.h"
#include "BEUserDefinedType.h"
#include "Compiler.h"
#include "fe/FEFile.h"
#include "fe/FEUnionType.h"
#include "Attribute-Type.h"
#include "Compiler.h"
#include <cassert>

CBEUnionType::CBEUnionType()
: m_sTag(),
  m_UnionCases(0, this)
{ }

CBEUnionType::CBEUnionType(CBEUnionType & src)
: CBEType(src),
  m_UnionCases(src.m_UnionCases)
{
    m_sTag = src.m_sTag;
    m_UnionCases.Adopt(this);
}

/** \brief destructor of this instance */
CBEUnionType::~CBEUnionType()
{ }

/** \brief creates this class' part of the back-end
 *  \param pFEType the respective type to crete from
 *
 * This implementation calls the base class to perform basic initialization
 * first.
 */
void
CBEUnionType::CreateBackEnd(CFETypeSpec * pFEType)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, 
	"CBEUnionType::%s(fe) called\n", __func__);
    
    // sets m_sName to "union"
    CBEType::CreateBackEnd(pFEType);

    string exc = string(__func__);

    CFEUnionType *pFEUnion = dynamic_cast<CFEUnionType*>(pFEType);
    assert (pFEUnion);
    
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    vector<CFEUnionCase*>::iterator iterUC;
    for (iterUC = pFEUnion->m_UnionCases.begin();
	 iterUC != pFEUnion->m_UnionCases.end();
	 iterUC++)
    {
        CBEUnionCase *pUnionCase = pCF->GetNewUnionCase();
        m_UnionCases.Add(pUnionCase);
	try
	{
	    pUnionCase->CreateBackEnd(*iterUC);
	}
	catch (CBECreateException *e)
        {
	    m_UnionCases.Remove(pUnionCase);
            delete pUnionCase;
	    e->Print();
	    delete e;

	    exc += " failed because union case could not be added";
	    throw new CBECreateException(exc);
        }
    }
    // set tag
    string sTag = pFEUnion->GetTag();
    if (!sTag.empty())
    {
	CBENameFactory *pNF = CCompiler::GetNameFactory();
        // see if we can find the original struct
        CFEFile *pFERoot = dynamic_cast<CFEFile*>(pFEType->GetRoot());
        assert(pFERoot);
        CFEConstructedType *pFETaggedDecl = pFERoot->FindTaggedDecl(sTag);
        if (pFETaggedDecl)
            sTag = pNF->GetTypeName(pFETaggedDecl, sTag);
    }
    m_sTag = sTag;

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
	"CBEUnionType::%s(fe) returns\n", __func__);
}

/** \brief creates a union
 *  \param sTag the tag of the union
 */
void
CBEUnionType::CreateBackEnd(string sTag)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBEUnionType::%s(%s) called\n", __func__, sTag.c_str());
    
    string exc = string(__func__);

    CBENameFactory *pNF = CCompiler::GetNameFactory();
    m_sName = pNF->GetTypeName(TYPE_UNION, false);
    if (m_sName.empty())
    {
        // user defined type overloads this function -> m_sName.c_str()
        // should always be set
	exc += " failed, because no type name could be assigned";
	throw new CBECreateException(exc);
    }
    m_nFEType = TYPE_UNION;
    m_sTag = sTag;

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBEUnionType::%s(%s) returns\n", __func__, sTag.c_str());
}

/** \brief writes the union to the target file
 *  \param pFile the file to write to
 *
 * A union is rather simple:
 * union \<tag\>
 * {
 *  \<member list\>
 * }
 */
void CBEUnionType::Write(CBEFile * pFile)
{
    // write union
    *pFile << m_sName;
    if (!m_sTag.empty())
	*pFile << " " << m_sTag;
    if (!m_UnionCases.empty())
    {
	*pFile << "\n";
	*pFile << "\t{\n";
        pFile->IncIndent();

        // write members
	vector<CBEUnionCase*>::iterator iterU;
	for (iterU = m_UnionCases.begin();
	    iterU != m_UnionCases.end();
	    iterU++)
	{
	    *pFile << "\t";
	    (*iterU)->WriteDeclaration(pFile);
	    *pFile << ";\n";
	}

        // close union
        pFile->DecIndent();
	*pFile << "\t}";
    }
}

/** \brief write the declaration of this type
 *  \param pFile the file to write to
 *
 * Only write a 'struct \<tag\>'.
 */
void CBEUnionType::WriteDeclaration(CBEFile * pFile)
{
    *pFile << m_sName;
    if (!m_sTag.empty())
	*pFile << " " << m_sTag;
}

/** \brief calculates the size of this union type
 *  \return the size in bytes
 *
 * The union type's size is the size of the biggest member and the switch type.
 *
 * Do NOT use cached size, because the size of a member might have changed.
 */
int CBEUnionType::GetSize()
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, 
	"CBEUnionType::%s\n", __func__);
    
    int nSize = 0;
    vector<CBEUnionCase*>::iterator iter;
    for (iter = m_UnionCases.begin();
	 iter != m_UnionCases.end();
	 iter++)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	    "CBEUnionType::%s determining size of union case %s\n", 
	    __func__, (*iter)->m_Declarators.First()->GetName().c_str());
	
        int nUnionSize = (*iter)->GetSize();
	
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	    "CBEUnionType::%s size of union case %s is %d\n", __func__,
	    (*iter)->m_Declarators.First()->GetName().c_str(), nUnionSize);

        // if we have one variable sized union member, the
        // whole union is variable sized, because we cannot pinpoint
        // its exact size to determine a message buffer size
        if (nUnionSize < 0)
	{
	    nSize = nUnionSize;
	    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEUnionType::%s var sized, return -1\n", __func__);
            return nUnionSize;
	}
        if (nSize < nUnionSize)
            nSize = nUnionSize;
    }

    if (m_UnionCases.empty() && (nSize == 0))
    {
	// forward declared union -> find definition of union
	if (m_sTag.empty())
	{
	    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
		"CBEUnionType::%s returns %d (empty)\n", __func__, nSize);
	    return nSize;
	}

	CBERoot *pRoot = GetSpecificParent<CBERoot>();
	assert(pRoot);
	CBEType *pType = pRoot->FindTaggedType(TYPE_UNION, m_sTag);
	if (!pType)
	{
	    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
		"CBEUnionType::%s returns %d (no union %s found)\n",
		__func__, nSize, m_sTag.c_str());
	    return nSize;
	}
	nSize = pType->GetSize();
    }

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEUnionType::%s return %d\n", 
	__func__, nSize);
    return nSize;
}

/** \brief calculates the maximum size of this union type
 *  \return the maxmimum size in bytes
 *
 * This implemetation tries to determine the usual size. If that is negative,
 * i.e., the union is of variable size, we have rerun the algorithm and
 * retrieve the maximum sizes for the members.
 *
 * Do NOT use cached max size, because size might have changed.
 */
int CBEUnionType::GetMaxSize()
{
    int nMaxSize = GetSize();
    if (nMaxSize > 0)
	return nMaxSize;
    
    // reset nMaxSize, because it could have been negative before
    nMaxSize = 0;
    vector<CBEUnionCase*>::iterator iter;
    for (iter = m_UnionCases.begin();
	 iter != m_UnionCases.end();
	 iter++)
    {
        int nUnionSize = 0;
	(*iter)->GetMaxSize(true, nUnionSize);

	// here we try to determine the maximum size. If there is a variable
	// sized member, it has some fixed size or anything else replica
	// somewhere that will fit its needs. Therefore we simply skip these
	// members here.
        if (nUnionSize < 0)
	{
	    continue;
	}
        if (nMaxSize < nUnionSize)
            nMaxSize = nUnionSize;
    }
    
    if (m_UnionCases.empty() && (nMaxSize == 0))
    {
	// forward declared union -> find definition of union
	if (m_sTag.empty())
	    return nMaxSize;

	CBERoot *pRoot = GetSpecificParent<CBERoot>();
	assert(pRoot);
	CBEType *pType = pRoot->FindTaggedType(TYPE_UNION, m_sTag);
	if (!pType)
	    return nMaxSize;
	nMaxSize = pType->GetMaxSize();
    }

    return nMaxSize;
}

/** \brief counts the union cases
 *  \return the number of union cases
 */
int CBEUnionType::GetUnionCaseCount()
{
    return m_UnionCases.size();
}

/** \brief writes the cast for this type
 *  \param pFile the file to write to
 *  \param bPointer true if the cast should produce a pointer
 *
 * The cast of a union type is '(union tag)'.
 */
void CBEUnionType::WriteCast(CBEFile * pFile, bool bPointer)
{
    *pFile << "(";
    if (m_sTag.empty())
    {
	// no tag -> we need a typedef to save us
	// the alias can be used for the cast
	CBETypedef *pTypedef = GetTypedef();
	assert(pTypedef);
	// get first declarator (without stars)
	vector<CBEDeclarator*>::iterator iterD;
	for (iterD = pTypedef->m_Declarators.begin();
	    iterD != pTypedef->m_Declarators.end();
	    iterD++)
	{
	    if ((*iterD)->GetStars() <= (bPointer?1:0))
		break;
	}
        assert(iterD != pTypedef->m_Declarators.end());
	*pFile << (*iterD)->GetName();
        if (bPointer && ((*iterD)->GetStars() == 0))
	    *pFile << "*";
    }
    else
    {
	*pFile << m_sName << " " << m_sTag;
        if (bPointer)
	    *pFile << "*";
    }
    *pFile << ")";
}

/** \brief write the zero init code for a union
 *  \param pFile the file to write to
 *
 * init union similar to struct, but use only first member.
 */
void CBEUnionType::WriteZeroInit(CBEFile * pFile)
{
    *pFile << "{ ";
    pFile->IncIndent();
    CBEUnionCase *pMember = m_UnionCases.First();
    if (pMember)
    {
	// get type
	CBEType *pType = pMember->GetType();
	// get declarator
	vector<CBEDeclarator*>::iterator iterD;
	for (iterD = pMember->m_Declarators.begin();
	     iterD != pMember->m_Declarators.end();
	     iterD++)
	{
	    // be C99 compliant:
	    *pFile << (*iterD)->GetName() << " : ";
	    if ((*iterD)->IsArray())
		WriteZeroInitArray(pFile, pType, (*iterD), 
		    (*iterD)->m_Bounds.begin());
	    else if (pType)
		pType->WriteZeroInit(pFile);
	}
    }
    pFile->DecIndent();
    *pFile << " }";
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
 *  \param pStack the declarator stack for constructed types with variable
 *         sized members 
 *  \param pUsingFunc the function to use as reference for members
 *
 * This is not the maximum of all cases, because this would mean that all
 * cases are of equal type (to determine their size), but they are not.
 * Therefore we have to write code which test the switch variable.  We can
 * write it as recursive function.
 *
 * \todo what if default case is variable sized?
 */
void CBEUnionType::WriteGetSize(CBEFile * pFile,
    vector<CDeclaratorStackLocation*> *pStack,
    CBEFunction *pUsingFunc)
{
    int nFixedSize = GetFixedSize();

    // build vector with var sized members
    vector<CBEUnionCase*> vVarSizedUnionCase;
    vVarSizedUnionCase.clear();
    vector<CBEUnionCase*>::iterator iter;
    for (iter = m_UnionCases.begin();
	 iter != m_UnionCases.end();
	 iter++)
    {
        if ((*iter)->IsVariableSized())
            vVarSizedUnionCase.push_back(*iter);
    }
    // call recursive write
    if (!vVarSizedUnionCase.empty())
    {
	*pFile << "_dice_max(";
        WriteGetMaxSize(pFile, &vVarSizedUnionCase, vVarSizedUnionCase.begin(), pStack, pUsingFunc);
	*pFile << ", " << nFixedSize << ")";
    }
    else
	// should never get here, becaus WriteGetSize is only called if
	// Var-Sized
	*pFile << nFixedSize;
}

/** \brief calculates the maximum size of the fixed sized mebers
 *  \return the maximum fixed size in bytes
 */
int CBEUnionType::GetFixedSize()
{
    int nSize = 0;
    vector<CBEUnionCase*>::iterator iter;
    for (iter = m_UnionCases.begin();
	 iter != m_UnionCases.end();
	 iter++)
    {
        int nUnionSize = (*iter)->GetSize();
        // if this is negative size, then its variable sized
        // and we skip it here (the max operation will simply ignore it)
        if (nSize < nUnionSize)
            nSize = nUnionSize;
    }

    if (m_UnionCases.empty() && (nSize == 0))
    {
       // forward declared union -> find definition of union
       if (m_sTag.empty())
           return nSize;

        CBERoot *pRoot = GetSpecificParent<CBERoot>();
        assert(pRoot);
        CBEType *pType = pRoot->FindTaggedType(TYPE_UNION, m_sTag);
       if (!pType)
           return nSize;
       nSize = pType->GetSize();
    }

    return nSize;
}

/** \brief writes the maximum of the variable sized members
 *  \param pFile the file to write to
 *  \param pMembers the vector containing the variable sized members
 *  \param iter the current position in the vector
 *  \param pStack contains the declarator stack for variables sized parameters
 *  \param pUsingFunc the function to use as reference for members
 */
void CBEUnionType::WriteGetMaxSize(CBEFile *pFile,
    const vector<CBEUnionCase*> *pMembers,
    vector<CBEUnionCase*>::iterator iter,
    vector<CDeclaratorStackLocation*> *pStack,
    CBEFunction *pUsingFunc)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s: called for func %s\n", __func__,
	pUsingFunc->GetName().c_str());

    assert(pMembers);
    assert(iter != pMembers->end());
    CBEUnionCase *pMember = *iter++;
    assert(pMember);
    bool bNext = (iter != pMembers->end());
    if (bNext)
	*pFile << "_dice_max(";
    WriteGetMemberSize(pFile, pMember, pStack, pUsingFunc);
    // if we have successor, add ':(<max(next)>)'
    // otherwise nothing
    if (bNext)
    {
	*pFile << ", ";
        WriteGetMaxSize(pFile, pMembers, iter, pStack, pUsingFunc);
	*pFile << ")";
    }
}

/** \brief writes the whole size string for a member
 *  \param pFile the file to write to
 *  \param pMember the member to write for
 *  \param pStack contains the declarator stack for variable sized parameters
 *  \param pUsingFunc the function to use as reference for members
 *
 * This code is taken from
 * CBEMsgBufferType::WriteInitializationVarSizedParameters if something is not
 * working, check if something changed there as well.
 */
void CBEUnionType::WriteGetMemberSize(CBEFile *pFile,
    CBEUnionCase *pMember,
    vector<CDeclaratorStackLocation*> *pStack,
    CBEFunction *pUsingFunc)
{
    assert(pMember);

    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s: called for %s\n", __func__,
	pMember->m_Declarators.First()->GetName().c_str());

    pMember->WriteGetSize(pFile, pStack, pUsingFunc);
    if ((pMember->GetType()->GetSize() > 1) && !(pMember->IsString()))
    {
	*pFile << "*sizeof";
        pMember->GetType()->WriteCast(pFile, false);
    }
    else if (pMember->IsString())
    {
        // add terminating zero
	*pFile << "+1";
        bool bHasSizeAttr = 
	    pMember->m_Attributes.Find(ATTR_SIZE_IS) ||
	    pMember->m_Attributes.Find(ATTR_LENGTH_IS) ||
	    pMember->m_Attributes.Find(ATTR_MAX_IS);
        if (!bHasSizeAttr)
	    *pFile << "+" << 
		CCompiler::GetSizes()->GetSizeOfType(TYPE_INTEGER);
    }
}

/** \brief tries to find a member with a declarator stack
 *  \param pStack contains the list of members to search for
 *  \param iCurr the iterator pointing to the currently searched element
 *  \return the member found or 0 if not found
 *
 * Gets the first element on the stack and tries to find
 */
CBETypedDeclarator* 
CBEUnionType::FindMember(vector<CDeclaratorStackLocation*> *pStack,
    vector<CDeclaratorStackLocation*>::iterator iCurr)
{
    // if at end, return
    if (iCurr == pStack->end())
	return 0;
    // try to find member for current declarator
    string sName = (*iCurr)->pDeclarator->GetName();
    CBETypedDeclarator *pMember = m_UnionCases.Find(sName);
    if (!pMember)
	return pMember;

    // no more elements in stack, we are finished
    if (++iCurr == pStack->end())
	return pMember;

    // check member types
    CBEType *pType = pMember->GetType();
    while (dynamic_cast<CBEUserDefinedType*>(pType))
	pType = dynamic_cast<CBEUserDefinedType*>(pType);

    CBEStructType *pStruct = dynamic_cast<CBEStructType*>(pType);
    if (pStruct)
	return pStruct->FindMember(pStack, iCurr);

    CBEUnionType *pUnion = dynamic_cast<CBEUnionType*>(pType);
    if (pUnion)
	return pUnion->FindMember(pStack, iCurr);

    return 0;
}
