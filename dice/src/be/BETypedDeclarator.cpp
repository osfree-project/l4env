/**
 *  \file    dice/src/be/BETypedDeclarator.cpp
 *  \brief   contains the implementation of the class CBETypedDeclarator
 *
 *  \date    01/18/2002
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

#include "BETypedDeclarator.h"
#include "BEContext.h"
#include "BEFile.h"
#include "BEAttribute.h"
#include "BEType.h"
#include "BETypedef.h"
#include "BEStructType.h"
#include "BEUnionType.h"
#include "BEUnionCase.h"
#include "BEUserDefinedType.h"
#include "BEDeclarator.h"
#include "BERoot.h"
#include "BEClient.h"
#include "BEFunction.h"
#include "BEComponentFunction.h"
#include "BEExpression.h"
#include "BEConstant.h"
#include "BESizes.h"
#include "BEMsgBufferType.h"
#include "BEMsgBuffer.h"
#include "BEClass.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEDeclarator.h"
#include "fe/FETypeSpec.h"
#include "TypeSpec-Type.h"
#include "fe/FEIsAttribute.h"
#include "fe/FEExpression.h"
#include "Compiler.h"
#include "Messages.h"
#include <cassert>
#include <algorithm>

CBETypedDeclarator::CBETypedDeclarator()
: CBEObject(),
  m_pType(NULL),
  m_sDefaultInitString(),
  m_Attributes(0, this),
  m_Declarators(0, this)
{
    m_mProperties.clear();
}

CBETypedDeclarator::CBETypedDeclarator(CBETypedDeclarator & src)
: CBEObject(src),
  m_mProperties(src.m_mProperties),
  m_Attributes(src.m_Attributes),
  m_Declarators(src.m_Declarators)
{
    m_Attributes.Adopt(this);
    m_Declarators.Adopt(this);
    CLONE_MEM(CBEType, m_pType);
    m_sDefaultInitString = src.m_sDefaultInitString;
}

/** \brief destructor of this instance */
CBETypedDeclarator::~CBETypedDeclarator()
{
    m_mProperties.clear();
    if (m_pType)
        delete m_pType;
}

/** \brief creates a new instance of this class 
 *  \return a reference to the copy
 */
CObject *CBETypedDeclarator::Clone()
{ 
    return new CBETypedDeclarator(*this); 
}

/** \brief writes the declaration of an variable
 *  \param pFile the file to write to
 *
 * A typed declarator, such as a parameter, contain a type, name(s) and
 * optional attributes.
 */
void
CBETypedDeclarator::WriteDeclaration(CBEFile * pFile)
{
    if (!pFile->IsOpen())
        return;

    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s called for %s\n",
	__func__, m_Declarators.First()->GetName().c_str());
    
    WriteAttributes(pFile);
    WriteType(pFile);
    *pFile << " ";
    WriteDeclarators(pFile);
    WriteProperties(pFile);
    
    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s returned\n", __func__);
}

/** \brief writes the code to set a variable to a zero value
 *  \param pFile the file to write to
 */
void
CBETypedDeclarator::WriteSetZero(CBEFile* pFile)
{
    CBEType *pType = GetType();
    if (pType->IsVoid())
        return;

    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s called\n", __func__);
    
    vector<CBEDeclarator*>::iterator iterD;
    for (iterD = m_Declarators.begin();
	 iterD != m_Declarators.end();
	 iterD++)
    {
	*pFile << "\t";
        (*iterD)->WriteDeclaration(pFile);
        if (pType->DoWriteZeroInit())
        {
	    *pFile << " = ";
            pType->WriteZeroInit(pFile);
        }
	*pFile << ";\n";
    }

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s returns\n", __func__);
}

/** \brief writes the size of a variable
 *  \param pFile the file to write to
 *  \param pStack the current declarator stack text of the write
 *  \param pUsingFunc the function to use as reference for members
 *
 * If this is variable sized, there has to be some attribute, defining the
 * actual size of this parameter at run-time. So what we do is to search for
 * size_is, length_is or max_is attributes.
 *
 * This is different if this is a string. So if it is, we simply use the
 * strlen function.
 */
void
CBETypedDeclarator::WriteGetSize(CBEFile * pFile,
    CDeclStack* pStack,
    CBEFunction *pUsingFunc)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s called for %s\n",
	__func__, m_Declarators.First()->GetName().c_str());
    
    CBEType *pType = GetType();
    CBEAttribute *pAttr = 0;
    if ((pAttr = m_Attributes.Find(ATTR_SIZE_IS)) == 0)
    {
        if ((pAttr = m_Attributes.Find(ATTR_LENGTH_IS)) == 0)
        {
	    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
		"CBETypedDeclarator::%s no size and length\n", __func__);
            // we prefer the actual size of a string to its max-size,
            // so we first test for the string attribute
            // might be string
            if (m_Attributes.Find(ATTR_STRING))
            {
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
		    "CBETypedDeclarator::%s string attr\n", __func__);
                // get declarator
                CBEDeclarator *pDecl = m_Declarators.First();
                if (pDecl)
                {
		    *pFile << "strlen(";
		    if (!pStack)
		    {
			CDeclStack vStack;
			vStack.push_back(pDecl);
			CDeclaratorStackLocation::Write(pFile, &vStack, true);
		    }
		    else
			CDeclaratorStackLocation::Write(pFile, pStack, true);
                    // restore old number of stars
		    *pFile << ")";
                }
                // wrote size parameter
		CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		    "CBETypedDeclarator::%s write strlen\n", __func__);
                return;
            }
            // check max-is attribute
            if ((pAttr = m_Attributes.Find(ATTR_MAX_IS)) == 0)
            {
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
		    "CBETypedDeclarator::%s no max attr\n", __func__);
                // this only happends, when this is variable sized
                // because of its variable sized type
                // => we have to animate the type to write the size
		CDeclStack vStack;
                if (!pStack)
                    pStack = &vStack;
                vector<CBEDeclarator*>::iterator iterD;
		for (iterD = m_Declarators.begin();
		     iterD != m_Declarators.end();
		     iterD++)
                {
                    pStack->push_back(*iterD);
                    pType->WriteGetSize(pFile, pStack, pUsingFunc);
                    pStack->pop_back();
                }

		CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
		    "CBETypedDeclarator::%s wrote type's size\n", 
		    __func__);
                return;
            }
        }
    }
    if (!pAttr)
    {
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
	    "CBETypedDeclarator::%s returns (no attr)\n", __func__);
        return;
    }
    if (pAttr->IsOfType(ATTR_CLASS_IS))
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	    "CBETypedDeclarator::%s an IS attr found\n", __func__);
	CDeclStack vStack;
	if (pStack)
	    vStack = *pStack;
	bool bFoundInStruct = false;
	CBETypedDeclarator *pSizeParameter = GetSizeVariable(pAttr, &vStack,
	    pUsingFunc, bFoundInStruct);

	if (bFoundInStruct)
	{
	    CDeclaratorStackLocation::Write(pFile, &vStack, false);
	    if (vStack.size() > 0)
		*pFile << ".";
	    // has only one declarator
    	    pSizeParameter->WriteDeclarators(pFile);

	    // done
	    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
		"CBETypedDeclarator::%s size in struct written\n", 
		__func__);
	    return;
	}
	
	if (pSizeParameter)
        {
	    // that one must be from the functions
	    CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
	    if (!pFunction && pUsingFunc)
		pFunction = pUsingFunc;
	    
	    // and now get original declarator, since size_is declarator
	    // might have different reference count...
    	    CBEDeclarator *pSizeName = pSizeParameter->m_Declarators.First();
	    if (pFunction->HasAdditionalReference(pSizeName))
		*pFile << "*";
	    // has only one declarator
	    pSizeParameter->WriteDeclarators(pFile);

	    // done
	    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
		"CBETypedDeclarator::%s size attr written\n", __func__);
	    return;
	}

	CBEConstant *pConstant = GetSizeConstant(pAttr);
	// now at least this one should have been found
	if (!pConstant)
	{
    	    CBEDeclarator *pSizeName = pAttr->m_Parameters.First();
	    CMessages::Warning(
	"Size attribute (%s) is neither parameter nor defined as constant.",
		pSizeName->GetName().c_str());
	}
	assert(pConstant);
	*pFile << pConstant->GetName();
    }
    else if (pAttr->IsOfType(ATTR_CLASS_INT))
    {
	*pFile << pAttr->GetIntValue();
    }
    else if (pAttr->IsOfType(ATTR_CLASS_STRING))
    {
	*pFile << pAttr->GetString();
    }

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s returns\n", __func__);
}

