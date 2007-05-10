/**
 *  \file    dice/src/be/BEDeclarator.cpp
 *  \brief   contains the implementation of the class CBEDeclarator
 *
 *  \date    01/15/2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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

#include "BEDeclarator.h"
#include "BEContext.h"
#include "BEExpression.h"
#include "BEFile.h"
#include "BEFunction.h"
#include "BETarget.h"
#include "BEComponent.h"
#include "BEClient.h"
#include "BETypedDeclarator.h"
#include "BEType.h"
#include "BEAttribute.h"
#include "BESwitchCase.h"
#include "BEStructType.h"
#include "BEUnionType.h"
#include "Compiler.h"
#include "Attribute-Type.h"
#include "fe/FEDeclarator.h"
#include "fe/FEEnumDeclarator.h"
#include "fe/FEArrayDeclarator.h"
#include "fe/FEBinaryExpression.h"
#include <sstream>
#include <cassert>
#include <iostream>

/** \brief writes the declarator stack to a file
 *  \param pFile the file to write to
 *  \param pStack the declarator stack to write
 *  \param bUsePointer true if one star should be ignored
 */
void CDeclaratorStackLocation::Write(CBEFile *pFile,
    CDeclStack* pStack,
    bool bUsePointer)
{
    string sOut;
    CDeclaratorStackLocation::WriteToString(sOut, pStack, bUsePointer);
    *pFile << sOut;
}

/** \brief writes the declarator stack to a string
 *  \param sResult the string to append the decl stack to
 *  \param pStack the declarator stack to write
 *  \param bUsePointer true if one star should be ignored
 */
void CDeclaratorStackLocation::WriteToString(string &sResult,
    CDeclStack* pStack,
    bool bUsePointer)
{
    assert(pStack);

    CBEFunction *pFunction = 0;
    if (!pStack->empty() && pStack->front().pDeclarator)
        pFunction = pStack->front().pDeclarator->GetSpecificParent<CBEFunction>();

    CDeclStack::iterator iter;
    for (iter = pStack->begin(); iter != pStack->end(); iter++)
    {
        CBEDeclarator *pDecl = iter->pDeclarator;
        assert(pDecl);
        int nStars = pDecl->GetStars();
        // test for empty array dimensions, which are treated as
        // stars
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
	    "CDeclaratorStackLocation::%s for %s with %d stars (1)\n",
	    __func__, pDecl->GetName().c_str(), nStars);
        if (pDecl->GetArrayDimensionCount())
        {
            int nLevel = 0;
            vector<CBEExpression*>::iterator iterB;
	    for (iterB = pDecl->m_Bounds.begin();
		 iterB != pDecl->m_Bounds.end();
		 iterB++)
            {
                if ((*iterB)->IsOfType(EXPR_NONE))
                {
                    if (!iter->HasIndex(nLevel++))
                        nStars++;
                }
            }
        }
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
	    "CDeclaratorStackLocation::%s for %s with %d stars (2)\n",
	    __func__, pDecl->GetName().c_str(), nStars);
        if ((iter + 1)  == pStack->end())
        {
             // only apply to last element in row
            if (bUsePointer)
                nStars--;
            else
            {
                // test for an array, which only has stars as array dimensions
                if (iter->HasIndex() && 
		    (pDecl->GetArrayDimensionCount() == 0) && 
		    (nStars > 0))
                    nStars--;
            }
        }
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
	    "CDeclaratorStackLocation::%s for %s with %d stars (3)\n",
	    __func__, pDecl->GetName().c_str(), nStars);
        if (pFunction && (iter->nIndex[0] != -3))
        {
	    // this only works if the declarator really belongs to the
	    // parameter
	    CBETypedDeclarator *pTrueParameter = 
		pFunction->FindParameter(pDecl->GetName());
	    // now we do an additional check if we found a parameter
	    if (pTrueParameter &&
		pFunction->HasAdditionalReference(
	    	    pTrueParameter->m_Declarators.First()))
    		nStars++;
        }
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
	    "CDeclaratorStackLocation::%s for %s with %d stars func at %p (4)\n",
	    __func__, pDecl->GetName().c_str(), nStars, pFunction);
        // check if the type is a pointer type
        if (pFunction)
        {
            CBETypedDeclarator *pParameter = 
		pFunction->FindParameter(pDecl->GetName(), false);
	    if (!pParameter)
		pParameter = pFunction->m_LocalVariables.Find(pDecl->GetName());
	    CBEType *pType = pParameter ? pParameter->GetType() : 0;
	    // check transmit-as
	    CBEAttribute *pAttr = pParameter ?
		pParameter->m_Attributes.Find(ATTR_TRANSMIT_AS) : 0;
	    if (pAttr)
		pType = pAttr->GetAttrType();
	    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
	"CDeclaratorStackLocation::%s for %s: pType %p, pointer? %s, string: %s\n",
		__func__, pDecl->GetName().c_str(), pType,
		pType ? (pType->IsPointerType() ? "yes" : "no") : "(no type)",
		pParameter ? (pParameter->IsString() ? "yes" : "no") : 
		"(no param)");
	    // XXX removed test for string 
            if (pType && pType->IsPointerType())
                nStars++;
        }
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	    "CDeclaratorStackLocation::%s for %s with %d stars (5)\n", __func__,
	    pDecl->GetName().c_str(), nStars);
	if (nStars > 0)
	    sResult += "(";
	for (int i=0; i<nStars; i++)
	    sResult += "*";
	sResult += pDecl->GetName();
	if (nStars > 0)
	    sResult += ")";
        for (int i = 0; i < iter->GetUsedIndexCount(); i++)
        {
            if (iter->nIndex[i] >= 0)
	    {
		std::ostringstream os;
		os << iter->nIndex[i];
		sResult += string("[") + os.str() + "]";
	    }
            if (iter->nIndex[i] == -2)
		sResult += string("[") + iter->sIndex[i] + "]";
        }
        if ((iter + 1) != pStack->end())
	    sResult += ".";
    }
}

