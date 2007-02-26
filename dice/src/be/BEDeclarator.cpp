 /**
  *    \file    dice/src/be/BEDeclarator.cpp
  *    \brief   contains the implementation of the class CBEDeclarator
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

#include "BEDeclarator.h"
#include "BEContext.h"
#include "BEExpression.h"
#include "BEFile.h"
#include "BEFunction.h"
#include "BETarget.h"
#include "BEComponent.h"
#include "BEClient.h"
#include "BETestsuite.h"
#include "BETypedDeclarator.h"
#include "BEType.h"
#include "BEAttribute.h"
#include "BESwitchCase.h"
#include "BEStructType.h"
#include "BEUnionType.h"

#include "Attribute-Type.h"
#include "fe/FEDeclarator.h"
#include "fe/FEEnumDeclarator.h"
#include "fe/FEArrayDeclarator.h"
#include "fe/FEBinaryExpression.h"


/** \brief writes the declarator stack
 *  \param pFile the file to write to
 *  \param pStack the declarator stack to write
 *  \param bUsePointer true if one star should be ignored
 *  \param bFirstIsGlobal true if the first declarator is global
 *  \param pContext the context of the write operation
 */
void CDeclaratorStackLocation::Write(CBEFile *pFile,
    vector<CDeclaratorStackLocation*> *pStack,
    bool bUsePointer,
    bool bFirstIsGlobal,
    CBEContext *pContext)
{
    assert(pStack);

    CBEFunction *pFunction = NULL;
    CDeclaratorStackLocation *pLast = pStack->front();
    if (pLast && pLast->pDeclarator)
        pFunction = pLast->pDeclarator->GetSpecificParent<CBEFunction>();
    bool bIsFirst = bFirstIsGlobal;

    vector<CDeclaratorStackLocation*>::iterator iter;
    for (iter = pStack->begin(); iter != pStack->end(); iter++)
    {
        CBEDeclarator *pDecl = (*iter)->pDeclarator;
        assert(pDecl);
        int nStars = pDecl->GetStars();
        // test for empty array dimensions, which are treated as
        // stars
        if (pDecl->GetArrayDimensionCount())
        {
            int nLevel = 0;
            vector<CBEExpression*>::iterator iterB = pDecl->GetFirstArrayBound();
            CBEExpression *pExpr;
            while ((pExpr = pDecl->GetNextArrayBound(iterB)) != 0)
            {
                if (pExpr->IsOfType(EXPR_NONE))
                {
                    if (!(*iter)->HasIndex(nLevel++))
                        nStars++;
                }
            }
        }
        if ((iter + 1)  == pStack->end())
        {
             // only apply to last element in row
            if (bUsePointer)
                nStars--;
            else
            {
                // test for an array, which only has stars as array dimensions
                if ((*iter)->HasIndex() && (pDecl->GetArrayDimensionCount() == 0) && (nStars > 0))
                    nStars--;
            }
        }
        if ((pFunction) && ((*iter)->nIndex[0] != -3))
        {
            if (pFunction->HasAdditionalReference(pDecl, pContext))
                nStars++;
        }
        // check if the type is a pointer type
        if (pFunction)
        {
            CBETypedDeclarator *pParameter = pFunction->FindParameter(pDecl->GetName(), false);
            // XXX FIXME: transmit_as ?
            if (pParameter && pParameter->GetType() &&
                pParameter->GetType()->IsPointerType() &&
                !pParameter->IsString())
                nStars++;
        }
        if (!bIsFirst)
        {
            if (nStars > 0)
                pFile->Print("(");
            for (int i=0; i<nStars; i++)
                pFile->Print("*");
            pDecl->WriteName(pFile, pContext);
            if (nStars > 0)
                pFile->Print(")");
        }
        else
            pDecl->WriteGlobalName(pFile, 0, pContext);
        for (int i=0; i<(*iter)->GetUsedIndexCount(); i++)
        {
            if ((*iter)->nIndex[i] >= 0)
                pFile->Print("[%d]", (*iter)->nIndex[i]);
            if ((*iter)->nIndex[i] == -2)
                pFile->Print("[%s]", (*iter)->sIndex[i].c_str());
        }
        if ((iter + 1) != pStack->end())
            pFile->Print(".");
        bIsFirst = false;
    }
}

CBEDeclarator::CBEDeclarator()
{
    m_nStars = 0;
    m_nBitfields = 0;
    m_nType = DECL_NONE;
    m_pInitialValue = 0;
}

