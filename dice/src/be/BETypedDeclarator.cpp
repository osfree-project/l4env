/**
 *    \file    dice/src/be/BETypedDeclarator.cpp
 *    \brief   contains the implementation of the class CBETypedDeclarator
 *
 *    \date    01/18/2002
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

#include "be/BETypedDeclarator.h"
#include "be/BEContext.h"
#include "be/BEAttribute.h"
#include "be/BEType.h"
#include "be/BETypedef.h"
#include "be/BEStructType.h"
#include "be/BEUnionType.h"
#include "be/BEUnionCase.h"
#include "be/BEUserDefinedType.h"
#include "be/BEDeclarator.h"
#include "be/BERoot.h"
#include "be/BEClient.h"
#include "be/BEFunction.h"
#include "be/BEComponentFunction.h"
#include "be/BEExpression.h"
#include "be/BETestFunction.h"
#include "be/BEMsgBufferType.h"
#include "be/BEConstant.h"

#include "fe/FETypedDeclarator.h"
#include "fe/FETypeSpec.h"
#include "TypeSpec-Type.h"
#include "fe/FEIsAttribute.h"
#include "fe/FEExpression.h"
#include "Compiler.h"

CBETypedDeclarator::CBETypedDeclarator()
{
    m_pType = 0;
}

CBETypedDeclarator::CBETypedDeclarator(CBETypedDeclarator & src)
: CBEObject(src)
{
    vector<CBEAttribute*>::iterator iterA;
    for (iterA = src.m_vAttributes.begin(); iterA != src.m_vAttributes.end(); iterA++)
    {
        CBEAttribute *pNew = (CBEAttribute*)((*iterA)->Clone());
        m_vAttributes.push_back(pNew);
        pNew->SetParent(this);
    }
    vector<CBEDeclarator*>::iterator iterD;
    for (iterD = src.m_vDeclarators.begin(); iterD != src.m_vDeclarators.end(); iterD++)
    {
        CBEDeclarator *pNew = (CBEDeclarator*)((*iterD)->Clone());
        m_vDeclarators.push_back(pNew);
        pNew->SetParent(this);
    }
    m_pType = (CBEType*)src.m_pType->Clone();
    m_pType->SetParent(this);
}

/** \brief destructor of this instance */
CBETypedDeclarator::~CBETypedDeclarator()
{
    while (!m_vAttributes.empty())
    {
        delete m_vAttributes.back();
        m_vAttributes.pop_back();
    }
    while (!m_vDeclarators.empty())
    {
        delete m_vDeclarators.back();
        m_vDeclarators.pop_back();
    }
    if (m_pType)
        delete m_pType;
}

/** \brief creates the back-end structure for a parameter
 *  \param pFEParameter the corresponding front-end parameter
 *  \param pContext the context of the code generation
 *  \return true if code generation was successful
 *
 * This implementation extracts the type, name and attributes from the front-end class.
 * Since a back-end parameter is expected to have only _ONE_ name, but the front-end typed
 * declarator may have several names, we expect that this function is only called for
 * the first of the declarators. The calling function has to take care of creating a seperate
 * parameter for each declarator.
 */
bool CBETypedDeclarator::CreateBackEnd(CFETypedDeclarator * pFEParameter, CBEContext * pContext)
{
    assert(pFEParameter);

    // call CBEObject's CreateBackEnd method
    if (!CBEObject::CreateBackEnd(pFEParameter))
        return false;

    // if we already have declarators, attributes or type, remove them (clean myself)
    if (m_pType)
    {
        delete m_pType;
        m_pType = 0;
    }
    while (!m_vAttributes.empty())
    {
        delete m_vAttributes.back();
        m_vAttributes.pop_back();
    }
    while (!m_vDeclarators.empty())
    {
        delete m_vDeclarators.back();
        m_vDeclarators.pop_back();
    }
    // get names
    vector<CFEDeclarator*>::iterator iterD = pFEParameter->GetFirstDeclarator();
    CFEDeclarator *pFEDecl;
    while ((pFEDecl = pFEParameter->GetNextDeclarator(iterD)) != 0)
    {
        CBEDeclarator *pDecl = pContext->GetClassFactory()->GetNewDeclarator();
        AddDeclarator(pDecl);
        if (!pDecl->CreateBackEnd(pFEDecl, pContext))
        {
            RemoveDeclarator(pDecl);
            delete pDecl;
            VERBOSE("CBETypedDeclarator::CreateBE failed because declarator could not be created\n");
            return false;
        }
    }
    // get type
    m_pType = pContext->GetClassFactory()->GetNewType(pFEParameter->GetType()->GetType());
    m_pType->SetParent(this);
    if (!m_pType->CreateBackEnd(pFEParameter->GetType(), pContext))
    {
        delete m_pType;
        m_pType = 0;
        VERBOSE("CBETypedDeclarator::CreateBE failed because type could not be created\n");
        return false;
    }
    // get attributes
    vector<CFEAttribute*>::iterator iterA = pFEParameter->GetFirstAttribute();
    CFEAttribute *pFEAttribute;
    while ((pFEAttribute = pFEParameter->GetNextAttribute(iterA)) != 0)
    {
        CBEAttribute *pAttribute = pContext->GetClassFactory()->GetNewAttribute();
        AddAttribute(pAttribute);
        if (!pAttribute->CreateBackEnd(pFEAttribute, pContext))
        {
            RemoveAttribute(pAttribute);
            delete pAttribute;
            VERBOSE("CBETypedDeclarator::CreateBE failed because attribute could not be created\n");
            return false;
        }
    }
    return true;
}

/** \brief creates the typed declarator using user defined type and name
 *  \param sUserDefinedType the user defined type
 *  \param sName the name of the typed declarator
 *  \param nStars the number of stars for the declarator
 *  \param pContext the context of the code generation
 *  \return true if successful
 */
bool CBETypedDeclarator::CreateBackEnd(string sUserDefinedType, string sName, int nStars, CBEContext * pContext)
{
    VERBOSE("CBETypedDeclarator::CreateBE(user defined)\n");

    // create decl
    CBEDeclarator *pDecl = pContext->GetClassFactory()->GetNewDeclarator();
    AddDeclarator(pDecl);
    if (!pDecl->CreateBackEnd(sName, nStars, pContext))
    {
        RemoveDeclarator(pDecl);
        delete pDecl;
        VERBOSE("CBETypedDeclarator::CreateBE failed because declarator could not be added\n");
        return false;
    }
    // create type
    m_pType = pContext->GetClassFactory()->GetNewUserDefinedType();
    m_pType->SetParent(this);    // has to be set before calling CreateBE
    if (!(((CBEUserDefinedType *) m_pType)->CreateBackEnd(sUserDefinedType, pContext)))
    {
        delete m_pType;
        m_pType = 0;
        VERBOSE("CBETypedDeclarator::CreateBE failed because type could not be added\n");
        return false;
    }

    return true;
}