/*********************************************************************/
/* 
 * Declarator 
 */

CBEDeclarator::CBEDeclarator()
: m_sName(),
  m_Bounds(0, this)
{
    m_nStars = 0;
    m_nBitfields = 0;
    m_nType = DECL_NONE;
    m_nOldType = DECL_NONE;
    m_pInitialValue = 0;
}

CBEDeclarator::CBEDeclarator(CBEDeclarator & src)
: CBEObject(src),
  m_Bounds(src.m_Bounds)
{
    m_sName = src.m_sName;
    m_nStars = src.m_nStars;
    m_nBitfields = src.m_nBitfields;
    m_nType = src.m_nType;
    m_nOldType = src.m_nOldType;
    CLONE_MEM(CBEExpression, m_pInitialValue);
    m_Bounds.Adopt(this);
}

/** \brief destructor of this instance */
CBEDeclarator::~CBEDeclarator()
{
    if (m_pInitialValue)
        delete m_pInitialValue;
}

/** \brief creates a copy of this object
 *  \return a reference to the copy
 */
CObject * CBEDeclarator::Clone()
{ 
    return new CBEDeclarator(*this); 
}

/** \brief prepares this instance for the code generation
 *  \param pFEDeclarator the corresponding front-end declarator
 *
 * This implementation extracts the name, stars and bitfields from the
 * front-end declarator.
 */
void 
CBEDeclarator::CreateBackEndDecl(CFEDeclarator * pFEDeclarator)
{
    // call CBEObject's CreateBackEnd method
    CBEObject::CreateBackEnd(pFEDeclarator);

    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEDeclarator::%s called\n", 
	__func__);
    
    m_sName = pFEDeclarator->GetName();
    m_nBitfields = pFEDeclarator->GetBitfields();
    m_nStars = pFEDeclarator->GetStars();
    m_nType = pFEDeclarator->GetType();
    switch (m_nType)
    {
    case DECL_ARRAY:
        CreateBackEndArray((CFEArrayDeclarator *) pFEDeclarator);
        break;
    case DECL_ENUM:
        CreateBackEndEnum((CFEEnumDeclarator *) pFEDeclarator);
        break;
    }
    
    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEDeclarator::%s done\n",
	__func__);
}

/** \brief create a simple declarator using a front-end identifier
 *  \param pFEIdentifier the front-end identifier
 */
