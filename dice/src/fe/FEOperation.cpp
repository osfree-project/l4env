/**
 *	\file	dice/src/fe/FEOperation.cpp
 *	\brief	contains the implementation of the class CFEOperation
 *
 *	\date	01/31/2001
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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

#include "fe/FEOperation.h"
#include "fe/FEFile.h"
#include "fe/FEUserDefinedType.h"
#include "fe/FEStructType.h"
#include "fe/FEIsAttribute.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEDeclarator.h"
#include "fe/FESimpleType.h"
#include "fe/FEUnionType.h"
#include "fe/FEArrayType.h"
#include "fe/FEInterface.h"
#include "Vector.h"
#include "Compiler.h"
#include "File.h"

IMPLEMENT_DYNAMIC(CFEOperation)
    
CFEOperation::CFEOperation(CFETypeSpec * pReturnType,
			   String sName,
			   Vector * pParameters,
			   Vector * pAttributes,
			   Vector * pRaisesDeclarators)
{
    IMPLEMENT_DYNAMIC_BASE(CFEOperation, CFEInterfaceComponent);

    m_pReturnType = pReturnType;
    m_sOpName = sName;
    m_pOpParameters = pParameters;
    m_pOpAttributes = pAttributes;
    m_pRaisesDeclarators = pRaisesDeclarators;
}

CFEOperation::CFEOperation(CFEOperation & src)
:CFEInterfaceComponent(src)
{
    IMPLEMENT_DYNAMIC_BASE(CFEOperation, CFEInterfaceComponent);
    m_sOpName = src.m_sOpName;
    if (src.m_pReturnType)
      {
	  m_pReturnType = (CFETypeSpec *) (src.m_pReturnType->Clone());
	  m_pReturnType->SetParent(this);
      }
    else
	m_pReturnType = 0;
    if (src.m_pOpParameters)
      {
	  m_pOpParameters = src.m_pOpParameters->Clone();
	  m_pOpParameters->SetParentOfElements(this);
      }
    else
	m_pOpParameters = 0;
    if (src.m_pOpAttributes)
      {
	  m_pOpAttributes = src.m_pOpAttributes->Clone();
	  m_pOpAttributes->SetParentOfElements(this);
      }
    else
	m_pOpAttributes = 0;
    if (src.m_pRaisesDeclarators)
      {
	  m_pRaisesDeclarators = src.m_pRaisesDeclarators->Clone();
	  m_pRaisesDeclarators->SetParentOfElements(this);
      }
    else
	m_pRaisesDeclarators = 0;
}

/** the operation's destructor and of all its members */
CFEOperation::~CFEOperation()
{
    if (m_pReturnType)
	delete m_pReturnType;
    if (m_pOpParameters)
	delete m_pOpParameters;
    if (m_pOpAttributes)
	delete m_pOpAttributes;
    if (m_pRaisesDeclarators)
	delete m_pRaisesDeclarators;
}

/**
 *	\brief retrieves the operations name
 *	\return the name of the operation
 */
String CFEOperation::GetName()
{
    return m_sOpName;
}

/**
 *	\brief retrieves a pointer to the first parameter
 *	\return an interator, which points to the first parameter
 */
VectorElement *CFEOperation::GetFirstParameter()
{
    if (!m_pOpParameters)
	return 0;
    return m_pOpParameters->GetFirst();
}

/**
 *	\brief retrieves the next parameter
 *	\param iter the iterator pointing to the next parameter
 *	\return a pointer to an object containing the next parameter
 */
CFETypedDeclarator *CFEOperation::GetNextParameter(VectorElement * &iter)
{
    if (!m_pOpParameters)
        return 0;
    if (!iter)
        return 0;
    CFETypedDeclarator *pRet = (CFETypedDeclarator *) (iter->GetElement());
    iter = iter->GetNext();
    if (!pRet)
        return GetNextParameter(iter);
    return pRet;
}

/**
 *	\brief retrieves a pointer to the first attribute
 *	\return the iterator pointing to the first attribute
 */
VectorElement *CFEOperation::GetFirstAttribute()
{
    if (!m_pOpAttributes)
	return 0;
    return m_pOpAttributes->GetFirst();
}

