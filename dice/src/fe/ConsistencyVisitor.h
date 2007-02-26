/**
 *  \file    dice/src/fe/ConsistencyVisitor.h
 *  \brief   contains the declaration of the class CConsistencyVisitor
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
#ifndef __DICE_CONSISTENCYVISITOR_H__
#define __DICE_CONSISTENCYVISITOR_H__

#include "Visitor.h"
#include "Attribute-Type.h"

/** \class CConsistencyVisitor
 *  \ingroup frontend
 *  \brief class to check the consistency of the AST after parsing
 */
class CConsistencyVisitor : public CVisitor
{
public:
    /** empty default constructor */
    CConsistencyVisitor()
    { }
    /** empty destructor */
    virtual ~CConsistencyVisitor()
    { }

    /** Declare all the visit methods for all the classes in the syntax tree.
     * There are two opinions on how to write this: opinion A is that the
     * method name should be distinct for each class and opinion B is that
     * there should beonly one visit method name and use virtual functions to
     * distinguish the different class arguments.
     */
    virtual void Visit(CFEInterface&); // derives from CFEFileComponent
    virtual void Visit(CFEConstDeclarator&); // derives from CFEInterfaceComponent
    virtual void Visit(CFEOperation&); // derives from CFEInterfaceComponent
    virtual void Visit(CFETypeSpec&); // derives from CFEInterfaceComponent
    virtual void Visit(CFEArrayType&); // derives from CFETypeSpec
    virtual void Visit(CFEEnumType&); // derives from CFEConstructedType
    virtual void Visit(CFEStructType&); // derives from CFEConstructedType
    virtual void Visit(CFEUnionType&); // derives from CFEConstructedType
    virtual void Visit(CFESimpleType&); // derives from CFETypeSpec
    virtual void Visit(CFEUserDefinedType&); // derives from CFETypeSpec
    virtual void Visit(CFETypedDeclarator&); // derives from CFEInterfaceComponent
    virtual void Visit(CFEUnionCase&); // derives from CFEBase

protected:
    void CheckForOpInInterface(CFEOperation*, CFEInterface*);
    void CheckAttributesOfParams(CFEOperation&, ATTR_TYPE);
    void CheckReturnType(CFEOperation&);
    void CheckAttributeParameters(CFEOperation&, CFETypedDeclarator*,
	ATTR_TYPE, const char*);
};

#endif                // __DICE_CONSISTENCYVISITOR_H__
