/**
 *  \file    dice/src/Visitor.cpp
 *  \brief   contains the implementation of the class CVisitor
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

#include "Visitor.h"
#include "fe/FEAttribute.h"
#include "fe/FEOperation.h"
#include "fe/FEInterface.h"
#include "fe/FELibrary.h"
#include "fe/FEFile.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEConstDeclarator.h"
#include "fe/FEAttributeDeclarator.h"
#include "fe/FEUnionCase.h"
#include "fe/FEArrayType.h"
#include "fe/FEConstructedType.h"
#include "fe/FEUnionType.h"
#include "fe/FEStructType.h"
#include "fe/FEExpression.h"
#include <vector>
using std::vector;

CVisitor::~CVisitor()
{}

/** \brief visit a class of type CObject */
void CVisitor::Visit(CObject&)
{ }

/** \brief visit a class of type CFEBase */
void CVisitor::Visit(CFEBase&)
{ }

/** \brief visit a class of type CFEAttribute */
void CVisitor::Visit(CFEAttribute&)
{ }

/** \brief visit a class of type CFEEndPointAttribute */
void CVisitor::Visit(CFEEndPointAttribute&)
{ }

/** \brief visit a class of type CFEExceptionAttribute */
void CVisitor::Visit(CFEExceptionAttribute&)
{ }

/** \brief visit a class of type CFEIntAttribute */
void CVisitor::Visit(CFEIntAttribute&)
{ }

/** \brief visit a class of type CFEIsAttribute */
void CVisitor::Visit(CFEIsAttribute&)
{ }

/** \brief visit a class of type CFEPtrDefaultAttribute */
void CVisitor::Visit(CFEPtrDefaultAttribute&)
{ }

/** \brief visit a class of type CFEStringAttribute */
void CVisitor::Visit(CFEStringAttribute&)
{ }

/** \brief visit a class of type CFETypeAttribute */
void CVisitor::Visit(CFETypeAttribute&)
{ }

/** \brief visit a class of type CFEVersionAttribute */
void CVisitor::Visit(CFEVersionAttribute&)
{ }

/** \brief visit a class of type CFEExpression */
void CVisitor::Visit(CFEExpression&)
{ }

/** \brief visit a class of type CFEPrimaryExpression */
void CVisitor::Visit(CFEPrimaryExpression&)
{ }

/** \brief visit a class of type CFEUnaryExpression */
void CVisitor::Visit(CFEUnaryExpression&)
{ }

/** \brief visit a class of type CFEBinaryExpression */
void CVisitor::Visit(CFEBinaryExpression&)
{ }

/** \brief visit a class of type CFEConditionalExpression */
void CVisitor::Visit(CFEConditionalExpression&)
{ }

/** \brief visit a class of type CFESizeOfExpression */
void CVisitor::Visit(CFESizeOfExpression&)
{ }

/** \brief visit a class of type CFEUserDefinedExpression */
void CVisitor::Visit(CFEUserDefinedExpression&)
{ }

/** \brief visit a class of type CFEFile */
void CVisitor::Visit(CFEFile& f)
{
	// included files
	vector<CFEFile*>::iterator iterF;
	for (iterF = f.m_ChildFiles.begin();
		iterF != f.m_ChildFiles.end();
		iterF++)
	{
		(*iterF)->Accept(*this);
	}
	// check constants
	vector<CFEConstDeclarator*>::iterator iterC;
	for (iterC = f.m_Constants.begin();
		iterC != f.m_Constants.end();
		iterC++)
	{
		(*iterC)->Accept(*this);
	}
	// check types
	vector<CFETypedDeclarator*>::iterator iterT;
	for (iterT = f.m_Typedefs.begin();
		iterT != f.m_Typedefs.end();
		iterT++)
	{
		(*iterT)->Accept(*this);
	}
	// check type declarations
	vector<CFEConstructedType*>::iterator iterCT;
	for (iterCT = f.m_TaggedDeclarators.begin();
		iterCT != f.m_TaggedDeclarators.end();
		iterCT++)
	{
		(*iterCT)->Accept(*this);
	}
	// check interfaces
	vector<CFEInterface*>::iterator iterI;
	for (iterI = f.m_Interfaces.begin();
		iterI != f.m_Interfaces.end();
		iterI++)
	{
		(*iterI)->Accept(*this);
	}
	// check libraries
	vector<CFELibrary*>::iterator iterL;
	for (iterL = f.m_Libraries.begin();
		iterL != f.m_Libraries.end();
		iterL++)
	{
		(*iterL)->Accept(*this);
	}
}