CBEDeclarator::CBEDeclarator(CBEDeclarator & src)
: CBEObject(src)
{
    m_nStars = src.m_nStars;
    m_nBitfields = src.m_nBitfields;
    m_sName = src.m_sName;
    m_nType = src.m_nType;
    vector<CBEExpression*>::iterator iter;
    for (iter = src.m_vBounds.begin(); iter != src.m_vBounds.end(); iter++)
    {
        CBEExpression *pNew = (CBEExpression*)((*iter)->Clone());
        m_vBounds.push_back(pNew);
        pNew->SetParent(this);
    }
    if (src.m_pInitialValue)
    {
        m_pInitialValue = (CBEExpression*)src.m_pInitialValue->Clone();
        m_pInitialValue->SetParent(this);
    }
    else
        m_pInitialValue = 0;
}

/**    \brief destructor of this instance */
CBEDeclarator::~CBEDeclarator()
{
    while (!m_vBounds.empty())
    {
        delete m_vBounds.back();
        m_vBounds.pop_back();
    }
    if (m_pInitialValue)
        delete m_pInitialValue;
}

/** \brief prepares this instance for the code generation
 *     \param pFEDeclarator the corresponding front-end declarator
 *     \param pContext the context of the code generation
 *     \return true if the code generation was successful
 *
 * This implementation extracts the name, stars and bitfields from the front-end declarator.
 */
bool CBEDeclarator::CreateBackEnd(CFEDeclarator * pFEDeclarator, CBEContext * pContext)
{
    // call CBEObject's CreateBackEnd method
    if (!CBEObject::CreateBackEnd(pFEDeclarator))
        return false;

    m_sName = pFEDeclarator->GetName();
    m_nBitfields = pFEDeclarator->GetBitfields();
    m_nStars = pFEDeclarator->GetStars();
    m_nType = pFEDeclarator->GetType();
    switch (m_nType)
    {
    case DECL_ARRAY:
        return CreateBackEndArray((CFEArrayDeclarator *) pFEDeclarator, pContext);
        break;
    case DECL_ENUM:
        return CreateBackEndEnum((CFEEnumDeclarator *) pFEDeclarator, pContext);
        break;
    }
    return true;
}

/**    \brief create a simple declarator using name and number of stars
 *    \param sName the name of the declarator
 *    \param nStars the number of asterisks
 *    \param pContext the context of the code creation
 *    \return true if successful
 */
bool CBEDeclarator::CreateBackEnd(string sName, int nStars, CBEContext * pContext)
{
    VERBOSE("CBEDeclarator::CreateBE(string: %s)\n", sName.c_str());
    m_sName = sName;
    m_nStars = nStars;
    return true;
}

/**    \brief creates the back-end representation for the enum declarator
 *    \param pFEEnumDeclarator the front-end declarator
 *    \param pContext the context of the code generation
 *    \return true if code generation was successful
 */
bool CBEDeclarator::CreateBackEndEnum(CFEEnumDeclarator * pFEEnumDeclarator, CBEContext * pContext)
{
    VERBOSE("%s(enum)\n",__PRETTY_FUNCTION__);
    if (pFEEnumDeclarator->GetInitialValue())
    {
        m_pInitialValue = pContext->GetClassFactory()->GetNewExpression();
        m_pInitialValue->SetParent(this);
        if (!m_pInitialValue->CreateBackEnd(pFEEnumDeclarator->GetInitialValue(), pContext))
        {
            delete m_pInitialValue;
            m_pInitialValue = 0;
            VERBOSE("%s failed because initial value could not be initialized\n",__PRETTY_FUNCTION__);
            return false;
        }
    }
    return true;
}

/** \brief creates the back-end representation for the array declarator
 *     \param pFEArrayDeclarator the respective front-end declarator
 *     \param pContext the context of the code generation
 *     \return true if code generation was successful
 */
bool CBEDeclarator::CreateBackEndArray(CFEArrayDeclarator * pFEArrayDeclarator, CBEContext * pContext)
{
    // iterate over front-end bounds
    int nMax = pFEArrayDeclarator->GetDimensionCount();
    for (int i = 0; i < nMax; i++)
    {
        CFEExpression *pLower = pFEArrayDeclarator->GetLowerBound(i);
        CFEExpression *pUpper = pFEArrayDeclarator->GetUpperBound(i);
        CBEExpression *pBound = GetArrayDimension(pLower, pUpper, pContext);
        AddArrayBound(pBound);
    }
    return true;
}

