/**
 *    \file    dice/src/be/BEAttribute.cpp
 *    \brief   contains the implementation of the class CBEAttribute
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

#include "be/BEAttribute.h"
#include "be/BEContext.h"
#include "be/BEType.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "be/BEFunction.h"
#include "be/BEStructType.h"

#include "fe/FEAttribute.h"
#include "fe/FEIntAttribute.h"
#include "fe/FEIsAttribute.h"
#include "fe/FEStringAttribute.h"
#include "fe/FETypeAttribute.h"
#include "fe/FEVersionAttribute.h"
#include "fe/FEOperation.h"
#include "fe/FEStructType.h"
#include "fe/FEDeclarator.h"

#include "fe/FEFile.h"

CBEAttribute::CBEAttribute()
{
    m_nType = ATTR_NONE;
    m_nMajorVersion = 0;
    m_nMinorVersion = 0;
    m_nAttrClass = ATTR_CLASS_NONE;
    m_nIntValue = 0;
    m_pPtrDefault = 0;
    m_pType = 0;
}

CBEAttribute::CBEAttribute(CBEAttribute & src)
: CBEObject(src)
{
    // init everything with default values
    m_nMajorVersion = 0;
    m_nMinorVersion = 0;
    m_nAttrClass = ATTR_CLASS_NONE;
    m_nIntValue = 0;
    m_pPtrDefault = 0;
    m_pType = 0;
    // depending on type do specialized init
    m_nType = src.m_nType;
    switch (m_nType)
    {
    case ATTR_VERSION:    // version
        m_nMajorVersion = src.m_nMajorVersion;
        m_nMinorVersion = src.m_nMinorVersion;
        break;
    case ATTR_UUID:        // string
    case ATTR_HELPFILE:    // string
    case ATTR_HELPSTRING:    // string
        m_sString = src.m_sString;
        break;
    case ATTR_LCID:        // int
    case ATTR_HELPCONTEXT:    // int
        m_nIntValue = src.m_nIntValue;
        break;
    case ATTR_SWITCH_IS:    // Is
    case ATTR_FIRST_IS:    // IS
    case ATTR_LAST_IS:    // Is
    case ATTR_LENGTH_IS:    // Is
    case ATTR_MIN_IS:    // Is
    case ATTR_MAX_IS:    // Is
    case ATTR_SIZE_IS:    // Is
    case ATTR_IID_IS:    // Is
        {
            vector<CBEDeclarator*>::iterator iter;
            for (iter = src.m_vParameters.begin();
                 iter != src.m_vParameters.end(); iter++)
            {
                CBEDeclarator* pNew = (CBEDeclarator*)((*iter)->Clone());
                pNew->SetParent(this);
                m_vParameters.push_back(pNew);
            }
        }
        break;
    case ATTR_TRANSMIT_AS:    // type
    case ATTR_SWITCH_TYPE:    // type
        m_pType = (CBEType*)src.m_pType->Clone();
        break;
    default:
        // nothing to be done
        break;
    }
}

/**    \brief destructor of this instance */
CBEAttribute::~CBEAttribute()
{
    while (!m_vParameters.empty())
    {
        delete m_vParameters.back();
        m_vParameters.pop_back();
    }
    if (m_pType)
        delete m_pType;
}

/**    \brief prepares this instance for the code generation
 *    \param pFEAttribute the corresponding front-end attribute
 *    \param pContext the context of the code generation
 *    \return true if the code generation was successful
 *
 * This implementation only reads the attribute's type from the front-end. It should also contain
 * the different members of the front-end attributes. Because we do not really support those, we ignore
 * them for now.
 */
