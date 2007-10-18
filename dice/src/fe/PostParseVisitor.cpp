/**
 *  \file    dice/src/fe/PostParseVisitor.cpp
 *  \brief   contains the implementation of the class CPostParseVisitor
 *
 *  \date    09/15/2006
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006-2007
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

#include "PostParseVisitor.h"
#include "FEInterface.h"
#include "FELibrary.h"
#include "FEFile.h"
#include "FEIdentifier.h"
#include "FEVersionAttribute.h"
#include "FESimpleType.h"
#include "FEOperation.h"
#include "FETypedDeclarator.h"
#include "FEArrayDeclarator.h"
#include "FETypeAttribute.h"
#include "FEIntAttribute.h"
#include "FEIsAttribute.h"
#include "FEPrimaryExpression.h"
#include "Compiler.h"
#include "Messages.h"
#include "Error.h"
#include <cassert>

/** empty destructor */
CPostParseVisitor::~CPostParseVisitor()
{ }

/** \brief post parse processing for interface
 *  \param interface the interface to post process
 */
void CPostParseVisitor::Visit(CFEInterface& interface)
{
	// set base interfaces
	CFEFile *pRoot = interface.GetRoot();
	assert(pRoot);
	vector<CFEIdentifier*>::iterator iterBIN;
	for (iterBIN = interface.m_BaseInterfaceNames.begin();
		iterBIN != interface.m_BaseInterfaceNames.end();
		iterBIN++)
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
			"%s: checking base interface \"%s\" of interface \"%s\"\n", __func__,
			(*iterBIN)->GetName().c_str(), interface.GetName().c_str());
		CFEInterface *pBase = 0;
		if ((*iterBIN)->GetName().find("::") != string::npos)
			pBase = pRoot->FindInterface((*iterBIN)->GetName());
		else
		{
			CFELibrary *pFELibrary = interface.GetSpecificParent<CFELibrary>();
			// should be in same library
			if (pFELibrary)
				pBase = pFELibrary->FindInterface((*iterBIN)->GetName());
			else // no library
				pBase = pRoot->FindInterface((*iterBIN)->GetName());
		}

		if (pBase)
		{
			// check if interface is already referenced
			if (!interface.FindBaseInterface((*iterBIN)->GetName()))
				interface.AddBaseInterface(pBase);
		}
		else
		{
			CMessages::GccError(&interface, "Base interface %s not declared.\n",
				(*iterBIN)->GetName().c_str());
			throw error::postparse_error();
		}
	}

	CVisitor::Visit(interface);
}

/** \brief post parse processing of operation
 *  \param operation the operation to process
 */
void CPostParseVisitor::Visit(CFEOperation& operation)
{
	// if function has both directional attributes, delete them
	CFEAttribute* pAttrIn = operation.m_Attributes.Find(ATTR_IN);
	CFEAttribute* pAttrOut = operation.m_Attributes.Find(ATTR_OUT);
	if (pAttrIn && pAttrOut)
	{
		operation.m_Attributes.Remove(pAttrIn);
		operation.m_Attributes.Remove(pAttrOut);
		// already return here, because later checks rely on at least one of
		// the directional attributes
		CVisitor::Visit(operation);
		return;
	}
	// return if neither directional attribute is set
	if (!pAttrIn && !pAttrOut)
	{
		// check if parameters have no directional attributes. If so, add at
		// least the IN attribute.
		vector<CFETypedDeclarator*>::iterator i;
		for (i = operation.m_Parameters.begin();
			i != operation.m_Parameters.end();
			i++)
		{
			if (!(*i)->m_Attributes.Find(ATTR_IN) &&
				!(*i)->m_Attributes.Find(ATTR_OUT))
				(*i)->m_Attributes.Add(new CFEAttribute(ATTR_IN));
		}
		CVisitor::Visit(operation);
		return;
	}
	// if the function has a directional attribute, then apply it to all
	// parameters. The consistency check will then find any mismaches.
	ATTR_TYPE nAttr;
	if (pAttrIn)
		nAttr = ATTR_IN;
	else
		nAttr = ATTR_OUT;
	// iterate parameters and add directional attributes. If parameters have
	// ATTR_NONE set, remove it.
	vector<CFETypedDeclarator*>::iterator i;
	for (i = operation.m_Parameters.begin();
		i != operation.m_Parameters.end();
		i++)
	{
		(*i)->m_Attributes.Add(new CFEAttribute(nAttr));
		CFEAttribute *pAttr = (*i)->m_Attributes.Find(ATTR_NONE);
		(*i)->m_Attributes.Remove(pAttr);
	}

	CVisitor::Visit(operation);
}

/** \brief post parse processing for typed declarator
 *  \param typeddecl the typed declarator to process
 */