/**    \brief writes the declarator to the target file
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * This implementation writes the declarator as is (all stars, bitfields, etc.).
 */
void CBEDeclarator::WriteDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    if (m_nType == DECL_ENUM)
    {
        WriteEnum(pFile, pContext);
        return;
    }
    // write stars
    for (int i = 0; i < m_nStars; i++)
        pFile->Print("*");
    // write name
    pFile->Print("%s", m_sName.c_str());
    // write bitfields
    if (m_nBitfields > 0)
        pFile->Print(":%d", m_nBitfields);
    // array dimensions
    if (IsArray())
        WriteArray(pFile, pContext);
}

/**    \brief only returns a reference to the internal name
 *    \return the name of the declarator (without stars and such)
 */
string CBEDeclarator::GetName()
{
    return m_sName;
}

/**    \brief writes an array declarator
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * This implementation has to write the array dimensions only. This is done by iterating over
 * them and writing them into brackets ('[]').
 */
void CBEDeclarator::WriteArray(CBEFile * pFile, CBEContext * pContext)
{
    vector<CBEExpression*>::iterator iterB = GetFirstArrayBound();
    CBEExpression *pBound;
    while ((pBound = GetNextArrayBound(iterB)) != 0)
    {
        pFile->Print("[");
        pBound->Write(pFile, pContext);
        pFile->Print("]");
    }
}

/**    \brief write an array declarator
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * The differene to WriteArray is to skip unbound array dimensions.
 * We can determine an unbound array dimension by checking its integer value.
 * It it is 0 then its an unbound dimension.
 */
void CBEDeclarator::WriteArrayIndirect(CBEFile * pFile, CBEContext * pContext)
{
    vector<CBEExpression*>::iterator iterB = GetFirstArrayBound();
    CBEExpression *pBound;
    while ((pBound = GetNextArrayBound(iterB)) != 0)
    {
        if (pBound->GetIntValue() == 0)
            continue;
        pFile->Print("[");
        pBound->Write(pFile, pContext);
        pFile->Print("]");
    }
}

/**    \brief writes an enum declarator
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 */
void CBEDeclarator::WriteEnum(CBEFile * pFile, CBEContext * pContext)
{
    assert(false);
}

/** \brief creates a new back-end array bound using front-end array bounds
 *  \param pLower the lower array bound
 *  \param pUpper the upper array bound
 *  \param pContext the context of the code generation
 *  \return the new back-end array bound
 *
 * The expression may have the following values:
 * -# pLower == 0 && pUpper == 0
 * -# pLower == 0 && pUpper != 0
 * -# pLower != 0 && pUpper != 0
 * There is not such case, that pLower != 0 and pUpper == 0.
 *
 * To let the CreateBE function calculate the value of the expression, we simply define the following
 * resulting front-end expression used by the CreateBE function (respective to the above cases):
 * -# pNew = '*'
 * -# pNew = pUpper
 * -# pNew = (pUpper)-(pLower)
 *
 * For the latter case this implementation creates two primary expression, which are the parenthesis and
 * a binary expression used to subract them.
 */
CBEExpression *CBEDeclarator::GetArrayDimension(CFEExpression * pLower, CFEExpression * pUpper, CBEContext * pContext)
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
            return GetArrayDimension((CFEExpression *) 0, pUpper, pContext);
        }
        // if upper is '*' than array has no bound
        if ((pUpper->GetType() == EXPR_CHAR) && (pUpper->GetChar() == '*'))
        {
            pNew = new CFEExpression(EXPR_NONE);
        }
        else
        {
            CFEExpression *pLowerP = new CFEPrimaryExpression(EXPR_PAREN, pLower);
            CFEExpression *pUpperP = new CFEPrimaryExpression(EXPR_PAREN, pUpper);
            pNew = new CFEBinaryExpression(EXPR_BINARY, pUpperP, EXPR_MINUS, pLowerP);
        }
    }
    // create new back-end expression
    CBEExpression *pReturn = pContext->GetClassFactory()->GetNewExpression();
    pReturn->SetParent(this);
    if (!pReturn->CreateBackEnd(pNew, pContext))
    {
        delete pReturn;
        return 0;
    }

    return pReturn;
}

