/**
 *    \file    dice/src/fe/FETypedDeclarator.cpp
 *    \brief   contains the implementation of the class CFETypedDeclarator
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

#include "fe/FETypedDeclarator.h"
#include "fe/FESimpleType.h"
#include "fe/FEStructType.h"
#include "fe/FEUnionType.h"
#include "fe/FEUserDefinedType.h"
#include "fe/FEArrayType.h"
#include "fe/FEArrayDeclarator.h"
#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "fe/FEExpression.h"
#include "fe/FEIntAttribute.h"
#include "fe/FEIsAttribute.h"
#include "fe/FETypeAttribute.h"
#include "fe/FEPrimaryExpression.h"
#include "fe/FEConstDeclarator.h"
#include "fe/FEAttribute.h"
#include "Compiler.h"

CFETypedDeclarator::CFETypedDeclarator(TYPEDDECL_TYPE nType,
    CFETypeSpec * pType,
    vector<CFEDeclarator*> * pDeclarators,
    vector<CFEAttribute*> * pTypeAttributes)
{
    m_nType = nType;
    m_pType = pType;
    if (m_pType)
        m_pType->SetParent(this);
    if (pDeclarators)
        m_vDeclarators.swap(*pDeclarators);
    vector<CFEDeclarator*>::iterator iterD = m_vDeclarators.begin();
    for (; iterD != m_vDeclarators.end(); iterD++)
    {
        (*iterD)->SetParent(this);
    }
    if (pTypeAttributes)
        m_vTypeAttributes.swap(*pTypeAttributes);
    vector<CFEAttribute*>::iterator iterA = m_vTypeAttributes.begin();
    for (; iterA != m_vTypeAttributes.end(); iterA++)
    {
        (*iterA)->SetParent(this);
    }
}

CFETypedDeclarator::CFETypedDeclarator(CFETypedDeclarator & src)
: CFEInterfaceComponent(src)
{
    m_nType = src.m_nType;
    if (src.m_pType)
    {
        m_pType = (CFETypeSpec *) (src.m_pType->Clone());
        m_pType->SetParent(this);
    }
    else
        m_pType = 0;

    COPY_VECTOR(CFEDeclarator, m_vDeclarators, iterD);
    COPY_VECTOR(CFEAttribute, m_vTypeAttributes, iterA);
}

/** cleans up the typed declarator object and all its members (type, parameters and attributes) */
CFETypedDeclarator::~CFETypedDeclarator()
{
    if (m_pType)
        delete m_pType;

    DEL_VECTOR(m_vDeclarators);
    DEL_VECTOR(m_vTypeAttributes);
}

/**
 *    \brief retrives a pointer to the first declarator
 *    \return an iterator, which points to the first declarator
 */
vector<CFEDeclarator*>::iterator CFETypedDeclarator::GetFirstDeclarator()
{
    return m_vDeclarators.begin();
}

/**
 *    \brief retrieves the next declarator
 *    \param iter a pointer to the next declarator
 *    \return the object, the iterator pointed at
 */
CFEDeclarator *CFETypedDeclarator::GetNextDeclarator(vector<CFEDeclarator*>::iterator &iter)
{
    if (iter == m_vDeclarators.end())
        return 0;
    return *iter++;
}

/**
 *    \brief retrieves a pointer to the first attribute
 *    \return an iterator, which points to the first attribute
 */
vector<CFEAttribute*>::iterator CFETypedDeclarator::GetFirstAttribute()
{
    return m_vTypeAttributes.begin();
}

/**
 *    \brief retrieves the next attribute
 *    \param iter an iterator, which points to the next attribute
 *    \return the next attribute object
 */
CFEAttribute *CFETypedDeclarator::GetNextAttribute(vector<CFEAttribute*>::iterator &iter)
{
    if (iter == m_vTypeAttributes.end())
        return 0;
    return *iter++;
}

/**
 *    \brief tries to find an attribute
 *    \param eAttrType the attribute to find
 *    \return a reference to the specified attribute, 0 if not found
 */