/**
 *	\brief retrieves the next attribute
 *	\param iter the iterator pointing to the next attribute
 *	\return a pointer to the object cotaining the next attribute
 */
CFEAttribute *CFEOperation::GetNextAttribute(VectorElement * &iter)
{
    if (!m_pOpAttributes)
	return 0;
    if (!iter)
	return 0;
    CFEAttribute *pRet = (CFEAttribute *) (iter->GetElement());
    iter = iter->GetNext();
    return pRet;
}

/** retrieves the first raises declarator
 *	\return a pointer to the first raises declarator
 */
VectorElement *CFEOperation::GetFirstRaisesDeclarator()
{
    if (!m_pRaisesDeclarators)
	return 0;
    return m_pRaisesDeclarators->GetFirst();
}

/** \brief retrieves the next raises declarator
 *  \param iter a pointer to the next raises declarator
 *  \return a reference to the next raises declarator
 */
CFEIdentifier *CFEOperation::GetNextRaisesDeclarator(VectorElement * &iter)
{
    if (!m_pRaisesDeclarators)
	return 0;
    if (!iter)
	return 0;
    CFEIdentifier *pRet = (CFEIdentifier *) (iter->GetElement());
    iter = iter->GetNext();
    return pRet;
}

/**
 *	\brief tries to locate a parameter
 *	\param sName the name of the parameter
 *	\return a pointer to the parameter
 *
 * This function searches the operation's parameters for a parameter with the
 * specified name.
 */
CFETypedDeclarator *CFEOperation::FindParameter(String sName)
{
    if (!m_pOpParameters)
	return 0;
    if (sName.IsEmpty())
	return 0;

    // check for a structural seperator ("." or "->")
    String sBase,
	sMember;
    int iDot = sName.Find('.');
    int iPtr = sName.Find("->");
    int iUse = (iDot < iPtr) ? iDot : iPtr;
    if (iUse > 0)
      {
	  sBase = sName.Left(iDot);
	  if (iUse == iDot)
	      sMember = sName.Mid(iDot + 1);
	  else
	      sMember = sName.Mid(iDot + 2);
      }
    else
	sBase = sName;

    // iterate over parameter
    VectorElement *pIter = GetFirstParameter();
    CFETypedDeclarator *pTD;
    while ((pTD = GetNextParameter(pIter)) != 0)
      {
	  // if the parameter is the searched one
	  if (pTD->FindDeclarator(sBase))
	    {
		// the declarator is constructed (it has '.' or '->'
		if (iUse > 0)
		  {
		      // if the found typed declarator has a constructed type (struct)
		      // search for the second part of the name there
		      if (pTD->GetType()->IsKindOf(RUNTIME_CLASS(CFEStructType)))
			{
			    if (!(((CFEStructType *)(pTD->GetType()))->FindMember(sMember)))
			      {
				  // no nested member with that name found in the member
				  // -> this must be an invalid name
				  return 0;
			      }
			}
		  }
		// return the found typed declarator
		return pTD;
	    }
      }

    return 0;
}

/**
 *	\brief returns the return type
 *	\return the return type
 */
CFETypeSpec *CFEOperation::GetReturnType()
{
    return m_pReturnType;
}

/**
 *	\brief locates an attribute of a specific type
 *	\param eAttrType the attribute type to search for
 *	\return a pointer to the found attribute, 0 if none found
 *
 * This function can be used to check, whether an attribute is set for this operation
 * or not.
 */
CFEAttribute *CFEOperation::FindAttribute(ATTR_TYPE eAttrType)
{
    if (!m_pOpAttributes)
	return 0;
    CFEAttribute *pAttr;
    VectorElement *pIter = GetFirstAttribute();
    while ((pAttr = GetNextAttribute(pIter)) != 0)
      {
	  if (pAttr->GetAttrType() == eAttrType)
	      return pAttr;
      }
    return 0;
}

/** \brief checks if the parameter of IS attributes are declared somewhere
 *  \param pParameter this parameter's attributes should be checked
 *  \param nAttribute the attribute to check
 *  \param sAttribute a string describing the attribute
 *  \return true if no errors
 */