void 
CBEDeclarator::CreateBackEnd(CFEIdentifier * pFEIdentifier)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBEDeclarator::%s(identifier)\n", __func__);

    if (dynamic_cast<CFEDeclarator*>(pFEIdentifier))
	return CreateBackEndDecl(dynamic_cast<CFEDeclarator*>(pFEIdentifier));
    
    m_sName = pFEIdentifier->GetName();
    m_nType = DECL_IDENTIFIER;
    
    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
	"CBEDeclarator::%s(identifier) done\n", __func__);
}

/** \brief create a simple declarator using name and number of stars
 *  \param sName the name of the declarator
 *  \param nStars the number of asterisks
 *
 * Do not overwrite type, because this function might be called to
 * reinitialize (set new name) the declarator.
 */
void 
CBEDeclarator::CreateBackEnd(string sName, 
    int nStars)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBEDeclarator::%s(string: %s)\n", __func__, sName.c_str());
    
    m_sName = sName;
    m_nStars = nStars;
    if (m_nType == DECL_NONE)
	m_nType = DECL_IDENTIFIER;
    
    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
	"CBEDeclarator::%s(string) done\n", __func__);
}

/** \brief creates the back-end representation for the enum declarator
 *  \param pFEEnumDeclarator the front-end declarator
 *  \return true if code generation was successful
 */
void 
CBEDeclarator::CreateBackEndEnum(CFEEnumDeclarator * pFEEnumDeclarator)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEDeclarator::%s(enum)\n", 
	__func__);
    
    if (pFEEnumDeclarator->GetInitialValue())
    {
	CBEClassFactory *pCF = CCompiler::GetClassFactory();
        m_pInitialValue = pCF->GetNewExpression();
        m_pInitialValue->SetParent(this);
	try
	{
	    m_pInitialValue->CreateBackEnd(pFEEnumDeclarator->GetInitialValue());
	}
	catch (CBECreateException *e)
        {
            delete m_pInitialValue;
            m_pInitialValue = 0;
            throw;
        }
    }
    
    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBEDeclarator::%s(enum) done\n", __func__);
}

/** \brief creates the back-end representation for the array declarator
 *  \param pFEArrayDeclarator the respective front-end declarator
 *  \return true if code generation was successful
 */
void 
CBEDeclarator::CreateBackEndArray(CFEArrayDeclarator * pFEArrayDeclarator)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEDeclarator::%s(array)\n", 
	__func__);
    // iterate over front-end bounds
    int nMax = pFEArrayDeclarator->GetDimensionCount();
    for (int i = 0; i < nMax; i++)
    {
        CFEExpression *pLower = pFEArrayDeclarator->GetLowerBound(i);
        CFEExpression *pUpper = pFEArrayDeclarator->GetUpperBound(i);
        CBEExpression *pBound = GetArrayDimension(pLower, pUpper);
        AddArrayBound(pBound);
    }
    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
	"CBEDeclarator::%s(array) done\n", __func__);
}

/** \brief writes the declarator to the target file
 *  \param pFile the file to write to
 *
 * This implementation writes the declarator as is (all stars, bitfields,
 * etc.).
 */
void 
CBEDeclarator::WriteDeclaration(CBEFile * pFile)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, 
	"CBEDeclarator::%s called for %s\n", __func__,
        m_sName.c_str());
    if (m_nType == DECL_ENUM)
    {
        WriteEnum(pFile);
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
	    "CBEDeclarator::%s returns\n", __func__);
        return;
    }
    // write stars
    for (int i = 0; i < m_nStars; i++)
        *pFile << "*";
    // write name
    *pFile << m_sName;
    // write bitfields
    if (m_nBitfields > 0)
        *pFile << ":" << m_nBitfields;
    // array dimensions
    if (IsArray())
        WriteArray(pFile);
    
    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEDeclarator::%s returned\n", 
	__func__);
}

/** \brief writes an array declarator
 *  \param pFile the file to write to
 *
 * This implementation has to write the array dimensions only. This is done by
 * iterating over them and writing them into brackets ('[]').
 */
void
CBEDeclarator::WriteArray(CBEFile * pFile)
{
    vector<CBEExpression*>::iterator iterB;
    for (iterB = m_Bounds.begin();
	 iterB != m_Bounds.end();
	 iterB++)
    {
	*pFile << "[";
        (*iterB)->Write(pFile);
	*pFile << "]";
    }
}