void CPostParseVisitor::Visit(CFETypedDeclarator& typeddecl)
{
	CheckStrings(typeddecl);
	CheckVoidArray(typeddecl);
	CheckInConstructed(typeddecl);
	CheckIsArray(typeddecl);
}

/** \brief checks and transforms typeddecl such that strings are represented uniformally
 *  \param typeddecl the typed declarator to check
 */
void
CPostParseVisitor::CheckStrings(CFETypedDeclarator& typeddecl)
{
	// replace refstrings
	CFETypeSpec *pType = typeddecl.GetType();
	if (pType->GetType() == TYPE_REFSTRING)
	{
		// replace type REFSTRING with type CHAR_ASTERISK
		CFETypeSpec *pOldType =
			typeddecl.ReplaceType(new CFESimpleType(TYPE_CHAR_ASTERISK));
		delete pOldType;
		// add attribute ref
		if (!typeddecl.m_Attributes.Find(ATTR_REF))
			typeddecl.m_Attributes.Add(new CFEAttribute(ATTR_REF));
		if (!typeddecl.m_Attributes.Find(ATTR_STRING))
			typeddecl.m_Attributes.Add(new CFEAttribute(ATTR_STRING));
		pType = typeddecl.GetType();
	}
	// replace string and wstring
	if ((pType->GetType() == TYPE_STRING) ||
		(pType->GetType() == TYPE_WSTRING))
	{
		// replace type
		if (pType->GetType() == TYPE_STRING)
		{
			CFETypeSpec *pOldType = typeddecl.ReplaceType(new CFESimpleType(TYPE_CHAR));
			delete pOldType;
		}
		else
		{
			CFETypeSpec *pOldType = typeddecl.ReplaceType(new CFESimpleType(TYPE_WCHAR));
			delete pOldType;
		}
		pType = typeddecl.GetType();
		// set string attribute
		if (!typeddecl.m_Attributes.Find(ATTR_STRING))
			typeddecl.m_Attributes.Add(new CFEAttribute(ATTR_STRING));
		// now we search all declarators. We make from any more than 1 star
		// only one star (a string can have only one star (per definition)
		// set star with declarator (can only be one or array dimension)
		vector<CFEDeclarator*>::iterator iterD;
		for (iterD = typeddecl.m_Declarators.begin();
			iterD != typeddecl.m_Declarators.end(); )
		{
			CFEDeclarator *pDecl = *iterD;
			CFEArrayDeclarator *pArrayDecl =
				dynamic_cast<CFEArrayDeclarator*>(pDecl);
			if (pArrayDecl)
			{
				// we also check uninitialized array dimension (create error
				// in C). We replace these with stars.
				for (unsigned int i = 0;
					i < pArrayDecl->GetDimensionCount();
					i++)
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
					CFEDeclarator *pNewDecl =
						new CFEDeclarator(DECL_IDENTIFIER,
							pArrayDecl->GetName(), pArrayDecl->GetStars());
					typeddecl.m_Declarators.Remove(pArrayDecl);
					delete pArrayDecl;
					typeddecl.m_Declarators.Add(pNewDecl);
					// because current pDecl is deleted we start all over again
					iterD = typeddecl.m_Declarators.begin();
					continue;
				}
				else
					iterD++;
			}
			else
				iterD++;
			// we check if there are too many stars
			int nMaxStars = 1;
			if (typeddecl.m_Attributes.Find(ATTR_OUT))
				nMaxStars = 2;
			// and fix it.
			pDecl->SetStars(nMaxStars);
		}
	}
	// if we have an unsigned CHAR_ASTERISK, then we replace the
	// CHAR_ASTERISK with CHAR and add the star to the first declarator
	if ((pType->GetType() == TYPE_CHAR_ASTERISK) &&
		((CFESimpleType*)pType)->IsUnsigned())
	{
		// replace the type
		CFETypeSpec *pOldType = typeddecl.ReplaceType(new CFESimpleType(TYPE_CHAR, true));
		delete pOldType;
		pType = typeddecl.GetType();
		// set the star of the first declarator
		CFEDeclarator *pDecl = typeddecl.m_Declarators.First();
		if (pDecl)
			pDecl->SetStars(pDecl->GetStars()+1);
	}
	// make from CHAR with one star an CHAR_ASTERISK
	// only if string attribute is given
	if ((pType->GetType() == TYPE_CHAR) &&
		typeddecl.m_Attributes.Find(ATTR_STRING))
	{
		// first check if _all_ declarators have at least one star
		bool bHaveStar = true;
		vector<CFEDeclarator*>::iterator iterD;
		for (iterD = typeddecl.m_Declarators.begin();
			iterD != typeddecl.m_Declarators.end();
			iterD++)
		{
			// if at least one does _not_ have a star
			if ((*iterD)->GetStars() == 0)
				bHaveStar = false;
		}
		// only if all decls have a star, we replace this type
		if (bHaveStar)
		{
			// replace type
			CFETypeSpec *pOldType =
				typeddecl.ReplaceType(new CFESimpleType(TYPE_CHAR_ASTERISK));
			delete pOldType;
			pType = typeddecl.GetType();
			// do not need to add the string attribute since this has
			// to be set to get here

			// get all declarator and reduce their declarators by one
			// if it is an [out] string, set it to two
			for (iterD = typeddecl.m_Declarators.begin();
				iterD != typeddecl.m_Declarators.end();
				iterD++)
			{
				// first add the pointer from char*
				(*iterD)->SetStars((*iterD)->GetStars()-1);
			}
		}
	}
	// CHAR_ASTERISK with size/length attribute should be CHAR and decl with
	// star
	if ((pType->GetType() == TYPE_CHAR_ASTERISK) &&
		!typeddecl.m_Attributes.Find(ATTR_STRING))
	{
		// char* var implies [string] attribute
		// only if no size or length attribute
		// FIXME: what about '[out] char*' which only wants to transmit one
		// character?
		if (!typeddecl.m_Attributes.Find(ATTR_SIZE_IS) &&
			!typeddecl.m_Attributes.Find(ATTR_LENGTH_IS))
		{
			typeddecl.m_Attributes.Add(new CFEAttribute(ATTR_STRING));
		}
		else
		{
			// if it is a char* and there is no string attribute,
			// but a size or length attribute, we convert it into
			// a char and add a star to the declarator
			CFETypeSpec *pOldType = typeddecl.ReplaceType(new CFESimpleType(TYPE_CHAR));
			delete pOldType;
			pType = typeddecl.GetType();
			// set star of declarators
			vector<CFEDeclarator*>::iterator iterD;
			for (iterD = typeddecl.m_Declarators.begin();
				iterD != typeddecl.m_Declarators.end();
				iterD++)
			{
				CFEDeclarator *pArray = (CFEDeclarator*)0;
				// if declarator is simple, we make it an array now
				if (((*iterD)->GetType() != DECL_ARRAY) &&
					((*iterD)->GetType() != DECL_ENUM))
				{
					pArray = new CFEArrayDeclarator(*iterD);
					CFEDeclarator *pDecl = *iterD;
					typeddecl.m_Declarators.Remove(pDecl);
					delete pDecl;
					typeddecl.m_Declarators.Add(pArray);
					// because pDecl has been removed, reset iterator
					iterD = typeddecl.m_Declarators.begin();
				}
				// first add the pointer from char*
				if (pArray)
					pArray->SetStars(pArray->GetStars()+1);
			}
		}
	}
}

