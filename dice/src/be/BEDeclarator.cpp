 /**
  *	\file	dice/src/be/BEDeclarator.cpp
  *	\brief	contains the implementation of the class CBEDeclarator
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

#include "be/BEDeclarator.h"
#include "be/BEContext.h"
#include "be/BEExpression.h"
#include "be/BEFile.h"
#include "be/BEFunction.h"
#include "be/BETarget.h"
#include "be/BEComponent.h"
#include "be/BEClient.h"
#include "be/BETestsuite.h"
#include "be/BETypedDeclarator.h"

#include "fe/FEDeclarator.h"
#include "fe/FEEnumDeclarator.h"
#include "fe/FEArrayDeclarator.h"
#include "fe/FEBinaryExpression.h"

IMPLEMENT_DYNAMIC(CBEDeclarator);
IMPLEMENT_DYNAMIC(CDeclaratorStackLocation);
IMPLEMENT_DYNAMIC(CDeclaratorStack);

/** \brief writes the declarator stack
 *  \param pFile the file to write to
 *  \param bUsePointer true if one star should be ignored
 *  \param bFirstIsGlobal true if the first declarator is global
 *  \param pContext the context of the write operation
 */
void CDeclaratorStack::Write(CBEFile *pFile, bool bUsePointer, bool bFirstIsGlobal, CBEContext *pContext)
{
    CBEFunction *pFunction = NULL;
    if (GetTop())
        if (GetTop()->pDeclarator)
            pFunction = GetTop()->pDeclarator->GetFunction();
    bool bIsFirst = bFirstIsGlobal;
    for (VectorElement *pIter = vStack.GetLast(); pIter; pIter = pIter->GetPrev())
    {
        CDeclaratorStackLocation *pCurLoc = (CDeclaratorStackLocation*)pIter->GetElement();
        ASSERT(pCurLoc);
        CBEDeclarator *pDecl = pCurLoc->pDeclarator;
        int nStars = pDecl->GetStars();
        if (bUsePointer)
            nStars--;
        else
        {
            // test for an array, which only has stars as array dimensions
            if (((pCurLoc->nIndex >= 0) || (pCurLoc->nIndex == -2))
                && (pDecl->GetArrayDimensionCount() == 0) && (nStars > 0))
                nStars--;
        }
        if ((pFunction) && (pCurLoc->nIndex != -3))
        {
            if (pFunction->HasAdditionalReference(pCurLoc->pDeclarator, pContext))
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
            pDecl->WriteGlobalName(pFile, pContext);
        if (pCurLoc->nIndex >= 0)
            pFile->Print("[%d]", pCurLoc->nIndex);
        if (pCurLoc->nIndex == -2)
            pFile->Print("[%s]", (const char*)pCurLoc->sIndex);
        if (pIter->GetPrev())
            pFile->Print(".");
        bIsFirst = false;
    }
}

CBEDeclarator::CBEDeclarator()
{
    m_nStars = 0;
    m_nBitfields = 0;
    m_nType = DECL_NONE;
    m_pBounds = 0;
    m_pInitialValue = 0;
    IMPLEMENT_DYNAMIC_BASE(CBEDeclarator, CBEObject);
}

CBEDeclarator::CBEDeclarator(CBEDeclarator & src):CBEObject(src)
{
    m_nStars = src.m_nStars;
    m_nBitfields = src.m_nBitfields;
    m_sName = src.m_sName;
    m_nType = src.m_nType;
    m_pBounds = src.m_pBounds;
    m_pInitialValue = src.m_pInitialValue;
    IMPLEMENT_DYNAMIC_BASE(CBEDeclarator, CBEObject);
}

/**	\brief destructor of this instance */
CBEDeclarator::~CBEDeclarator()
{

}

/** \brief prepares this instance for the code generation
 *	 \param pFEDeclarator the corresponding front-end declarator
 *	 \param pContext the context of the code generation
 *	 \return true if the code generation was successful
 *
 * This implementation extracts the name, stars and bitfields from the front-end declarator.
 */
bool CBEDeclarator::CreateBackEnd(CFEDeclarator * pFEDeclarator, CBEContext * pContext)
{
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

/**	\brief create a simple declarator using name and number of stars
 *	\param sName the name of the declarator
 *	\param nStars the number of asterisks
 *	\param pContext the context of the code creation
 *	\return true if successful
 */
bool CBEDeclarator::CreateBackEnd(String sName, int nStars, CBEContext * pContext)
{
    VERBOSE("CBEDeclarator::CreateBE(string)\n");
    m_sName = sName;
    m_nStars = nStars;
    return true;
}

/**	\brief creates the back-end representation for the enum declarator
 *	\param pFEEnumDeclarator the front-end declarator
 *	\param pContext the context of the code generation
 *	\return true if code generation was successful
 */
bool CBEDeclarator::CreateBackEndEnum(CFEEnumDeclarator * pFEEnumDeclarator, CBEContext * pContext)
{
    VERBOSE("%s(enum)\n",__PRETTY_FUNCTION__);
    if (pFEEnumDeclarator->GetInitialValue())
    {
        m_pInitialValue  = pContext->GetClassFactory()->GetNewExpression();
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
 *	 \param pFEArrayDeclarator the respective front-end declarator
 *	 \param pContext the context of the code generation
 *	 \return true if code generation was successful
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

/**	\brief writes the declarator to the target file
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
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
    pFile->Print("%s", (const char *) m_sName);
    // write bitfields
    if (m_nBitfields > 0)
	    pFile->Print(":%d", m_nBitfields);
    // array dimensions
    if (IsArray())
	    WriteArray(pFile, pContext);
}

/**	\brief only returns a reference to the internal name
 *	\return the name of the declarator (without stars and such)
 */
String CBEDeclarator::GetName()
{
    return m_sName;
}

/**	\brief writes an array declarator
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation has to write the array dimensions only. This is done by iterating over
 * them and writing them into brackets ('[]').
 */
void CBEDeclarator::WriteArray(CBEFile * pFile, CBEContext * pContext)
{
    VectorElement *pIter = GetFirstArrayBound();
    CBEExpression *pBound;
    while ((pBound = GetNextArrayBound(pIter)) != 0)
    {
        pFile->Print("[");
        pBound->Write(pFile, pContext);
        pFile->Print("]");
    }
}

/**	\brief write an array declarator
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The differene to WriteArray is to skip unbound array dimensions.
 * We can determine an unbound array dimension by checking its integer value.
 * It it is 0 then its an unbound dimension.
 */
void CBEDeclarator::WriteArrayIndirect(CBEFile * pFile, CBEContext * pContext)
{
    VectorElement *pIter = GetFirstArrayBound();
    CBEExpression *pBound;
    while ((pBound = GetNextArrayBound(pIter)) != 0)
    {
        if (pBound->GetIntValue() == 0)
            continue;
        pFile->Print("[");
        pBound->Write(pFile, pContext);
        pFile->Print("]");
    }
}

/**	\brief writes an enum declarator
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 */
void CBEDeclarator::WriteEnum(CBEFile * pFile, CBEContext * pContext)
{
    ASSERT(false);
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
    if (!m_pBounds)
    {
        m_pBounds = new Vector(RUNTIME_CLASS(CBEExpression));
        m_nOldType = m_nType;
        m_nType = DECL_ARRAY;
    }
    m_pBounds->Add(pBound);
    pBound->SetParent(this);
}

/** \brief removes an array bound from the bounds vector
 *  \param pBound the bound to remove
 */
void CBEDeclarator::RemoveArrayBound(CBEExpression *pBound)
{
    if (!pBound)
        return;
    if (!m_pBounds)
        return;
    m_pBounds->Remove(pBound);
    if (m_pBounds->GetSize() == 0)
    {
        delete m_pBounds;
        m_pBounds = 0;
        m_nType = m_nOldType;
    }
}

/** \brief retrieves a pointer to the first array bound
 *  \return pointer to first array bound
 */
VectorElement *CBEDeclarator::GetFirstArrayBound()
{
    if (!m_pBounds)
        return 0;
    return m_pBounds->GetFirst();
}

/** \brief retrieves a reference to the next array bound
 *  \param pIter the pointer to the next array boundary
 *  \return a reference to the next array boundary
 */
CBEExpression *CBEDeclarator::GetNextArrayBound(VectorElement * &pIter)
{
    if (!pIter)
        return 0;
    if (!m_pBounds)
        return 0;
    CBEExpression *pRet = (CBEExpression *) pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextArrayBound(pIter);
    return pRet;
}

/**	\brief calculates the size of the declarator
 *	\return size of the declarator (or -x if pointer, where x is the number of stars)
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

    VectorElement *pIter = GetFirstArrayBound();
    CBEExpression *pBound;
    while ((pBound = GetNextArrayBound(pIter)) != 0)
    {
        int nVal = pBound->GetIntValue();
        if (nVal == 0)	// no integer value
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

    int nSize = 1;
    int nFakeStars = 0;

    VectorElement *pIter = GetFirstArrayBound();
    CBEExpression *pBound;
    while ((pBound = GetNextArrayBound(pIter)) != 0)
    {
        int nVal = pBound->GetIntValue();
        if (nVal == 0)	// no integer value
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
 *  \param pContext the context of the write operation
 *  \param bWriteArray true if the array dimensions should be written (you sometime want to turn them off)
 *
 * We do not write the pointers and not the bitfields, but the array dimension we write.
 */
void CBEDeclarator::WriteGlobalName(CBEFile * pFile, CBEContext * pContext, bool bWriteArray)
{
    String sGlobalVar = pContext->GetNameFactory()->GetGlobalTestVariable(this, pContext);
    pFile->Print("%s", (const char *) sGlobalVar);
    if (IsArray() && bWriteArray)
        WriteArray(pFile, pContext);
}

/** \brief simply prints the name of the declarator
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBEDeclarator::WriteName(CBEFile * pFile, CBEContext * pContext)
{
    pFile->Print("%s", (const char *) m_sName);
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
    if (bUsePointer && (nStartStars > 0))
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
        pFile->Print("%s", (const char *) m_sName);
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
    if (bUsePointer)
        nStartStars--;
    for (int nStars = nStartStars; nStars > nFakeStars; nStars--)
    {
        pFile->PrintIndent("");
        // write name (one _ less)
        for (i = 0; i < (nStartStars - nStars); i++)
            pFile->Print("_");
        pFile->Print("%s = ", (const char *) m_sName);
        // write name (one more _)
        pFile->Print("&");
        for (i = 0; i < (nStartStars - nStars) + 1; i++)
            pFile->Print("_");
        pFile->Print("%s;\n", (const char *) m_sName);
    }
    /** now we have to initialize the fake stars (unbound array dimensions).
     * problem is to use an allocation routine appropriate for this. If there is
     * an max_is or upper bound it is used, but we have no upper bound.
     * CORBA defines to use CORBA_alloc, but this requires size and we don't
     * know how big it actually will be
     *
     * \todo maybe we can propagate the max size when connecting to the server...
     */
	CBEFunction *pFunction = GetFunction();
    for (i = 0; i < nFakeStars; i++)
    {
        if (pContext->IsWarningSet(PROGRAM_WARNING_PREALLOC))
        {
            if (pFunction)
            {
                String sFuncName = pFunction->GetName();
                CCompiler::Warning("CORBA_alloc is used to allocate memory for %s in %s.", (const char*)m_sName, (const char*)sFuncName);
            }
            else
                CCompiler::Warning("CORBA_alloc is used to allocate memory for %s.", (const char*)m_sName);
        }
		pFile->PrintIndent("%s = ", (const char *) m_sName);
		if ((pFunction && 
		    ((pContext->IsOptionSet(PROGRAM_SERVER_PARAMETER) && pFunction->IsComponentSide()) ||
			  !pFunction->IsComponentSide())) &&
			  !pContext->IsOptionSet(PROGRAM_FORCE_CORBA_ALLOC))
		{
			CBETypedDeclarator* pEnv = pFunction->GetEnvironment();
			CBEDeclarator *pDecl = 0;
			if (pEnv)
			{
				VectorElement* pIter = pEnv->GetFirstDeclarator();
				pDecl = pEnv->GetNextDeclarator(pIter);
			}
			if (pDecl)
			{
				pFile->Print("(%s", (const char*)pDecl->GetName());
				if (pDecl->GetStars())
					pFile->Print("->malloc)");
				else
					pFile->Print(".malloc)");
			}
			else
				pFile->Print("CORBA_alloc");
		}
		else
		    pFile->Print("CORBA_alloc");
        pFile->Print("(MAX_ARRAY_SIZE * sizeof(*%s));\n", (const char *) m_sName);
    }
}

/** \brief tests if this declarator is an array declarator
 *  \return true if it is, false if not
 */
bool CBEDeclarator::IsArray()
{
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
    if (!m_pBounds)
        return 0;
    return m_pBounds->GetSize();
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
    VectorElement *pIter = GetFirstArrayBound();
    CBEExpression *pBound;
    while ((pBound = GetNextArrayBound(pIter)) != 0)
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