/** \brief write an array declarator
 *  \param pFile the file to write to
 *
 * The differene to WriteArray is to skip unbound array dimensions.
 * We can determine an unbound array dimension by checking its integer value.
 * It it is 0 then its an unbound dimension.
 */
void 
CBEDeclarator::WriteArrayIndirect(CBEFile * pFile)
{
    vector<CBEExpression*>::iterator iterB;
    for (iterB = m_Bounds.begin();
	 iterB != m_Bounds.end();
	 iterB++)
    {
        if ((*iterB)->GetIntValue() == 0)
            continue;
	*pFile << "[";
        (*iterB)->Write(pFile);
	*pFile << "]";
    }
}

/** \brief writes an enum declarator
 *  \param pFile the file to write to
 */
void 
CBEDeclarator::WriteEnum(CBEFile* /*pFile*/)
{
    assert(false);
}

/** \brief creates a new back-end array bound using front-end array bounds
 *  \param pLower the lower array bound
 *  \param pUpper the upper array bound
 *  \return the new back-end array bound
 *
 * The expression may have the following values:
 * -# pLower == 0 && pUpper == 0
 * -# pLower == 0 && pUpper != 0
 * -# pLower != 0 && pUpper != 0
 * There is not such case, that pLower != 0 and pUpper == 0.
 *
 * To let the CreateBE function calculate the value of the expression, we
 * simply define the following resulting front-end expression used by the
 * CreateBE function (respective to the above cases):
 * -# pNew = '*'
 * -# pNew = pUpper
 * -# pNew = (pUpper)-(pLower)
 * This behaviour is also specified in the DCE Specification.
 *
 * For the latter case this implementation creates two primary expression,
 * which are the parenthesis and a binary expression used to subract them.
 */
CBEExpression*
CBEDeclarator::GetArrayDimension(CFEExpression * pLower, 
    CFEExpression * pUpper)
{
    CFEExpression *pNew = 0;
    // first get new front-end expression
    if ((!pLower) && (!pUpper))
    {
        pNew = new CFEExpression(EXPR_NONE);
    }
    else if ((!pLower) && (pUpper))
    {
        if ((pUpper->GetType() == EXPR_CHAR) && (pUpper->GetChar() == '*'))
            pNew = new CFEExpression(EXPR_NONE);
        else
            pNew = pUpper;
    }
    else if ((pLower) && (pUpper))
    {
        // if lower is '*' than there is no lower bound
        if ((pLower->GetType() == EXPR_CHAR) && (pLower->GetChar() == '*'))
        {
            return GetArrayDimension((CFEExpression *) 0, pUpper);
        }
        // if upper is '*' than array has no bound
        if ((pUpper->GetType() == EXPR_CHAR) && (pUpper->GetChar() == '*'))
        {
            pNew = new CFEExpression(EXPR_NONE);
        }
        else
        {
            CFEExpression *pLowerP = new CFEPrimaryExpression(EXPR_PAREN, 
		pLower);
            CFEExpression *pUpperP = new CFEPrimaryExpression(EXPR_PAREN, 
		pUpper);
            pNew = new CFEBinaryExpression(EXPR_BINARY, pUpperP, EXPR_MINUS, 
		pLowerP);
        }
    }
    // create new back-end expression
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBEExpression *pReturn = pCF->GetNewExpression();
    pReturn->SetParent(this);
    try
    {
	pReturn->CreateBackEnd(pNew);
    }
    catch (CBECreateException *e)
    {
        delete pReturn;
	e->Print();
	delete e;
        return 0;
    }

    return pReturn;
}

/** \brief adds another array bound to the bounds vector
 *  \param pBound the bound to add
 *
 * This implementation creates the vector if not existing
 */
void 
CBEDeclarator::AddArrayBound(CBEExpression * pBound)
{
    if (!pBound)
        return;
    if (m_Bounds.empty())
    {
        m_nOldType = m_nType;
        m_nType = DECL_ARRAY;
    }
    m_Bounds.Add(pBound);
}

/** \brief removes an array bound from the bounds vector
 *  \param pBound the bound to remove
 */
void 
CBEDeclarator::RemoveArrayBound(CBEExpression *pBound)
{
    m_Bounds.Remove(pBound);
    if (m_Bounds.empty())
        m_nType = m_nOldType;
}

