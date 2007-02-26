/**
 *	\file	dice/src/be/BETypedDeclarator.cpp
 *	\brief	contains the implementation of the class CBETypedDeclarator
 *
 *	\date	01/18/2002
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
#include "be/BEExpression.h"
#include "be/BETestFunction.h"
#include "be/BEMsgBufferType.h"

#include "fe/FETypedDeclarator.h"
#include "fe/FETypeSpec.h"
#include "fe/FEIsAttribute.h"
#include "fe/FEExpression.h"

IMPLEMENT_DYNAMIC(CBETypedDeclarator);

CBETypedDeclarator::CBETypedDeclarator()
: m_vAttributes(RUNTIME_CLASS(CBEAttribute)),
  m_vDeclarators(RUNTIME_CLASS(CBEDeclarator))
{
    m_pType = 0;
    IMPLEMENT_DYNAMIC_BASE(CBETypedDeclarator, CBEObject);
}

CBETypedDeclarator::CBETypedDeclarator(CBETypedDeclarator & src)
: CBEObject(src),
  m_vAttributes(RUNTIME_CLASS(CBEAttribute)),
  m_vDeclarators(RUNTIME_CLASS(CBEDeclarator))
{
    m_vAttributes.Add(&src.m_vAttributes);
    m_vAttributes.SetParentOfElements(this);
    m_vDeclarators.Add(&src.m_vDeclarators);
    m_vDeclarators.SetParentOfElements(this);
    m_pType = src.m_pType;
    m_pType->SetParent(this);
    IMPLEMENT_DYNAMIC_BASE(CBETypedDeclarator, CBEObject);
}

/**	\brief destructor of this instance */
CBETypedDeclarator::~CBETypedDeclarator()
{
    m_vAttributes.DeleteAll();
    m_vDeclarators.DeleteAll();
}

/**	\brief creates the back-end structure for a parameter
 *	\param pFEParameter the corresponding front-end parameter
 *	\param pContext the context of the code generation
 *	\return true if code generation was successful
 *
 * This implementation extracts the type, name and attributes from the front-end class.
 * Since a back-end parameter is expected to have only _ONE_ name, but the front-end typed
 * declarator may have several names, we expect that this function is only called for
 * the first of the declarators. The calling function has to take care of creating a seperate
 * parameter for each declarator.
 */
bool CBETypedDeclarator::CreateBackEnd(CFETypedDeclarator * pFEParameter, CBEContext * pContext)
{
    VERBOSE("CBETypedDeclarator::CreateBE (front-end type) (%s at 0x%x)\n", GetClassName(), pFEParameter);
    if (!pFEParameter)
    {
        VERBOSE("CBETypedDeclarator::CreateBE failed because front-end parameter is 0\n");
        return false;
    }
    // if we already have declarators, attributes or type, remove them (clean myself)
    if (m_pType)
    {
        delete m_pType;
        m_pType = 0;
    }
    m_vAttributes.DeleteAll();
    m_vDeclarators.DeleteAll();
    // get names
    VectorElement *pIter = pFEParameter->GetFirstDeclarator();
    CFEDeclarator *pFEDecl;
    while ((pFEDecl = pFEParameter->GetNextDeclarator(pIter)) != 0)
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
    pIter = pFEParameter->GetFirstAttribute();
    CFEAttribute *pFEAttribute;
    while ((pFEAttribute = pFEParameter->GetNextAttribute(pIter)) != 0)
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

/**	\brief creates the typed declarator using user defined type and name
 *	\param sUserDefinedType the user defined type
 *	\param sName the name of the typed declarator
 *	\param nStars the number of stars for the declarator
 *  \param pContext the context of the code generation
 *	\return true if successful
 */
bool CBETypedDeclarator::CreateBackEnd(String sUserDefinedType, String sName, int nStars, CBEContext * pContext)
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
    m_pType->SetParent(this);	// has to be set before calling CreateBE
    if (!(((CBEUserDefinedType *) m_pType)->CreateBackEnd(sUserDefinedType, pContext)))
    {
        delete m_pType;
        m_pType = 0;
        VERBOSE("CBETypedDeclarator::CreateBE failed because type could not be added\n");
        return false;
    }

    return true;
}

/**	\brief creates the typed declarator using a given back-end type and a name
 *	\param pType the type of the typed declarator
 *	\param sName the name of the declarator
 *	\param pContext the context of the code generation
 *	\return true if successful
 */
bool CBETypedDeclarator::CreateBackEnd(CBEType * pType, String sName, CBEContext * pContext)
{
    VERBOSE("CBETypedDeclarator::CreateBE(given type)\n");

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

    return true;
}