/** \brief adds another array bound to the bounds vector
 *  \param pBound the bound to add
 *
 * This implementation creates the vector if not existing
 */
void CBEDeclarator::AddArrayBound(CBEExpression * pBound)
{
    if (!pBound)
        return;
    if (m_vBounds.empty())
    {
        m_nOldType = m_nType;
        m_nType = DECL_ARRAY;
    }
    m_vBounds.push_back(pBound);
    pBound->SetParent(this);
}

/** \brief removes an array bound from the bounds vector
 *  \param pBound the bound to remove
 */
void CBEDeclarator::RemoveArrayBound(CBEExpression *pBound)
{
    if (!pBound)
        return;
    vector<CBEExpression*>::iterator iter;
    for (iter = m_vBounds.begin(); iter != m_vBounds.end(); iter++)
    {
        if (*iter == pBound)
        {
            m_vBounds.erase(iter);
            break;
        }
    }
    if (m_vBounds.empty())
        m_nType = m_nOldType;
}

/** \brief retrieves a pointer to the first array bound
 *  \return pointer to first array bound
 */
vector<CBEExpression*>::iterator CBEDeclarator::GetFirstArrayBound()
{
    return m_vBounds.begin();
}

/** \brief retrieves a reference to the next array bound
 *  \param iter the pointer to the next array boundary
 *  \return a reference to the next array boundary
 */
CBEExpression *CBEDeclarator::GetNextArrayBound(vector<CBEExpression*>::iterator &iter)
{
    if (iter == m_vBounds.end())
        return 0;
    return *iter++;
}

/**    \brief calculates the size of the declarator
 *    \return size of the declarator (or -x if pointer, where x is the number of stars)
 *
 * The size of a declarator is 1. An exception is the array declarator, which's size is
 * calculated from the array dimensions. If the declarator has any stars in front of it,
 * it may have a undefined size, since the asterisk indicates pointers.
 *
 * If the integer value of an array bound is not determinable, we return the number of
 * unbound dimension plus the number of asterisks. We add these two values, so we can later
 * determine if the value returned by GetSize were only asterisks or also unbound array
 * dimensions.
 *
 * Another exception is the usage of bitfields. If the declarator has bitfields asscoiated
 * with it the function returns zero. The caller has to know that the size of zero means
 * possible bitfields and ask for them seperately.
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

    vector<CBEExpression*>::iterator iterB = GetFirstArrayBound();
    CBEExpression *pBound;
    while ((pBound = GetNextArrayBound(iterB)) != 0)
    {
        int nVal = pBound->GetIntValue();
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
 *  \param pContext the context of the size calculation
 *  \return maximum size in bytes
 */
int CBEDeclarator::GetMaxSize(CBEContext *pContext)
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
    else
    {
        if ((GetArrayDimensionCount() == 0) &&
            (m_nStars > 0))
        {
            // this is a weird situation:
            // we have an unbound array, but express it
            // using '*' instead of '[]'
            // Nonetheless the DECL_ARRAY is set...
            //
            // To make the MAX algorithms work, this has to
            // return a negative value
            return -(m_nStars);
        }
    }

    int nSize = 1;
    int nFakeStars = 0;

    vector<CBEExpression*>::iterator iterB = GetFirstArrayBound();
    CBEExpression *pBound;
    while ((pBound = GetNextArrayBound(iterB)) != 0)
    {
        int nVal = pBound->GetIntValue();
        if (nVal == 0)    // no integer value
            nFakeStars++;
        else
            nSize *= ((nVal < 0) ? -nVal : nVal);
    }

    if (nFakeStars > 0)
        return -(nFakeStars + m_nStars);
    return nSize;
}

/** \brief returns the number of stars
 *  \return the value of m_nStars
 */
int CBEDeclarator::GetStars()
{
    return m_nStars;
}

/** \brief writes the name of the declarator for a global test variable declaration
 *  \param pFile the file to write to
 *  \param pStack the current stack when writing the declarator
 *  \param pContext the context of the write operation
 *  \param bWriteArray true if the array dimensions should be written (you sometime want to turn them off)
 *
 * We do not write the pointers and not the bitfields, but the array dimension we write.
 * We skip those array dimensions, which have no boundary.
 */