/** \brief calculates the size of the declarator
 *  \return size of the declarator (or -x if pointer, where x is the number of stars)
 *
 * The size of a declarator is 1. An exception is the array declarator,
 * which's size is calculated from the array dimensions. If the declarator has
 * any stars in front of it, it may have a undefined size, since the asterisk
 * indicates pointers.
 *
 * If the integer value of an array bound is not determinable, we return the
 * number of unbound dimension plus the number of asterisks. We add these two
 * values, so we can later determine if the value returned by GetSize were
 * only asterisks or also unbound array dimensions.
 *
 * Another exception is the usage of bitfields. If the declarator has
 * bitfields asscoiated with it the function returns zero. The caller has to
 * know that the size of zero means possible bitfields and ask for them
 * seperately.
 */
int CBEDeclarator::GetSize()
{
    if (!IsArray())
    {
        if (m_nStars > 0)
            return -(m_nStars);
        // if bitfields: return 0
        if (m_nBitfields)
            return 0;
        return 1;
    }

    int nSize = 1;
    int nFakeStars = 0;

    vector<CBEExpression*>::iterator iterB;
    for (iterB = m_Bounds.begin();
	 iterB != m_Bounds.end();
	 iterB++)
    {
        int nVal = (*iterB)->GetIntValue();
        if (nVal == 0)    // no integer value
            nFakeStars++;
        else
            nSize *= ((nVal < 0) ? -nVal : nVal);
    }
    if (nFakeStars > 0)
        return -(nFakeStars + m_nStars);
    return nSize;
}

/** \brief return the maximum size of the declarator
 *  \return maximum size in bytes
 */
int CBEDeclarator::GetMaxSize()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	"CBEDeclarator::%s called for %s\n", __func__, GetName().c_str());
    
    int nStars = m_nStars;
    // deduct from stars one if this is an out reference
    CBETypedDeclarator *pParameter = GetSpecificParent<CBETypedDeclarator>();
    if (pParameter && pParameter->m_Attributes.Find(ATTR_OUT))
	nStars--;

    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
	"CBEDeclarator::%s stars %d array? %s\n", __func__,
	nStars, IsArray() ? "yes" : "no");
    
    if (!IsArray())
    {
        if (nStars > 0)
            return -(nStars);
        // if bitfields: return 0
        if (m_nBitfields)
            return 0;
        return 1;
    }
    else
    {
        if ((GetArrayDimensionCount() == 0) &&
            (nStars > 0))
        {
            // this is a weird situation:
            // we have an unbound array, but express it
            // using '*' instead of '[]'
            // Nonetheless the DECL_ARRAY is set...
            //
            // To make the MAX algorithms work, this has to
            // return a negative value
            return -(nStars);
        }
    }

    int nSize = 1;
    int nFakeStars = 0;
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
	"CBEDeclarator::%s check array bounds\n", __func__);

    vector<CBEExpression*>::iterator iterB;
    for (iterB = m_Bounds.begin();
	 iterB != m_Bounds.end();
	 iterB++)
    {
        int nVal = (*iterB)->GetIntValue();
        if (nVal == 0)    // no integer value
            nFakeStars++;
        else
            nSize *= ((nVal < 0) ? -nVal : nVal);
    }

    if (nFakeStars > 0)
	nSize = -(nFakeStars + nStars);

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	"CBEDeclarator::%s return %d\n", __func__, nSize);
    return nSize;
}

/** \brief simply prints the name of the declarator
 *  \param pFile the file to write to
 */
void CBEDeclarator::WriteName(CBEFile * pFile)
{
    *pFile << m_sName;
}

/** \brief simply prints the name of the declarator
 *  \param str the string to write to
 */
void CBEDeclarator::WriteNameToStr(string& str)
{
    str += m_sName;
}

/** \brief writes for every pointer an extra declarator without this pointer
 *  \param pFile the file to write to
 *  \param bUsePointer true if the variable is intended to be used as a pointer
 *  \param bHasPointerType true if the decl's type is a pointer type
 *
 * If we have an array declarator with unbound array dimensions, we
 * have to write them as pointers as well. We can determine these "fake"
 * pointers by checking GetSize.
 *
 * We do not need dereferenced declarators for these unbound arrays.
 *
 * \todo indirect var by underscore hard coded => replace with configurable
 */