/**	\brief adds another attribute to the vector
 *	\param pAttribute the attribute to add
 */
void CBETypedDeclarator::AddAttribute(CBEAttribute * pAttribute)
{
    if (!pAttribute)
        return;
    m_vAttributes.Add(pAttribute);
    pAttribute->SetParent(this);
}

/**	\brief removes an attribute from the attributes vector
 *	\param pAttribute the attribute to remove
 */
void CBETypedDeclarator::RemoveAttribute(CBEAttribute * pAttribute)
{
    if (!pAttribute)
        return;
    m_vAttributes.Remove(pAttribute);
}

/**	\brief retrieves a pointer to the first attribute
 *	\return a pointer to the first attribute element
 */
VectorElement *CBETypedDeclarator::GetFirstAttribute()
{
    return m_vAttributes.GetFirst();
}

/**	\brief retrieves the next attribute
 *	\param pIter the pointer to the next attribute
 *	\return a reference to the next attribute
 */
CBEAttribute *CBETypedDeclarator::GetNextAttribute(VectorElement * &pIter)
{
    if (!pIter)
        return 0;
    CBEAttribute *pRet = (CBEAttribute *) pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextAttribute(pIter);
    return pRet;
}

/**	\brief writes the content of a typed declarator to the target file
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
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
    if (m_vDeclarators.GetSize() > 0)
        pFile->Print(" ");
    WriteDeclarators(pFile, pContext);
}

/**	\brief writes the attributes of the typed declarator
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The current implementation does nothing.
 */
void CBETypedDeclarator::WriteAttributes(CBEFile * pFile, CBEContext * pContext)
{

}

/**	\brief writes the type to the target file
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *  \param bUseConst true if the 'const' keyword should be used
 *
 * Calls the Write operation of the type.
 *
 * - An IN string is always const (its automatically an char*) on client's side.
 * (server side may need to fiddle with pointer)
 * - An OUT string is never const, because it is set by the  server.
 * Exception is a send function from server to client, which
 * can send const string as OUT. -> we do not handle this yet.
 * - An IN string is not const if its a global test variable
 *
 * If we do not use C or L4 types, we have to use the CORBA-types, which
 * define a 'const char*' as 'const_CORBA_char_ptr'
 */
void CBETypedDeclarator::WriteType(CBEFile * pFile, CBEContext * pContext, bool bUseConst)
{
    // string check
    if (IsString() && bUseConst)
    {
        if (pFile->IsOfFileType(FILETYPE_CLIENT) || pFile->IsOfFileType(FILETYPE_TESTSUITE))
        {
            if ((FindAttribute(ATTR_IN)) && (!FindAttribute(ATTR_OUT)))
            {
                if (pContext->IsOptionSet(PROGRAM_USE_CTYPES) ||
                    pContext->IsOptionSet(PROGRAM_USE_L4TYPES))
                    pFile->Print("const ");
                else
                    pFile->Print("const_");
            }
        }
    }
    if (m_pType)
        m_pType->Write(pFile, pContext);
}

/**	\brief writes the declarators to the target file
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 */
void CBETypedDeclarator::WriteDeclarators(CBEFile * pFile, CBEContext * pContext)
{
    if (!pFile->IsOpen())
        return;

    VectorElement *pIter = GetFirstDeclarator();
    bool bComma = false;
    CBEDeclarator *pDecl;
    while ((pDecl = GetNextDeclarator(pIter)) != 0)
    {
        if (bComma)
            pFile->Print(", ");
        pDecl->WriteDeclaration(pFile, pContext);
        bComma = true;
    }
}

/**	\brief adds another declarator to the vector
 *	\param pDeclarator the new declarator to add
 */
void CBETypedDeclarator::AddDeclarator(CBEDeclarator * pDeclarator)
{
    if (!pDeclarator)
        return;
    m_vDeclarators.Add(pDeclarator);
    pDeclarator->SetParent(this);
}

/**	\brief removes a declarator from the decls vector
 *	\param pDeclarator the declarator to remove
 */
void CBETypedDeclarator::RemoveDeclarator(CBEDeclarator * pDeclarator)
{
    if (!pDeclarator)
        return;
    m_vDeclarators.Remove(pDeclarator);
}

/**	\brief retrieves a pointer to the first declarator
 *	\return a pointer to the first declarator
 */
VectorElement *CBETypedDeclarator::GetFirstDeclarator()
{
    return m_vDeclarators.GetFirst();
}

/**	\brief retrieves a reference to the next declarator
 *	\param pIter the pointer to the next declarator
 *	\return a reference to the next declarator
 */