/** \brief creates the typed declarator using a given back-end type and a name
 *  \param pType the type of the typed declarator
 *  \param sName the name of the declarator
 *  \param pContext the context of the code generation
 *  \return true if successful
 */
bool CBETypedDeclarator::CreateBackEnd(CBEType * pType, string sName, CBEContext * pContext)
{
    VERBOSE("%s called\n", __PRETTY_FUNCTION__);

    // create decl
    CBEDeclarator *pDecl = pContext->GetClassFactory()->GetNewDeclarator();
    AddDeclarator(pDecl);
    if (!pDecl->CreateBackEnd(sName, 0, pContext))
    {
        RemoveDeclarator(pDecl);
        delete pDecl;
        VERBOSE("CBETypedDeclarator::CreateBE failed because declarator could not be added\n");
        return false;
    }
    // create type
    m_pType = (CBEType *) pType->Clone();
    m_pType->SetParent(this);
    // do not need to call create, because original has been created before.

    VERBOSE("%s returns true\n", __PRETTY_FUNCTION__);
    return true;
}

/** \brief adds another attribute to the vector
 *  \param pAttribute the attribute to add
 */
void CBETypedDeclarator::AddAttribute(CBEAttribute * pAttribute)
{
    if (!pAttribute)
        return;
    m_vAttributes.push_back(pAttribute);
    pAttribute->SetParent(this);
}

/** \brief removes an attribute from the attributes vector
 *  \param pAttribute the attribute to remove
 */
void CBETypedDeclarator::RemoveAttribute(CBEAttribute * pAttribute)
{
    if (!pAttribute)
        return;
    vector<CBEAttribute*>::iterator iter;
    for (iter = m_vAttributes.begin(); iter != m_vAttributes.end(); iter++)
    {
        if (*iter == pAttribute)
        {
            m_vAttributes.erase(iter);
            return;
        }
    }
}

/** \brief retrieves a pointer to the first attribute
 *  \return a pointer to the first attribute element
 */
vector<CBEAttribute*>::iterator CBETypedDeclarator::GetFirstAttribute()
{
    return m_vAttributes.begin();
}

/** \brief retrieves the next attribute
 *  \param iter the pointer to the next attribute
 *  \return a reference to the next attribute
 */
CBEAttribute *CBETypedDeclarator::GetNextAttribute(vector<CBEAttribute*>::iterator &iter)
{
    if (iter == m_vAttributes.end())
        return 0;
    return *iter++;
}

/** \brief writes the content of a typed declarator to the target file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * A typed declarator, such as a parameter, contain a type, name(s) and optional attributes.
 *
 */
void CBETypedDeclarator::WriteDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    if (!pFile->IsOpen())
        return;

    WriteAttributes(pFile, pContext);
    WriteType(pFile, pContext);
    if (m_vDeclarators.size() > 0)
        pFile->Print(" ");
    WriteDeclarators(pFile, pContext);
}

/** \brief writes the attributes of the typed declarator
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * The current implementation does nothing.
 */
void CBETypedDeclarator::WriteAttributes(CBEFile * pFile, CBEContext * pContext)
{

}

/** \brief writes the type to the target file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \param bUseConst true if the 'const' keyword should be used
 *
 * Calls the Write operation of the type.
 *
 * - An IN string is always const (its automatically an char*) on client's side.
 * (server side may need to fiddle with pointer)
 * - An exception at server side is the component's function.
 * - An OUT string is never const, because it is set by the  server.
 * Exception is a send function from server to client, which
 * can send const string as OUT. -> we do not handle this yet.
 * - An IN string is not const if its a global test variable
 * - IN arrays are also const
 *
 * If we do not use C or L4 types, we have to use the CORBA-types, which
 * define a '' as 'const_CORBA_char_ptr'
 */
void 
CBETypedDeclarator::WriteType(CBEFile * pFile, 
    CBEContext * pContext, 
    bool bUseConst)
{
    bool bConstructed = (m_pType && m_pType->IsConstructedType());
    // test if string
    bool bIsArray = IsString();
    bool bCheckedArrayDims = false;
    // test all declarators for strings
    if (!bIsArray)
    {
        vector<CBEDeclarator*>::iterator iterD = GetFirstDeclarator();
        CBEDeclarator *pDecl;
        while ((pDecl = GetNextDeclarator(iterD)) != 0)
        {
            if (pDecl->IsArray())
            {
                vector<CBEExpression*>::iterator iterB = 
		    pDecl->GetFirstArrayBound();
                CBEExpression *pExpr;
                while ((pExpr = pDecl->GetNextArrayBound(iterB)) != 0)
                {
                    if (pExpr->IsOfType(EXPR_INT) || // either fixed number in bound
                        (pExpr->GetIntValue() == 0)) // or no fixed array boundary
                        bIsArray = true;
                }
                bIsArray |= pDecl->GetArrayDimensionCount() == 0;
                bCheckedArrayDims = true;
            }
        }
    }
    // test for size/length/max attributes, which indicate arrays
    if (!bIsArray && !bCheckedArrayDims)
    {
        bIsArray |= FindAttribute(ATTR_SIZE_IS) != 0;
        bIsArray |= FindAttribute(ATTR_LENGTH_IS) != 0;
    }
    // we do not test the max_is attribute because:
    // if the parameter had brackets ([]) then max_is has been converted
    // to a value there, or
    // if max_is is found now its a pointer, which can use some
    // const, but this has been enabled by the above checks (array-dims == 0)
    // already
    bool bUseNotCorba = pContext->IsOptionSet(PROGRAM_USE_ALLTYPES);
    // there is one exception to this rule: if this is already a
    // CORBA type. CORBA types start with a "CORBA_" string. Therefore:
    // test if type is user defined and check beginning of alias.
    if (bUseNotCorba  && m_pType->IsOfType(TYPE_USER_DEFINED) &&
        dynamic_cast<CBEUserDefinedType*>(m_pType))
    {
        CBEUserDefinedType *pType = dynamic_cast<CBEUserDefinedType*>(m_pType);
        string sName = pType->GetName();
        if ((sName.length() > 6) &&
            (sName.substr(0, 6) == "CORBA_"))
            bUseNotCorba = false;
    }
    // string check
    if (bUseConst && (bConstructed || bIsArray))
    {
        if (pFile->IsOfFileType(FILETYPE_CLIENT) ||
            pFile->IsOfFileType(FILETYPE_TESTSUITE))
        {
            if ((FindAttribute(ATTR_IN)) && (!FindAttribute(ATTR_OUT)))
            {
                if (!bUseNotCorba && m_pType->IsPointerType())
                    pFile->Print("const_");
                else
                    pFile->Print("const ");
            }
        }
        if (pFile->IsOfFileType(FILETYPE_COMPONENT) ||
            pFile->IsOfFileType(FILETYPE_TEMPLATE))
        {
            if ((FindAttribute(ATTR_IN)) &&
                (!FindAttribute(ATTR_OUT)) &&
                (GetSpecificParent<CBEComponentFunction>()))
            {
                if (!bUseNotCorba && m_pType->IsPointerType())
                    pFile->Print("const_");
                else
                    pFile->Print("const ");
            }
        }
    }
    if (m_pType)
        m_pType->Write(pFile, pContext);
}