/** \brief writes the maximum size of a variable
 *  \param pFile the file to write to
 *  \param pStack the current declarator stack text of the write
 *  \param pUsingFunc the function to use as reference for members
 */
void
CBETypedDeclarator::WriteGetMaxSize(CBEFile * pFile,
    CDeclStack* pStack,
    CBEFunction *pUsingFunc)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s called for %s\n", __func__,
	m_Declarators.First()->GetName().c_str());
    
    CBEType *pType = GetTransmitType();
    CBEAttribute *pAttr = 0;

    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s called for %s\n", __func__,
	m_Declarators.First()->GetName().c_str());

    if ((pAttr = m_Attributes.Find(ATTR_MAX_IS)) == 0)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s no max_is\n", __func__);
	
	// get declarator
	CBEDeclarator *pDecl = m_Declarators.First();
	if (!pDecl)
	{
	    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
		"CBETypedDeclarator::%s returns (no decl)\n", __func__);
	    return;
	}
	// we prefer the actual size of a string to its max-size,
	// so we first test for the string attribute
	// might be string
	CBESizes *pSizes = CCompiler::GetSizes();
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
	"%s decl %s has string? %s, length? %s, size? %s, char type? %s, stars? %d\n",
	    __func__, pDecl->GetName().c_str(),
	    m_Attributes.Find(ATTR_STRING) ? "yes" : "no",
	    m_Attributes.Find(ATTR_LENGTH_IS) ? "yes" : "no",
	    m_Attributes.Find(ATTR_SIZE_IS) ? "yes" : "no",
	    pType->IsOfType(TYPE_CHAR) ? "yes" : "no",
	    pDecl->GetStars());
	if (m_Attributes.Find(ATTR_STRING) ||
	    ((m_Attributes.Find(ATTR_LENGTH_IS) ||
	      m_Attributes.Find(ATTR_SIZE_IS)) &&
	     pType->IsOfType(TYPE_CHAR) &&
	     pDecl->GetStars() > 0))
	{
	    int nMaxSize = pSizes->GetMaxSizeOfType(pType->GetFEType());
	    WarnNoMax(nMaxSize);
	    *pFile << nMaxSize;
	    // wrote string size parameter
	    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
		"CBETypedDeclarator::%s returns\n", __func__);
	    return;
	}

	// check if the declarator is an array declarator. If so, get its
	// bounds and write those (if exits)
	// 
	// check max size of decl -> that's the easiest way to find out if
	// there is a variable sized dimension
	int nMaxSize = pDecl->GetMaxSize();
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
	    "%s decl %s max size is %d and is array? %s\n", __func__,
	    pDecl->GetName().c_str(), nMaxSize, pDecl->IsArray() ? "yes" : "no");
	if (pDecl->IsArray() &&
	    nMaxSize > 0)
	{
	    *pFile << nMaxSize;
	    if (pType->GetSize() > 1)
	    {
		*pFile << "*sizeof";
		pType->WriteCast(pFile, false);
	    }
	    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBETypedDeclarator::%s returns\n", __func__);
	    return;
	}
	
	// if max-size is negative, and no array, then we check for size_is
	// attribute, which may indicate an array none-the-less
	if (!pDecl->IsArray() && 
	    nMaxSize < 0 &&
	    (m_Attributes.Find(ATTR_SIZE_IS) ||
	     m_Attributes.Find(ATTR_LENGTH_IS)))
	{
	    nMaxSize = pSizes->GetMaxSizeOfType(pType->GetFEType());
	    WarnNoMax(nMaxSize);
	    *pFile << nMaxSize;
	    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBETypedDeclarator::%s returns\n", __func__);
	    return;
	}
	
	// this only happends, when this is variable sized
	// because of its variable sized type
	// => we have to animate the type to write the size
	CDeclStack vStack;
	if (!pStack)
	    pStack = &vStack;
	vector<CBEDeclarator*>::iterator iterD;
	for (iterD = m_Declarators.begin();
	     iterD != m_Declarators.end();
	     iterD++)
	{
	    pStack->push_back(*iterD);
	    pType->WriteGetMaxSize(pFile, pStack, pUsingFunc);
	    pStack->pop_back();
	}

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBETypedDeclarator::%s returns\n", __func__);
	return;
    }
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s max_is found\n", __func__);
    if (pAttr->IsOfType(ATTR_CLASS_IS))
    {
	CDeclStack vStack;
	if (!pStack)
	    pStack = &vStack;
	bool bFoundInStruct = false;
	CBETypedDeclarator *pSizeParameter = GetSizeVariable(pAttr, pStack,
	    pUsingFunc, bFoundInStruct);

	if (bFoundInStruct)
	{
	    CDeclaratorStackLocation::Write(pFile, pStack, false);
	    if (pStack->size() > 0)
		*pFile << ".";
	    // has only one declarator
    	    pSizeParameter->WriteDeclarators(pFile);

	    // done
	    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBETypedDeclarator::%s returns\n", __func__);
	    return;
	}
	
	if (pSizeParameter)
        {
	    // that one must be from the functions
	    CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
	    if (!pFunction && pUsingFunc)
		pFunction = pUsingFunc;
	    
	    // and now get original declarator, since size_is declarator
	    // might have different reference count...
    	    CBEDeclarator *pSizeName = pSizeParameter->m_Declarators.First();
	    if (pFunction->HasAdditionalReference(pSizeName))
		*pFile << "*";
	    // has only one declarator
	    pSizeParameter->WriteDeclarators(pFile);

	    // done
	    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBETypedDeclarator::%s returns\n", __func__);
	    return;
	}

	CBEConstant *pConstant = GetSizeConstant(pAttr);
	// now at least this one should have been found
	if (!pConstant)
	{
    	    CBEDeclarator *pSizeName = pAttr->m_Parameters.First();
	    CMessages::Warning(
"Size attribute (%s) is neither parameter nor defined as constant.",
		pSizeName->GetName().c_str());
	}
	assert(pConstant);
	*pFile << pConstant->GetName();
    }
    else if (pAttr->IsOfType(ATTR_CLASS_INT))
    {
	*pFile << pAttr->GetIntValue();
    }
    else if (pAttr->IsOfType(ATTR_CLASS_STRING))
    {
	*pFile << pAttr->GetString();
    }

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s returns\n", __func__);
}

/** \brief get the size variable that matches the IS parameter of the attrib
 *  \param pIsAttribute the attribute
 *  \param pStack the stack of declarators if this one belongs to a struct
 *  \param pUsingFunc the function that might contain the local variable
 *  \param bFoundInStruct set to true if found in struct
 *  \return the size variable
 *
 * The size parameter has locality. Therefore we first check if this parameter
 * belongs to a struct or union. If so we check them for the size variable. If
 * we do not find the size variable there, we check the parent function. We
 * might notdirectly have a function if this is a member of a typedef struct
 * or union. Then we need the function parameter to check for a local variable
 * with the size name. If all that fails the size name might be a constant or
 * a fixed size expression (such as size is). Getting the constant is done
 * elsewhere.
 */