CBEDeclarator *CBETypedDeclarator::GetNextDeclarator(VectorElement * &pIter)
{
    if (!pIter)
        return 0;
    CBEDeclarator *pRet = (CBEDeclarator *) pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextDeclarator(pIter);
    return pRet;
}

/**	\brief searches for an attribute of the specified type
 *	\param nAttrType the type to search for
 *	\return a reference to the attribute or 0 if not found
 */
CBEAttribute *CBETypedDeclarator::FindAttribute(int nAttrType)
{
    VectorElement *pIter = GetFirstAttribute();
    CBEAttribute *pAttr;
    while ((pAttr = GetNextAttribute(pIter)) != 0)
    {
        if (pAttr->GetType() == nAttrType)
            return pAttr;
    }
    return 0;
}

/**	\brief test if this is a string variable
 *	\return true if it is
 *
 * A string is everything which has a base type 'char', is of a size bigger than 1 character
 * and has the string attribute. This function only checks the first declarator.
 */
bool CBETypedDeclarator::IsString()
{
    if (!FindAttribute(ATTR_STRING))
        return false;
    if (m_pType->IsOfType(TYPE_CHAR_ASTERISK))
        return true;
    if (m_pType->IsOfType(TYPE_CHAR))
    {
        VectorElement *pIter = GetFirstDeclarator();
        CBEDeclarator *pDeclarator = GetNextDeclarator(pIter);
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

/**	\brief tests if this is a parameter with a variable size
 *	\return true if it is
 *
 * A "variable sized parameter" is a parameter which has to be marshalled into a variable
 * sized message buffer. Or with other words: If this function returns true, this indicates
 * that the parameter has to be marshaled 'with care'.
 *
 * A variable sized parameter is a parameter which has a size_is, length_is or max_is attribute or is a
 * variable sized array. The *_is attributes can be of type CFEIntAttributes, which indicates
 * a concrete value. This invalidates the information about the attribute. A size_is attribute
 * is 'stronger' than a max_is attribute (meaning: if we find a size_is attribute, we ignore the
 * max_is attribute). A parameter is a variable sized array if it has the size/max attributes
 * or explicit array bounds or it is a string, which is indicated by the string attribute.
 *
 * All other parameters, even the ones with asterisks, are not variable sized. They have to be
 * dereferenced to be marshalled with their scalar values.
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
    CBEAttribute *pAttr;
    if ((pAttr = FindAttribute(ATTR_SIZE_IS)) != 0)
    {
        if (pAttr->IsOfType(ATTR_CLASS_IS))
            return true;
    }
    if ((pAttr = FindAttribute(ATTR_LENGTH_IS)) != 0)
    {
        if (pAttr->IsOfType(ATTR_CLASS_IS))
            return true;
    }
    if ((pAttr = FindAttribute(ATTR_MAX_IS)) != 0)
    {
        if (pAttr->IsOfType(ATTR_CLASS_IS))
            return true;
    }
    if (IsString())
        return true;
    // if type is variable sized, then this variable
    // is too (e.g. a struct with a var-sized member
    if (GetType())
        if (GetType()->GetSize() < 0)
            return true;
    // test declarators for arrays
    VectorElement *pIter = GetFirstDeclarator();
    CBEDeclarator *pDeclarator;
    while ((pDeclarator = GetNextDeclarator(pIter)) != 0)
    {
        if (!pDeclarator->IsArray())
            continue;
        VectorElement *pIter = pDeclarator->GetFirstArrayBound();
        CBEExpression *pExpr;
        while ((pExpr = pDeclarator->GetNextArrayBound(pIter)) != 0)
        {
            if (pExpr->IsOfType(EXPR_NONE)) // no bound
                return true;
        }
    }
    // no size/max attribute and no unbound array:
    // should be treated as scalar
// <DEBUG>
//    pIter = GetFirstDeclarator();
//    pDeclarator = GetNextDeclarator(pIter);
//    DTRACE("CBETypedDeclarator::IsVariableSized(%s): size_is=%s(%s), string=%s, ref=%s, type=%d, stars=%d, size=%d, OUT=%s, constr.=%s, func=%s\n",
//           (const char*)pDeclarator->GetName(),
//           (FindAttribute(ATTR_SIZE_IS))?"true":"false",
//           (FindAttribute(ATTR_SIZE_IS))?(FindAttribute(ATTR_SIZE_IS)->GetClassName()):"null",
//           (FindAttribute(ATTR_STRING))?"true":"false",
//           (FindAttribute(ATTR_REF))?"true":"false",
//           GetType()->GetFEType(),
//           pDeclarator->GetStars(),
//           pDeclarator->GetSize(),
//           (FindAttribute(ATTR_OUT))?"true":"false",
//           GetType()->IsConstructedType()?"true":"false",
//           (const char*)GetFunction()->GetName());
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

/**	\brief calculates the size of a typed declarator
 *	\return the number of bytes needed for the declarators
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
    if (m_pType->IsVoid())
        return 0;
    // get type's size
    int nTypeSize = m_pType->GetSize();
    int nSize = 0;
    VectorElement *pIter = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = GetNextDeclarator(pIter)) != 0)
    {
        int nDeclSize = pDecl->GetSize();
        // if referenced OUT, this is the size
        if ((nDeclSize == -1) &&
            (pDecl->GetStars() == 1) &&
            (FindAttribute(ATTR_OUT)))
        {
            nSize += nTypeSize;
            continue;
        }
        // if reference struct, this is the size
        if ((nDeclSize == -1) &&
            (pDecl->GetStars() == 1) &&
            (m_pType->IsConstructedType()))
        {
            nSize += nTypeSize;
            continue;
        }
        // if variables sized, return -1
        if (nDeclSize < 0)
            return -1;
        // if bitfield: it multiplies with zero and returns zero
        // const array or simple? -> return array-dimension * type's size
        nSize += nDeclSize * nTypeSize;
    }
    return nSize;
}

/** \brief calculates the max size of the paramater
 *  \param pContext teh context of the size calculation
 *  \return the size in bytes
 */
int CBETypedDeclarator::GetMaxSize(CBEContext *pContext)
{
    if (m_pType->IsVoid())
        return 0;
    // get type's size
    int nTypeSize = m_pType->GetSize();
    int nSize = 0;
    bool bVarSized = false;
    VectorElement *pIter = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = GetNextDeclarator(pIter)) != 0)
    {
        int nDeclSize = pDecl->GetMaxSize(pContext);
        // if decl is array, but returns a negative maximum size,
        // it is an unbound array. Its max size is the max size of it's
        // type times the dimensions
        if ((nDeclSize < 0) && pDecl->IsArray())
        {
            nSize += pContext->GetSizes()->GetMaxSizeOfType(m_pType->GetFEType()) * -nDeclSize;
            continue;
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
    if (bVarSized || m_pType->IsPointerType())
    {
		// check max attributes
		CBEAttribute *pAttr;
		if ((pAttr = FindAttribute(ATTR_MAX_IS)) != 0)
		{
			if (pAttr->IsOfType(ATTR_CLASS_INT))
				nSize = pAttr->GetIntValue();
			else
				nSize = -1;
		}
        else
            nSize = -1;
    }
    return nSize;
}

/**	\brief allows access to type of typed declarator
 *	\return reference to m_pType
 */
CBEType *CBETypedDeclarator::GetType()
{
    return m_pType;
}

/**	\brief writes the declaration of the global test variables
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
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

/**	\brief writes the list of global declarators
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 */
void CBETypedDeclarator::WriteGlobalDeclarators(CBEFile * pFile, CBEContext * pContext)
{
    VectorElement *pIter = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    bool bComma = false;
    while ((pDecl = GetNextDeclarator(pIter)) != 0)
    {
        if (bComma)
            pFile->Print(", ");
        pDecl->WriteGlobalName(pFile, pContext, true);
        if (pDecl->GetArrayDimensionCount() == 0)
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

/**	\brief write variable with an extra declarator for each indirection
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
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

    WriteType(pFile, pContext, false);
    pFile->Print(" ");

    // test for pointer types
    // if it is simple and we do not generate C-Types, than ignore the
    // pointer type, since it is typedefed, which makes the purpose of
    // this bool-parameter obsolete
    bool bIsPointerType = GetType()->IsPointerType();
    if (!pContext->IsOptionSet(PROGRAM_USE_CTYPES) &&
        GetType()->IsSimpleType())
        bIsPointerType = false;
    // test if we need a pointer of this variable
    bool bUsePointer = IsString() && !(GetType()->IsPointerType());
    bUsePointer = bUsePointer || FindAttribute(ATTR_SIZE_IS) || FindAttribute(ATTR_LENGTH_IS);
    // loop over declarators
    VectorElement *pIter = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    bool bComma = false;
    while ((pDecl = GetNextDeclarator(pIter)) != 0)
    {
        if (bComma)
            pFile->Print(", ");
        pDecl->WriteIndirect(pFile, bUsePointer, bIsPointerType, pContext);
        bComma = true;
    }
}

/**	\brief initializes indirect variables as declared by WriteIndirect
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This functin does assign a pointered variable a reference to a "unpointered" variable.
 */
void CBETypedDeclarator::WriteIndirectInitialization(CBEFile * pFile, CBEContext * pContext)
{
    bool bUsePointer = IsString() && !(GetType()->IsPointerType());
    bUsePointer = bUsePointer || FindAttribute(ATTR_SIZE_IS) || FindAttribute(ATTR_LENGTH_IS);
    VectorElement *pIter = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = GetNextDeclarator(pIter)) != 0)
    {
        pDecl->WriteIndirectInitialization(pFile, bUsePointer, pContext);
    }
}

/**	\brief checks if a declarator is member of this typed declarator
 *	\param sName the name of the searched declarator
 *	\return a reference to the found declarator or 0 if not found
 */
CBEDeclarator *CBETypedDeclarator::FindDeclarator(String sName)
{
    if (sName.IsEmpty())
        return 0;
    VectorElement *pIter = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = GetNextDeclarator(pIter)) != 0)
    {
        if (pDecl->GetName() == sName)
            return pDecl;
    }
    return 0;
}