/** \brief writes the declarators to the target file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBETypedDeclarator::WriteDeclarators(CBEFile * pFile, CBEContext * pContext)
{
    if (!pFile->IsOpen())
        return;

    vector<CBEDeclarator*>::iterator iterD = GetFirstDeclarator();
    bool bComma = false;
    CBEDeclarator *pDecl;
    while ((pDecl = GetNextDeclarator(iterD)) != 0)
    {
        if (bComma)
            pFile->Print(", ");
        pDecl->WriteDeclaration(pFile, pContext);
        bComma = true;
    }
}

/** \brief adds another declarator to the vector
 *  \param pDeclarator the new declarator to add
 */
void CBETypedDeclarator::AddDeclarator(CBEDeclarator * pDeclarator)
{
    if (!pDeclarator)
        return;
    m_vDeclarators.push_back(pDeclarator);
    pDeclarator->SetParent(this);
}

/** \brief removes a declarator from the decls vector
 *  \param pDeclarator the declarator to remove
 */
void CBETypedDeclarator::RemoveDeclarator(CBEDeclarator * pDeclarator)
{
    if (!pDeclarator)
        return;
    vector<CBEDeclarator*>::iterator iter;
    for (iter = m_vDeclarators.begin(); iter != m_vDeclarators.end(); iter++)
    {
        if (*iter == pDeclarator)
        {
            m_vDeclarators.erase(iter);
            return;
        }
    }
}

/** \brief retrieves a pointer to the first declarator
 *  \return a pointer to the first declarator
 */
vector<CBEDeclarator*>::iterator CBETypedDeclarator::GetFirstDeclarator()
{
    return m_vDeclarators.begin();
}

/** \brief retrieves a pointer to the last declarator
 *  \return a pointer to the first declarator
 */
bool CBETypedDeclarator::IsLastDeclarator(vector<CBEDeclarator*>::iterator iter)
{
    if (m_vDeclarators.empty())
        return true;
    return (iter == m_vDeclarators.end() - 1);
}

/** \brief retrieves a reference to the next declarator
 *  \param iter the pointer to the next declarator
 *  \return a reference to the next declarator
 */
CBEDeclarator *CBETypedDeclarator::GetNextDeclarator(vector<CBEDeclarator*>::iterator &iter)
{
    if (iter == m_vDeclarators.end())
        return 0;
    return *iter++;
}

/** \brief retrieves a reference to the declarator
 *  \return a reference to the declarator
 */
CBEDeclarator *CBETypedDeclarator::GetDeclarator()
{
    vector<CBEDeclarator*>::iterator iter = m_vDeclarators.begin();
    if (iter == m_vDeclarators.end())
        return 0;
    return *iter;
}

/** \brief retrieves a reference to the call declarator
 *  \return a reference to the call declarator
 */
CBEDeclarator *CBETypedDeclarator::GetCallDeclarator()
{
    vector<CBEDeclarator*>::iterator iter = m_vDeclarators.begin();
    if (iter++ == m_vDeclarators.end())
        return 0;
    if (iter == m_vDeclarators.end())
        return 0;
    return *iter;
}
/** \brief searches for an attribute of the specified type
 *  \param nAttrType the type to search for
 *  \return a reference to the attribute or 0 if not found
 */
CBEAttribute *CBETypedDeclarator::FindAttribute(int nAttrType)
{
    vector<CBEAttribute*>::iterator iter = GetFirstAttribute();
    CBEAttribute *pAttr;
    while ((pAttr = GetNextAttribute(iter)) != 0)
    {
        if (pAttr->GetType() == nAttrType)
            return pAttr;
    }
    return 0;
}

/** \brief test if this is a string variable
 *  \return true if it is
 *
 * A string is everything which has a base type 'char', is of a size bigger than 1 character
 * and has the string attribute. This function only checks the first declarator.
 */
bool CBETypedDeclarator::IsString()
{
    if (!FindAttribute(ATTR_STRING))
        return false;
    if (m_pType->IsOfType(TYPE_CHAR_ASTERISK) &&
        !m_pType->IsUnsigned())
        return true;
    if (m_pType->IsOfType(TYPE_CHAR) &&
        !m_pType->IsUnsigned())
    {
        vector<CBEDeclarator*>::iterator iter = GetFirstDeclarator();
        CBEDeclarator *pDeclarator = GetNextDeclarator(iter);
        if (pDeclarator)
        {
            int nSize = pDeclarator->GetSize();
            // can be either <0 -> pointer
            // or >1 -> array
            // == 1 -> normal char
            // == 0 -> bitfield
            if ((nSize < 0) || (nSize > 1))
                return true;
        }
    }
    return false;
}

/** \brief tests if this is a parameter with a variable size
 *  \return true if it is
 *
 * A "variable sized parameter" is a parameter which has to be marshalled into
 * a variable sized message buffer. Or with other words: If this function
 * returns true, this indicates that the parameter has to be marshaled 'with
 * care'.
 *
 * A variable sized parameter is a parameter which has a size_is, length_is or
 * max_is attribute or is a variable sized array. The *_is attributes can be of
 * type CFEIntAttributes, which indicates a concrete value. This invalidates
 * the information about the attribute. A size_is attribute is 'stronger' than
 * a max_is attribute (meaning: if we find a size_is attribute, we ignore the
 * max_is attribute). A parameter is a variable sized array if it has the
 * size/max attributes or explicit array bounds or it is a string, which is
 * indicated by the string attribute.
 *
 * All other parameters, even the ones with asterisks, are not variable sized.
 * They have to be dereferenced to be marshalled with their scalar values.
 *
 * According to the CORBA language mapping, a variable sized parameter is:
 * # the type 'any'
 * # a bounded or unbounded string
 * # a bounded or unbounded sequence
 * # an object reference
 * # a struct or union with variable length member type
 * # an array with variable length element type
 * # a typedef to a veriable length type
 */
