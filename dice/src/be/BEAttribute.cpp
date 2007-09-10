/**
 *  \file    dice/src/be/BEAttribute.cpp
 *  \brief   contains the implementation of the class CBEAttribute
 *
 *  \date    01/15/2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "be/BEFunction.h"
#include "be/BEStructType.h"
#include "be/BEClassFactory.h"
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
#include "Compiler.h"
#include "Error.h"
#include <sstream>
#include <cassert>

CBEAttribute::CBEAttribute()
: m_pPtrDefault(0),
  m_sString(),
  m_pType(0),
  m_Parameters(0, this)
{
    m_nType = ATTR_NONE;
    m_nAttrClass = ATTR_CLASS_NONE;
    m_vPortSpecs.clear();
    m_vExceptions.clear();
    m_nIntValue = 0;
    m_nMajorVersion = 0;
    m_nMinorVersion = 0;
}

CBEAttribute::CBEAttribute(CBEAttribute* src)
: CBEObject(src),
  m_pPtrDefault(0),
  m_sString(),
  m_pType(0),
  m_Parameters(src->m_Parameters)
{
	// init everything with default values
	m_vPortSpecs.clear();
	m_vExceptions.clear();
	m_nIntValue = 0;
	m_Parameters.Adopt(this);
	m_nMajorVersion = 0;
	m_nMinorVersion = 0;
	// depending on type do specialized init
	m_nAttrClass = src->m_nAttrClass;
	m_nType = src->m_nType;
	switch (m_nType)
	{
	case ATTR_VERSION:    // version
		m_nMajorVersion = src->m_nMajorVersion;
		m_nMinorVersion = src->m_nMinorVersion;
		break;
	case ATTR_UUID:        // string
	case ATTR_HELPFILE:    // string
	case ATTR_HELPSTRING:    // string
		m_sString = src->m_sString;
		break;
	case ATTR_LCID:        // int
	case ATTR_HELPCONTEXT:    // int
		m_nIntValue = src->m_nIntValue;
		break;
	case ATTR_SWITCH_IS:    // Is
	case ATTR_FIRST_IS:    // IS
	case ATTR_LAST_IS:    // Is
	case ATTR_LENGTH_IS:    // Is
	case ATTR_MIN_IS:    // Is
	case ATTR_MAX_IS:    // Is
	case ATTR_SIZE_IS:    // Is
	case ATTR_IID_IS:    // Is
		if (m_nAttrClass == ATTR_CLASS_STRING)
			m_sString = src->m_sString;
		else if (m_nAttrClass == ATTR_CLASS_INT)
			m_nIntValue = src->m_nIntValue;
		// m_Parameters has been copied alread (empty or full)
		break;
	case ATTR_TRANSMIT_AS:    // type
	case ATTR_SWITCH_TYPE:    // type
		CLONE_MEM(CBEType, m_pType);
		break;
	default:
		// nothing to be done
		break;
	}
}

/** \brief destructor of this instance */
CBEAttribute::~CBEAttribute()
{
    if (m_pType)
        delete m_pType;
}

/** \brief create a copy of this object
 *  \return a reference to the clone
 */
CObject* CBEAttribute::Clone()
{
	return new CBEAttribute(this);
}

/** \brief prepares this instance for the code generation
 *  \param pFEAttribute the corresponding front-end attribute
 *  \return true if the code generation was successful
 *
 * This implementation only reads the attribute's type from the front-end. It
 * should also contain the different members of the front-end attributes.
 * Because we do not really support those, we ignore them for now.
 */