void
CBEDeclarator::WriteIndirect(CBEFile * pFile,
    bool bUsePointer,
    bool bHasPointerType)
{
    if (m_nType == DECL_ENUM)
    {
        WriteEnum(pFile);
        return;
    }

    // get function and parameter
    CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
    assert(pFunction);
    CBETypedDeclarator *pParameter = pFunction->FindParameter(m_sName);
    assert(pParameter);
    // calculate stars
    int nFakeStars = GetEmptyArrayDims();
    int nTypeStars = pParameter->GetType()->GetIndirectionCount();
    int nStartStars = m_nStars + nTypeStars + nFakeStars;
 
    // for something like:
    // 'char *buf' we want a declaration of 'char *buf'
    //  (m_nStars:1 nFakeStars:0 bUsePointer:true -> nStartStars:1 nFakeStars:1)
    // 'char buf[]' we want a declaration of 'char *buf'
    //  (m_nStars:0 nFakeStars:1 bUsePointer:true -> nStartStars:1 nFakeStars:1)
    // 'char *buf[]' we want a declaration of 'char **buf, *_buf'
    //  (m_nStars:1 nFakeStars:1 bUsePointer:true -> nStartStars:2 nFakeStars:1)
    // 'char **buf' we want a declaration of 'char **buf, *_buf'
    //  (m_nStars:2 nFakeStars:0 bUsePointer:true -> nStartStars:2 nFakeStars:1)
    if (bUsePointer && (nFakeStars == 0) && (m_nStars > 0))
        nFakeStars = 1;
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
	"%s: for %s, nFakeStars: %d, nStartStars: %d\n", __func__,
	m_sName.c_str(), nFakeStars, nStartStars);
    bool bComma = false;
    for (int nStars = nStartStars; nStars >= nFakeStars; nStars--)
    {
        if (bComma)
	    *pFile << ", ";
        int i;
        // if not first and we have pointer type, write a stars for it
        if (bComma && bHasPointerType)
	    *pFile << "*";
        // write stars
        for (i = 0; i < nStars; i++)
	    *pFile << "*";
        // write underscores
        for (i = 0; i < (nStartStars - nStars); i++)
	    *pFile << "_";
        // write name
	*pFile << m_sName;
        // next please
        bComma = true;
    }

    // we make the last declarator the array declarator
    if (IsArray())
        WriteArrayIndirect(pFile);

    // write bitfields
    //     if (m_nBitfields > 0)
    //         *pFile << ":" << m_nBitfields;
}

/** \brief assigns pointered variables a reference to "unpointered" variables
 *  \param pFile the file to write to
 *  \param bUsePointer true if the variable is intended to be used as a pointer
 *
 * Does something like "t1 = \&_t1;" for a variable "CORBA_long *t1"
 *
 * \todo indirect var by underscore hard coded => replace with configurable
 */
void 
CBEDeclarator::WriteIndirectInitialization(CBEFile * pFile,
    bool bUsePointer)
{
    // get function and parameter
    CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
    assert(pFunction);
    CBETypedDeclarator *pParameter = pFunction->FindParameter(m_sName);
    assert(pParameter);

    int nFakeStars = GetEmptyArrayDims();
    int nTypeStars = pParameter->GetType()->GetIndirectionCount();
    int i, nStartStars = m_nStars + nTypeStars + nFakeStars;
    if (bUsePointer && (m_nStars > 0) && (nFakeStars == 0))
        nFakeStars++;
    // FIXME: when adding variable declaration use "temp" var for indirection
    for (int nStars = nStartStars; nStars > nFakeStars; nStars--)
    {
	*pFile << "\t";
        // write name (one _ less)
        for (i = 0; i < (nStartStars - nStars); i++)
	    *pFile << "_";
	*pFile << m_sName << " = ";
        // write name (one more _)
	*pFile << "&";
        for (i = 0; i < (nStartStars - nStars) + 1; i++)
	    *pFile << "_";
	*pFile << m_sName << ";\n";
    }
}