bool CBETypedDeclarator::IsVariableSized()
{
    // need the root
    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);
    CBEConstant *pConstant = 0;
    CBEAttribute *pAttr;
    if ((pAttr = FindAttribute(ATTR_SIZE_IS)) != 0)
    {
        if (pAttr->IsOfType(ATTR_CLASS_IS))
        {
            // if declarator is a constant, then this is const as well
            vector<CBEDeclarator*>::iterator iter = pAttr->GetFirstIsAttribute();
            CBEDeclarator *pSizeName = pAttr->GetNextIsAttribute(iter);
            assert(pSizeName);
            // this might by a constant declarator
            pConstant = pRoot->FindConstant(pSizeName->GetName());
            // not a constant, return true
            if (!pConstant)
                return true;
        }
    }
    if ((pAttr = FindAttribute(ATTR_LENGTH_IS)) != 0)
    {
        if (pAttr->IsOfType(ATTR_CLASS_IS))
        {
            // if declarator is a constant, then this is const as well
            vector<CBEDeclarator*>::iterator iter = pAttr->GetFirstIsAttribute();
            CBEDeclarator *pSizeName = pAttr->GetNextIsAttribute(iter);
            assert(pSizeName);
            // this might by a constant declarator
            pConstant = pRoot->FindConstant(pSizeName->GetName());
            // not a constant, return true
            if (!pConstant)
                return true;
        }
    }
    if ((pAttr = FindAttribute(ATTR_MAX_IS)) != 0)
    {
        if (pAttr->IsOfType(ATTR_CLASS_IS))
        {
            // if declarator is a constant, then this is const as well
            vector<CBEDeclarator*>::iterator iter = pAttr->GetFirstIsAttribute();
            CBEDeclarator *pSizeName = pAttr->GetNextIsAttribute(iter);
            assert(pSizeName);
            // this might by a constant declarator
            pConstant = pRoot->FindConstant(pSizeName->GetName());
            // not a constant, return true
            if (!pConstant)
                return true;
        }

    }
    if (IsString())
        return true;
    // if type is variable sized, then this variable
    // is too (e.g. a struct with a var-sized member
    if (GetType() && (GetType()->GetSize() < 0))
        return true;
    // if we have an fixed sized array with a size_is attribute,
    // the parameter is actually fixed, right?
    // test declarators for arrays
    vector<CBEDeclarator*>::iterator iterD = GetFirstDeclarator();
    CBEDeclarator *pDeclarator;
    while ((pDeclarator = GetNextDeclarator(iterD)) != 0)
    {
        if (!pDeclarator->IsArray())
            continue;
        vector<CBEExpression*>::iterator iterB = pDeclarator->GetFirstArrayBound();
        CBEExpression *pExpr;
        // check for size/length/max parameters parallel
        vector<CBEDeclarator*>::iterator iterAttr;
        if (pAttr)
            iterAttr = pAttr->GetFirstIsAttribute();
        CBEDeclarator *pAttrName;
        while ((pExpr = pDeclarator->GetNextArrayBound(iterB)) != 0)
        {
            if (pExpr->IsOfType(EXPR_NONE)) // no bound
            {
                // check attribute parameter
                pConstant = 0;
                pAttrName = (pAttr) ? pAttr->GetNextIsAttribute(iterAttr) : 0;
                if (pAttrName)
                    pConstant = pRoot->FindConstant(pAttrName->GetName());
                if (!pConstant)
                    return true;
            }
        }
    }
    // no size/max attribute and no unbound array:
    // should be treated as scalar
// <DEBUG>
/*    pIter = GetFirstDeclarator();
    pDeclarator = GetNextDeclarator(pIter);
    DTRACE("CBETypedDeclarator::IsVariableSized(%s): size_is=%s, string=%s, ref=%s, type=%d, stars=%d, size=%d, OUT=%s, constr.=%s, func=%s\n",
           ()pDeclarator->GetName(),
           (FindAttribute(ATTR_SIZE_IS))?"true":"false",
           (FindAttribute(ATTR_STRING))?"true":"false",
           (FindAttribute(ATTR_REF))?"true":"false",
           GetType()->GetFEType(),
           pDeclarator->GetStars(),
           pDeclarator->GetSize(),
           (FindAttribute(ATTR_OUT))?"true":"false",
           (GetType() && GetType()->IsConstructedType())?"true":"false",
           GetSpecificParent<CBEFunction>()->GetName());*/
// </DEBUG>
    return false;
}

/** \brief checks if this parameter is of fixed size
 *  \return true if it is of fixed size.
 *
 * A parameter, which is not variable size, does not necessarily has to be of fixed
 * size. Since we support a message buffer with multiple data types, it is possible,
 * that a parameter of a specific type has to be marshalled not as fixed and not as
 * varaible sized, but as something else. So this parameter will return on both functions
 * false.
 *
 * This implementation simply assigns a variable sized parameter to be not a fixed sized
 * parameter and vice versa.
 */
bool CBETypedDeclarator::IsFixedSized()
{
    return !IsVariableSized();
}

/** \brief calculates the size of a typed declarator
 *  \return the number of bytes needed for the declarators
 *
 * The size of a typed declarator depends on the size of the type and the size of the declarator.
 * This implementation only tests the first declarator. If the size of the declarator is:
 * - negative, then it has unbound array dimensions or pointers
 * - equals zero, it has a bitfield value
 * - equals one, it is a "normal" simple declarator
 * - larger than one, it is a array of bound size
 * .
 * If the declarator has one pointer and an OUT attribute it is probably referenced to obtain the
 * base type's value, so we return the base type's size as size.
 *
 * The only other exception to watch for is, if this function returns zero. Then the parameter has
 * bitfields. Because this functions returns the size in bytes, we cannot express the bitfields as
 * bytes (usually smaller than 8). Test for the return value of zero and get the bitfield-size explicetly.
 */
int CBETypedDeclarator::GetSize()
{
    int nSize = 0;

    vector<CBEDeclarator*>::iterator iterD = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = GetNextDeclarator(iterD)) != 0)
    {
        int nDeclSize = GetSizeOfDeclarator(pDecl);
        if (nDeclSize < 0)
            return -1;
        nSize += nDeclSize;
    }

    return nSize;
}

/** \brief calculates the size of a typed declarator
 *  \param sName the name of the parameter to use for size calculation
 *  \return the number of bytes needed for the declarators
 *
 * The size is calculated just as above, but only for one declarator.
 */
int CBETypedDeclarator::GetSize(string sName)
{
    CBEDeclarator *pDecl = FindDeclarator(sName);
    if (!pDecl)
        return 0; // not existent

    return GetSizeOfDeclarator(pDecl);
}