/** \brief visit a class of type CFEFileComponent */
void CVisitor::Visit(CFEFileComponent&)
{ }

/** \brief visit a class of type CFEInterface */
void CVisitor::Visit(CFEInterface& i)
{
	// iterate attribues
	vector<CFEAttribute*>::iterator iA;
	for (iA = i.m_Attributes.begin();
		iA != i.m_Attributes.end();
		iA++)
	{
		(*iA)->Accept(*this);
	}
	// check constants
	vector<CFEConstDeclarator*>::iterator iC;
	for (iC = i.m_Constants.begin();
		iC != i.m_Constants.end();
		iC++)
	{
		(*iC)->Accept(*this);
	}
	// iterate attribute declarators
	vector<CFEAttributeDeclarator*>::iterator iAD;
	for (iAD = i.m_AttributeDeclarators.begin();
		iAD != i.m_AttributeDeclarators.end();
		iAD++)
	{
		(*iAD)->Accept(*this);
	}
	// iterate tagged declarators
	vector<CFEConstructedType*>::iterator iTD;
	for (iTD = i.m_TaggedDeclarators.begin();
		iTD != i.m_TaggedDeclarators.end();
		iTD++)
	{
		(*iTD)->Accept(*this);
	}
	// iterate operations
	vector<CFEOperation*>::iterator iO;
	for (iO = i.m_Operations.begin();
		iO != i.m_Operations.end();
		iO++)
	{
		(*iO)->Accept(*this);
	}
	// check typedefs
	vector<CFETypedDeclarator*>::iterator iT;
	for (iT = i.m_Typedefs.begin();
		iT != i.m_Typedefs.end();
		iT++)
	{
		(*iT)->Accept(*this);
	}
	// check exceptions
	for (iT = i.m_Exceptions.begin();
		iT != i.m_Exceptions.end();
		iT++)
	{
		(*iT)->Accept(*this);
	}
}

/** \brief visit a class of type CFEInterfaceComponent */
void CVisitor::Visit(CFEInterfaceComponent&)
{ }

/** \brief visit a class of type CFEConstDeclarator */
void CVisitor::Visit(CFEConstDeclarator&)
{ }

/** \brief visit a class of type CFEOperation */
void CVisitor::Visit(CFEOperation& o)
{
	// now visit attributes
	vector<CFEAttribute*>::iterator iA;
	for (iA = o.m_Attributes.begin();
		 iA != o.m_Attributes.end();
		 iA++)
	{
		(*iA)->Accept(*this);
	}
	// now visit parameters
	vector<CFETypedDeclarator*>::iterator iP;
	for (iP = o.m_Parameters.begin();
		 iP != o.m_Parameters.end();
		 iP++)
	{
		(*iP)->Accept(*this);
	}
}

/** \brief visit a class of type CFETypeSpec */
void CVisitor::Visit(CFETypeSpec&)
{ }

/** \brief visit a class of type CFEArrayType */
void CVisitor::Visit(CFEArrayType& t)
{
	if (t.GetBaseType())
		t.GetBaseType()->Accept(*this);
	if (t.GetBound())
		t.GetBound()->Accept(*this);
}

/** \brief visit a class of type CFEConstructedType */
void CVisitor::Visit(CFEConstructedType&)
{ }