bool CFEOperation::CheckAttributeParameters(CFETypedDeclarator *pParameter, ATTR_TYPE nAttribute, const char* sAttribute)
{
    assert(pParameter);
    VectorElement *pIter = pParameter->GetFirstDeclarator();
	CFEDeclarator *pDecl = pParameter->GetNextDeclarator(pIter);
	assert(pDecl);
	CFEFile *pRoot = GetRoot();
	assert(pRoot);
	CFEAttribute *pAttr;
	if ((pAttr = pParameter->FindAttribute(nAttribute)) != 0)
	{
	    if (!pAttr->IsKindOf(RUNTIME_CLASS(CFEIsAttribute)))
		    return true;
        CFEIsAttribute *pIsAttr = (CFEIsAttribute*)pAttr;
		// check if it has a declarator
		VectorElement *pIAttr = pIsAttr->GetFirstAttrParameter();
		CFEDeclarator *pAttrParam;
		while ((pAttrParam = pIsAttr->GetNextAttrParameter(pIAttr)) != 0)
		{
			// check if parameter exists
			if (FindParameter(pAttrParam->GetName()))
				continue;
			// check if it is a const
			if (pRoot->FindConstDeclarator(pAttrParam->GetName()))
				continue;
			// nothing found, assume its wrongly used
			CCompiler::GccError(this, 0, "The argument \"%s\" of attribute %s for parameter %s is not declared as a parameter or constant.\n",
				(const char*)pAttrParam->GetName(), sAttribute, (const char*)pDecl->GetName());
			return false;
		}
	}
	return true;
}


/** \brief checks the consitency of the operation
 *  \return true if this operation is consistent, false if not
 *
 * This function checks whether or not the operation's syntax and grammar have been parsed
 * correctly. It return false if there inconsitencies.
 *
 * It first performs some integrity checks and returns an error if these fail.
 *
 * This function then replaces some types with a simpler equivalent to ease the
 * latter implementations of the back-end (REFSTRING, STRING, WSTRING). These
 * replacements are usually done when performing the consistency check for each
 * parameter.
 *
 * Currently this function tests (and corrects the following conditions):
 * - if the operation has both direction specified they are eliminated
 * - does the operation has a directional IN attribute it may not have any OUT parameters
 * - the same as above, only the directions exchanged
 * - if the parameter have no directional attribute set, it is replaced with the standard
 *   attribute ATTR_IN
 * - do not allow refstring as return type
 * - if an in-parameter is a struct or union type it is referenced
 * - parameter names may not appear mutltiple times
 *
 * The following functionality is obsolete
 * - does the operation have any OUT flexpages an additional parameter is added
 *   -> because we have the receive fpage parameter inside the CORBA_Environment
 */