void
CBEDeclarator::WriteGlobalName(CBEFile * pFile,
    vector<CDeclaratorStackLocation*> *pStack,
    CBEContext * pContext,
    bool bWriteArray)
{
    if (GetSpecificParent<CBEFunction>())
    {
        string sGlobalVar = pContext->GetNameFactory()->GetGlobalTestVariable(this, pContext);
        pFile->Print("%s", sGlobalVar.c_str());
        if (IsArray() && bWriteArray)
            WriteArrayIndirect(pFile, pContext);
    }
    else if (GetSpecificParent<CBEStructType>() ||
             GetSpecificParent<CBEUnionType>())
    {
        if (pStack)
        {
            CDeclaratorStackLocation *pTop = pStack->back();
            pStack->pop_back();
            CDeclaratorStackLocation::Write(pFile, pStack, false, true, pContext);
            pStack->push_back(pTop);
            pFile->Print(".");
        }
        WriteName(pFile, pContext);
    }
}

/** \brief simply prints the name of the declarator
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBEDeclarator::WriteName(CBEFile * pFile, CBEContext * pContext)
{
    pFile->Print("%s", m_sName.c_str());
}

/** \brief writes for every pointer an extra declarator without this pointer
 *  \param pFile the file to write to
 *  \param bUsePointer true if the variable is intended to be used as a pointer
 *  \param bHasPointerType true if the decl's type is a pointer type
 *  \param pContext the context of the write operation
 *
 * If we have an array declarator with unbound array dimensions, we
 * have to write them as pointers as well. We can determine these "fake"
 * pointers by checking GetSize.
 *
 * We do not need dereferenced declarators for these unbound arrays.
 *
 * \todo indirect var by underscore hard coded => replace with configurable
 */
void CBEDeclarator::WriteIndirect(CBEFile * pFile, bool bUsePointer, bool bHasPointerType, CBEContext * pContext)
{
    //     if (m_nType == DECL_ENUM)
    //     {
    //             WriteEnum(pFile, pContext);
    //             return;
    //     }

    int nFakeStars = GetFakeStars();
    int nStartStars = m_nStars + nFakeStars;
    // for something like:
    // 'char *buf' we want a declaration of 'char *buf'
    //   (m_nStars:1 nFakeStars:0 bUsePointer:true -> nStartStars:1 nFakeStars:1)
    // 'char buf[]' we want a declaration of 'char *buf'
    //   (m_nStars:0 nFakeStars:1 bUsePointer:true -> nStartStars:1 nFakeStars:1)
    // 'char *buf[]' we want a declaration of 'char **buf, *_buf'
    //   (m_nStars:1 nFakeStars:1 bUsePointer:true -> nStartStars:2 nFakeStars:1)
    // 'char **buf' we want a declaration of 'char **buf, *_buf'
    //   (m_nStars:2 nFakeStars:0 bUsePointer:true -> nStartStars:2 nFakeStars:1)
    if (bUsePointer && (nFakeStars == 0) && (m_nStars > 0))
        nFakeStars++;
    bool bComma = false;
    for (int nStars = nStartStars; nStars >= nFakeStars; nStars--)
    {
        if (bComma)
            pFile->Print(", ");
        int i;
        // if not first and we have pointer type, write a stars for it
        if (bComma && bHasPointerType)
            pFile->Print("*");
        // write stars
        for (i = 0; i < nStars; i++)
             pFile->Print("*");
        // write underscores
        for (i = 0; i < (nStartStars - nStars); i++)
             pFile->Print("_");
        // write name
        pFile->Print("%s", m_sName.c_str());
        // next please
        bComma = true;
    }

    // we make the last declarator the array declarator
    if (IsArray())
        WriteArrayIndirect(pFile, pContext);

    // write bitfields
    //     if (m_nBitfields > 0)
    //             pFile->Print(":%d", m_nBitfields);
}

/** \brief assigns pointered variables a reference to "unpointered" variables
 *  \param pFile the file to write to
 *  \param bUsePointer true if the variable is intended to be used as a pointer
 *  \param pContext the context of the write operation
 *
 * Does something like "t1 = \&_t1;" for a variable "CORBA_long *t1"
 *
 * \todo indirect var by underscore hard coded => replace with configurable
 */
