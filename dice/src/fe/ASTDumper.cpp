/**
 *  \file    dice/src/fe/ASTDumper.cpp
 *  \brief   contains the implementation of the class CASTDumper
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

#include "ASTDumper.h"
#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "fe/FETypeSpec.h"
#include "fe/FEStructType.h"
#include "fe/FEUnionType.h"
#include "fe/FESimpleType.h"
#include "fe/FEConstDeclarator.h"
#include "fe/FESizeOfExpression.h"

ASTDumper::~ASTDumper()
{
	if (o.is_open())
		o.close();
}

/** \brief visit a class of type CObject */
void ASTDumper::Visit(CObject&)
{ }

/** \brief visit a class of type CFEBase */
void ASTDumper::Visit(CFEBase&)
{ }

/** \brief visit a class of type CFEAttribute */
void ASTDumper::Visit(CFEAttribute&)
{ }

/** \brief visit a class of type CFEEndPointAttribute */
void ASTDumper::Visit(CFEEndPointAttribute&)
{ }

/** \brief visit a class of type CFEExceptionAttribute */
void ASTDumper::Visit(CFEExceptionAttribute&)
{ }

/** \brief visit a class of type CFEIntAttribute */
void ASTDumper::Visit(CFEIntAttribute&)
{ }

/** \brief visit a class of type CFEIsAttribute */
void ASTDumper::Visit(CFEIsAttribute&)
{ }

/** \brief visit a class of type CFEPtrDefaultAttribute */
void ASTDumper::Visit(CFEPtrDefaultAttribute&)
{ }

/** \brief visit a class of type CFEStringAttribute */
void ASTDumper::Visit(CFEStringAttribute&)
{ }

/** \brief visit a class of type CFETypeAttribute */
void ASTDumper::Visit(CFETypeAttribute&)
{ }

/** \brief visit a class of type CFEVersionAttribute */
void ASTDumper::Visit(CFEVersionAttribute&)
{ }

/** \brief visit a class of type CFEExpression */
void ASTDumper::Visit(CFEExpression&)
{ }

/** \brief visit a class of type CFEPrimaryExpression */
void ASTDumper::Visit(CFEPrimaryExpression&)
{ }

/** \brief visit a class of type CFEUnaryExpression */
void ASTDumper::Visit(CFEUnaryExpression&)
{ }

/** \brief visit a class of type CFEBinaryExpression */
void ASTDumper::Visit(CFEBinaryExpression&)
{ }

/** \brief visit a class of type CFEConditionalExpression */
void ASTDumper::Visit(CFEConditionalExpression&)
{ }

/** \brief visit a class of type CFESizeOfExpression */
void ASTDumper::Visit(CFESizeOfExpression& e)
{
	o << "<expression sizeof >" << std::endl;
	if (e.GetSizeOfType())
	{
		o << "<type>" << std::endl;
		e.GetSizeOfType()->Accept(*this);
		o << "</type>" << std::endl;
	}
	else if (e.GetSizeOfExpression())
	{
		o << "<expression>" << std::endl;
		e.GetSizeOfExpression()->Accept(*this);
		o << "</expression>" << std::endl;
	}
	else
		o << "<string value=\"" << e.GetString() << "\" />" << std::endl;
	o << "</expression>" << std::endl;
}

/** \brief visit a class of type CFEUserDefinedExpression */
void ASTDumper::Visit(CFEUserDefinedExpression&)
{ }

/** \brief visit a class of type CFEFile */
void ASTDumper::Visit(CFEFile& f)
{
	o << "<file name=\"" << f.GetFileName() << "\" constants=\"" << f.m_Constants.size() <<
		"\" typedefs=\"" << f.m_Typedefs.size() << "\" tagged decl=\"" << f.m_TaggedDeclarators.size() <<
		"\" libs=\"" << f.m_Libraries.size() << "\" interfaces=\"" << f.m_Interfaces.size() << "\" >" <<
		std::endl;

	CVisitor::Visit(f);

	o << "</file>" << std::endl;
}

/** \brief visit a class of type CFEInterface */
void ASTDumper::Visit(CFEInterface& i)
{
    o << "<interface name=\"" << i.GetName() << "\" >" << std::endl;

	CVisitor::Visit(i);

	o << "</interface>" << std::endl;
}

/** \brief visit a class of type CFEConstDeclarator */
void ASTDumper::Visit(CFEConstDeclarator& c)
{
	o << "<const name=\"" << c.GetName() << "\" >" << std::endl;
	c.GetType()->Accept(*this);
	o << "<value>" << std::endl;
	c.GetValue()->Accept(*this);
	o << "</vale>" << std::endl;
	o << "</const name=\"" << c.GetName() << "\" >" << std::endl;
}

/** \brief visit a class of type CFEOperation */
void ASTDumper::Visit(CFEOperation& op)
{
	o << "<operation name=\"" << op.GetName() << "\" attributes=\"" <<
		op.m_Attributes.size() << "\" parameters=\"" << op.m_Parameters.size() <<
		"\" >" << std::endl;

	CVisitor::Visit(op);

	o << "</operation>" << std::endl;
}

/** \brief visit a class of type CFETypeSpec */
void ASTDumper::Visit(CFETypeSpec&)
{ }

