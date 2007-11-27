/**
 *  \file    dice/src/fe/ASTDumper.h
 *  \brief   contains the declaration of the class CASTDumper
 *
 *  \date    09/15/2006
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006
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

/** preprocessing symbol to check header file */
#ifndef __DICE_ASTDUMPER_H__
#define __DICE_ASTDUMPER_H__

#include "Visitor.h"
#include <fstream>

/** \class ASTDumper
 *  \ingroup frontend
 *  \brief class to check the consistency of the AST after parsing
 */
class ASTDumper : public CVisitor
{
	/** \var std::ofstream o
	 *  \brief output stream
	 */
    std::ofstream o;
public:
    /** empty default constructor */
    ASTDumper(std::string fName)
	: o(fName.c_str())
    { }
    /** empty destructor */
    virtual ~ASTDumper();

    virtual void Visit(CObject&);
    virtual void Visit(CFEBase&); // derives from CObject
    virtual void Visit(CFEAttribute&); // derives from CFEBase
    virtual void Visit(CFEEndPointAttribute&); // derives from CFEAttribute
    virtual void Visit(CFEExceptionAttribute&); // derives from CFEAttribute
    virtual void Visit(CFEIntAttribute&); // derives from CFEAttribute
    virtual void Visit(CFEIsAttribute&); // derives from CFEAttribute
    virtual void Visit(CFEPtrDefaultAttribute&); // derives from CFEAttribute
    virtual void Visit(CFEStringAttribute&); // derives from CFEAttribute
    virtual void Visit(CFETypeAttribute&); // derives from CFEAttribute
    virtual void Visit(CFEVersionAttribute&); // derives from CFEAttribute
    virtual void Visit(CFEExpression&); // derives from CFEBase
    virtual void Visit(CFEPrimaryExpression&); // derives from CFEExpression
    virtual void Visit(CFEUnaryExpression&); // derives from CFEPrimaryExpression
    virtual void Visit(CFEBinaryExpression&); // derives from CFEUnaryExpression
    virtual void Visit(CFEConditionalExpression&); // derives from CFEBinaryExpression
    virtual void Visit(CFESizeOfExpression&); // derives from CFEExpression
    virtual void Visit(CFEUserDefinedExpression&); // derives from CFEExpression
    virtual void Visit(CFEFile&); // derives from CFEBase
    virtual void Visit(CFEInterface&); // derives from CFEFileComponent
    virtual void Visit(CFEConstDeclarator&); // derives from CFEInterfaceComponent
    virtual void Visit(CFEOperation&); // derives from CFEInterfaceComponent
    virtual void Visit(CFETypeSpec&); // derives from CFEInterfaceComponent
    virtual void Visit(CFEConstructedType&); // derives from CFETypeSpec
    virtual void Visit(CFEEnumType&); // derives from CFEConstructedType
    virtual void Visit(CFEPipeType&); // derives from CFEConstructedType
    virtual void Visit(CFEStructType&); // derives from CFEConstructedType
    virtual void Visit(CFEUnionType&); // derives from CFEConstructedType
    virtual void Visit(CFEIDLUnionType&); // derives from CFEUnionType
    virtual void Visit(CFESimpleType&); // derives from CFETypeSpec
    virtual void Visit(CFEUserDefinedType&); // derives from CFETypeSpec
    virtual void Visit(CFETypedDeclarator&); // derives from CFEInterfaceComponent
    virtual void Visit(CFEAttributeDeclarator&); // derives from CFETypedDeclarator
    virtual void Visit(CFELibrary&); // derives from CFEFileComponent
    virtual void Visit(CFEIdentifier&); // derives from CFEBase
    virtual void Visit(CFEDeclarator&); // derives from CFEIdentifier
    virtual void Visit(CFEArrayDeclarator&); // derives from CFEDeclarator
    virtual void Visit(CFEEnumDeclarator&); // derives from CFEDeclarator
    virtual void Visit(CFEFunctionDeclarator&); // derives from CFEDeclarator
    virtual void Visit(CFEUnionCase&); // derives from CFEBase
};

#endif                // __DICE_CONSISTENCYVISITOR_H__