/** \brief check arrays of type void*
 *  \param typeddecl the typed declarator to check
 */
void
CPostParseVisitor::CheckVoidArray(CFETypedDeclarator& typeddecl)
{
	// check for void* type and size parameter
	// then this is a array. To be able to transmit it
	// we add the transmit-as attribute with a character
	// type
	CFETypeSpec *pType = typeddecl.GetType();
	if ((pType->GetType() == TYPE_VOID_ASTERISK) &&
		(typeddecl.m_Attributes.Find(ATTR_SIZE_IS) ||
		 typeddecl.m_Attributes.Find(ATTR_LENGTH_IS)))
	{
		if (!typeddecl.m_Attributes.Find(ATTR_TRANSMIT_AS))
		{
			CFETypeSpec *pAttrType = new CFESimpleType(TYPE_CHAR);
			CFEAttribute *pAttr = new CFETypeAttribute(ATTR_TRANSMIT_AS,
				pAttrType);
			pAttrType->SetParent(pAttr);
			typeddecl.m_Attributes.Add(pAttr);
		}
		// to handle the parameter correctly we have to move the
		// '*' from 'void*' to the declarators
		CFETypeSpec *pOldType = typeddecl.ReplaceType(new CFESimpleType(TYPE_VOID));
		delete pOldType;
		pType = typeddecl.GetType();
		// set declarator stars
		vector<CFEDeclarator*>::iterator iterD;
		for (iterD = typeddecl.m_Declarators.begin();
			iterD != typeddecl.m_Declarators.end();
			iterD++)
		{
			CFEDeclarator *pArray = (CFEDeclarator*)0;
			// if declarator is simple, we make it an array now
			if (((*iterD)->GetType() != DECL_ARRAY) &&
				((*iterD)->GetType() != DECL_ENUM))
			{
				pArray = new CFEArrayDeclarator(*iterD);
				CFEDeclarator *pDecl = *iterD;
				typeddecl.m_Declarators.Remove(pDecl);
				delete pDecl;
				typeddecl.m_Declarators.Add(pArray);
				// because pDecl has been removed, reset iterator
				iterD = typeddecl.m_Declarators.begin();
			}
			// first add the pointer from char*
			if (pArray)
				pArray->SetStars(pArray->GetStars()+1);
		}
	}
}