CBETypedDeclarator*
CBETypedDeclarator::GetSizeVariable(CBEAttribute *pIsAttribute,
    CDeclStack* pStack,
    CBEFunction *pUsingFunc,
    bool& bFoundInStruct)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s (%p, %p, %p, %s) called\n", __func__,
	pIsAttribute, pStack, pUsingFunc, 
	bFoundInStruct ? "true" : "false");
    
    // first: get the name
    CBEDeclarator *pSizeName = pIsAttribute->m_Parameters.First();
    assert(pSizeName);
    string sSizeName = pSizeName->GetName();

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s size param is %s\n", __func__,
	sSizeName.c_str());

    CBETypedDeclarator *pSizeParameter = 0;

    // now check for struct or union parents
    CBEStructType *pStruct = GetSpecificParent<CBEStructType>();
    CBEUnionType *pUnion = GetSpecificParent<CBEUnionType>();
    while (!pSizeParameter && (pStruct || pUnion))
    {
	// since we operate on a copy we might simply pop the end (the element
	// has not been copied, only the vector with the references)
	pStack->pop_back();
	
	if (pStruct && pUnion)
	{
	    // check if union is parent of struct
	    if (pStruct->IsParent(pUnion))
		pUnion = NULL;
	    // check if struct is parent of union
	    else if (pUnion->IsParent(pStruct))
		pStruct = NULL;
	}
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "try to find member %s in %s\n",
	    sSizeName.c_str(), (pStruct) ? "struct" : "union");

	if (pStruct)
	    pSizeParameter = pStruct->m_Members.Find(sSizeName);
	if (pUnion)
	    pSizeParameter = pUnion->m_UnionCases.Find(sSizeName);
	
	if (!pSizeParameter)
	{
	    if (pStruct)
	    {
		pStruct = pStruct->GetSpecificParent<CBEStructType>();
		pUnion = pStruct->GetSpecificParent<CBEUnionType>();
	    }
	    else if (pUnion)
	    {
		pStruct = pUnion->GetSpecificParent<CBEStructType>();
		pUnion = pUnion->GetSpecificParent<CBEUnionType>();
	    }
	}
    }
    // if found, return
    if (pSizeParameter)
    {
	bFoundInStruct = true;
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
	    "CBETypedDeclarator::%s returns %p (%s) (found in struct)\n",
	    __func__, pSizeParameter, 
	    pSizeParameter->m_Declarators.First()->GetName().c_str());
	return pSizeParameter;
    }

    // now try function
    CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
    if (!pFunction && pUsingFunc)
	pFunction = pUsingFunc;
    if (!pFunction)
    {
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBETypedDeclarator::%s returns 0 (no func)\n", __func__);
	return 0;
    }
    
    pSizeParameter = pFunction->FindParameter(sSizeName);
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s tried to find %s as param in func %s (-> %p)\n",
	__func__, sSizeName.c_str(), pFunction->GetName().c_str(),
	pSizeParameter);
    if (pSizeParameter)
    {
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
	    "CBETypedDeclarator::%s returns param %s\n", __func__,
	    pSizeParameter->m_Declarators.First()->GetName().c_str());
	return pSizeParameter;
    }
    
    pSizeParameter = pFunction->m_LocalVariables.Find(sSizeName);
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s tried to find %s as var in func %s (-> %p)\n",
	__func__, sSizeName.c_str(), pFunction->GetName().c_str(),
	pSizeParameter);
    if (pSizeParameter)
    {
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBETypedDeclarator::%s returns local var %s\n", __func__,
	    pSizeParameter->m_Declarators.First()->GetName().c_str());
	return pSizeParameter;
    }

    // get class before resetting function
    CBEClass *pClass = pFunction->GetSpecificParent<CBEClass>();
    // get direction before getting real message buffer
    CMsgStructType nType = (m_Attributes.Find(ATTR_IN)) ? pFunction->GetSendDirection() :
	pFunction->GetReceiveDirection();
    // check message buffer
    pFunction = GetSpecificParent<CBEFunction>();
    CBEMsgBuffer *pMsgBuffer = NULL;
    if (pFunction)
	pMsgBuffer = pFunction->GetMessageBuffer();
    else if (pClass)
     	pMsgBuffer = pClass->GetMessageBuffer();
    if (pMsgBuffer)
	pSizeParameter = pMsgBuffer->FindMember(sSizeName, pFunction ? 
	    pFunction : pUsingFunc, nType);
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s tried to find %s as member in msgbuf (-> %p)\n",
	__func__, sSizeName.c_str(), pSizeParameter);
    if (pSizeParameter)
    {
	// create new stack with message buffer
	pStack->clear();
	CBETypedDeclarator *pParent = pSizeParameter;

	while ((pParent = pParent->GetSpecificParent<CBETypedDeclarator>())
	    != 0)
	{
	    CBEDeclarator *pDecl = pParent->m_Declarators.First();
	    if (dynamic_cast<CBETypedef*>(pParent) &&
		!pParent->GetSpecificParent<CBETypedDeclarator>())
	    {
		pParent = pFunction ? 
		    pFunction->FindParameterType(pDecl->GetName()) :
			pUsingFunc->FindParameterType(pDecl->GetName());
		if (!pParent)
		    continue;
	    }
	    pStack->insert(pStack->begin(), pParent->m_Declarators.First());
	}
	bFoundInStruct = true;

    }

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s returns %p\n", __func__, pSizeParameter);
    // return (either found or not)
    return pSizeParameter;
}

/** \brief tries to obtain the size constant from an attribute
 *  \param pIsAttribute the attribute containing the name
 *  \return a reference to the constant if found
 */
CBEConstant*
CBETypedDeclarator::GetSizeConstant(CBEAttribute *pIsAttribute)
{
    CBEDeclarator *pSizeName = pIsAttribute->m_Parameters.First();
    assert(pSizeName);
    string sSizeName = pSizeName->GetName();
    // this might by a constant declarator
    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);
    CBEConstant *pConstant = pRoot->FindConstant(sSizeName);
    return pConstant;
}

/** \brief writes cleanup
 *  \param pFile the file to write to
 *  \param bDeferred true if deferred cleanup
 */
void
CBETypedDeclarator::WriteCleanup(CBEFile* pFile, bool bDeferred)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s called\n", __func__);
    CBEType *pType = GetType();
    bool bUsePointer = IsString() && !pType->IsPointerType();
    // with size_is or length_is we use malloc to init the pointer,
    // we have no indirection variables
    bUsePointer = bUsePointer || m_Attributes.Find(ATTR_SIZE_IS) || 
	m_Attributes.Find(ATTR_LENGTH_IS);
    // iterate over declarators
    vector<CBEDeclarator*>::iterator iterD;
    for (iterD = m_Declarators.begin();
	 iterD != m_Declarators.end();
	 iterD++)
    {
        (*iterD)->WriteCleanup(pFile, bUsePointer, bDeferred);
    }
}

/** \brief writes the type
 *  \param pFile the file to write to
 *  \param bUseConst true if type should be const
 */
void
CBETypedDeclarator::WriteType(CBEFile * pFile,
    bool bUseConst)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s called\n", __func__);
    if (bUseConst)
	WriteConstPrefix(pFile);
    CBEType *pType = GetType();
    if (pType)
        pType->Write(pFile);
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s returned\n", __func__);
}

/** \brief writes indirect parameter declaration
 *  \param pFile the file to write to
 *
 * This is just like the normal Write method, but it adds for every pointer of
 * a declarator a declarator without this pointer (for *t1 it adds _t1);
 *
 * For arrays, we use a special treatment: unbound array dimensions ('[]') are
 * written as stars, bound array dimension are written correctly.
 */