bool CBEAttribute::CreateBackEnd(CFEAttribute * pFEAttribute, CBEContext * pContext)
{
    // call CBEObject's CreateBackEnd method
    if (!CBEObject::CreateBackEnd(pFEAttribute))
        return false;

    m_nType = pFEAttribute->GetAttrType();
    switch (m_nType)
    {
    case ATTR_VERSION:    // version
        m_nAttrClass = ATTR_CLASS_VERSION;
        return CreateBackEndVersion((CFEVersionAttribute *) pFEAttribute, pContext);
        break;
    case ATTR_UUID:        // string or int
        if (dynamic_cast<CFEIntAttribute*>(pFEAttribute))
        {
            m_nAttrClass = ATTR_CLASS_INT;
            return CreateBackEndInt((CFEIntAttribute*)pFEAttribute, pContext);
        }
        else
        {
            m_nAttrClass = ATTR_CLASS_STRING;
            return CreateBackEndString((CFEStringAttribute *) pFEAttribute, pContext);
        }
        break;
    case ATTR_HELPFILE:    // string
    case ATTR_HELPSTRING:    // string
    case ATTR_DEFAULT_FUNCTION: // string
    case ATTR_ERROR_FUNCTION:
    case ATTR_ERROR_FUNCTION_CLIENT:
    case ATTR_ERROR_FUNCTION_SERVER:
    case ATTR_INIT_RCVSTRING_CLIENT: // string
    case ATTR_INIT_RCVSTRING_SERVER: // string
        m_nAttrClass = ATTR_CLASS_STRING;
        return CreateBackEndString((CFEStringAttribute*)pFEAttribute, pContext);
        break;
    case ATTR_LCID:        // int
    case ATTR_HELPCONTEXT:    // int
        m_nAttrClass = ATTR_CLASS_INT;
        return CreateBackEndInt((CFEIntAttribute *) pFEAttribute, pContext);
        break;
    case ATTR_SWITCH_IS:    // Is
    case ATTR_FIRST_IS:    // IS
    case ATTR_LAST_IS:    // Is
    case ATTR_LENGTH_IS:    // Is
    case ATTR_MIN_IS:    // Is
    case ATTR_MAX_IS:    // Is
    case ATTR_SIZE_IS:    // Is
    case ATTR_IID_IS:    // Is
        m_nAttrClass = ATTR_CLASS_IS;
        // can be Is or Int attribute
        if (dynamic_cast<CFEIntAttribute*>(pFEAttribute))
        {
            m_nAttrClass = ATTR_CLASS_INT;
            return CreateBackEndInt((CFEIntAttribute *) pFEAttribute, pContext);
        }
        else
        {
            m_nAttrClass = ATTR_CLASS_IS;
            return CreateBackEndIs((CFEIsAttribute *) pFEAttribute, pContext);
        }
        break;
    case ATTR_TRANSMIT_AS:    // type
    case ATTR_SWITCH_TYPE:    // type
        m_nAttrClass = ATTR_CLASS_TYPE;
        return CreateBackEndType((CFETypeAttribute *) pFEAttribute, pContext);
        break;
    case ATTR_ABSTRACT:    // simple
    case ATTR_ENDPOINT:    // not supported
    case ATTR_EXCEPTIONS:    // not supported
    case ATTR_LOCAL:        // simple
    case ATTR_POINTER_DEFAULT:    // not supported
    case ATTR_OBJECT:    // simple
    case ATTR_UUID_REP:    // not supported
    case ATTR_CONTROL:    // simple
    case ATTR_HIDDEN:    // simple
    case ATTR_RESTRICTED:    // simple
    case ATTR_IDEMPOTENT:    // simple
    case ATTR_BROADCAST:    // simple
    case ATTR_MAYBE:        // simple
    case ATTR_REFLECT_DELETIONS:    // simple
    case ATTR_HANDLE:    // simple
    case ATTR_IGNORE:    // simple
    case ATTR_IN:        // simple
    case ATTR_OUT:        // simple
    case ATTR_REF:        // simple
    case ATTR_UNIQUE:    // simple
    case ATTR_PTR:        // simple
    case ATTR_STRING:    // simple
    case ATTR_CONTEXT_HANDLE:    // simple
    case ATTR_SERVER_PARAMETER: // simple
    case ATTR_PREALLOC:     // simple
    case ATTR_ALLOW_REPLY_ONLY: // simple
    case ATTR_L4_SCHED_DECEIT: // simple
        m_nAttrClass = ATTR_CLASS_SIMPLE;
        break;
    case ATTR_INIT_RCVSTRING: // simple or string
        if (dynamic_cast<CFEStringAttribute*>(pFEAttribute))
        {
            m_nAttrClass = ATTR_CLASS_STRING;
            return CreateBackEndString((CFEStringAttribute*)pFEAttribute, pContext);
        }
        else
        {
            m_nAttrClass = ATTR_CLASS_SIMPLE;
        }
        break;
    default:
        m_nAttrClass = ATTR_CLASS_NONE;
        // nothing to be done
        break;
    }
    return true;
}

/**    \brief creates a simple attribute
 *    \param nType the attribute type
 *    \param pContext the context of the code generation
 *    \return true if successful
 */
bool CBEAttribute::CreateBackEnd(int nType, CBEContext * pContext)
{
    m_nType = nType;
    return true;
}

/**    \brief creates the back-end attribute for a type attribute
 *    \param pFETypeAttribute the type attribute to use
 *    \param pContext the context of the code generation
 *    \return true if code generation was successful
 */