/** \brief get the actually to transmit type
 *  \return the type to transmit
 *
 * This function checks for transmit as attribute
 */
CBEType* CBETypedDeclarator::GetTransmitType()
{
    CBEType *pType = m_pType;
    CBEAttribute *pAttr = FindAttribute(ATTR_TRANSMIT_AS);
    if (pAttr && pAttr->GetType())
        pType = pAttr->GetAttrType();

    // get type
    return pType;
}

/** \brief calculate the size of a specific declarator
 *  \param pDeclarator the declarator to get the size of
 *  \return the size of the declarator
 */
int CBETypedDeclarator::GetSizeOfDeclarator(CBEDeclarator *pDeclarator)
{
    assert(pDeclarator);

    CBEType *pType = GetTransmitType();
    if (pType->IsVoid())
        return 0;

    int nTypeSize = pType->GetSize();
    int nDeclSize = pDeclarator->GetSize();
    // if referenced OUT, this is the size
    if ((nDeclSize == -1) &&
        (pDeclarator->GetStars() == 1) &&
        (FindAttribute(ATTR_OUT)))
    {
        return nTypeSize;
    }
    // if reference struct, this is the size
    if ((nDeclSize == -1) &&
        (pDeclarator->GetStars() == 1) &&
        (pType->IsConstructedType()))
    {
        return nTypeSize;
    }
    // if variables sized, return -1
    if (nDeclSize < 0)
        return -1;
    // if bitfield: it multiplies with zero and returns zero
    // const array or simple? -> return array-dimension * type's size
    return nDeclSize * nTypeSize;
}

/** \brief calculates the max size of the paramater
 *  \param bGuessSize true if we should guess the size of the parameter
 *  \param pContext teh context of the size calculation
 *  \return the size in bytes
 */
int CBETypedDeclarator::GetMaxSize(bool bGuessSize, CBEContext *pContext)
{
    CBEType *pType = GetTransmitType();
    // no size for void
    if (pType->IsVoid())
        return 0;
    // get type's size
    int nTypeSize = pType->GetSize();
    int nSize = 0;
    bool bVarSized = false;
    vector<CBEDeclarator*>::iterator iterD = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = GetNextDeclarator(iterD)) != 0)
    {
        int nDeclSize = pDecl->GetMaxSize(pContext);
        // if decl is array, but returns a negative maximum size,
        // it is an unbound array. Its max size is the max size of it's
        // type times the dimensions
        if ((nDeclSize < 0) && pDecl->IsArray() && bGuessSize)
        {
            int nTypeSize = pContext->GetSizes()->GetMaxSizeOfType(pType->GetFEType());
            nSize += nTypeSize * -nDeclSize;
            if (pContext->IsWarningSet(PROGRAM_WARNING_NO_MAXSIZE))
            {
                vector<CBEDeclarator*>::iterator iterD = GetFirstDeclarator();
                CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
                if (pFunction)
                    CCompiler::Warning("%s in %s has no maximum size (guessing size %d)",
                        (*iterD)->GetName().c_str(),
                        pFunction->GetName().c_str(),
                        nTypeSize * -nDeclSize);
                else
                    CCompiler::Warning("%s has no maximum size (guessing size %d)",
                        (*iterD)->GetName().c_str(),
                        nTypeSize * -nDeclSize);
            }
            continue;
        }
        // if size_is or length_is, then this is an array
        if ((nDeclSize < 0) &&
            (FindAttribute(ATTR_SIZE_IS) ||
             FindAttribute(ATTR_LENGTH_IS) ||
             FindAttribute(ATTR_MAX_IS)))
        {
            int nDimensions = 1;
            CBEAttribute *pAttr;
            if ((pAttr = FindAttribute(ATTR_SIZE_IS)) != 0)
            {
                if (pAttr->IsOfType(ATTR_CLASS_INT))
                {
                    nSize += pAttr->GetIntValue() * nTypeSize;
                    continue;
                }
                if (pAttr->IsOfType(ATTR_CLASS_IS))
                    nDimensions = pAttr->GetRemainingNumberOfIsAttributes(pAttr->GetFirstIsAttribute());
            }
            if ((pAttr = FindAttribute(ATTR_LENGTH_IS)) != 0)
            {
                if (pAttr->IsOfType(ATTR_CLASS_INT))
                {
                    nSize += pAttr->GetIntValue() * nTypeSize;
                    continue;
                }
                if (pAttr->IsOfType(ATTR_CLASS_IS))
                {
                    int nTmp = pAttr->GetRemainingNumberOfIsAttributes(pAttr->GetFirstIsAttribute());
                    nDimensions = nDimensions > nTmp ? nDimensions : nTmp;
                }
            }
            if ((pAttr = FindAttribute(ATTR_MAX_IS)) != 0)
            {
                if (pAttr->IsOfType(ATTR_CLASS_INT))
                {
                    nSize += pAttr->GetIntValue() * nTypeSize;
                    continue;
                }
                if (pAttr->IsOfType(ATTR_CLASS_IS))
                {
                    int nTmp = pAttr->GetRemainingNumberOfIsAttributes(pAttr->GetFirstIsAttribute());
                    nDimensions = nDimensions > nTmp ? nDimensions : nTmp;
                }
            }
            if (bGuessSize)
            {
                if (pContext->IsWarningSet(PROGRAM_WARNING_NO_MAXSIZE))
                {
                    vector<CBEDeclarator*>::iterator iterD = GetFirstDeclarator();
                    CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
                    if (pFunction)
                        CCompiler::Warning("%s in %s has no maximum size (guessing size %d)",
                            (*iterD)->GetName().c_str(),
                            pFunction->GetName().c_str(),
                            nTypeSize * -nDeclSize);
                    else
                        CCompiler::Warning("%s has no maximum size (guessing size %d)",
                            (*iterD)->GetName().c_str(),
                            nTypeSize * -nDeclSize);
                }
                nSize += pContext->GetSizes()->GetMaxSizeOfType(pType->GetFEType()) * nDimensions;
                continue;
            }
        }
        // if referenced, this is the size
        if (nDeclSize == -(pDecl->GetStars()))
            nDeclSize = -nDeclSize;
        // if variables sized, return -1
        if (nDeclSize < 0)
            bVarSized = true;
        // if bitfield: it multiplies with zero and returns zero
        // const array or simple? -> return array-dimension * type's size
        nSize += nDeclSize * nTypeSize;
    }
    if (bVarSized || pType->IsPointerType())
    {
        // check max attributes
        CBEAttribute *pAttr;
        if ((pAttr = FindAttribute(ATTR_MAX_IS)) != 0)
        {
            if (pAttr->IsOfType(ATTR_CLASS_INT))
                nSize = pAttr->GetIntValue();
            else if (pAttr->IsOfType(ATTR_CLASS_IS))
            {
                // if declarator is a constant, then this is const as well
                vector<CBEDeclarator*>::iterator iter = pAttr->GetFirstIsAttribute();
                CBEDeclarator *pSizeName = pAttr->GetNextIsAttribute(iter);
                assert(pSizeName);
                // this might by a constant declarator
                CBERoot *pRoot = GetSpecificParent<CBERoot>();
                assert(pRoot);
                CBEConstant *pConstant = pRoot->FindConstant(pSizeName->GetName());
                // set size to value of constant
                if (pConstant && pConstant->GetValue())
                    nSize = pConstant->GetValue()->GetIntValue() * nTypeSize;
                else
                    nSize = -1;
            }
            else
                nSize = -1;
        }
        else
            nSize = -1;
    }
    return nSize;
}