void
CBETypedDeclarator::WriteIndirect(CBEFile * pFile)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s called\n", __func__);
    if (!pFile->IsOpen())
        return;

    CBEType *pType = GetType();
    pType->WriteIndirect(pFile);
    *pFile << " ";

    // test for pointer types
    bool bIsPointerType = pType->IsPointerType();
    // if this is pointer type but has indirections during
    // write than this is because the pointer type is a typedef
    // this has been removed and the base type (without pointer has
    // been written: therefore this is not a pointer type anymore
    if (bIsPointerType && (pType->GetIndirectionCount() > 0))
        bIsPointerType = false;
    // if it is simple and we do not generate C-Types, than ignore the
    // pointer type, since it is typedefed, which makes the purpose of
    // this bool-parameter obsolete
    if (CCompiler::IsOptionSet(PROGRAM_USE_CORBA_TYPES) &&
        pType->IsSimpleType())
        bIsPointerType = false;
    // test if we need a pointer of this variable
    bool bUsePointer = IsString() && !pType->IsPointerType();
    // size_is or length_is attributes indicate an array, where we will need
    // a pointer to use.
    bUsePointer = bUsePointer || m_Attributes.Find(ATTR_SIZE_IS) ||
	m_Attributes.Find(ATTR_LENGTH_IS);
    // loop over declarators
    bool bComma = false;
    vector<CBEDeclarator*>::iterator iterD;
    for (iterD = m_Declarators.begin();
	 iterD != m_Declarators.end();
	 iterD++)
    {
        if (bComma)
	    *pFile << ", ";
        (*iterD)->WriteIndirect(pFile, bUsePointer, bIsPointerType);
        bComma = true;
    }
}

/** \brief writes indirect parameter initialization
 *  \param pFile the file to write to
 *  \param bMemory true if memory should be assigned
 *
 * This functin does assign a pointered variable a reference to a
 * "unpointered" variable or a dynamic memory region depending on \a bMemory.
 */
void
CBETypedDeclarator::WriteIndirectInitialization(CBEFile * pFile, 
    bool bMemory)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s called\n", __func__);
    CBEType *pType = GetTransmitType();

    bool bUsePointer = IsString() && !pType->IsPointerType();
    // with size_is, length_is we use malloc to init the pointer,
    // we have no indirection variables
    bUsePointer = bUsePointer || m_Attributes.Find(ATTR_SIZE_IS) || 
	m_Attributes.Find(ATTR_LENGTH_IS);
    // iterate over declarators
    vector<CBEDeclarator*>::iterator iterD;
    for (iterD = m_Declarators.begin();
	 iterD != m_Declarators.end();
	 iterD++)
    {
	if (bMemory)
	    (*iterD)->WriteIndirectInitializationMemory(pFile, bUsePointer);
	else
	    (*iterD)->WriteIndirectInitialization(pFile, bUsePointer);
    }
}

/** \brief writes init declaration
 *  \param pFile the file to write to
 *  \param sInitString the string to use for initialization
 */
void
CBETypedDeclarator::WriteInitDeclaration(CBEFile* pFile,
	string sInitString)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s called\n", __func__);
    CBEType *pType = GetType();
    if (pType->IsVoid())
        return;
    vector<CBEDeclarator*>::iterator iterD;
    for (iterD = m_Declarators.begin();
	 iterD != m_Declarators.end();
	 iterD++)
    {
	*pFile << "\t";
        WriteType(pFile);
	*pFile << " ";
        (*iterD)->WriteDeclaration(pFile);
	WriteProperties(pFile);
        string sDefault = GetDefaultInitString();
        if (!sInitString.empty())
            *pFile << " = " << sInitString;
        else if (!sDefault.empty())
        {
            if (sDefault == "0")
            {
                if (pType->DoWriteZeroInit())
                {
                    *pFile << " = ";
                    pType->WriteZeroInit(pFile);
                }
            }
            else
                *pFile << " = " << sDefault;
        }
	*pFile << ";\n";
    }
}

/** \brief creates the back-end structure for a parameter
 *  \param pFEParameter the corresponding front-end parameter
 *  \return true if code generation was successful
 *
 * This implementation extracts the type, name and attributes from the
 * front-end class.  Since a back-end parameter is expected to have only _ONE_
 * name, but the front-end typed declarator may have several names, we expect
 * that this function is only called for the first of the declarators. The
 * calling function has to take care of creating a seperate parameter for each
 * declarator.
 */
void 
CBETypedDeclarator::CreateBackEnd(CFETypedDeclarator * pFEParameter)
{
    assert(pFEParameter);
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s(fe) called\n", __func__);

    // call CBEObject's CreateBackEnd method
    CBEObject::CreateBackEnd(pFEParameter);

    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    // if we already have declarators, attributes or type, remove them (clean
    // myself)
    if (m_pType)
    {
        delete m_pType;
        m_pType = 0;
    }

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s(fe) cleaned\n", __func__);

    // get names
//     for_each(pFEParameter->m_Declarators.begin(),
// 	pFEParameter->m_Declarators.end(),
// 	CBETypedDeclarator::AddDeclarator);
    CCollection<CFEDeclarator>::iterator iD;
    for (iD = pFEParameter->m_Declarators.begin();
	 iD != pFEParameter->m_Declarators.end();
	 iD++)
    {
	AddDeclarator(*iD);
    }

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s(fe) decl created\n", __func__);
    // get type
    m_pType = pCF->GetNewType(pFEParameter->GetType()->GetType());
    m_pType->SetParent(this);
    try
    {
	m_pType->CreateBackEnd(pFEParameter->GetType());
    }
    catch (CBECreateException *e)
    {
        delete m_pType;
        m_pType = 0;
	throw;
    }
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s(fe) type created\n", __func__);

    // get attributes
//     for_each(pFEParameter->m_Attributes.begin(),
// 	pFEParameter->m_Attributes.end(),
// 	CBETypedDeclarator::AddAttribute);
    CCollection<CFEAttribute>::iterator iA;
    for (iA = pFEParameter->m_Attributes.begin();
	 iA != pFEParameter->m_Attributes.end();
	 iA++)
    {
	AddAttribute(*iA);
    }

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s(fe) returns true\n", __func__);
}

/** \brief creates the typed declarator using user defined type and name
 *  \param sUserDefinedType the user defined type
 *  \param sName the name of the typed declarator
 *  \param nStars the number of stars for the declarator
 *  \return true if successful
 */
void
CBETypedDeclarator::CreateBackEnd(string sUserDefinedType,
    string sName,
    int nStars)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s(%s, %s, %d) called\n", __func__,
	sUserDefinedType.c_str(), sName.c_str(), nStars);

    
    // create decl
    AddDeclarator(sName, nStars);
    // create type
    if (m_pType)
	delete m_pType;
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    m_pType = pCF->GetNewUserDefinedType();
    m_pType->SetParent(this);    // has to be set before calling CreateBE
    try
    {
	CBEUserDefinedType *pType = static_cast<CBEUserDefinedType*>(m_pType);
	pType->CreateBackEnd(sUserDefinedType);
    }
    catch (CBECreateException *e)
    {
        delete m_pType;
        m_pType = 0;
	throw;
    }
    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s() returns\n", __func__);
}

/** \brief creates the typed declarator using a given back-end type and a name
 *  \param pType the type of the typed declarator
 *  \param sName the name of the declarator
 *  \return true if successful
 */
void
CBETypedDeclarator::CreateBackEnd(CBEType * pType, 
    string sName)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s(%p, %s) called\n", __func__, pType,
	sName.c_str());

    // create decl
    AddDeclarator(sName, 0);
    // create type
    assert(pType);
    if (m_pType)
	delete m_pType;
    m_pType = (CBEType *) pType->Clone();
    m_pType->SetParent(this);
    // do not need to call create, because original has been created before.

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s(type, name) returns true\n", __func__);
}

/** \brief add an attribute constructed from a front-end attribute
 *  \param pFEAttribute the front-end attribute to use as reference
 */
void 
CBETypedDeclarator::AddAttribute(CFEAttribute *pFEAttribute)
{
    CBEAttribute *pAttribute = CCompiler::GetClassFactory()->GetNewAttribute();
    m_Attributes.Add(pAttribute);
    try
    {
	pAttribute->CreateBackEnd(pFEAttribute);
    }
    catch (CBECreateException *e)
    {
	m_Attributes.Remove(pAttribute);
        delete pAttribute;
	throw;
    }
}

/** \brief adds a declarator constructed from a front-end declarator
 *  \param pFEDeclarator the front-end declarator
 */
void CBETypedDeclarator::AddDeclarator(CFEDeclarator * pFEDeclarator)
{
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBEDeclarator *pDecl = pCF->GetNewDeclarator();
    m_Declarators.Add(pDecl);
    try
    {
	pDecl->CreateBackEnd(pFEDeclarator);
    }
    catch (CBECreateException *e)
    {
	m_Declarators.Remove(pDecl);
	delete pDecl;
	throw;
    }
}