bool CBEAttribute::CreateBackEndType(CFETypeAttribute * pFETypeAttribute, CBEContext * pContext)
{
    CFETypeSpec *pType = pFETypeAttribute->GetType();
    m_pType = pContext->GetClassFactory()->GetNewType(pType->GetType());
    m_pType->SetParent(this);
    if (!m_pType->CreateBackEnd(pType, pContext))
    {
        VERBOSE("%s failed because type could not be created\n", __PRETTY_FUNCTION__);
        delete m_pType;
        m_pType = 0;
        return false;
    }
    return true;
}

/**    \brief creates the back-end attribute for a string attribute
 *    \param pFEstringAttribute the respective string attribute
 *    \param pContext the context of the code generation
 *    \return true if the code generation was successful
 */
bool CBEAttribute::CreateBackEndString(CFEStringAttribute * pFEstringAttribute, CBEContext * pContext)
{
    m_sString = pFEstringAttribute->GetString();
    return true;
}

/**    \brief creates the back-end attribute for an IS-attribute
 *    \param pFEIsAttribute the respective front-end IS attribute
 *    \param pContext the context of the code generation
 *    \return true if the code generation was successful
 *
 * The is parameter usually contain variable name to determine the size of something
 * dynamically. To write this variable later correctly, we search for a reference to
 * it instead of creating an own instance.
 */
bool CBEAttribute::CreateBackEndIs(CFEIsAttribute * pFEIsAttribute, CBEContext * pContext)
{
    vector<CFEDeclarator*>::iterator iterAP = pFEIsAttribute->GetFirstAttrParameter();
    CFEDeclarator *pFEDeclarator;
    while ((pFEDeclarator = pFEIsAttribute->GetNextAttrParameter(iterAP)) != 0)
    {
        CBEDeclarator *pDeclarator = 0;
        string sName = pFEDeclarator->GetName();
        // if this attribute belongs to a parameter of a function, we should get a function here
        CBETypedDeclarator *pParameter = 0;
        // check if we can find a parameter of this function with this name
        CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
        if (pFunction)
            pParameter = pFunction->FindParameter(sName);
        // if no function or parameter found, check if constructed type
        CBEStructType *pStruct = GetSpecificParent<CBEStructType>();
        if (!pParameter && pStruct)
            pParameter = pStruct->FindMember(sName);
        if (!pParameter)
        {
            // not found -> the parameter might be declared after its use
            // -> we search the FE function for the parameter
            CFEOperation *pOperation = pFEIsAttribute->GetSpecificParent<CFEOperation>();
            CFEStructType *pFEStruct = (CFEStructType*)pFEIsAttribute->GetSpecificParent<CFEConstructedType>();
            CFETypedDeclarator *pFEParameter = 0;
            if (pOperation)
                pFEParameter = pOperation->FindParameter(sName);
            if (!pFEParameter && pFEStruct)
                pFEParameter = pFEStruct->FindMember(sName);
            // check const if no parameter or struct member found
            CFEConstDeclarator *pFEConstant = 0;
            if (!pFEParameter)
            {
                // the declarator might be a constant -> search for this one
                CFEFile *pFERoot = dynamic_cast<CFEFile*>(pFEIsAttribute->GetRoot());
                assert(pFERoot);
                pFEConstant = pFERoot->FindConstDeclarator(sName);
            }
            if (!pFEParameter && !pFEConstant)
            {
                // the is attribute has no parameter
                VERBOSE("%s failed because the IS-attribute's parameter (%s) is no function parameter or constant\n",
                        __PRETTY_FUNCTION__, sName.c_str());
                assert(false);
                return false;
            }
            pDeclarator = pContext->GetClassFactory()->GetNewDeclarator();
            AddIsParameter(pDeclarator);
            if (!pDeclarator->CreateBackEnd(pFEDeclarator, pContext))
            {
                VERBOSE("%s failed because declarator could not be created\n", __PRETTY_FUNCTION__);
                delete pDeclarator;
                return false;
            }
        }
        else
        {
            pDeclarator = pParameter->FindDeclarator(sName);
            if (!pDeclarator)
            {
                // this cannot happend, because FindParameter uses FindDeclarator
                // so if pParameter != 0 this should be != 0 too
                return false;
            }
            AddIsParameter(pDeclarator);
        }
        if (!pDeclarator)
        {
            VERBOSE("Attribute %s is declared in invalid context.\n", sName.c_str());
            assert(false);
            return false;
        }
    }
    return true;
}

/**    \brief creates the back-end attribute for an integer attribute
 *    \param pFEIntAttribute the repective front-end attribute
 *    \param pContext the context of the code generation
 *    \return true if code generation was successful
 */