CFEAttribute *CFETypedDeclarator::FindAttribute(ATTR_TYPE eAttrType)
{
    CFEAttribute *pAttr;
    vector<CFEAttribute*>::iterator iter = GetFirstAttribute();
    while ((pAttr = GetNextAttribute(iter)) != 0)
    {
        if (pAttr->GetAttrType() == eAttrType)
            return pAttr;
    }
    return 0;
}

/**
 *    \brief tries to find an declarator
 *    \param sName the name of the declarator
 *    \return a referece to the declarator, 0 if not found
 */
CFEDeclarator *CFETypedDeclarator::FindDeclarator(string sName)
{
    CFEDeclarator *pDecl;
    vector<CFEDeclarator*>::iterator iter = GetFirstDeclarator();
    while ((pDecl = GetNextDeclarator(iter)) != 0)
    {
        if (sName == pDecl->GetName())
            return pDecl;
    }
    return 0;
}

/**
 *    \brief returns the type of the typed declarator (typedef, parameter, exception, ...)
 *    \return the type of the typed declarator
 */
TYPEDDECL_TYPE CFETypedDeclarator::GetTypedDeclType()
{
    return m_nType;
}

/**
 *    \brief replaces the contained type
 *    \param pNewType the new type for this declarator
 *    \return the old type
 */
CFETypeSpec *CFETypedDeclarator::ReplaceType(CFETypeSpec * pNewType)
{
    assert(pNewType);
    CFETypeSpec *pRet = m_pType;
    m_pType = pNewType;
    m_pType->SetParent(this);
    return pRet;
}

/**
 *    \brief creates a copy of this object
 *    \return an exact copy of this object
 */
CObject *CFETypedDeclarator::Clone()
{
    return new CFETypedDeclarator(*this);
}

/**
 *    \brief returns the contained type
 *    \return the contained type
 */
CFETypeSpec *CFETypedDeclarator::GetType()
{
    return m_pType;
}

/**
 *    \brief test if this declarator is a typedef
 *    \return true if the typed declarator's type is TYPEDEF
 */
bool CFETypedDeclarator::IsTypedef()
{
    return m_nType == TYPEDECL_TYPEDEF;
}

/**
 *  \brief removes a name from this declarator
 *  \param pDeclarator a reference to the declarator, which should be removed
 *  \return if the remove operation was succesful (the param was in the collection)
 *
 * Because of the behavior of the "vector" collection this function invalidates
 * all iterators, which have been aquired for declarator. So refresh your local
 * iterators after using this function.
 */
bool CFETypedDeclarator::RemoveDeclarator(CFEDeclarator * pDeclarator)
{
    if (!pDeclarator)
        return false;
    vector<CFEDeclarator*>::iterator iter = m_vDeclarators.begin();
    for (; iter != m_vDeclarators.end(); iter++)
    {
        if (*iter == pDeclarator)
        {
            m_vDeclarators.erase(iter);
            return true;
        }
    }
    return false;
}

/**
 *    \brief adds an new declarator to this typed declarator
 *    \param pDeclarator the new declarator
 */
void CFETypedDeclarator::AddDeclarator(CFEDeclarator * pDeclarator)
{
    if (!pDeclarator)
        return;
    m_vDeclarators.push_back(pDeclarator);
    pDeclarator->SetParent(this);
}

/**
 *    \brief remove an attribute from this typed declarator
 *    \param eAttrType the attribute's type which should be removed
 */
void CFETypedDeclarator::RemoveAttribute(ATTR_TYPE eAttrType)
{
    vector<CFEAttribute*>::iterator iter;
    bool bRemoved;
    do {
        bRemoved = false;
        for (iter = m_vTypeAttributes.begin();
             iter != m_vTypeAttributes.end(); iter++)
        {
            if ((*iter)->GetAttrType() == eAttrType)
            {
                CFEAttribute *pAttr = *iter;
                m_vTypeAttributes.erase(iter);
                delete pAttr;
                bRemoved = true;
                break; // break out of for loop
            }
        }
    } while (bRemoved);
}