/** \brief adds a declarator constructed from name and stars
 *  \param sName the name of the decl
 *  \param nStars the number of stars of this decl
 */
void CBETypedDeclarator::AddDeclarator(string sName, int nStars)
{
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBEDeclarator *pDecl = pCF->GetNewDeclarator();
    m_Declarators.Add(pDecl);
    try
    {
	pDecl->CreateBackEnd(sName, nStars);
    }
    catch (CBECreateException *e)
    {
	m_Declarators.Remove(pDecl);
	delete pDecl;
	throw;
    }
}

/** \brief retrieves a reference to the call declarator
 *  \return a reference to the call declarator
 */
CBEDeclarator *CBETypedDeclarator::GetCallDeclarator()
{
    vector<CBEDeclarator*>::iterator iter = m_Declarators.begin();
    if (iter++ == m_Declarators.end())
        return 0;
    if (iter == m_Declarators.end())
        return 0;
    return *iter;
}

/** \brief remove the call declarator
 */
void
CBETypedDeclarator::RemoveCallDeclarator()
{
    // check if there is a call declarator
    CBEDeclarator *pDecl = GetCallDeclarator();
    if (!pDecl)
	return;
    m_Declarators.Remove(pDecl);
    delete pDecl;
}

/** \brief test if this is a string variable
 *  \return true if it is
 *
 * A string is everything which has a base type 'char', is of a size bigger
 * than 1 character and has the string attribute. This function only checks
 * the first declarator.
 */
bool CBETypedDeclarator::IsString()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s called for %s in func %s\n",
	__func__, m_Declarators.First()->GetName().c_str(),
	GetSpecificParent<CBEFunction>() ?
	GetSpecificParent<CBEFunction>()->GetName().c_str() : 
	"(no func)");

    if (!m_Attributes.Find(ATTR_STRING))
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	    "CBETypedDeclarator::%s returns false: no string attribute\n", __func__);
        return false;
    }
    /* do NOT test for size_is/length_is because:
     * a) parameter has [string] attribute
     * b) size_is might point to member of message buffer or local variable
     */
    if (m_pType->IsOfType(TYPE_CHAR_ASTERISK) &&
        !m_pType->IsUnsigned())
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	    "CBETypedDeclarator::%s returns true: char* and !unsigned\n", __func__);
        return true;
    }
    if (m_pType->IsOfType(TYPE_CHAR) &&
        !m_pType->IsUnsigned())
    {
        CBEDeclarator *pDeclarator = m_Declarators.First();
        if (pDeclarator)
        {
            int nSize = pDeclarator->GetSize();
            // can be either <0 -> pointer
            // or >1 -> array
            // == 1 -> normal char
            // == 0 -> bitfield
            if ((nSize < 0) || (nSize > 1))
	    {
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
		    "CBETypedDeclarator::%s returns true: size is %d\n", __func__, 
		    nSize);
                return true;
	    }
        }
    }
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s returns false: type is %d\n", __func__,
	m_pType->GetFEType());
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
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s called for %s\n", __func__,
	m_Declarators.First()->GetName().c_str());
    
    // need the root
    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);
    CBEConstant *pConstant = 0;
    CBEAttribute *pAttr;
    if ((pAttr = m_Attributes.Find(ATTR_SIZE_IS)) != 0)
    {
        if (pAttr->IsOfType(ATTR_CLASS_IS))
        {
            // if declarator is a constant, then this is const as well
            CBEDeclarator *pSizeName = pAttr->m_Parameters.First();
            assert(pSizeName);
            // this might by a constant declarator
            pConstant = pRoot->FindConstant(pSizeName->GetName());
            // not a constant, return true
            if (!pConstant)
	    {
		CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		    "CBETypedDeclarator::%s returns true (SIZE)\n",
		    __func__);
                return true;
	    }
        }
    }
    if ((pAttr = m_Attributes.Find(ATTR_LENGTH_IS)) != 0)
    {
        if (pAttr->IsOfType(ATTR_CLASS_IS))
        {
            // if declarator is a constant, then this is const as well
            CBEDeclarator *pSizeName = pAttr->m_Parameters.First();
            assert(pSizeName);
            // this might by a constant declarator
            pConstant = pRoot->FindConstant(pSizeName->GetName());
            // not a constant, return true
            if (!pConstant)
	    {
		CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
		    "CBETypedDeclarator::%s returns true (LENGTH)\n",
		    __func__);
                return true;
	    }
        }
    }
    if ((pAttr = m_Attributes.Find(ATTR_MAX_IS)) != 0)
    {
        if (pAttr->IsOfType(ATTR_CLASS_IS))
        {
            // if declarator is a constant, then this is const as well
            CBEDeclarator *pSizeName = pAttr->m_Parameters.First();
            assert(pSizeName);
            // this might by a constant declarator
            pConstant = pRoot->FindConstant(pSizeName->GetName());
            // not a constant, return true
            if (!pConstant)
	    {
		CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		    "CBETypedDeclarator::%s returns true (MAX)\n",
		    __func__);
                return true;
	    }
        }

    }
    if (IsString())
    {
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBETypedDeclarator::%s return true (isString)\n",
	    __func__);
        return true;
    }
    // if type is variable sized, then this variable
    // is too (e.g. a struct with a var-sized member
    if (GetType() && (GetType()->GetSize() < 0))
    {
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBETypedDeclarator::%s returns true (type has neg size)\n",
	    __func__);
        return true;
    }
    // if we have an fixed sized array with a size_is attribute,
    // the parameter is actually fixed, right?
    // test declarators for arrays
    vector<CBEDeclarator*>::iterator iter;
    for (iter = m_Declarators.begin();
	 iter != m_Declarators.end();
	 iter++)
    {
        if (!(*iter)->IsArray())
            continue;
        vector<CBEExpression*>::iterator iterB;
	for (iterB = (*iter)->m_Bounds.begin();
	     iterB != (*iter)->m_Bounds.end();
	     iterB++)
	{
	    // EXPR_NONE == no bound
	    if (!(*iterB)->IsOfType(EXPR_NONE))
		continue;

	    CBEDeclarator *pAttrName = (CBEDeclarator*)0;
	    if (pAttr)
		pAttrName = pAttr->m_Parameters.First();
	    pConstant = (CBEConstant*)0;
	    if (pAttrName)
		pConstant = pRoot->FindConstant(pAttrName->GetName());
	    if (!pConstant)
    	    {
		CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
		    "CBETypedDeclarator::%s return true (var array)\n",
		    __func__);
		return true;
            }
        }
    }
    // no size/max attribute and no unbound array:
    // should be treated as scalar
    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s returns false\n", __func__);
    return false;
}

/** \brief checks if this parameter is of fixed size
 *  \return true if it is of fixed size.
 *
 * A parameter, which is not variable size, does not necessarily has to be of
 * fixed size. Since we support a message buffer with multiple data types, it
 * is possible, that a parameter of a specific type has to be marshalled not
 * as fixed and not as varaible sized, but as something else. So this
 * parameter will return on both functions false.
 *
 * This implementation simply assigns a variable sized parameter to be not a
 * fixed sized parameter and vice versa.
 */
bool CBETypedDeclarator::IsFixedSized()
{
    return !IsVariableSized();
}

/** \brief calculates the size of a typed declarator
 *  \return the number of bytes needed for the declarators
 *
 * The size of a typed declarator depends on the size of the type and the size
 * of the declarator.  This implementation only tests the first declarator. If
 * the size of the declarator is:
 * - negative, then it has unbound array dimensions or pointers
 * - equals zero, it has a bitfield value
 * - equals one, it is a "normal" simple declarator
 * - larger than one, it is a array of bound size
 * .
 * If the declarator has one pointer and an OUT attribute it is probably
 * referenced to obtain the base type's value, so we return the base type's
 * size as size.
 *
 * The only other exception to watch for is, if this function returns zero.
 * Then the parameter has bitfields. Because this functions returns the size
 * in bytes, we cannot express the bitfields as bytes (usually smaller than
 * 8). Test for the return value of zero and get the bitfield-size explicetly.
 */