/** \brief visit a class of type CFEEnumType */
void CVisitor::Visit(CFEEnumType&)
{ }

/** \brief visit a class of type CFEPipeType */
void CVisitor::Visit(CFEPipeType&)
{ }

/** \brief visit a class of type CFEStructType */
void CVisitor::Visit(CFEStructType& t)
{
	vector<CFETypedDeclarator*>::iterator iter;
	for (iter = t.m_Members.begin();
		iter != t.m_Members.end();
		iter++)
	{
		(*iter)->Accept(*this);
	}
}

/** \brief visit a class of type CFEUnionType */
void CVisitor::Visit(CFEUnionType& t)
{
	vector<CFEUnionCase*>::iterator iter;
	for (iter = t.m_UnionCases.begin();
		iter != t.m_UnionCases.end();
		iter++)
	{
		(*iter)->Accept(*this);
	}
}

/** \brief visit a class of type CFEIDLUnionType */
void CVisitor::Visit(CFEIDLUnionType&)
{ }

/** \brief visit a class of type CFESimpleType */
void CVisitor::Visit(CFESimpleType&)
{ }

/** \brief visit a class of type CFEUserDefinedType */
void CVisitor::Visit(CFEUserDefinedType&)
{ }

/** \brief visit a class of type CFETypedDeclarator */
void CVisitor::Visit(CFETypedDeclarator&)
{ }

/** \brief visit a class of type CFEAttributeDeclarator */
void CVisitor::Visit(CFEAttributeDeclarator&)
{ }

/** \brief visit a class of type CFELibrary */
void CVisitor::Visit(CFELibrary& l)
{
	// attributes
	vector<CFEAttribute*>::iterator iterA;
	for (iterA = l.m_Attributes.begin();
		iterA != l.m_Attributes.end();
		iterA++)
	{
		(*iterA)->Accept(*this);
	}
	// constants
	vector<CFEConstDeclarator*>::iterator iterC;
	for (iterC = l.m_Constants.begin();
		iterC != l.m_Constants.end();
		iterC++)
	{
		(*iterC)->Accept(*this);
	}
	// typedefs
	vector<CFETypedDeclarator*>::iterator iterT;
	for (iterT = l.m_Typedefs.begin();
		iterT != l.m_Typedefs.end();
		iterT++)
	{
		(*iterT)->Accept(*this);
	}
	// tagged decls
	vector<CFEConstructedType*>::iterator iterTa;
	for (iterTa = l.m_TaggedDeclarators.begin();
		iterTa != l.m_TaggedDeclarators.end();
		iterTa++)
	{
		(*iterTa)->Accept(*this);
	}
	// interfaces
	vector<CFEInterface*>::iterator iterI;
	for (iterI = l.m_Interfaces.begin();
		iterI != l.m_Interfaces.end();
		iterI++)
	{
		(*iterI)->Accept(*this);
	}
	// nested libraries
	vector<CFELibrary*>::iterator iterL;
	for (iterL = l.m_Libraries.begin();
		iterL != l.m_Libraries.end();
		iterL++)
	{
		(*iterL)->Accept(*this);
	}
}

/** \brief visit a class of type CFEIdentifier */
void CVisitor::Visit(CFEIdentifier&)
{ }

/** \brief visit a class of type CFEDeclarator */
void CVisitor::Visit(CFEDeclarator&)
{ }

/** \brief visit a class of type CFEArrayDeclarator */
void CVisitor::Visit(CFEArrayDeclarator&)
{ }

/** \brief visit a class of type CFEEnumDeclarator */
void CVisitor::Visit(CFEEnumDeclarator&)
{ }

/** \brief visit a class of type CFEFunctionDeclarator */
void CVisitor::Visit(CFEFunctionDeclarator&)
{ }

/** \brief visit a class of type CFEUnionCase */
void CVisitor::Visit(CFEUnionCase&)
{ }