/**	\brief replaces the current type with the new one
 *	\param pNewType the new type
 *	\return the old type
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

/**	\brief write the variable declaration and initializes the variables to their zero value
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * We do not init user defined types, because we don't know how.
 */
void CBETypedDeclarator::WriteZeroInitDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    if (GetType()->IsVoid())
        return;
    VectorElement *pIter = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = GetNextDeclarator(pIter)) != 0)
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

/**	\brief writes the code to obtain the size of a variabel sized parameter
 *	\param pFile the file to write to
 *  \param pStack the declarator stack used for variable sized members of structs, union
 *	\param pContext the context of the write operation
 *
 * If this is variable sized, there has to be some attribute, defining the actual size
 * of this parameter at run-time. So what we do is to search for size_is, length_is or max_is
 * attributes.
 *
 * This is different if this is a string. So if it is, we simply use the strlen function.
 */
void CBETypedDeclarator::WriteGetSize(CBEFile * pFile, CDeclaratorStack *pStack, CBEContext * pContext)
{
//    if (!IsVariableSized() && !IsString())
//        return;

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
                VectorElement *pIter = GetFirstDeclarator();
                CBEDeclarator *pDecl = GetNextDeclarator(pIter);
                if (pDecl)
                {
                    pFile->Print("strlen(");
                    // check if parameter has additional reference
                    bool bAddRef = false;
                    CBEFunction *pFunction = GetFunction();
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
                        pStack->Write(pFile, false, false, pContext);
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
                    pStack = new CDeclaratorStack();
                VectorElement *pIter = GetFirstDeclarator();
                CBEDeclarator *pDecl;
                while ((pDecl = GetNextDeclarator(pIter)) != 0)
                {
                    pStack->Push(pDecl);
                    m_pType->WriteGetSize(pFile, pStack, pContext);
                    pStack->Pop();
                }
                return;
            }
        }
    }
    if (!pAttr)
        return;
    if (pAttr->IsOfType(ATTR_CLASS_IS))
    {
        VectorElement *pIter = pAttr->GetFirstIsAttribute();
        CBEDeclarator *pSizeName = pAttr->GetNextIsAttribute(pIter);
        CBEFunction *pFunction = GetFunction();
        CBETypedDeclarator *pSizeParameter = 0;
        if (pFunction)
        {
            // get original parameter
            pSizeParameter = pFunction->FindParameter(pSizeName->GetName());

            if (pFunction->HasAdditionalReference(pSizeName, pContext))
                pFile->Print("*");
        }
        if (pSizeParameter)
            pSizeParameter->WriteDeclarators(pFile, pContext); // has only one declarator
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
    VectorElement *pIter = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    int nSize = 0;
    while ((pDecl = GetNextDeclarator(pIter)) != 0)
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
	return ((((nDirection & DIRECTION_IN) != 0) && (FindAttribute(ATTR_IN) != 0)) ||
            (((nDirection & DIRECTION_OUT) != 0) && (FindAttribute(ATTR_OUT) != 0)));
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
    VectorElement *pIter = GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = GetNextDeclarator(pIter)) != 0)
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
CBEAttribute* CBETypedDeclarator::FindIsAttribute(String sDeclName)
{
    VectorElement *pIter = GetFirstAttribute();
    CBEAttribute *pAttr;
    while ((pAttr = GetNextAttribute(pIter)) != 0)
    {
        if (pAttr->FindIsParameter(sDeclName))
            return pAttr;
    }
    return 0;
}