int CBETypedDeclarator::GetSize()
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s called (decl %s)\n",
	__func__, m_Declarators.First()->GetName().c_str());
    
    int nSize = 0;

    vector<CBEDeclarator*>::iterator iterD;
    for (iterD = m_Declarators.begin();
	 iterD != m_Declarators.end();
	 iterD++)
    {
        int nDeclSize = GetSizeOfDeclarator(*iterD);
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	    "CBETypedDeclarator::%s size of %s is %d\n",
	    __func__, (*iterD)->GetName().c_str(), nDeclSize);
        if (nDeclSize < 0)
	{
	    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBETypedDeclarator::%s returns -1\n", __func__);
            return -1;
	}
        nSize += nDeclSize;
    }
    
    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s returns %d\n", __func__, nSize);
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
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s(%s) called\n", __func__, sName.c_str());
    
    CBEDeclarator *pDecl = m_Declarators.Find(sName);
    if (!pDecl)
        return 0; // not existent

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s(%s) has decl\n", __func__, sName.c_str());
    return GetSizeOfDeclarator(pDecl);
}

/** \brief get the actually to transmit type
 *  \return the type to transmit
 *
 * This function checks for transmit as attribute
 */
CBEType* CBETypedDeclarator::GetTransmitType()
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s called\n", __func__);
    
    CBEType *pType = m_pType;
    CBEAttribute *pAttr = m_Attributes.Find(ATTR_TRANSMIT_AS);
    if (pAttr && pAttr->GetType())
        pType = pAttr->GetAttrType();

    // get type
    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s returns %p\n", __func__, pType);
    return pType;
}

/** \brief calculate the size of a specific declarator
 *  \param pDeclarator the declarator to get the size of
 *  \return the size of the declarator
 *
 * To avoid circular calls for constructs like this:
 * typdef struct A A_t;
 * struct A { A_t *next; };
 *
 * we have to test if the type of the declarator (pType)
 * is the same as the enclosing struct type (if any).
 */
int CBETypedDeclarator::GetSizeOfDeclarator(CBEDeclarator *pDeclarator)
{
    assert(pDeclarator);
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s(%s) called\n", __func__,
	pDeclarator->GetName().c_str());

    CBEType *pType = GetTransmitType();
    if (pType->IsVoid())
    {
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBETypedDeclarator::%s(%s) returns void (trans-type 0)\n",
	    __func__, pDeclarator->GetName().c_str());
        return 0;
    }

    int nTypeSize = pType->GetSize();
    int nDeclSize = pDeclarator->GetSize();
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s %s: type(%d): %d, decl: %d\n", __func__,
	m_Declarators.First()->GetName().c_str(), pType->GetFEType(), 
	nTypeSize, nDeclSize);

    // if referenced OUT, this is the size
    if ((nDeclSize == -1) &&
        (pDeclarator->GetStars() == 1) &&
        (m_Attributes.Find(ATTR_OUT)))
    {
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBETypedDeclarator::%s returns ref-out %d\n", __func__, nTypeSize);
        return nTypeSize;
    }
    // if reference struct, this is the size
    if ((nDeclSize == -1) &&
        (pDeclarator->GetStars() == 1) &&
        (pType->IsConstructedType()))
    {
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBETypedDeclarator::%s returns ts:%d\n", __func__, nTypeSize);
        return nTypeSize;
    }
    // if variables sized, return -1
    if (nDeclSize < 0)
    {
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBETypedDeclarator::%s returns -1\n", __func__);
        return -1;
    }

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s returns %d\n", __func__, nDeclSize*nTypeSize);
    // if bitfield: it multiplies with zero and returns zero
    // const array or simple? -> return array-dimension * type's size
    return nDeclSize * nTypeSize;
}

/** \brief print warning if no max-is is defined
 *  \param nSize the size that is guessed
 */
void CBETypedDeclarator::WarnNoMax(int nSize)
{
    if (!CCompiler::IsWarningSet(PROGRAM_WARNING_NO_MAXSIZE))
	return;

    CBEDeclarator *pD = m_Declarators.First();
    CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
    if (pFunction)
	CMessages::Warning("%s in %s has no maximum size (guessing size %d)",
	    pD->GetName().c_str(),
	    pFunction->GetName().c_str(),
	    nSize);
    else
	CMessages::Warning("%s has no maximum size (guessing size %d)",
	    pD->GetName().c_str(),
	    nSize);
}
 
/** \brief determine the size or dimension encoded in a size attribute
 *  \param nAttr the attribute to test
 *  \param nSize reference to the size variable to increase if appropriate
 *  \param nDimension the dimension to set if appropriate
 *  \return true if size was modified, otherwise the dimension was set
 */
bool CBETypedDeclarator::GetSizeOrDimensionOfAttr(ATTR_TYPE nAttr,
    int& nSize,
    int& nDimension)
{
    CBEAttribute *pAttr;
    if ((pAttr = m_Attributes.Find(nAttr)) != 0)
    {
	if (pAttr->IsOfType(ATTR_CLASS_INT))
	{
	    nSize += pAttr->GetIntValue() * GetTransmitType()->GetMaxSize();
	    return true;
	}
	if (pAttr->IsOfType(ATTR_CLASS_IS))
	{
	    int nTmp = pAttr->GetRemainingNumberOfIsAttributes(
		pAttr->m_Parameters.begin());
	    nDimension = std::max(nDimension, nTmp);
	}
    }
    return false;
}

/** \brief calculates the max size of the paramater
 *  \param bGuessSize true if we should guess the size of the parameter
 *  \param nSize the maximum size
 *  \param sName the name of the declarator to determine the size of, empty if
 *         all
 *  \return true if size could be determined
 *
 * Platform dependent size preceedes language dependent size.
 */