/** \brief visit a class of type CFEConstructedType */
void ASTDumper::Visit(CFEConstructedType&)
{ }

/** \brief visit a class of type CFEEnumType */
void ASTDumper::Visit(CFEEnumType&)
{ }

/** \brief visit a class of type CFEPipeType */
void ASTDumper::Visit(CFEPipeType&)
{ }

/** \brief visit a class of type CFEStructType */
void ASTDumper::Visit(CFEStructType& s)
{
	o << "<struct tag=\"" << s.GetTag() << "\" members=\"" <<
		s.m_Members.size() << "\" >" << std::endl;

	CVisitor::Visit(s);

	o << "</struct>" << std::endl;
}

/** \brief visit a class of type CFEUnionType */
void ASTDumper::Visit(CFEUnionType& u)
{
	o << "<union tag=\"" << u.GetTag() << "\" members=\"" <<
		u.m_UnionCases.size() << "\" >" << std::endl;

	CVisitor::Visit(u);

	o << "</union>" << std::endl;
}

/** \brief visit a class of type CFEIDLUnionType */
void ASTDumper::Visit(CFEIDLUnionType&)
{ }

/** \brief visit a class of type CFESimpleType */
void ASTDumper::Visit(CFESimpleType& t)
{
	o << "<type type=\"" << t.GetType() << "\" size=\"" <<
		t.GetSize() << "\" unsigned=\"" << t.IsUnsigned() <<
		"\" >" << std::endl;
}

/** \brief visit a class of type CFEUserDefinedType */
void ASTDumper::Visit(CFEUserDefinedType&)
{ }

/** \brief visit a class of type CFETypedDeclarator */
void ASTDumper::Visit(CFETypedDeclarator& t)
{
	std::string type;
	switch (t.GetTypedDeclType())
	{
	case TYPEDECL_VOID:
		type = "void";
		break;
	case TYPEDECL_EXCEPTION:
		type = "exception";
		break;
	case TYPEDECL_PARAM:
		type = "parameter";
		break;
	case TYPEDECL_FIELD:
		type = "field (member)";
		break;
	case TYPEDECL_TYPEDEF:
		type = "typedef";
		break;
	case TYPEDECL_MSGBUF:
		type = "msgbuf";
		break;
	case TYPEDECL_ATTRIBUTE:
		type = "attribute";
		break;
	default:
		type = "none";
		break;
	}
	o << "<typed declarator type=\"" << type << "\" attributes=\"" <<
		t.m_Attributes.size() << "\" declarators=\"" << t.m_Declarators.size() <<
		"\" >" << std::endl;
	if (t.GetType())
	{
		t.GetType()->Accept(*this);
	}
	vector<CFEAttribute*>::iterator iA;
	for (iA = t.m_Attributes.begin();
		iA != t.m_Attributes.end();
		iA++)
	{
		o << "<attribute>" << std::endl;
		(*iA)->Accept(*this);
		o << "</attribute>" << std::endl;
	}
	vector<CFEDeclarator*>::iterator iD;
	for (iD = t.m_Declarators.begin();
		iD != t.m_Declarators.end();
		iD++)
	{
		(*iD)->Accept(*this);
	}
	o << "</typed declarator type=\"" << type << "\" >" << std::endl;
}

/** \brief visit a class of type CFEAttributeDeclarator */
void ASTDumper::Visit(CFEAttributeDeclarator&)
{ }

/** \brief visit a class of type CFELibrary */
void ASTDumper::Visit(CFELibrary& l)
{
    o << "<library name=\"" << l.GetName() << "\" >" << std::endl;

	CVisitor::Visit(l);

	o << "</library>" << std::endl;
}

/** \brief visit a class of type CFEIdentifier */
void ASTDumper::Visit(CFEIdentifier&)
{ }

/** \brief visit a class of type CFEDeclarator */
void ASTDumper::Visit(CFEDeclarator& d)
{
	o << "<declarator stars=\"" << d.GetStars() << "\" name=\"" <<
		d.GetName() << "\" bitfields=\"" << d.GetBitfields() << "\" type=\"" <<
		d.GetType() << "\" >" << std::endl;
}

/** \brief visit a class of type CFEArrayDeclarator */
void ASTDumper::Visit(CFEArrayDeclarator&)
{ }

/** \brief visit a class of type CFEEnumDeclarator */
void ASTDumper::Visit(CFEEnumDeclarator&)
{ }

/** \brief visit a class of type CFEFunctionDeclarator */
void ASTDumper::Visit(CFEFunctionDeclarator&)
{ }

/** \brief visit a class of type CFEUnionCase */
void ASTDumper::Visit(CFEUnionCase& u)
{
	o << "<union case default=\"" << u.IsDefault() << "\" labels=\"" <<
		u.m_UnionCaseLabelList.size() << " >" << std::endl;

	vector<CFEExpression*>::iterator i;
	for (i = u.m_UnionCaseLabelList.begin();
		i != u.m_UnionCaseLabelList.end();
		i++)
	{
		o << "<union label>" << std::endl;
		(*i)->Accept(*this);
		o << "</union label>" << std::endl;
	}

	o << "<union arm>" << std::endl;
	u.GetUnionArm()->Accept(*this);
	o << "</union arm>" << std::endl;

	o << "</union case>" << std::endl;
}