void CBEDeclarator::WriteIndirectInitialization(CBEFile * pFile, bool bUsePointer, CBEContext * pContext)
{
    int nFakeStars = GetFakeStars();
    int i, nStartStars = m_nStars + nFakeStars;
    if (bUsePointer && (m_nStars > 0) && (nFakeStars == 0))
        nFakeStars++;
    // TODO: when adding variable declaration use "temp" var for indirection
    for (int nStars = nStartStars; nStars > nFakeStars; nStars--)
    {
        pFile->PrintIndent("");
        // write name (one _ less)
        for (i = 0; i < (nStartStars - nStars); i++)
            pFile->Print("_");
        pFile->Print("%s = ", m_sName.c_str());
        // write name (one more _)
        pFile->Print("&");
        for (i = 0; i < (nStartStars - nStars) + 1; i++)
            pFile->Print("_");
        pFile->Print("%s;\n", m_sName.c_str());
    }
}

/** \brief assigns pointered variables a reference to "unpointered" variables
 *  \param pFile the file to write to
 *  \param bUsePointer true if the variable is intended to be used as a pointer
 *  \param pContext the context of the write operation
 *
 * Does something like "t1 = \&_t1;" for a variable "CORBA_long *t1"
 *
 * \todo indirect var by underscore hard coded => replace with configurable
 */
void CBEDeclarator::WriteIndirectInitializationMemory(CBEFile * pFile, bool bUsePointer, CBEContext * pContext)
{
    assert(pFile);
    int nFakeStars = GetFakeStars();
    int nStartStars = m_nStars + nFakeStars;
    if (bUsePointer && (m_nStars > 0) && (nFakeStars == 0))
        nFakeStars++;
    // get function and parameter
    CBETypedDeclarator *pParameter = 0;
    CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
    if (pFunction)
        pParameter = pFunction->FindParameter(m_sName);
    /** now we have to initialize the fake stars (unbound array dimensions).
     * problem is to use an allocation routine appropriate for this. If there is
     * an max_is or upper bound it is used, but we have no upper bound.
     * CORBA defines to use CORBA_alloc, but this requires size and we don't
     * know how big it actually will be
     *
     * \todo maybe we can propagate the max size when connecting to the server...
     */
    for (int i = 0; i < nFakeStars; i++)
    {
        vector<CDeclaratorStackLocation*> vStack;
        CDeclaratorStackLocation *pLoc = new CDeclaratorStackLocation(this);
        vStack.push_back(pLoc);
        pFile->PrintIndent("");
        for (i = 0; i < nStartStars - nFakeStars; i++)
            pFile->Print("_");
        pFile->Print("%s = ", m_sName.c_str());
        pParameter->GetType()->WriteCast(pFile, true, pContext);
        pContext->WriteMalloc(pFile, pFunction);
        pFile->Print("(");
        /**
         * if the parameter is out and size parameter, then this
         * initialization is done before a call(testsuite) or component function.
         * We can use the size parameter only if it is set (it can't just
         * before the call, but it is before the component if the size
         * parameter is an IN.) So we check if parent func is CBESwitchCase
         * and size parameter is IN. Then we can use it to set the size
         * otherwise we have to use max.
         */
        bool bUseSize = false;
        if (pFunction && dynamic_cast<CBESwitchCase*>(pFunction))
        {
            // get size parameter
            CBETypedDeclarator *pSizeParam = 0;
            CBEAttribute *pAttr = pParameter->FindAttribute(ATTR_SIZE_IS);
            if (!pAttr)
                pAttr = pParameter->FindAttribute(ATTR_LENGTH_IS);
            if (!pAttr)
                pAttr = pParameter->FindAttribute(ATTR_MAX_IS);
            if (pAttr && pAttr->IsOfType(ATTR_CLASS_IS))
            {
                vector<CBEDeclarator*>::iterator iterD =
                    pAttr->GetFirstIsAttribute();
                CBEDeclarator *pD = pAttr->GetNextIsAttribute(iterD);
                if (pD)
                    pSizeParam = pFunction->FindParameter(pD->GetName());
            }
            if (pSizeParam && pSizeParam->FindAttribute(ATTR_IN))
                bUseSize = true;
        }
        if (bUseSize)
            pParameter->WriteGetSize(pFile, &vStack, pContext);
        else
            pFile->Print("%d", pParameter->GetMaxSize(true, pContext));
        pFile->Print("); /* fake */\n");
        delete pLoc;
    }
}

/** \brief writes the cleanup routines for dynamically allocated variables
 *  \param pFile the file to write to
 *  \param bUsePointer true if the variable uses a pointer
 *  \param pContext the context of the write operation
 */