/** \brief allows access to type of typed declarator
 *  \return reference to m_pType
 */
CBEType *CBETypedDeclarator::GetType()
{
    return m_pType;
}

/** \brief writes the declaration of the global test variables
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBETypedDeclarator::WriteGlobalTestVariable(CBEFile * pFile, CBEContext * pContext)
{
    if (GetType()->IsVoid())
        return;
    WriteType(pFile, pContext, false);
    pFile->Print(" ");
    WriteGlobalDeclarators(pFile, pContext);
    pFile->Print(";\n");
}

/** \brief writes the list of global declarators
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBETypedDeclarator::WriteGlobalDeclarators(CBEFile * pFile, CBEContext * pContext)
{
    vector<CBEDeclarator*>::iterator iterD = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    bool bComma = false;
    while ((pDecl = GetNextDeclarator(iterD)) != 0)
    {
        if (bComma)
            pFile->Print(", ");
        pDecl->WriteGlobalName(pFile, 0, pContext, true);
        int nArrayDims = 0;
        vector<CBEExpression*>::iterator iterB = pDecl->GetFirstArrayBound();
        CBEExpression *pBound;
        while ((pBound = pDecl->GetNextArrayBound(iterB)) != 0)
        {
            if (pBound->GetIntValue() > 0)
                nArrayDims++;
        }
        if (nArrayDims == 0)
        {
            // no array written by WriteGlobalName :
            // we don't have a dimension, so get the maximum for this type
            int nMax = pContext->GetSizes()->GetMaxSizeOfType(GetType()->GetFEType());
            // if this parameter has stars, and a size or length attribute
            // it is an array nonetheless, so we write its maximum size
            if ((FindAttribute(ATTR_SIZE_IS) ||
                FindAttribute(ATTR_LENGTH_IS)) &&
                pDecl->GetStars())
            {
                pFile->Print("[%d]", nMax);
            }
        }
        bComma = true;
    }
}

/** \brief write variable with an extra declarator for each indirection
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This is just like the normal Write method, but it adds for every pointer of a declarator a
 * declarator without this pointer (for *t1 it adds _t1);
 *
 * For arrays, we use a special treatment: unbound array dimensions ('[]') are written as stars,
 * bound array dimension are written correctly.
 */
void CBETypedDeclarator::WriteIndirect(CBEFile * pFile, CBEContext * pContext)
{
    if (!pFile->IsOpen())
        return;

    m_pType->WriteIndirect(pFile, pContext);
    pFile->Print(" ");

    // test for pointer types
    bool bIsPointerType = m_pType->IsPointerType();
    // get the number of indirections if its a pointer type
    int nIndirections = m_pType->GetIndirectionCount();
    // if this is pointer type but has indirections during
    // write than this is because the pointer type is a typedef
    // this has been removed and the base type (without pointer has
    // been written: therefore this is not a pointer type anymore
    if (bIsPointerType && (nIndirections > 0))
        bIsPointerType = false;
    // if it is simple and we do not generate C-Types, than ignore the
    // pointer type, since it is typedefed, which makes the purpose of
    // this bool-parameter obsolete
    if (!pContext->IsOptionSet(PROGRAM_USE_ALLTYPES) &&
        m_pType->IsSimpleType())
        bIsPointerType = false;
    // test if we need a pointer of this variable
    bool bUsePointer = IsString() && !m_pType->IsPointerType();
    // size_is or length_is attributes indicate an array, where we will need
    // a pointer to use.
    bUsePointer = bUsePointer || FindAttribute(ATTR_SIZE_IS) || FindAttribute(ATTR_LENGTH_IS);
    // loop over declarators
    vector<CBEDeclarator*>::iterator iterD = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    bool bComma = false;
    while ((pDecl = GetNextDeclarator(iterD)) != 0)
    {
        // temporarely boost number of stars
        pDecl->IncStars(nIndirections);
        if (bComma)
            pFile->Print(", ");
        pDecl->WriteIndirect(pFile, bUsePointer, bIsPointerType, pContext);
        bComma = true;
        // remove star boost
        pDecl->IncStars(-nIndirections);
    }
}

/** \brief initializes indirect variables as declared by WriteIndirect
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This functin does assign a pointered variable a reference to a "unpointered" variable.
 */
void CBETypedDeclarator::WriteIndirectInitialization(CBEFile * pFile, CBEContext * pContext)
{
    // get the number of indirections if its a pointer type
    int nIndirections = m_pType->GetIndirectionCount();

    bool bUsePointer = IsString() && !m_pType->IsPointerType();
    // with size_is or length_is we use malloc to init the pointer,
    // we have no indirection variables
    bUsePointer = bUsePointer || FindAttribute(ATTR_SIZE_IS) || FindAttribute(ATTR_LENGTH_IS);
    // iterate over declarators
    vector<CBEDeclarator*>::iterator iterD = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = GetNextDeclarator(iterD)) != 0)
    {
        // temporarely boost the number of stars
        pDecl->IncStars(nIndirections);
        pDecl->WriteIndirectInitialization(pFile, bUsePointer, pContext);
        // revert star boost
        pDecl->IncStars(-nIndirections);
    }
}

/** \brief initializes indirect variables as declared by WriteIndirect
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This functin does assign a pointered variable a dynamic memory region
 */
void CBETypedDeclarator::WriteIndirectInitializationMemory(CBEFile * pFile, CBEContext * pContext)
{
    // get the number of indirections if its a pointer type
    int nIndirections = m_pType->GetIndirectionCount();

    bool bUsePointer = IsString() && !m_pType->IsPointerType();
    // with size_is or length_is we use malloc to init the pointer,
    // we have no indirection variables
    bUsePointer = bUsePointer || FindAttribute(ATTR_SIZE_IS) || FindAttribute(ATTR_LENGTH_IS);
    // iterate over declarators
    vector<CBEDeclarator*>::iterator iterD = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = GetNextDeclarator(iterD)) != 0)
    {
        // temporarely boost the number of stars
        pDecl->IncStars(nIndirections);
        pDecl->WriteIndirectInitializationMemory(pFile, bUsePointer, pContext);
        // revert star boost
        pDecl->IncStars(-nIndirections);
    }
}