void
CBEAttribute::CreateBackEnd(CFEAttribute * pFEAttribute)
{
    // call CBEObject's CreateBackEnd method
    CBEObject::CreateBackEnd(pFEAttribute);

    m_nType = pFEAttribute->GetAttrType();
    switch (m_nType)
    {
    case ATTR_VERSION:    // version
        m_nAttrClass = ATTR_CLASS_VERSION;
        CreateBackEndVersion((CFEVersionAttribute *) pFEAttribute);
        break;
    case ATTR_UUID:        // string or int
        if (dynamic_cast<CFEIntAttribute*>(pFEAttribute))
        {
            m_nAttrClass = ATTR_CLASS_INT;
            CreateBackEndInt((CFEIntAttribute*)pFEAttribute);
        }
        else
        {
            m_nAttrClass = ATTR_CLASS_STRING;
            CreateBackEndString((CFEStringAttribute *) pFEAttribute);
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
        CreateBackEndString((CFEStringAttribute*)pFEAttribute);
        break;
    case ATTR_LCID:        // int
    case ATTR_HELPCONTEXT:    // int
        m_nAttrClass = ATTR_CLASS_INT;
        CreateBackEndInt((CFEIntAttribute *) pFEAttribute);
        break;
    case ATTR_SWITCH_IS:    // Is
    case ATTR_FIRST_IS:    // IS
    case ATTR_LAST_IS:    // Is
    case ATTR_LENGTH_IS:    // Is
    case ATTR_MIN_IS:    // Is
    case ATTR_MAX_IS:    // Is
    case ATTR_SIZE_IS:    // Is
    case ATTR_IID_IS:    // Is
        // can be Is or Int attribute
        if (dynamic_cast<CFEIntAttribute*>(pFEAttribute))
        {
            m_nAttrClass = ATTR_CLASS_INT;
            CreateBackEndInt((CFEIntAttribute *) pFEAttribute);
        }
        else
        {
            m_nAttrClass = ATTR_CLASS_IS;
            CreateBackEndIs((CFEIsAttribute *) pFEAttribute);
        }
        break;
    case ATTR_TRANSMIT_AS:    // type
    case ATTR_SWITCH_TYPE:    // type
        m_nAttrClass = ATTR_CLASS_TYPE;
        CreateBackEndType((CFETypeAttribute *) pFEAttribute);
        break;
    case ATTR_ABSTRACT:    // not supported
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
    case ATTR_PREALLOC_CLIENT:     // simple
    case ATTR_PREALLOC_SERVER:     // simple
    case ATTR_ALLOW_REPLY_ONLY: // simple
    case ATTR_SCHED_DONATE: // simple
    case ATTR_DEDICATED_PARTNER:
    case ATTR_DEFAULT_TIMEOUT:
        m_nAttrClass = ATTR_CLASS_SIMPLE;
        break;
    case ATTR_INIT_RCVSTRING: // simple or string
        if (dynamic_cast<CFEStringAttribute*>(pFEAttribute))
        {
            m_nAttrClass = ATTR_CLASS_STRING;
            CreateBackEndString((CFEStringAttribute*)pFEAttribute);
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
}

/** \brief creates a simple attribute
 *  \param nType the attribute type
 *  \return true if successful
 */
void
CBEAttribute::CreateBackEnd(ATTR_TYPE nType)
{
    m_nType = nType;
    switch (m_nType)
    {
    case ATTR_ABSTRACT:    // not supported
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
    case ATTR_PREALLOC_CLIENT:     // simple
    case ATTR_PREALLOC_SERVER:     // simple
    case ATTR_ALLOW_REPLY_ONLY: // simple
    case ATTR_SCHED_DONATE: // simple
    case ATTR_DEDICATED_PARTNER: // simple
    case ATTR_DEFAULT_TIMEOUT: // simple
    case ATTR_INIT_RCVSTRING: // simple or string
        m_nAttrClass = ATTR_CLASS_SIMPLE;
        break;
    default:
	{
	    std::ostringstream os;
	    os << m_nType;

	    string exc = string(__func__);
	    exc += "Attribute Type " + os.str() +
		" requires other CreateBackEnd method.";
	    throw new error::create_error(exc);
	}
	break;
    }
}

/** \brief creates the back-end attribute for a type attribute
 *  \param pFETypeAttribute the type attribute to use
 *  \return true if code generation was successful
 */
void
CBEAttribute::CreateBackEndType(CFETypeAttribute * pFETypeAttribute)
{
    CFETypeSpec *pType = pFETypeAttribute->GetType();
    CBEClassFactory *pCF = CBEClassFactory::Instance();
    m_pType = pCF->GetNewType(pType->GetType());
    m_pType->SetParent(this);
    m_pType->CreateBackEnd(pType);
}

/** \brief creates the back-end attribute for a string attribute
 *  \param pFEstringAttribute the respective string attribute
 *  \return true if the code generation was successful
 */
void
CBEAttribute::CreateBackEndString(CFEStringAttribute * pFEstringAttribute)
{
    m_sString = pFEstringAttribute->GetString();
}

/** \brief creates the back-end attribute for an IS-attribute
 *  \param pFEIsAttribute the respective front-end IS attribute
 *  \return true if the code generation was successful
 *
 * The is parameter usually contain variable name to determine the size of
 * something dynamically. To write this variable later correctly, we search
 * for a reference to it instead of creating an own instance.
 */
void
CBEAttribute::CreateBackEndIs(CFEIsAttribute * pFEIsAttribute)
{
	vector<CFEDeclarator*>::iterator iterAP;
	for (iterAP = pFEIsAttribute->m_AttrParameters.begin();
		iterAP != pFEIsAttribute->m_AttrParameters.end();
		iterAP++)
	{
		string exc = string(__func__);
		CBEDeclarator *pDeclarator = 0;
		string sName = (*iterAP)->GetName();
		// if this attribute belongs to a parameter of a function, we should
		// get a function here
		CBETypedDeclarator *pParameter = 0;
		// check if we can find a parameter of this function with this name
		CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
		if (pFunction)
			pParameter = pFunction->FindParameter(sName);
		// if no function or parameter found, check if constructed type
		CBEStructType *pStruct = GetSpecificParent<CBEStructType>();
		if (!pParameter && pStruct)
			pParameter = pStruct->m_Members.Find(sName);
		if (!pParameter)
		{
			// not found -> the parameter might be declared after its use
			// -> we search the FE function for the parameter
			CFEOperation *pOperation =
				pFEIsAttribute->GetSpecificParent<CFEOperation>();
			CFEStructType *pFEStruct =
				pFEIsAttribute->GetSpecificParent<CFEStructType>();
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
				CFEFile *pFERoot = pFEIsAttribute->GetRoot();
				assert(pFERoot);
				pFEConstant = pFERoot->FindConstDeclarator(sName);
			}
			if (!pFEParameter && !pFEConstant)
			{
				// the is attribute has no parameter
				exc += " failed because the IS-attribute's parameter (" +
					sName + ") is no function parameter or constant.";
				throw new error::create_error(exc);
			}
			CBEClassFactory *pCF = CBEClassFactory::Instance();
			pDeclarator = pCF->GetNewDeclarator();
			AddIsParameter(pDeclarator);
			pDeclarator->CreateBackEnd(*iterAP);
		}
		else
		{
			pDeclarator = pParameter->m_Declarators.Find(sName);
			assert(pDeclarator);
			CBEDeclarator *pNew = static_cast<CBEDeclarator*>(pDeclarator->Clone());
			AddIsParameter(pNew);
		}
		if (!pDeclarator)
		{
			exc += ": Attribute " + sName + " is declared in invalid context.";
			throw new error::create_error(exc);
		}
	}
}

/** \brief creates the back-end IS attribute for an declarator
 *  \param nType the exact type
 *  \param pDeclarator the respective declarator
 *  \return true if successful
 */
void CBEAttribute::CreateBackEndIs(ATTR_TYPE nType, CBEDeclarator *pDeclarator)
{
	string exc = string(__func__);
	// check type
	switch (nType)
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
		exc += " failed, beacuse invalid type.";
		throw new error::create_error(exc);
	}
	m_nType = nType;
	m_nAttrClass = ATTR_CLASS_IS;
	// check decl
	assert(pDeclarator);
	AddIsParameter(pDeclarator);
}

/** \brief creates the back-end INT attribute for an integer value
 *  \param nType the exact type
 *  \param nValue the  the value to set
 */
void CBEAttribute::CreateBackEndInt(ATTR_TYPE nType, int nValue)
{
	string exc = string(__func__);
	// check type
	switch (nType)
	{
	case ATTR_LCID:        // int
	case ATTR_HELPCONTEXT:    // int
	case ATTR_FIRST_IS:    // IS
	case ATTR_LAST_IS:    // Is
	case ATTR_LENGTH_IS:    // Is
	case ATTR_MIN_IS:    // Is
	case ATTR_MAX_IS:    // Is
	case ATTR_SIZE_IS:    // Is
	case ATTR_IID_IS:    // Is
		break;
	default:
		exc += " failed, because invalid type.";
		throw new error::create_error(exc);
	}
	m_nType = nType;
	m_nAttrClass = ATTR_CLASS_INT;
	m_nIntValue = nValue;
}

/** \brief prepares this instance for the code generation
 *  \param nType the type of the attribute
 *  \param pType the type to set
 */
void CBEAttribute::CreateBackEndType(ATTR_TYPE nType, CBEType *pType)
{
	string exc = string(__func__);
	// check type
	switch (nType)
	{
	case ATTR_TRANSMIT_AS:    // type
	case ATTR_SWITCH_TYPE:    // type
		break;
	default:
		exc += " failed, because invalid type.";
		throw new error::create_error(exc);
	}
	m_nType = nType;
	m_nAttrClass = ATTR_CLASS_TYPE;
	m_pType = pType;
}

/** \brief creates the back-end attribute for an integer attribute
 *  \param pFEIntAttribute the repective front-end attribute
 *  \return true if code generation was successful
 */
void CBEAttribute::CreateBackEndInt(CFEIntAttribute * pFEIntAttribute)
{
	m_nIntValue = pFEIntAttribute->GetIntValue();
}

/** \brief creates the back-end attribute for a version attribute
 *  \param pFEVersionAttribute the respective front-end attribute
 *  \return true if code generation was successful
 */
void
CBEAttribute::CreateBackEndVersion(CFEVersionAttribute *pFEVersionAttribute)
{
    pFEVersionAttribute->GetVersion(m_nMajorVersion, m_nMinorVersion);
}

/** \brief adds a new parameter to the Is-parameter vector
 *  \param pDecl the declarator, which is the parameter
 *
 * This function adds a declarator to the parameter vector in the union. It
 * first checks, whether this is a IS attribute. If not the function returns
 * immediately.
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
        m_Parameters.Add(pDecl);
        pDecl->SetParent(this);
    }
}

/** \brief delivers the number of is attributes from the given iterator to the end of the list
 *  \param iter the iterator pointing to the next Is Attribute
 *  \return the number of remaining is attributes
 *
 * Since the iterator is no reference parameter we can use it in this function
 * to iterate over the IS attributes without manipulating the iterator of the
 * caller.  We simply count the number of times GetNextIsAttribute returns a
 * new IS attribute parameter.
 */
int
CBEAttribute::GetRemainingNumberOfIsAttributes(
    vector<CBEDeclarator*>::iterator iter)
{
    int nCount = 0;
    for (; iter != m_Parameters.end(); iter++, nCount++) ;
    return nCount;
}

/** \brief writes the value of the parameter to a file
 *  \param pFile the file to write
 */
void
CBEAttribute::Write(CBEFile& pFile)
{
    string s;
    WriteToStr(s);
    pFile << s;
}

/** \brief writes the attribute to the string
 *  \param str the string to write to
 */
void
CBEAttribute::WriteToStr(string& str)
{
    switch(m_nAttrClass)
    {
    case ATTR_CLASS_STRING:
	str += m_sString;
	break;
    case ATTR_CLASS_INT:
	{
	    std::ostringstream os;
	    os << m_nIntValue;
	    str += os.str();
	}
	break;
    case ATTR_CLASS_IS:
	{
	    bool bComma = false;
	    vector<CBEDeclarator*>::iterator i;
	    for (i = m_Parameters.begin();
		 i != m_Parameters.end();
		 i++)
	    {
		if (bComma)
		    str += ", ";
		(*i)->WriteNameToStr(str);
		bComma = true;
	    }
	}
	break;
    case ATTR_CLASS_TYPE:
	m_pType->WriteToStr(str);
	break;
    default:
	break;
    }
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s: str = %s (class is %d)\n",
	__func__, str.c_str(), m_nAttrClass);
}