void CBEDeclarator::WriteCleanup(CBEFile * pFile, bool bUsePointer, CBEContext * pContext)
{
    int nFakeStars = GetFakeStars();
    int nStartStars = m_nStars + nFakeStars;
    if (bUsePointer && (m_nStars > 0) && (nFakeStars == 0))
        nFakeStars++;
    /** now we have to free the fake stars (unbound array dimensions).
     *
     * \todo maybe we can propagate the max size when connecting to the server...
     */
    for (int i = 0; i < nFakeStars; i++)
    {
        pFile->PrintIndent("");
        pContext->WriteFree(pFile, GetSpecificParent<CBEFunction>());
        pFile->Print("(");
        for (i = 0; i < nStartStars - nFakeStars; i++)
            pFile->Print("_");
        pFile->Print("%s);\n", m_sName.c_str());
    }
}

/** \brief writes the deferred cleanup, which means that this pointer is only rgistered to be cleaned later
 *  \param pFile the file to write to
 *  \param bUsePointer true if variable uses a pointer
 *  \param pContext the context of the write operation
 *
 * Same code as in \ref WriteCleanup but uses dice_set_ptr(Env) instead of Free.
 */
void CBEDeclarator::WriteDeferredCleanup(CBEFile* pFile,  bool bUsePointer,  CBEContext* pContext)
{
    int nFakeStars = GetFakeStars();
    int nStartStars = m_nStars + nFakeStars;
    if (bUsePointer && (m_nStars > 0) && (nFakeStars == 0))
        nFakeStars++;
    // get environment
    CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
    CBEDeclarator *pDecl = 0;
    if (pFunction)
    {
        CBETypedDeclarator *pEnv = pFunction->GetEnvironment();
        vector<CBEDeclarator*>::iterator iterE = pEnv->GetFirstDeclarator();
        pDecl = pEnv->GetNextDeclarator(iterE);
    }
    if (!pDecl)
        return;

    /** now we have to free the fake stars (unbound array dimensions).
     *
     * \todo maybe we can propagate the max size when connecting to the server...
     */
    for (int i = 0; i < nFakeStars; i++)
    {
        pFile->PrintIndent("dice_set_ptr(");
        if (pDecl->GetStars() == 0)
            pFile->Print("&");
        pDecl->WriteName(pFile, pContext);
        pFile->Print(", ");
        for (i = 0; i < nStartStars - nFakeStars; i++)
            pFile->Print("_");
        pFile->Print("%s);\n", m_sName.c_str());
    }
}

/** \brief tests if this declarator is an array declarator
 *  \return true if it is, false if not
 */
bool CBEDeclarator::IsArray()
{
//     TRACE("Test if %s is array (%s)\n", m_sName.c_str(), (m_nType == DECL_ARRAY)?"yes":"no");
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
    return m_vBounds.size();
}

/** \brief calculates the number of "fake" pointers
 *  \return the number of unbound array dimensions
 *
 * A fake pointer is an unbound array dimension. It is declared as array dimension,
 * but basicaly handled in C as pointer.
 */
int CBEDeclarator::GetFakeStars()
{
    if (!IsArray())
        return 0;
    int nFakeStars = 0;
    vector<CBEExpression*>::iterator iterB = GetFirstArrayBound();
    CBEExpression *pBound;
    while ((pBound = GetNextArrayBound(iterB)) != 0)
    {
        if (pBound->GetIntValue() == 0)
            nFakeStars++;
    }
    return nFakeStars;
}

/** \brief returns the number of bitfields used by this declarator
 *  \return the value of the member m_nBitfields
 */
int CBEDeclarator::GetBitfields()
{
    return m_nBitfields;
}

/** \brief modifies the number of stars
 *  \param nBy the number to add to m_nStars (if it is negative it decrements)
 *  \return the new number of stars
 */
int CBEDeclarator::IncStars(int nBy)
{
    m_nStars += nBy;
    return m_nStars;
}

/** \brief creates a new instance of this class */
CObject * CBEDeclarator::Clone()
{
    return new CBEDeclarator(*this);
}

/** \brief calculates the number of array bounds from the given iterator to the end
 *  \param iter the iterator pointing to the next array bounds
 *  \return the number of array bounds from the iterator to the end of the vector
 */
int CBEDeclarator::GetRemainingNumberOfArrayBounds(vector<CBEExpression*>::iterator iter)
{
    int nCount = 0;
    while (GetNextArrayBound(iter)) nCount++;
    return nCount;
}