bool
CBETypedDeclarator::GetMaxSize(bool bGuessSize,
    int & nSize, string sName)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s(%s, %s) called.\n",
	__func__, bGuessSize ? "true" : "false",
	sName.empty() ? "(none)" : sName.c_str());
    
    CBESizes *pSizes = CCompiler::GetSizes();
    
    CBEType *pType = GetTransmitType();
    // no size for void
    nSize = 0;
    if (pType->IsVoid())
    {
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
	    "CBETypedDeclarator::%s void type, return 0\n", __func__);
        return true;
    }
    // get type's size (this returns either the simple type's size or the
    // maximum size of a constructed type)
    int nTypeSize = pType->GetMaxSize();
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s size of type (%d) is %d\n", __func__,
	pType->GetFEType(), nTypeSize);
    
    bool bVarSized = false;
    vector<CBEDeclarator*>::iterator iterD;
    for (iterD = m_Declarators.begin();
	 iterD != m_Declarators.end();
	 iterD++)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	    "%s sName empty? %s || \"%s\" != \"%s\" (%s)\n",
	    __func__, sName.empty() ? "true" : "false",
	    (*iterD)->GetName().c_str(), sName.empty() ? "" : sName.c_str(),
	    (sName.empty() || (*iterD)->GetName() != sName) ? "continue" : "calc");
	/* skip if we are looking for a special member */
	if (!sName.empty() && (*iterD)->GetName() != sName)
	    continue;
	
        int nDeclSize = (*iterD)->GetMaxSize();
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	    "CBETypedDeclarator::%s size of %s is %d with type size being %d\n",
	    __func__, (*iterD)->GetName().c_str(), nDeclSize, nTypeSize);
	
        // if decl is array, but returns a negative maximum size,
        // it is an unbound array. Its max size is the max size of it's
        // type times the dimensions
        if ((nDeclSize < 0) && (*iterD)->IsArray() && bGuessSize)
        {
            nTypeSize = pSizes->GetMaxSizeOfType(pType->GetFEType());
            nSize += nTypeSize * -nDeclSize;
	    WarnNoMax(nTypeSize * -nDeclSize);
            continue;
        }
        // if size_is or length_is, then this is an array
        if ((nDeclSize < 0) &&
            (m_Attributes.Find(ATTR_SIZE_IS) ||
             m_Attributes.Find(ATTR_LENGTH_IS) ||
             m_Attributes.Find(ATTR_MAX_IS)))
        {
	    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s some size attribute\n",
		__func__);
            int nDimensions = 1;
	    if (GetSizeOrDimensionOfAttr(ATTR_SIZE_IS, nSize, nDimensions))
		continue;
	    if (GetSizeOrDimensionOfAttr(ATTR_LENGTH_IS, nSize, nDimensions))
		continue;
	    if (GetSizeOrDimensionOfAttr(ATTR_MAX_IS, nSize, nDimensions))
		continue;
            if (bGuessSize)
            {
		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
		    "%s guessing size for size attr\n", __func__);
		WarnNoMax(nTypeSize * -nDeclSize);
                nSize += pSizes->GetMaxSizeOfType(pType->GetFEType()) * nDimensions;
                continue;
            }
        }
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s decl size is %d\n", __func__,
	    nDeclSize);
        // if referenced, this is the size
        if (nDeclSize == -((*iterD)->GetStars()))
            nDeclSize = -nDeclSize;
        // if variables sized, return -1
        if (nDeclSize < 0)
            bVarSized = true;
        // if bitfield: it multiplies with zero and returns zero
        // const array or simple? -> return array-dimension * type's size
        nSize += nDeclSize * nTypeSize;

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s total now %d\n", __func__,
	    nSize);
    }
    

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s up to now its %d\n", __func__, nSize);
    
    if (bVarSized || pType->IsPointerType())
    {
        // check max attributes
        CBEAttribute *pAttr = m_Attributes.Find(ATTR_MAX_IS);
	// if no max-is attribute found and bVarSized NOT set, then use the
	// maximum size determined so far (if non-negative)
	if (!pAttr && !bVarSized && (nSize > 0))
	{
	    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBETypedDeclarator::%s finished max-size\n", __func__);
	    return true;
	}

	// otherwise check again
	nSize = -1;
        if ((pAttr = m_Attributes.Find(ATTR_MAX_IS)) != 0)
        {
            if (pAttr->IsOfType(ATTR_CLASS_INT))
                nSize = pAttr->GetIntValue();
            else if (pAttr->IsOfType(ATTR_CLASS_IS))
            {
                // if declarator is a constant, then this is const as well
                CBEDeclarator *pSizeName = pAttr->m_Parameters.First();
                assert(pSizeName);
                // this might by a constant declarator
                CBERoot *pRoot = GetSpecificParent<CBERoot>();
                assert(pRoot);
                CBEConstant *pConstant = 
		    pRoot->FindConstant(pSizeName->GetName());
                // set size to value of constant
                if (pConstant && pConstant->GetValue())
                    nSize = pConstant->GetValue()->GetIntValue() * nTypeSize;
            }
        }
    }

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s returning %d\n", __func__, nSize);
    return (nSize > 0);
}

/** \brief allows access to type of typed declarator
 *  \return reference to m_pType
 */
CBEType *CBETypedDeclarator::GetType()
{
    return m_pType;
}

/** \brief tries to match the name to the internally stored name
 *  \param sName the name to match against
 *  \return true if match
 */
bool
CBETypedDeclarator::Match(string sName)
{
    return m_Declarators.Find(sName) != 0;
}

/** \brief replaces the current type with the new one
 *  \param pNewType the new type
 */
void CBETypedDeclarator::ReplaceType(CBEType * pNewType)
{
    if (!pNewType)
        return;
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, 
	"CBETypedDeclarator::%s(%p) called\n", __func__, pNewType);
    if (dynamic_cast<CBEUserDefinedType*>(pNewType))
    {
	assert(dynamic_cast<CBEUserDefinedType*>(pNewType)->GetName() !=
	    m_Declarators.First()->GetName());
    }
    if (m_pType)
	delete m_pType;
    m_pType = pNewType;
    pNewType->SetParent(this);

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s returns\n", __func__);
}

/** \brief returns the size of the bitfield declarator in bits
 *  \return the bitfields of the declarator
 */
int CBETypedDeclarator::GetBitfieldSize()
{
    int nSize = 0;
    vector<CBEDeclarator*>::iterator iter;
    for (iter = m_Declarators.begin();
	 iter != m_Declarators.end();
	 iter++)
    {
        nSize += (*iter)->GetBitfields();
    }
    return nSize;
}

/** \brief checks if the parameter is transmitted into the given direction
 *  \param nDirection the direction to check
 *  \return true if it is transmitted that way
 */
bool CBETypedDeclarator::IsDirection(DIRECTION_TYPE nDirection)
{
    if ((nDirection == DIRECTION_IN) && m_Attributes.Find(ATTR_IN))
	return true;
    if ((nDirection == DIRECTION_OUT) && m_Attributes.Find(ATTR_OUT))
	return true;
    if ((nDirection == DIRECTION_INOUT) && 
	(m_Attributes.Find(ATTR_IN) || m_Attributes.Find(ATTR_OUT)))
	return true;
    return false;
}

/** \brief checks if this parameter is referenced
 *  \return true if ons declarator is a pointer
 */
bool CBETypedDeclarator::HasReference()
{
    vector<CBEDeclarator*>::iterator iter;
    for (iter = m_Declarators.begin();
	 iter != m_Declarators.end();
	 iter++)
    {
        if ((*iter)->GetStars() > 0)
            return true;
    }
    return false;
}

/** \brief checks if the given name belongs to an IS attribute if it has one
 *  \param sDeclName the name of the IS attr declarator
 *  \return true if found, false if not
 *
 * We have to search all attributes of IS type (SIZE_IS, LENGTH_IS, MIN_IS,
 * MAX_IS, etc.) Then we have to check its declarator with the given name. If
 * we found a match, we return a reference to the attribute.
 *
 * We can simply search the attributes parameters, because that list would be
 * empty if the attribute is not an IS attribute.
 */
CBEAttribute* CBETypedDeclarator::FindIsAttribute(string sDeclName)
{
    vector<CBEAttribute*>::iterator iter;
    for (iter = m_Attributes.begin();
	 iter != m_Attributes.end();
	 iter++)
    {
        if ((*iter)->m_Parameters.Find(sDeclName))
            return *iter;
    }
    return 0;
}

/** \brief add language dependent property
 *  \param sProperty the property to add
 *  \param sPropertyString some value for the property
 *  \return true if successful
 */
bool
CBETypedDeclarator::AddLanguageProperty(string sProperty,
    string sPropertyString)
{
    m_mProperties.insert(
	map<string,string>::value_type(sProperty, sPropertyString));
    return true;
}

/** \brief search for a language property
 *  \param sProperty the identifier of the property
 *  \param sPropertyString will be set to the property string if found
 *  \return true if property is found
 */
bool
CBETypedDeclarator::FindLanguageProperty(string sProperty,
    string& sPropertyString)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s for %s (%s)\n", __func__,
	m_Declarators.First()->GetName().c_str(), sProperty.c_str());
    multimap<string,string>::iterator iter = m_mProperties.begin();
    for (; iter != m_mProperties.end(); iter++)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, " checking %s\n",
	    (*iter).first.c_str());
        if ((*iter).first == sProperty)
	{
	    sPropertyString = (*iter).second;
	    return true;
	}
    }
    return false;
}

/** \brief writes the declarators to the target file
 *  \param pFile the file to write to
 */
void 
CBETypedDeclarator::WriteDeclarators(CBEFile * pFile)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s called\n", __func__);
    if (!pFile->IsOpen())
        return;

    bool bComma = false;
    vector<CBEDeclarator*>::iterator iterD;
    for (iterD = m_Declarators.begin();
	 iterD != m_Declarators.end();
	 iterD++)
    {
        if (bComma)
	    *pFile << ", ";

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s for %s (%d) in %s (%p -> %p)\n",
            __func__, (*iterD)->GetName().c_str(),
            (*iterD)->GetStars(),
            (GetSpecificParent<CBEFunction>()) ? 
	    GetSpecificParent<CBEFunction>()->GetName().c_str() : "(none)",
            this, (*iterD));

        (*iterD)->WriteDeclaration(pFile);
        bComma = true;
    }
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBETypedDeclarator::%s returned\n", __func__);
}