bool CFEOperation::CheckConsistency()
{
    ///////////////////////////////////////////////////////////////
    // if both directional parameters have been set for this operation this is as
    // good as none set -> remove them
    if ((FindAttribute(ATTR_IN)) && (FindAttribute(ATTR_OUT)))
    {
        RemoveAttribute(ATTR_IN);
        RemoveAttribute(ATTR_OUT);
    }
    //////////////////////////////////////////////////////////////
    // if function is set to one way (ATTR_IN) it cannot have a return value
    if (FindAttribute(ATTR_IN))
    {
        if (GetReturnType()->GetType() != TYPE_VOID)
        {
            CCompiler::GccError(this, 0, "A function with attribute [in] cannot have a return type (%s).",
                                (const char *) GetName());
            return false;
        }
    }
    CFETypedDeclarator *pParam = 0;
    VectorElement *pIter = 0;
    ///////////////////////////////////////////////////////////////
    // check directional attribute of operation and directional attributes of parameter
    // we do this for the message passing stuff
    if (FindAttribute(ATTR_IN))
    {
        // check all parameters: if we find ATTR_OUT print error
        // if we find ATTR_NONE replace it with ATTR_IN
        pIter = GetFirstParameter();
        while ((pParam = GetNextParameter(pIter)) != 0)
        {
            if (pParam->FindAttribute(ATTR_OUT))
            {
                CCompiler::GccError(this, 0, "Operation %s cannot have [out] parameter and operation attribute [in]",
                                    (const char *) GetName());
                return false;
            }
            if (pParam->FindAttribute(ATTR_NONE))
            {
                pParam->RemoveAttribute(ATTR_NONE);
                pParam->AddAttribute(new CFEAttribute(ATTR_IN));
            }
        }
    }
    if (FindAttribute(ATTR_OUT))
    {
        // check all parameters: if we find ATTR_NONE replace it with ATTR_OUT
        // if we find ATTR_IN print error
        pIter = GetFirstParameter();
        while ((pParam = GetNextParameter(pIter)) != 0)
        {
            if (pParam->FindAttribute(ATTR_IN))
            {
                CCompiler::GccError(this, 0, "Operation %s cannot have [in] parameter and operation attribute [out]",
                                    (const char *) GetName());
                return false;
            }
            if (pParam->FindAttribute(ATTR_NONE))
            {
                pParam->RemoveAttribute(ATTR_NONE);
                pParam->AddAttribute(new CFEAttribute(ATTR_OUT));
            }
        }
    }
    ///////////////////////////////////////////////////////////////
    // check if the parameters have their directional parameters set
    // if none replace it with standard attribute ATTR_IN
    // this cannot be done in Typed decl, because the directional attribute
    // only applies to parameters
    pIter = GetFirstParameter();
    while ((pParam = GetNextParameter(pIter)) != 0)
    {
        if (pParam->FindAttribute(ATTR_NONE))
        {
            pParam->RemoveAttribute(ATTR_NONE);
            pParam->AddAttribute(new CFEAttribute(ATTR_IN));
        }
    }
    ////////////////////////////////////////////////////////////////
	// check if [out] parameters are referenced
	pIter = GetFirstParameter();
	while ((pParam = GetNextParameter(pIter)) != 0)
	{
	    if (pParam->FindAttribute(ATTR_OUT))
		{
		    // get declarator
			VectorElement *pI = pParam->GetFirstDeclarator();
			CFEDeclarator *pD = pParam->GetNextDeclarator(pI);
			if (pD && !pD->IsReference())
			{
			    CCompiler::GccError(pParam, 0, "[out] parameter (%s) must be reference", (const char*)pD->GetName());
				return false;
			}
		}
	}
    ////////////////////////////////////////////////////////////////
    // check if return value is of type refstring
    if (m_pReturnType->GetType() == TYPE_REFSTRING)
    {
        CCompiler::GccError(this, 0, "Type \"refstring\" is not a valid return type of a function (%s).",
                            (const char *) GetName());
        return false;
    }
	///////////////////////////////////////////////////////////
	// check if all identifiers in _is attributes are defined
	// as parameters.
	pIter = GetFirstParameter();
	while ((pParam = GetNextParameter(pIter)) != 0)
	{
	    if (!CheckAttributeParameters(pParam, ATTR_SIZE_IS, "[size_is]"))
		    return false;
	    if (!CheckAttributeParameters(pParam, ATTR_LENGTH_IS, "[length_is]"))
		    return false;
	    if (!CheckAttributeParameters(pParam, ATTR_MAX_IS, "[max_is]"))
		    return false;
	}
    ////////////////////////////////////////////////////////////////
    // check the parameters
    pIter = GetFirstParameter();
    while ((pParam = GetNextParameter(pIter)) != 0)
    {
        if (!(pParam->CheckConsistency()))
            return false;
    }

    ////////////////////////////////////////////////////////////
    // check for double naming
    pIter = GetFirstParameter();
    while ((pParam = GetNextParameter(pIter)) != 0)
    {
        // get name of first param
        VectorElement *pIterD = pParam->GetFirstDeclarator();
        CFEDeclarator *pDecl = pParam->GetNextDeclarator(pIterD);
        String sName = pDecl->GetName();
        // now search if this name occures somewhere else
        VectorElement *pIter2 = pIter;	// start at current position
        CFETypedDeclarator *pParam2;
        while ((pParam2 = GetNextParameter(pIter2)) != 0)
        {
            if (pParam != pParam2)
            {
                // get name of second param
                pIterD = pParam2->GetFirstDeclarator();
                pDecl = pParam2->GetNextDeclarator(pIterD);
                if (sName == pDecl->GetName())
                {
                    CCompiler::GccError(this, 0, "The operation %s has the parameter %s defined more than once.\n\0",
                                        (const char *) GetName(), (const char *) sName);
                    return false;
                }
            }
        }
    }

    return true;
}