/** \brief check for constructed type and IN attribute
 *  \param typeddecl the typed declarator to check
 */
void
CPostParseVisitor::CheckInConstructed(CFETypedDeclarator& typeddecl)
{
	// check for in-parameter and struct or union type
	// FIXME is this really what we want? (shouldn't size of struct matter?)
	if (!typeddecl.m_Attributes.Find(ATTR_IN))
		return;

	// make structs reference parameters
	// except they are pointers already.
	if (typeddecl.GetType()->IsConstructedType() &&
		!typeddecl.GetType()->IsPointerType())
	{
		vector<CFEDeclarator*>::iterator iterD;
		for (iterD = typeddecl.m_Declarators.begin();
			iterD != typeddecl.m_Declarators.end();
			iterD++)
		{
			if ((*iterD)->GetStars() > 0)
				continue;
			CFEArrayDeclarator *pDecl = dynamic_cast<CFEArrayDeclarator*>(*iterD);
			if (pDecl && pDecl->GetDimensionCount() > 0)
				continue;
			// stars = 0 and no array bounds
			(*iterD)->SetStars(1);
		}
	}
}

/** \brief check whether IS attributes contain constants and set them as array bound
 *  \param typeddecl the typed declarator to check
 */
void
CPostParseVisitor::CheckIsArray(CFETypedDeclarator& typeddecl)
{
	// if we have a max_is or size_is attribute which is CFEIntAttribute, we
	// set the respective unbound array dimension to this value
	// check for size_is/length_is/max_is attributes
	// if size_is contains variable, check for max_is with const value
	CFEAttribute *pSizeAttrib = typeddecl.m_Attributes.Find(ATTR_SIZE_IS);
	if (!pSizeAttrib)
	{
		pSizeAttrib = typeddecl.m_Attributes.Find(ATTR_LENGTH_IS);
		if (!pSizeAttrib)
			pSizeAttrib = typeddecl.m_Attributes.Find(ATTR_MAX_IS);
	}
	else
	{
		if (!dynamic_cast<CFEIntAttribute*>(pSizeAttrib))
			pSizeAttrib = typeddecl.m_Attributes.Find(ATTR_MAX_IS);
	}
	if (!pSizeAttrib)
		return;

	bool bRemoveSize = false;
	vector<CFEDeclarator*>::iterator iterD;
	for (iterD = typeddecl.m_Declarators.begin();
		iterD != typeddecl.m_Declarators.end();
		iterD++)
	{
		CFEArrayDeclarator *pArray =
			dynamic_cast<CFEArrayDeclarator*>(*iterD);
		if (!pArray)
			continue;

		// find the first unbound array dimension
		int nMax = pArray->GetDimensionCount();
		for (int i = nMax - 1; i >= 0; i--)
		{
			CFEExpression *pUpper = pArray->GetUpperBound(i);
			if (pUpper)
				continue;

			// create new expression with value of size_is/max_is
			// if the value is declarator -> ignore it
			if (dynamic_cast<CFEIntAttribute*>(pSizeAttrib))
			{
				// use ReplaceUpperBound to set new expression.
				CFEExpression *pNewBound =
					new CFEPrimaryExpression(EXPR_INT,
						(long int) ((CFEIntAttribute *)
							pSizeAttrib)->GetIntValue());
				pArray->ReplaceUpperBound(i, pNewBound);
				// remove attribute
				bRemoveSize = true;
			}
			// test for constant in size attribute
			if (dynamic_cast<CFEIsAttribute*>(pSizeAttrib))
			{
				// test for parameter
				CFEDeclarator *pDAttr =
					((CFEIsAttribute*)pSizeAttrib)->
					m_AttrParameters.First();
				if (pDAttr)
				{
					// find constant
					CFEFile *pFERoot = typeddecl.GetRoot();
					assert(pFERoot);
					CFEConstDeclarator *pConstant =
						pFERoot->FindConstDeclarator(pDAttr->GetName());
					if (pConstant && pConstant->GetValue())
					{
						// replace bounds
						CFEExpression *pNewBound =
							new CFEPrimaryExpression(EXPR_INT,
								(long)pConstant->GetValue()->GetIntValue());
						pArray->ReplaceUpperBound(i, pNewBound);
						// do not delete the size attribute
					}
				}
			}
		}
	}
	if (bRemoveSize)
	{
		typeddecl.m_Attributes.Remove(pSizeAttrib);
		delete pSizeAttrib;
	}
}

