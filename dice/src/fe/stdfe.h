/**
 *	\file	dice/src/fe/stdfe.h 
 *	\brief	contains a collection of header files of the FE classes
 *
 *	\date	01/31/2001
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

#include "fe/FEBase.h"						// defines base class 
#include "fe/FEIdentifier.h"				// a basic name class

#include "fe/FELibrary.h"				// defines the library
#include "fe/FEInterface.h"				// defines the interface

#include "fe/FEExpression.h"				// the expression base class (simple expressions)
#include "fe/FEUserDefinedExpression.h"	// an expression which is an identifier
#include "fe/FEPrimaryExpression.h"		// simple expression (int, char, string, ...)
#include "fe/FEUnaryExpression.h"			// expression with an operator and one operand
#include "fe/FEBinaryExpression.h"			// expression with two operands and an operator
#include "fe/FEConditionalExpression.h"	// expression with condition and two branches

#include "fe/FEInterfaceComponent.h"		// everything inside the body
#include "fe/FEFileComponent.h"				// everything inside a file
#include "fe/FEFile.h"

#include "fe/FETypeSpec.h"					// the type base class
#include "fe/FESimpleType.h"				// the basic types (int, byte, ...)
#include "fe/FEUserDefinedType.h"			// types defines by the user and represented by a name
#include "fe/FEConstructedType.h"			// everything which is more complex
#include "fe/FEPipeType.h"					// a piped type
#include "fe/FEEnumType.h"					// an enumeration
#include "fe/FEStructType.h"				// a struct
#include "fe/FETaggedStructType.h"			// a tagged struct
#include "fe/FEUnionCase.h"				// a branch of a union statement
#include "fe/FEUnionType.h"				// a union
#include "fe/FETaggedUnionType.h"			// a tagged union
#include "fe/FETaggedEnumType.h"			// a tagged enum type (CORBA)
#include "fe/FEArrayType.h"				// an extra sequence type (CORBA)

#include "fe/FEConstDeclarator.h"			// a definition of a const value
#include "fe/FETypedDeclarator.h"			// a declarator consiting of type, attributes, and declarators
#include "fe/FEOperation.h"				// an operation

#include "fe/FEDeclarator.h"				// a var name representation
#include "fe/FEFunctionDeclarator.h"
#include "fe/FEArrayDeclarator.h"			// an array definition
#include "fe/FEEnumDeclarator.h"

#include "fe/FEAttribute.h"				// the attribute base class (every attribute without additional crap)
#include "fe/FEExceptionAttribute.h"		// the exception attribute
#include "fe/FEIsAttribute.h"				// every attribute which has an "IS" in it's name
#include "fe/FEVersionAttribute.h"			// a version
#include "fe/FETypeAttribute.h"			// contains a type
#include "fe/FEStringAttribute.h"			// UUID, HELPCONTEXT, HELPFILE
#include "fe/FEPtrDefaultAttribute.h"		// the ptr default value
#include "fe/FEEndPointAttribute.h"		// end point of an communication
#include "fe/FEIntAttribute.h"				// LCID, HELPCONTEXT