/** \brief assigns pointered variables a reference to "unpointered" variables
 *  \param pFile the file to write to
 *  \param bUsePointer true if the variable is intended to be used as a pointer
 *
 * Does something like "*t1 = malloc(size);" for variable char* *t1.
 *
 * \todo indirect var by underscore hard coded => replace with configurable
 */
void
CBEDeclarator::WriteIndirectInitializationMemory(CBEFile * pFile,
    bool bUsePointer)
{
    assert(pFile);
    // get function and parameter
    CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
    assert(pFunction);
    // is only called from CBESwitchCase
    assert(dynamic_cast<CBESwitchCase*>(pFunction));

    CBETypedDeclarator *pParameter = pFunction->FindParameter(m_sName);
    assert(pParameter);
    CBEType *pType = pParameter->GetTransmitType();

    int nFakeStars = GetEmptyArrayDims();
    int nTypeStars = pType->GetIndirectionCount();
    int nStartStars = m_nStars + nTypeStars + nFakeStars;
    if (bUsePointer && (m_nStars > 0) && (nFakeStars == 0))
        nFakeStars = 1;
    /** usually we allocate memory for temporary variables, because the
     * real variables are [out] pointers pointing to these temporary
     * variables. An exemption are constructed types. Here, the [out]
     * parameters point to preallocated memory. This function is called to
     * preallocate the memory, thus the parameter directly has to point to
     * the allocated memory. To take all the possible pointers into
     * account, we set the offset to zero if the type is constructed.
     * Otherwise (normal types, string pointers, etc.) the offset would be
     * 1 and thus allocate with the first temporary variable.
     */
    int nOffset = 1;
    if (pType->IsConstructedType() && (nTypeStars == 0) && (nFakeStars == 0))
	nOffset = 0;
    /** now we have to initialize the fake stars (unbound array dimensions).
     * problem is to use an allocation routine appropriate for this. If there is
     * an max_is or upper bound it is used, but we have no upper bound.
     * CORBA defines to use CORBA_alloc, but this requires size and we don't
     * know how big it actually will be
     *
     * \todo maybe we can propagate the max size when connecting to the server.
     */
    for (int nStars = nStartStars; nStars > nFakeStars; nStars--)
    {
        CDeclStack vStack;
        vStack.push_back(this);
	*pFile << "\t";
        for (int j = 0; j < nStartStars - nStars + nOffset; j++)
	    *pFile << "_";
	*pFile << m_sName << " = ";
	// use original parameter type (not transmit type)
        pParameter->GetType()->WriteCast(pFile, true);
        CBEContext::WriteMalloc(pFile, pFunction);
	*pFile << "(";
        /**
         * if the parameter is out and size parameter, then this
         * initialization is done before a component function.
         * We can use the size parameter only if it is set (it can't just
         * before the call, but it is before the component if the size
         * parameter is an IN.) So we check if parent func is CBESwitchCase
         * and size parameter is IN. Then we can use it to set the size
         * otherwise we have to use max.
         */
	// get size parameter
	CBETypedDeclarator *pSizeParam = 0;
	CBEAttribute *pAttr = pParameter->m_Attributes.Find(ATTR_SIZE_IS);
	if (!pAttr)
	    pAttr = pParameter->m_Attributes.Find(ATTR_LENGTH_IS);
	if (!pAttr)
	    pAttr = pParameter->m_Attributes.Find(ATTR_MAX_IS);
	if (pAttr && pAttr->IsOfType(ATTR_CLASS_IS))
	{
	    CBEDeclarator *pD = pAttr->m_Parameters.First();
	    if (pD)
		pSizeParam = pFunction->FindParameter(pD->GetName());
	}
	string sAppend;
	if (pSizeParam && pSizeParam->m_Attributes.Find(ATTR_IN))
	{
            pParameter->WriteGetSize(pFile, &vStack, pFunction);
	    if (pType->GetSize() > 1)
	    {
		*pFile << "*sizeof";
		pType->WriteCast(pFile, false);
		sAppend = " /* allocated for unbound array dimensions */";
	    }
	}
        else
        {
            int nSize = 0;
	    pAttr = pParameter->m_Attributes.Find(ATTR_MAX_IS);
	    if (pAttr && pAttr->IsOfType(ATTR_CLASS_INT))
	    {
		nSize = pAttr->GetIntValue();
		sAppend = " /* allocated using max_is attribute */";
	    }
	    else
	    {
		pParameter->GetMaxSize(nSize);
		sAppend = " /* allocated using max size of type */";
	    }
            *pFile << nSize;
        }
	*pFile << ");" << sAppend << "\n";
    }
}