/** \brief merely responsible to write the const prefix for the type
 *  \param pFile the file to write to
 *
 * - An IN string is always const (its automatically an char*) on client's
 * side.  (server side may need to fiddle with pointer)
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
CBETypedDeclarator::WriteConstPrefix(CBEFile *pFile)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBETypedDeclarator::%s called\n", __func__);
    // check if no-const property is set
    string sDummy;
    if (FindLanguageProperty(string("noconst"), sDummy))
	return;
    CBEType *pType = GetType();
    bool bConstructed = (pType && pType->IsConstructedType());
    // test if string
    bool bIsArray = IsString();
    bool bCheckedArrayDims = false;
    // test all declarators for strings
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
	"%s for %s (1): is-array %s, bChecked %s\n", __func__, 
	m_Declarators.First()->GetName().c_str(), (bIsArray) ? "true" : "false",
	(bCheckedArrayDims) ? "true" : "false");
    if (!bIsArray)
    {
        vector<CBEDeclarator*>::iterator iterD;
	for (iterD = m_Declarators.begin();
	     iterD != m_Declarators.end();
	     iterD++)
	{
            if (!(*iterD)->IsArray())
		continue;

	    vector<CBEExpression*>::iterator iterB;
	    for (iterB = (*iterD)->m_Bounds.begin();
		 iterB != (*iterD)->m_Bounds.end();
		 iterB++)
	    {
		// either fixed number in bound
	    	// or no fixed array boundary
		if ((*iterB)->IsOfType(EXPR_INT) || 
    		    ((*iterB)->GetIntValue() == 0)) 
		    bIsArray = true;
	    }

	    // FIXME: testing for array dimension == 0?
	    if ((*iterD)->GetArrayDimensionCount() == 0)
    		bIsArray = true;
	    bCheckedArrayDims = true;
	}
    }
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
	"%s for %s (2): is-array %s, bChecked %s\n", __func__, 
	m_Declarators.First()->GetName().c_str(), (bIsArray) ? "true" : "false",
	(bCheckedArrayDims) ? "true" : "false");
    // test for size/length/max attributes, which indicate arrays
    if (!bIsArray && !bCheckedArrayDims)
    {
	if (m_Attributes.Find(ATTR_SIZE_IS))
	    bIsArray = true;
	if (m_Attributes.Find(ATTR_LENGTH_IS))
	    bIsArray = true;
    }
    // we do not test the max_is attribute because:
    // if the parameter had brackets ([]) then max_is has been converted
    // to a value there, or
    // if max_is is found now its a pointer, which can use some
    // const, but this has been enabled by the above checks (array-dims == 0)
    // already
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
	"%s for %s (3): is-array %s, bChecked %s\n", __func__, 
	m_Declarators.First()->GetName().c_str(), (bIsArray) ? "true" : "false",
	(bCheckedArrayDims) ? "true" : "false");
    bool bNoCorbaType = !CCompiler::IsOptionSet(PROGRAM_USE_CORBA_TYPES);
    // there is one exception to this rule: if this is already a
    // CORBA type. CORBA types start with a "CORBA_" string. Therefore:
    // test if type is user defined and check beginning of alias.
    if (bNoCorbaType  && pType->IsOfType(TYPE_USER_DEFINED) &&
        dynamic_cast<CBEUserDefinedType*>(pType))
    {
        CBEUserDefinedType *pUserType = 
	    dynamic_cast<CBEUserDefinedType*>(pType);
        string sName = pUserType->GetName();
        if ((sName.length() > 6) &&
            (sName.substr(0, 6) == "CORBA_"))
            bNoCorbaType = false;
    }
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
	"%s for %s (4): is-array %s, constr %s, use corba type %s\n", 
	__func__, 
	m_Declarators.First()->GetName().c_str(), (bIsArray) ? "true" : "false",
	(bConstructed) ? "true" : "false",
	(bNoCorbaType) ? "true" : "false");
    // string check
    if (bConstructed || bIsArray)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
	    "%s for %s (5): constr %s, array %s -> client %s, no corba %s, ptr %s\n",
	    __func__,
	    m_Declarators.First()->GetName().c_str(), 
	    (bConstructed) ? "true" : "false",
	    (bIsArray) ? "true" : "false",
	    (pFile->IsOfFileType(FILETYPE_CLIENT)) ? "yes" : "no",
	    (bNoCorbaType) ? "yes" : "no",
	    (pType->IsPointerType()) ? "yes" : "no");
        if (pFile->IsOfFileType(FILETYPE_CLIENT))
        {
            if ((m_Attributes.Find(ATTR_IN)) && (!m_Attributes.Find(ATTR_OUT)))
            {
                if (!bNoCorbaType && pType->IsPointerType())
		    *pFile << "const_";
                else
		    *pFile << "const ";
            }
        }
        if (pFile->IsOfFileType(FILETYPE_COMPONENT) ||
            pFile->IsOfFileType(FILETYPE_TEMPLATE))
        {
            if ((m_Attributes.Find(ATTR_IN)) &&
                (!m_Attributes.Find(ATTR_OUT)) &&
                (GetSpecificParent<CBEComponentFunction>()))
            {
                if (!bNoCorbaType && pType->IsPointerType())
		    *pFile << "const_";
                else
		    *pFile << "const ";
            }
        }
    }
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBETypedDeclarator::%s returned\n", __func__);
}

/** \brief writes a forward type declaration
 *  \param pFile the file to write to
 *  \param bUseConst true if the 'const' keyword should be used
 *
 * Calls the WriteDeclaration operation of the type.
 */
void
CBETypedDeclarator::WriteForwardTypeDeclaration(CBEFile * pFile, 
    bool bUseConst)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBETypedDeclarator::%s called\n", __func__);
    if (bUseConst)
	WriteConstPrefix(pFile);
    CBEType *pType = GetType();
    if (pType)
        pType->WriteDeclaration(pFile);
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBETypedDeclarator::%s returned\n", __func__);
}

/** \brief writes a forward declaration of the typed declarator
 *  \param pFile the file to write to
 */
void
CBETypedDeclarator::WriteForwardDeclaration(CBEFile * pFile)
{
    if (!pFile->IsOpen())
        return;

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBETypedDeclarator::%s called for %s\n", __func__,
        m_Declarators.First()->GetName().c_str());
    WriteForwardTypeDeclaration(pFile);
    *pFile << " ";
    WriteDeclarators(pFile);
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBETypedDeclarator::%s returned\n", __func__);
}

/** \brief writes the definition of a typed declarator to the target file
 *  \param pFile the file to write to
 */
void
CBETypedDeclarator::WriteDefinition(CBEFile * pFile)
{
    if (!pFile->IsOpen())
        return;

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBETypedDeclarator::%s called for %s\n", __func__,
        m_Declarators.First()->GetName().c_str());
    WriteType(pFile);
    WriteProperties(pFile);
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBETypedDeclarator::%s returned\n", __func__);
}

/** \brief writes the attributes of the typed declarator
 *  \param pFile the file to write to
 *
 * The current implementation does nothing.
 */
void
CBETypedDeclarator::WriteAttributes(CBEFile * /*pFile*/)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBETypedDeclarator::%s called\n", __func__);
}

/** \brief writes the specific properties
 *  \param pFile the file to write to
 */
void
CBETypedDeclarator::WriteProperties(CBEFile *pFile)
{
    // check if we have something like "asm" or "__attribute__"
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBETypedDeclarator::%s: writing properties\n", __func__);
    multimap<string,string>::iterator iter = m_mProperties.begin();
    for (; iter != m_mProperties.end(); iter++)
    {
        if ((*iter).first == string("attribute"))
            *pFile << " " << (*iter).second;
    }
}