bool CBEAttribute::CreateBackEndInt(CFEIntAttribute * pFEIntAttribute, CBEContext * pContext)
{
    m_nIntValue = pFEIntAttribute->GetIntValue();
    return true;
}

/**    \brief creates the back-end attribute for a version attribute
 *    \param pFEVersionAttribute the respective front-end attribute
 *    \param pContext the context of the code generation
 *    \return true if code generation was successful
 */
bool CBEAttribute::CreateBackEndVersion(CFEVersionAttribute *pFEVersionAttribute, CBEContext * pContext)
{
    pFEVersionAttribute->GetVersion(m_nMajorVersion, m_nMinorVersion);
    return true;
}

/**    \brief adds a new parameter to the Is-parameter vector
 *    \param pDecl the declarator, which is the parameter
 *
 * This function adds a declarator to the parameter vector in the union. It first checks, whether this is a
 * IS attribute. If not the functionr returns immediately.
 */
void CBEAttribute::AddIsParameter(CBEDeclarator * pDecl)
{
    switch (m_nType)
    {
    case ATTR_SWITCH_IS:    // Is
    case ATTR_FIRST_IS:    // IS
    case ATTR_LAST_IS:    // Is
    case ATTR_LENGTH_IS:    // Is
    case ATTR_MIN_IS:    // Is
    case ATTR_MAX_IS:    // Is
    case ATTR_SIZE_IS:    // Is
    case ATTR_IID_IS:    // Is
        break;
    default:
        return;
        break;
    }
    if (pDecl)
    {
        m_vParameters.push_back(pDecl);
        pDecl->SetParent(this);
    }
}

/**    \brief returns the attribute's type
 *    \return the member m_nType
 */
int CBEAttribute::GetType()
{
    return m_nType;
}

/**    \brief retrieves a pointer to the first is attribute's parameter
 *    \return pointer to attribute parameter
 */
vector<CBEDeclarator*>::iterator CBEAttribute::GetFirstIsAttribute()
{
    return m_vParameters.begin();
}

/**    \brief gets the next attribute's parameter
 *    \param iter the pointer to the next parameter
 *    \return reference to next parameter
 */
CBEDeclarator*
CBEAttribute::GetNextIsAttribute(vector<CBEDeclarator*>::iterator &iter)
{
    if (iter == m_vParameters.end())
        return 0;
    return *iter++;
}

/** \brief checks the type of an attribute
 *  \param nType the type to compare the own type to
 *  \return true if the types are the same
 */
bool CBEAttribute::IsOfType(ATTR_CLASS nType)
{
    return (m_nAttrClass == nType);
}

/** \brief retrieves the integer value of this attribute
 *  \return the value of m_nIntValue or -1 if m_nAttrClass != ATTR_CLASS_INT
 */
int CBEAttribute::GetIntValue()
{
    return (m_nAttrClass == ATTR_CLASS_INT) ? m_nIntValue : -1;
}

/** \brief creates a new instance of this class */
CObject * CBEAttribute::Clone()
{
    return new CBEAttribute(*this);
}

/** \brief finds the IS declarator
 *  \param sName the name of the IS decl
 *  \return a reference to the IS decl
 */
CBEDeclarator* CBEAttribute::FindIsParameter(string sName)
{
    vector<CBEDeclarator*>::iterator iter = GetFirstIsAttribute();
    CBEDeclarator *pDecl;
    while ((pDecl = GetNextIsAttribute(iter)) != 0)
    {
        if (pDecl->GetName() == sName)
            return pDecl;
    }
    return 0;
}

/** \brief access string value
 *  \return string member if ATTR_CLASS_STRING
 */
string CBEAttribute::GetString()
{
    if (m_nAttrClass == ATTR_CLASS_STRING)
        return m_sString;
    return string();
}

/** \brief delivers the number of is attributes from the given iterator to the end of the list
 *  \param iter the iterator pointing to the next Is Attribute
 *  \return the number of remaining is attributes
 *
 * Since the iterator is no reference parameter we can use it in this function to
 * iterate over the IS attributes without manipulating the iterator of the caller.
 * We simply count the number of times GetNextIsAttribute returns a new IS attribute
 * parameter.
 */
int
CBEAttribute::GetRemainingNumberOfIsAttributes(vector<CBEDeclarator*>::iterator iter)
{
    int nCount = 0;
    while (GetNextIsAttribute(iter)) nCount++;
    return nCount;
}

/** \brief retrieve reference to the type of a type attribute (such as transmit_as)
 *  \return a reference to the type of a type attribute
 */
CBEType* CBEAttribute::GetAttrType()
{
    return m_pType;
}