/** \brief writes the cleanup routines for dynamically allocated variables
 *  \param pFile the file to write to
 *  \param bUsePointer true if the variable uses a pointer
 *  \param bDeferred true if deferred cleanup is intended
 */
void CBEDeclarator::WriteCleanup(CBEFile * pFile, bool bUsePointer,
    bool bDeferred)
{
    // get function and parameter
    CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
    assert(pFunction);
    CBETypedDeclarator *pParameter = pFunction->FindParameter(m_sName);
    assert(pParameter);
    CBEType *pType = pParameter->GetTransmitType();
    // skip the memory allocation if not preallocated
    if (!pParameter->m_Attributes.Find(ATTR_PREALLOC_CLIENT) &&
	!pParameter->m_Attributes.Find(ATTR_PREALLOC_SERVER) &&
	!bDeferred)
	return;

    CBETypedDeclarator *pEnv = pFunction->GetEnvironment();
    CBEDeclarator *pDecl = pEnv->m_Declarators.First();
    if (!pDecl && bDeferred)
	return;
    // calculate stars: see explaination above
    // (WriteIndirectInitializationMemory)
    int nFakeStars = GetEmptyArrayDims();
    int nTypeStars = pParameter->GetType()->GetIndirectionCount();
    int nStartStars = m_nStars + nTypeStars + nFakeStars;
    if (bUsePointer && (m_nStars > 0) && (nFakeStars == 0))
        nFakeStars = 1;
    int nOffset = 1;
    if (pType->IsConstructedType() && (nTypeStars == 0) && (nFakeStars == 0))
	nOffset = 0;
    for (int nStars = nStartStars; nStars > nFakeStars; nStars--)
    {
	*pFile << "\t";
	if (bDeferred)
	{
	    *pFile << "dice_set_ptr(";
	    if (pDecl->GetStars() == 0)
		*pFile << "&";
	    pDecl->WriteName(pFile);
	    *pFile << ", ";
	}
	else
	{
	    CBEContext::WriteFree(pFile, GetSpecificParent<CBEFunction>());
	    *pFile << "(";
	}
        for (int j = 0; j < nStartStars - nStars + nOffset; j++)
	    *pFile << "_";
	*pFile << m_sName << ");\n";
    }
}

/** \brief tests if this declarator is an array declarator
 *  \return true if it is, false if not
 */
bool CBEDeclarator::IsArray()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	"CBEDeclarator::%s Test if %s is array (%s)\n", __func__, 
	m_sName.c_str(), (m_nType == DECL_ARRAY) ? "yes" : "no");
    return (m_nType == DECL_ARRAY);
}

/** \brief calculates the number of array dimensions
 *  \return the number of array dimensions
 *
 * One array dimension is an expression in '['']' pairs.
 */
int CBEDeclarator::GetArrayDimensionCount()
{
    if (!IsArray())
        return 0;
    return m_Bounds.size();
}

/** \brief calculates the number of "fake" pointers
 *  \return the number of unbound array dimensions
 *
 * A fake pointer is an unbound array dimension. It is declared as array
 * dimension, but basicaly handled in C as pointer.
 */
int CBEDeclarator::GetEmptyArrayDims()
{
    if (!IsArray())
        return 0;
    int nFakeStars = 0;
    vector<CBEExpression*>::iterator iterB;
    for (iterB = m_Bounds.begin();
	 iterB != m_Bounds.end();
	 iterB++)
    {
        if ((*iterB)->GetIntValue() == 0)
            nFakeStars++;
    }
    return nFakeStars;
}

/** \brief calculates the number of array bounds from the given iterator to the end
 *  \param iter the iterator pointing to the next array bounds
 *  \return the number of array bounds from the iterator to the end of the vector
 */
int
CBEDeclarator::GetRemainingNumberOfArrayBounds(
    vector<CBEExpression*>::iterator iter)
{
    int nCount;
    for (nCount = 0; iter != m_Bounds.end(); iter++, nCount++) ;
    return nCount;
}