/** \brief writes the clean-up code of this parameter
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBETypedDeclarator::WriteCleanup(CBEFile* pFile, CBEContext* pContext)
{
    // get the number of indirections if its a pointer type
    int nIndirections = m_pType->GetIndirectionCount();

    bool bUsePointer = IsString() && !m_pType->IsPointerType();
    // with size_is or length_is we use malloc to init the pointer,
    // we have no indirection variables
    bUsePointer = bUsePointer || FindAttribute(ATTR_SIZE_IS) || FindAttribute(ATTR_LENGTH_IS);
    // iterate over declarators
    vector<CBEDeclarator*>::iterator iterD = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = GetNextDeclarator(iterD)) != 0)
    {
        pDecl->IncStars(nIndirections);
        pDecl->WriteCleanup(pFile, bUsePointer, pContext);
        pDecl->IncStars(-nIndirections);
    }
}

/** \brief writes the clean-up code of this parameter
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * Same code as in \ref WriteCleanup but uses the WriteDeferredCleanup method of the declarator.
 */
void CBETypedDeclarator::WriteDeferredCleanup(CBEFile* pFile, CBEContext* pContext)
{
    // get the number of indirections if its a pointer type
    int nIndirections = m_pType->GetIndirectionCount();

    bool bUsePointer = IsString() && !m_pType->IsPointerType();
    // with size_is or length_is we use malloc to init the pointer,
    // we have no indirection variables
    bUsePointer = bUsePointer || FindAttribute(ATTR_SIZE_IS) || FindAttribute(ATTR_LENGTH_IS);
    // iterate over declarators
    vector<CBEDeclarator*>::iterator iterD = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = GetNextDeclarator(iterD)) != 0)
    {
        pDecl->IncStars(nIndirections);
        pDecl->WriteDeferredCleanup(pFile, bUsePointer, pContext);
        pDecl->IncStars(-nIndirections);
    }
}

/** \brief checks if a declarator is member of this typed declarator
 *  \param sName the name of the searched declarator
 *  \return a reference to the found declarator or 0 if not found
 */
CBEDeclarator *CBETypedDeclarator::FindDeclarator(string sName)
{
    DTRACE("CBETypedDeclarator::FindDeclarator (%s) called\n",
        sName.c_str());
    if (sName.empty())
        return 0;
    vector<CBEDeclarator*>::iterator iterD = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = GetNextDeclarator(iterD)) != 0)
    {
        DTRACE("CBETypedDeclarator::FindDeclarator checking decl %s\n",
            pDecl->GetName().c_str());
        if (pDecl->GetName() == sName)
            return pDecl;
    }
    return 0;
}

/** \brief replaces the current type with the new one
 *  \param pNewType the new type
 *  \return the old type
 */
CBEType *CBETypedDeclarator::ReplaceType(CBEType * pNewType)
{
    if (!pNewType)
        return 0;
    CBEType *pOldType = m_pType;
    m_pType = pNewType;
    pNewType->SetParent(this);
    return pOldType;
}

/** \brief write the variable declaration and initializes the variables to their zero value
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * We do not init user defined types, because we don't know how.
 */
void CBETypedDeclarator::WriteZeroInitDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    if (GetType()->IsVoid())
        return;
    vector<CBEDeclarator*>::iterator iterD = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = GetNextDeclarator(iterD)) != 0)
    {
        pFile->PrintIndent("");
        WriteType(pFile, pContext);
        pFile->Print(" ");
        pDecl->WriteDeclaration(pFile, pContext);
        if (GetType()->DoWriteZeroInit())
        {
            pFile->Print(" = ");
            GetType()->WriteZeroInit(pFile, pContext);
        }
        pFile->Print(";\n");
    }
}

/** \brief writes the zero assignment
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBETypedDeclarator::WriteSetZero(CBEFile* pFile, CBEContext* pContext)
{
    if (GetType()->IsVoid())
        return;
    vector<CBEDeclarator*>::iterator iterD = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = GetNextDeclarator(iterD)) != 0)
    {
        pFile->PrintIndent("");
        pDecl->WriteDeclaration(pFile, pContext);
        if (GetType()->DoWriteZeroInit())
        {
            pFile->Print(" = ");
            GetType()->WriteZeroInit(pFile, pContext);
        }
        pFile->Print(";\n");
    }
}

/** \brief write the variable declaration and initializes the variables to the given string
 *  \param pFile the file to write to
 *  \param sInitString string to use as initializer (can be empty)
 *  \param pContext the context of the write operation
 */
void CBETypedDeclarator::WriteInitDeclaration(CBEFile* pFile, string sInitString, CBEContext* pContext)
{
    if (GetType()->IsVoid())
        return;
    vector<CBEDeclarator*>::iterator iterD = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = GetNextDeclarator(iterD)) != 0)
    {
        pFile->PrintIndent("");
        WriteType(pFile, pContext);
        pFile->Print(" ");
        pDecl->WriteDeclaration(pFile, pContext);
        if (!sInitString.empty())
            pFile->Print(" = %s", sInitString.c_str());
        pFile->Print(";\n");
    }
}

/** \brief writes the code to obtain the size of a variabel sized parameter
 *  \param pFile the file to write to
 *  \param pStack the declarator stack used for variable sized members of structs, union
 *  \param pContext the context of the write operation
 *
 * If this is variable sized, there has to be some attribute, defining the
 * actual size of this parameter at run-time. So what we do is to search for
 * size_is, length_is or max_is attributes.
 *
 * This is different if this is a string. So if it is, we simply use the
 * strlen function.
 */