/**
 *	\brief append new parameter to collection
 *	\param pParam the new parameter
 *	\param bStart true if this parameter should be added to the start of the parameter list
 *
 * Use this method to append a new parameter to this operation.
 */
void CFEOperation::AddParameter(CFETypedDeclarator * pParam, bool bStart)
{
    if (!m_pOpParameters)
	m_pOpParameters = new Vector(RUNTIME_CLASS(CFETypedDeclarator), 1, pParam);
    else
      {
	  if (bStart)
	      m_pOpParameters->AddHead(pParam);
	  else
	      m_pOpParameters->Add(pParam);
      }
    // set parent relationship
    pParam->SetParent(this);
}

/**
 *	\brief remove an attribute of the specified type from the attribute array
 *	\param eAttrType the type of the attribute to remove
 *
 * This function does also delete the attribute object. Clone th object if you wish
 * to use it past this function.
 */
void CFEOperation::RemoveAttribute(ATTR_TYPE eAttrType)
{
    if (!m_pOpAttributes)
	return;
    CFEAttribute *pAttr;
    VectorElement *pIter = GetFirstAttribute();
    while ((pAttr = GetNextAttribute(pIter)) != 0)
      {
	  if (pAttr->GetAttrType() == eAttrType)
	    {
		m_pOpAttributes->Remove(pAttr);
		delete pAttr;
		// erase messes up the iterator: reset it
		pIter = GetFirstAttribute();
	    }
      }
}

/**
 *	\brief add a new attribute to this operation
 *	\param pNewAttr the new attribute
 */
void CFEOperation::AddAttribute(CFEAttribute * pNewAttr)
{
    if (m_pOpAttributes)
      {
	  m_pOpAttributes->Add(pNewAttr);
	  pNewAttr->SetParent(this);
      }
}

/**
 *	\brief use this function to remove a specific parameter
 *	\param pParam the parameter to be remove
 *
 * This function also deletes the parameter. If you wish to use the
 * parameter past this function Clone it before the call to this function.
 * The erase operation messes up all iterators for the parameter vector. Reset
 * them after you called this function. This function removes all appearances
 * of the pointer pParam from the vector.
 */
void CFEOperation::RemoveParameter(CFETypedDeclarator * pParam)
{
    if (!m_pOpParameters)
        return;
    m_pOpParameters->Remove(pParam);
    delete pParam;
}

/** creates a copy of this object
 *	\return a copy of this object
 */
CObject *CFEOperation::Clone()
{
    return new CFEOperation(*this);
}

/** for debugging purposes only */
void CFEOperation::Dump()
{
    TRACE("Dump: CFEOperation (%s) has %d parameters\n", (const char *) GetName(), (m_pOpParameters!=0)?(m_pOpParameters->GetSize()):0);
}

/** serialize this object
 *	\param pFile the file to serialize from/to
 */
void CFEOperation::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        pFile->PrintIndent("<function>\n");
        pFile->IncIndent();
        pFile->PrintIndent("<name>%s</name>\n", (const char *) GetName());
        // write attributes
        VectorElement *pIter = GetFirstAttribute();
        CFEBase *pElement;
        while ((pElement = GetNextAttribute(pIter)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // write return type
        if (GetReturnType())
            GetReturnType()->Serialize(pFile);
        // write parameters
        pIter = GetFirstParameter();
        while ((pElement = GetNextParameter(pIter)) != 0)
        {
            pFile->PrintIndent("<parameter>\n");
            pFile->IncIndent();
            pElement->Serialize(pFile);
            pFile->DecIndent();
            pFile->PrintIndent("</parameter>\n");
        }
        // write exceptions
        pIter = GetFirstRaisesDeclarator();
        while ((pElement = GetNextRaisesDeclarator(pIter)) != 0)
        {
            pFile->PrintIndent("<raises>%s</raises>\n", (const char *) ((CFEIdentifier *)pElement)->GetName());
        }
        pFile->DecIndent();
        pFile->PrintIndent("</function>\n");
    }
}