/**
 *    \brief add an attribute to this type declarator
 *    \param pNewAttr the new attribute to add
 */
void CFETypedDeclarator::AddAttribute(CFEAttribute * pNewAttr)
{
    if (!pNewAttr)
        return;
    m_vTypeAttributes.push_back(pNewAttr);
    pNewAttr->SetParent(this);
}

/** \brief checks consistency of typed declarator
 *  \return true if element is consistent, false if not
 *
 * A typed declarator is consitent if it's type is consistent and the declarators
 * are globally unique. The declarators have to be unique only if this is a typedef.
 * Before we check the declarators, we make them valid within the namespace.
 *
 * - if we have a refstring type, replace it with a char* and [ref]
 *
 * \todo line 673: size_is(int,int) should work as well (use expression-list)
 */
bool CFETypedDeclarator::CheckConsistency()
{
    if (!GetType())
    {
        CCompiler::GccError(this, 0, "Typed Declarator has no type.");
        return false;
    }
    // check type
    if (!(GetType()->CheckConsistency()))
        return false;

    // check declarators
    if (IsTypedef())
    {
        CFEFile *pRoot = dynamic_cast<CFEFile*>(GetRoot());
        assert(pRoot);
        CFEFile *pMyFile = GetSpecificParent<CFEFile>(0);
        vector<CFEDeclarator*>::iterator iterD = GetFirstDeclarator();
        CFEDeclarator *pDecl;
        CFETypedDeclarator *pSecondType;
        while ((pDecl = GetNextDeclarator(iterD)) != 0)
        {
            // now check if name is unique
            if (!(pRoot->FindUserDefinedType(pDecl->GetName())))
            {
                CCompiler::GccError(this, 0, "The type %s is not defined.", pDecl->GetName().c_str());
                return false;
            }
            if ((pSecondType = pRoot->FindUserDefinedType(pDecl->GetName())) != this)
            {
//                 TRACE("\n\nCompare for \"%s\" (1st: %p; 2nd: %p)\n", ()pDecl->GetName(), this, pSecondType);
                // check if both are in C header files and if they are include in different trees
                bool bEqualPath = true;
                CFEFile *pSecondFile = pSecondType->GetSpecificParent<CFEFile>(0);
/*                TRACE("Check for equal path with:\n"
                    "\"%s\" (my) IDL? %s\n"
                    "\"%s\" (2nd) IDL? %s\n",
                    ()pMyFile->GetFileName(), (pMyFile->IsIDLFile())?"yes":"no",
                    ()pSecondFile->GetFileName(), (pSecondFile->IsIDLFile())?"yes":"no");*/
                if (!(pMyFile->IsIDLFile()) && !(pSecondFile->IsIDLFile()) &&
                    (pMyFile->GetFileName() == pSecondFile->GetFileName()))
                {
/*                TRACE("Check for equal path with:\n"
                    "\"%s\" (my) IDL? %s\n"
                    "\"%s\" (2nd) IDL? %s\n",
                    ()pMyFile->GetFileName(), (pMyFile->IsIDLFile())?"yes":"no",
                    ()pSecondFile->GetFileName(), (pSecondFile->IsIDLFile())?"yes":"no");*/
                    // they have to have different parents somewhere
                    // they start off being the same file
                    while (bEqualPath && pMyFile)
                    {
                        if (!pMyFile->GetParent())
                            break;
                        // then tey get the next parent file
                        pMyFile = pMyFile->GetSpecificParent<CFEFile>(1);
                        pSecondFile = pSecondFile->GetSpecificParent<CFEFile>(1);
                        if (!pSecondFile)
                        {
                            bEqualPath = false;
                            break;
                        }
                        // test if the file names are the same
                        if (pMyFile->GetFileName() != pSecondFile->GetFileName())
                        {
                            bEqualPath = false;
                            break;
                        }
                    }
                }
                // if the path is equal (which is also true if none of the first if conditions
                // are met) then we print an error message
                if (bEqualPath)
                {
                    CCompiler::GccError(this, 0, "The type %s is defined multiple times. "
                        "Previously defined here: %s at line %d.", pDecl->GetName().c_str(),
                        (pSecondFile)?(pSecondFile->GetFileName().c_str()):"", pSecondType->GetSourceLine());
                    return false;
                }
            }
        }
    }
    // replace refstrings
    if (GetType()->GetType() == TYPE_REFSTRING)
    {
        // replace type REFSTRING with type CHAR_ASTERISK
        CFETypeSpec *pOldType = ReplaceType(new CFESimpleType(TYPE_CHAR_ASTERISK));
        delete pOldType;
        // add attribute ref
        if (!FindAttribute(ATTR_REF))
            AddAttribute(new CFEAttribute(ATTR_REF));
        if (!FindAttribute(ATTR_STRING))
            AddAttribute(new CFEAttribute(ATTR_STRING));
    }
    // replace string and wstring
    if ((GetType()->GetType() == TYPE_STRING) ||
        (GetType()->GetType() == TYPE_WSTRING))
    {
        // replace type
        if (GetType()->GetType() == TYPE_STRING)
        {
            CFETypeSpec *pOldType = ReplaceType(new CFESimpleType(TYPE_CHAR));
            delete pOldType;
        }
        else
        {
            CFETypeSpec *pOldType = ReplaceType(new CFESimpleType(TYPE_WCHAR));
            delete pOldType;
        }
        // set string attribute
        if (!FindAttribute(ATTR_STRING))
            AddAttribute(new CFEAttribute(ATTR_STRING));
        // now we search all declarators. We make from any more than 1 star
        // only one star (a string can have only one star (per definition)
        // set star with declarator (can only be one or array dimension)
        vector<CFEDeclarator*>::iterator iterD = GetFirstDeclarator();
        CFEDeclarator *pDecl;
        while ((pDecl = GetNextDeclarator(iterD)) != 0)
        {
            if (dynamic_cast<CFEArrayDeclarator*>(pDecl))
            {
                CFEArrayDeclarator *pArrayDecl = (CFEArrayDeclarator *) pDecl;
                // we also check uninitialized array dimension (create error in C). We
                // replace these with stars.
                for (unsigned int i = 0; i < pArrayDecl->GetDimensionCount(); i++)
                {
                    CFEExpression *pBound = pArrayDecl->GetUpperBound(i);
                    if (!pBound)
                    {
                        // remove bound and increase stars
                        pArrayDecl->RemoveBounds(i);
                        pArrayDecl->SetStars(pArrayDecl->GetStars() + 1);
                    }
                }
                // if we removed all dimension make this a "normal declarator"
                if (!(pArrayDecl->GetDimensionCount()))
                {
                    CFEDeclarator *pNewDecl = new CFEDeclarator(DECL_IDENTIFIER, pArrayDecl->GetName(), pArrayDecl->GetStars());
                    RemoveDeclarator(pArrayDecl);
                    delete pArrayDecl;
                    AddDeclarator(pNewDecl);
                    // because current pDecl is delete we start all over again
                    iterD = GetFirstDeclarator();
                    continue;
                }
            }
            // we check if there are too many stars
            int nMaxStars = 1;
            if (FindAttribute(ATTR_OUT))
                nMaxStars = 2;
            if (pDecl->GetStars() > nMaxStars)
            {
                if (GetSpecificParent<CFEOperation>())
                    CCompiler::GccWarning(this, 0, "\"%s\" in function \"%s\" has more than one pointer (fixed)",
                                          pDecl->GetName().c_str(), GetSpecificParent<CFEOperation>()->GetName().c_str());
                else
                    CCompiler::GccWarning(this, 0, "\"%s\" has more than one pointer (fixed)", pDecl->GetName().c_str());
            }
            // and fix it. This is also a has to be there star, so if there hasn't been
            // there it is now
            pDecl->SetStars(nMaxStars);
        }
    }
    // if we have an unsigned CHAR_ASTERISK, then we replace the
    // CHAR_ASTERISK with CHAR and add the star to the first declarator
    if ((GetType()->GetType() == TYPE_CHAR_ASTERISK) &&
        ((CFESimpleType*)GetType())->IsUnsigned())
    {
        // replace the type
        CFETypeSpec *pOldType = ReplaceType(new CFESimpleType(TYPE_CHAR, true));
        delete pOldType;
        // set the star of the first declarator
        vector<CFEDeclarator*>::iterator iterD = GetFirstDeclarator();
        CFEDeclarator *pDecl = GetNextDeclarator(iterD);
        if (pDecl)
            pDecl->SetStars(pDecl->GetStars()+1);
    }
    // make from CHAR with one star an CHAR_ASTERISK
    // only if string attribute is given
    if ((GetType()->GetType() == TYPE_CHAR) &&
        FindAttribute(ATTR_STRING))
    {
        // first check if _all_ declarators have at least one star
        bool bHaveStar = true;
        vector<CFEDeclarator*>::iterator iterD = GetFirstDeclarator();
        CFEDeclarator *pDecl;
        while ((pDecl = GetNextDeclarator(iterD)) != 0)
        {
            // if at least one does _not_ have a star
            if (pDecl->GetStars() == 0)
                bHaveStar = false;
        }
        // only if all decls have a star, we replace this type
        if (bHaveStar)
        {
            // replace type
            CFETypeSpec *pOldType = ReplaceType(new CFESimpleType(TYPE_CHAR_ASTERISK));
            delete pOldType;
            // do not need to add the string attribute since this has
            // to be set to get here

            // get all declarator and reduce their declarators by one
            // if it is an [out] string, set it to two
            iterD = GetFirstDeclarator();
            while ((pDecl = GetNextDeclarator(iterD)) != 0)
            {
                // first add the pointer from char*
                pDecl->SetStars(pDecl->GetStars()-1);
            }
        }
    }
    if ((GetType()->GetType() == TYPE_CHAR_ASTERISK) &&
        !FindAttribute(ATTR_STRING))
    {
        // char* var implies [string] attribute
        // only if no size or length attribute
        // XXX: what about '[out] char*' which only wants to transmit one character?
        if (!FindAttribute(ATTR_SIZE_IS) &&
            !FindAttribute(ATTR_LENGTH_IS))
        {
            AddAttribute(new CFEAttribute(ATTR_STRING));
        }
        else
        {
            // if it is a char* and there is no string attribute,
            // but a size or length attribute, we convert it into
            // a char and add a star to the declarator
            CFETypeSpec *pOldType = ReplaceType(new CFESimpleType(TYPE_CHAR));
            delete pOldType;
            // set star of declarators
            vector<CFEDeclarator*>::iterator iterD = GetFirstDeclarator();
            CFEDeclarator *pDecl;
            while ((pDecl = GetNextDeclarator(iterD)) != 0)
            {
                // if declarator is simple, we make it an array now
                if ((pDecl->GetType() != DECL_ARRAY) &&
                    (pDecl->GetType() != DECL_ENUM))
                {
                    CFEDeclarator *pArray = new CFEArrayDeclarator(pDecl);
                    RemoveDeclarator(pDecl);
                    delete pDecl;
                    AddDeclarator(pArray);
                    pDecl = pArray;
                }
                // first add the pointer from char*
                pDecl->SetStars(pDecl->GetStars()+1);
            }
        }
    }
    // check for void* type and size parameter
    // then this is a array. To be able to transmit it
    // we add the transmit-as attribute with a character
    // type
    if ((GetType()->GetType() == TYPE_VOID_ASTERISK) &&
        (FindAttribute(ATTR_SIZE_IS) ||
         FindAttribute(ATTR_LENGTH_IS)))
    {
        if (!FindAttribute(ATTR_TRANSMIT_AS))
        {
            CFETypeSpec *pType = new CFESimpleType(TYPE_CHAR);
            CFEAttribute *pAttr = new CFETypeAttribute(ATTR_TRANSMIT_AS, pType);
            pType->SetParent(pAttr);
            AddAttribute(pAttr);
        }
        // to handle the parameter correctly we have to move the
        // '*' from 'void*' to the declarators
        CFETypeSpec *pOldType = ReplaceType(new CFESimpleType(TYPE_VOID));
        delete pOldType;
        // set declarator stars
        vector<CFEDeclarator*>::iterator iterD = GetFirstDeclarator();
        CFEDeclarator *pDecl;
        while ((pDecl = GetNextDeclarator(iterD)) != 0)
        {
            // if declarator is simple, we make it an array now
            if ((pDecl->GetType() != DECL_ARRAY) &&
                (pDecl->GetType() != DECL_ENUM))
            {
                CFEDeclarator *pArray = new CFEArrayDeclarator(pDecl);
                RemoveDeclarator(pDecl);
                delete pDecl;
                AddDeclarator(pArray);
                pDecl = pArray;
            }
            // first add the pointer from char*
            pDecl->SetStars(pDecl->GetStars()+1);
        }
    }
    // check for in-parameter and struct or union type
    if (FindAttribute(ATTR_IN))
    {
        // make structs reference parameters
        // except they are pointers already.
        if (CFETypeSpec::IsConstructedType(GetType()) &&
            !CFETypeSpec::IsPointerType(GetType()))
        {
            vector<CFEDeclarator*>::iterator iterD = GetFirstDeclarator();
            CFEDeclarator *pDecl;
            while ((pDecl = GetNextDeclarator(iterD)) != 0)
            {
                if (!(pDecl->IsReference()))
                    pDecl->SetStars(pDecl->GetStars() + 1);
            }
        }
    }
    // if we have a max_is or size_is attribute which is CFEIntAttribute, we
    // set the respective unbound array dimension to this value
    // check for size_is/length_is/max_is attributes
    // if size_is contains variable, check for max_is with const value
    CFEAttribute *pSizeAttrib = FindAttribute(ATTR_SIZE_IS);
    if (!pSizeAttrib)
    {
        pSizeAttrib = FindAttribute(ATTR_LENGTH_IS);
        if (!pSizeAttrib)
            pSizeAttrib = FindAttribute(ATTR_MAX_IS);
    }
    else
    {
        if (!dynamic_cast<CFEIntAttribute*>(pSizeAttrib))
            pSizeAttrib = FindAttribute(ATTR_MAX_IS);
    }
    bool bRemoveSize = false;
    if (pSizeAttrib)
    {
        vector<CFEDeclarator*>::iterator iterD = GetFirstDeclarator();
        CFEDeclarator *pDecl;
        while ((pDecl = GetNextDeclarator(iterD)) != 0)
        {
            if (dynamic_cast<CFEArrayDeclarator*>(pDecl))
            {
                CFEArrayDeclarator *pArray = (CFEArrayDeclarator *) pDecl;
                // find the first unbound array dimension
                int nMax = pArray->GetDimensionCount();
                for (int i = nMax - 1; i >= 0; i--)
                {
                    CFEExpression *pUpper = pArray->GetUpperBound(i);
                    if (!pUpper)
                    {
                        // create new expression with value of size_is/max_is
                        // if the value is declarator -> ignore it
                        if (dynamic_cast<CFEIntAttribute*>(pSizeAttrib))
                        {
                            // use ReplaceUpperBound to set new expression.
                            CFEExpression *pNewBound = new CFEPrimaryExpression(EXPR_INT, (long int) ((CFEIntAttribute *) pSizeAttrib)->GetIntValue());
                            pArray->ReplaceUpperBound(i, pNewBound);
                            // remove attribute
                            bRemoveSize = true;
                        }
                        // test for constant in size attribute
                        if (dynamic_cast<CFEIsAttribute*>(pSizeAttrib))
                        {
                            // test for parameter
                            vector<CFEDeclarator*>::iterator iterAttr = ((CFEIsAttribute*)pSizeAttrib)->GetFirstAttrParameter();
                            CFEDeclarator *pDAttr = ((CFEIsAttribute*)pSizeAttrib)->GetNextAttrParameter(iterAttr);
                            if (pDAttr)
                            {
                                // find constant
                                CFEFile *pFERoot = dynamic_cast<CFEFile*>(GetRoot());
                                assert(pFERoot);
                                CFEConstDeclarator *pConstant = pFERoot->FindConstDeclarator(pDAttr->GetName());
                                if (pConstant && pConstant->GetValue())
                                {
                                    // replace bounds
                                    CFEExpression *pNewBound = new CFEPrimaryExpression(EXPR_INT, (long int) pConstant->GetValue()->GetIntValue());
                                    pArray->ReplaceUpperBound(i, pNewBound);
                                    // do not delete the size attribute
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (bRemoveSize && (pSizeAttrib))
    {
        RemoveAttribute(pSizeAttrib->GetAttrType());
    }

    // check if unbound arrays have at least a size_is or length_is or max_is attribute
    vector<CFEDeclarator*>::iterator iterD = GetFirstDeclarator();
    CFEDeclarator *pDecl;
    while ((pDecl = GetNextDeclarator(iterD)) != 0)
    {
        if (dynamic_cast<CFEArrayDeclarator*>(pDecl))
        {
            // if array -> check bounds
            CFEArrayDeclarator *pArray = (CFEArrayDeclarator *) pDecl;
            for (unsigned int i = 0; i < pArray->GetDimensionCount(); i++)
            {
                CFEExpression *pLower = pArray->GetLowerBound(i);
                CFEExpression *pUpper = pArray->GetUpperBound(i);
                // if both not set -> unbound
                if ((!pLower) && (!pUpper))
                {
                    // check for size_is or length_is or max_is
                    // if OUT it only needs size_is or length_is -> user has to provide buffer which is large enough
                    // if IN we need max_is, so server loop can provide buffer
                    if ((FindAttribute(ATTR_IN)) && (!FindAttribute(ATTR_MAX_IS)))
                    {
                        CCompiler::GccError(this, 0, "Unbound array declarator \"%s\" with direction IN needs max_is attribute",
                                            pDecl->GetName().c_str());
                        return false;
                    }
                    if ((!FindAttribute(ATTR_SIZE_IS)) &&
                        (!FindAttribute(ATTR_MAX_IS)) &&
                        (!FindAttribute(ATTR_LENGTH_IS)))
                    {
                        CCompiler::GccError(this, 0, "Unbound array declarator \"%s\" needs size_is, length_is or max_is attribute",
                                            pDecl->GetName().c_str());
                        return false;
                    }
                }
            }
        }
    }
    // <DEBUG>
//    vector<CFEDeclarator*>::iterator testI = GetFirstDeclarator();
//    CFEDeclarator *pTestD = GetNextDeclarator(testI);
//    TRACE("Checked decl %s with type %d and %d stars\n", ()pTestD->GetName(), GetType()->GetType(), pTestD->GetStars());
    // </DEBUG>
    // everything is fine
    return true;
}

/**    serialize this object
 *    \param pFile the file to serialize to/from
 */
void CFETypedDeclarator::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        if (IsTypedef())
        {
            pFile->PrintIndent("<typedef>\n");
            pFile->IncIndent();
        }
        // write attributes
        vector<CFEAttribute*>::iterator iterA = GetFirstAttribute();
        CFEBase *pElement;
        while ((pElement = GetNextAttribute(iterA)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // write type
        GetType()->Serialize(pFile);
        // write declarators
        vector<CFEDeclarator*>::iterator iterD = GetFirstDeclarator();
        while ((pElement = GetNextDeclarator(iterD)) != 0)
        {
            pElement->Serialize(pFile);
        }
        if (IsTypedef())
        {
            pFile->DecIndent();
            pFile->PrintIndent("</typedef>\n");
        }
    }
}