void CBETypedDeclarator::WriteGetSize(CBEFile * pFile,
    vector<CDeclaratorStackLocation*> *pStack,
    CBEContext * pContext)
{
    CBEAttribute *pAttr = 0;
    if ((pAttr = FindAttribute(ATTR_SIZE_IS)) == 0)
    {
        if ((pAttr = FindAttribute(ATTR_LENGTH_IS)) == 0)
        {
            // we prefer the actual size of a string to its max-size,
            // so we first test for the string attribute
            // might be string
            if ((pAttr = FindAttribute(ATTR_STRING)) != 0)
            {
                // get declarator
                vector<CBEDeclarator*>::iterator iterD = GetFirstDeclarator();
                CBEDeclarator *pDecl = *iterD;
                if (pDecl)
                {
                    pFile->Print("strlen(");
                    // check if parameter has additional reference
                    bool bAddRef = false;
                    CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
                    if (pFunction)
                    {
                        if (pFunction->HasAdditionalReference(pDecl, pContext))
                            bAddRef = true;
                    }

                    // strlen operates on pointers, so decrement stars by one
                    int nIncStars = pDecl->GetStars();
                    if (!bAddRef)
                        nIncStars--;
                    // check if type is pointer type
                    if (m_pType->IsPointerType())
                        nIncStars++;
                    for (int i = 0; i<nIncStars; i++)
                        pFile->Print("*");
                    if (pStack)
                        CDeclaratorStackLocation::Write(pFile, pStack, false, false, pContext);
                    else
                        pDecl->WriteDeclaration(pFile, pContext);
                    // restore old number of stars
                    pFile->Print(")");
                }
                // wrote size parameter
                return;
            }
            // check max-is attribute
            if ((pAttr = FindAttribute(ATTR_MAX_IS)) == 0)
            {
                // this only happends, when this is variable sized
                // because of its variable sized type
                // => we have to animate the type to write the size
                if (!pStack)
                    pStack = new vector<CDeclaratorStackLocation*>();
                vector<CBEDeclarator*>::iterator iterD = GetFirstDeclarator();
                CBEDeclarator *pDecl;
                while ((pDecl = GetNextDeclarator(iterD)) != 0)
                {
                    CDeclaratorStackLocation *pLoc = new CDeclaratorStackLocation(pDecl);
                    pStack->push_back(pLoc);
                    m_pType->WriteGetSize(pFile, pStack, pContext);
                    pStack->pop_back();
                    delete pLoc;
                }
                return;
            }
        }
    }
    if (!pAttr)
        return;
    if (pAttr->IsOfType(ATTR_CLASS_IS))
    {
        vector<CBEDeclarator*>::iterator iter = pAttr->GetFirstIsAttribute();
        CBEDeclarator *pSizeName = pAttr->GetNextIsAttribute(iter);
        assert(pSizeName);
        CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
        CBEStructType *pStruct = GetSpecificParent<CBEStructType>();
        CBETypedDeclarator *pSizeParameter = 0;
        CBEConstant *pConstant = 0;
        // get original parameter
        if (pFunction)
            pSizeParameter = pFunction->FindParameter(pSizeName->GetName());
        // search for parameter in struct as well
        if (!pSizeParameter && pStruct)
            pSizeParameter = pStruct->FindMember(pSizeName->GetName());
        if (pFunction)
        {
            if (!pSizeParameter)
            {
                // this might by a constant declarator
                CBERoot *pRoot = GetSpecificParent<CBERoot>();
                assert(pRoot);
                pConstant = pRoot->FindConstant(pSizeName->GetName());
                // now at least this one should have been found
                if (!pConstant)
                    CCompiler::Error("Size attribute %s is neither parameter in %s nor defined as constant.",
                        pSizeName->GetName().c_str(), pFunction->GetName().c_str());
                assert(pConstant);
                // the declarator stays the same, because the constant does not
                // have a declarator and is without any references
            }
            else
            {
                // and now get original declarator, since size_is declarator
                // might have different reference count...
                pSizeName = pSizeParameter->FindDeclarator(pSizeName->GetName());
            }
            if (pFunction->HasAdditionalReference(pSizeName, pContext))
                pFile->Print("*");
        }
        else if (pStruct)
        {
            if (pSizeParameter && pStack)
            {
                CDeclaratorStackLocation *pTop = pStack->back();
                pStack->pop_back();
                CDeclaratorStackLocation::Write(pFile, pStack, false, false, pContext);
                pStack->push_back(pTop);
                pFile->Print(".");
            }
        }
        if (pSizeParameter)
            pSizeParameter->WriteDeclarators(pFile, pContext); // has only one declarator
        else if (pConstant)
            pFile->Print("%s", pConstant->GetName().c_str());
    }
    else if (pAttr->IsOfType(ATTR_CLASS_INT))
    {
        pFile->Print("%d", pAttr->GetIntValue());
    }
}

/** \brief returns the size of the bitfield declarator in bits
 *  \return the bitfields of the declarator
 */
int CBETypedDeclarator::GetBitfieldSize()
{
    vector<CBEDeclarator*>::iterator iter = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    int nSize = 0;
    while ((pDecl = GetNextDeclarator(iter)) != 0)
    {
        nSize += pDecl->GetBitfields();
    }
    return nSize;
}

/** \brief checks if the parameter is transmitted into the given direction
 *  \param nDirection the direction to check
 *  \return true if it is transmitted that way
 */
bool CBETypedDeclarator::IsDirection(int nDirection)
{
    return ((((nDirection & DIRECTION_IN) != 0) &&
              (FindAttribute(ATTR_IN) != 0)) ||
            (((nDirection & DIRECTION_OUT) != 0) &&
              (FindAttribute(ATTR_OUT) != 0)));
}

/** \brief test if this parameter has a size attribute
 *  \param nAttr the attribute to test for
 *  \return true if one of the specified attributes is found
 *
 * We can only specify one attribute. Therefore we can simply check for
 * it and return.
 */
bool CBETypedDeclarator::HasSizeAttr(int nAttr)
{
    if ((nAttr != ATTR_LENGTH_IS) &&
        (nAttr != ATTR_SIZE_IS) &&
        (nAttr != ATTR_MAX_IS))
        return false;
    if (FindAttribute(nAttr))
        return true;
    return false;
}

/** \brief creates a new instance of this class */
CObject * CBETypedDeclarator::Clone()
{
    return new CBETypedDeclarator(*this);
}

/** \brief checks if this parameter is referenced
 *  \return true if ons declarator is a pointer
 */
bool CBETypedDeclarator::HasReference()
{
    vector<CBEDeclarator*>::iterator iter = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = GetNextDeclarator(iter)) != 0)
    {
        if (pDecl->GetStars() > 0)
            return true;
    }
    return false;
}

/** \brief checks if the given name belongs to an IS attribute if it has one
 *  \param sDeclName the name of the IS attr declarator
 *  \return true if found, false if not
 *
 * We have to search all attributes of IS type (SIZE_IS, LENGTH_IS, MIN_IS, MAX_IS,
 * etc.) Then we have to check its declarator with the given name. If we found a
 * match, we return a reference to the attribute.
 */
CBEAttribute* CBETypedDeclarator::FindIsAttribute(string sDeclName)
{
    vector<CBEAttribute*>::iterator iter = GetFirstAttribute();
    CBEAttribute *pAttr;
    while ((pAttr = GetNextAttribute(iter)) != 0)
    {
        if (pAttr->FindIsParameter(sDeclName))
            return pAttr;
    }
    return 0;
}
