%{
/*
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

#include <stdio.h>
#include <stdarg.h>

#include "defines.h"
#include "Vector.h"
#include "fe/stdfe.h"
#include "Compiler.h"

void corbaerror(char *);
void corbaerror2(char *fmt, ...);
void corbawarning(char *fmt, ...);
int corbalex();

#define YYDEBUG	1

// collection for elements
extern CFEFile *pCurFile;

// error stuff
extern int errcount;
extern int warningcount;
extern int erroccured;

// #include/import special treatment
extern int nIncludeLevelCORBA;
extern String sInFileName;
extern int nLineNbCORBA;

int nParseErrorCORBA = 0;

%}

%union {
  char*			_id;
  String*		_str;
  char			_chr;
  long			_int;
  long double	_flt;

  CFEInterface*			_interface;
  CFELibrary*			_library;
  CFEExpression*		_expression;
  CFETypeSpec*			_type_spec;
  CFEIdentifier*		_identifier;
  CFETypedDeclarator*	_typed_decl;
  CFEDeclarator*		_decl;
  CFEAttribute*			_attribute;
  CFEOperation*			_operation;
  CFETaggedEnumType*	_enum_type;
  CFETaggedUnionType*	_union_type;
  CFEStructType*		_struct_type;
  CFEUnionCase*			_union_case;
  CFESimpleType*		_simple_type;
  CFEConstructedType*	_constr_type;
  CFEInterfaceComponent*	_i_component;
  CFEConstDeclarator*	_const_decl;
  CFEFileComponent*		_file_component;
  CFEArrayDeclarator*	_array_decl;

  Vector*		_collection;
}

%token COMMA
%token SEMICOLON
%token COLON
%token SCOPE
%token LPAREN
%token RPAREN
%token LBRACE
%token RBRACE
%token LBRACKET
%token RBRACKET
%token LT
%token GT
%token IS
%token BITOR
%token BITXOR
%token BITAND
%token BITNOT
%token RSHIFT
%token LSHIFT
%token PLUS
%token MINUS
%token MUL
%token DIV
%token MOD
%token ABSTRACT
%token ANY
%token ATTRIBUTE
%token BOOLEAN
%token CASE
%token CHAR
%token CONST
%token CONTEXT
%token CUSTOM
%token DEFAULT
%token DOUBLE
%token ENUM
%token EXCEPTION
%token FALSE
%token FACTORY
%token FIXED
%token FLOAT
%token IN
%token INOUT
%token INTERFACE
%token MODULE
%token NATIVE
%token LONG
%token OBJECT
%token OCTET
%token ONEWAY
%token OUT
%token PRIVATE
%token PUBLIC
%token RAISES
%token READONLY
%token SEQUENCE
%token SHORT
%token STRING
%token STRUCT
%token SUPPORTS
%token SWITCH
%token TRUE
%token TRUNCABLE
%token TYPEDEF
%token UNION
%token UNSIGNED
%token VALUEBASE
%token VALUETYPE
%token VOID
%token WCHAR
%token WSTRING
%token FPAGE
%token REFSTRING

%token	<_id>		ID
%token	<_int>		LIT_INT
%token	<_str>		LIT_STR
%token	<_str>		LIT_WSTR
%token	<_chr>		LIT_CHAR
%token	<_chr>		LIT_WCHAR
%token	<_flt>		LIT_FLOAT
%token	<_str>		FILENAME

%type	<_expression>		add_expr
%type	<_expression>		and_expr
%type	<_simple_type>		any_type
%type	<_array_decl>		array_declarator
%type	<_collection>		attr_dcl
%type	<_type_spec>		base_type_spec
%type	<_type_spec>		boolean_type
%type	<_union_case>		case
%type	<_expression>		case_label
%type	<_collection>		case_label_list
%type	<_type_spec>		char_type
%type	<_const_decl>		const_dcl
%type	<_expression>		const_exp
%type	<_type_spec>		const_type
%type	<_constr_type>		constr_type_spec
%type	<_decl>				declarator
%type	<_collection>		declarators
%type	<_typed_decl>		element_spec
%type	<_collection>		enumerator_list
%type	<_enum_type>		enum_type
%type	<_i_component>		except_dcl
%type	<_i_component>		export
%type	<_collection>		export_list
%type	<_type_spec>		fixed_pt_const_type
%type	<_type_spec>		fixed_pt_type
%type	<_simple_type>		floating_pt_type
%type	<_typed_decl>		init_param_decl
%type	<_collection>		init_param_decls
%type	<_type_spec>		integer_type
%type	<_interface>		interface
%type	<_interface>		interface_dcl
%type	<_collection>		interface_inheritance_spec
%type	<_identifier>		interface_name
%type	<_collection>		interface_name_list
%type	<_typed_decl>		member
%type	<_collection>		member_list
%type	<_library>			module
%type	<_file_component>	module_element
%type	<_collection>		module_element_list
%type	<_expression>		mult_expr
%type	<_simple_type>		object_type
%type	<_simple_type>		octet_type
%type	<_attribute>		op_attribute
%type	<_operation>		op_dcl
%type	<_type_spec>		op_type_spec
%type	<_expression>		or_expr
%type	<_collection>		param_attribute
%type	<_typed_decl>		param_dcl
%type	<_collection>		param_dcl_list
%type	<_type_spec>		param_type_spec
%type	<_collection>		parameter_dcls
%type	<_expression>		positive_int_const
%type	<_simple_type>		predefined_type
%type	<_expression>		primary_expr
%type	<_collection>		raises_expr
%type	<_identifier>		scoped_name
%type	<_collection>		scoped_name_list
%type	<_type_spec>		sequence_type
%type	<_expression>		shift_expr
%type	<_decl>				simple_declarator
%type	<_collection>		simple_declarator_list
%type	<_type_spec>		simple_type_spec
%type	<_collection>		string_literal_list
%type	<_type_spec>		string_type
%type	<_struct_type>		struct_type
%type	<_collection>		switch_body
%type	<_type_spec>		switch_type_spec
%type	<_type_spec>		template_type_spec
%type	<_typed_decl>		type_dcl
%type	<_type_spec>		type_spec
%type	<_expression>		unary_expr
%type	<_union_type>		union_type
%type	<_type_spec>		value_base_type
%type	<_identifier>		value_name
%type	<_collection>		value_name_list
%type	<_simple_type>		wide_char_type
%type	<_type_spec>		wide_string_type
%type	<_expression>		xor_expr

%%

specification	:
	  definition_list
	;

definition_list :
	  definition_list definition
	| definition
	;

definition :
	  type_dcl semicolon
	{
		if (pCurFile == NULL)
		{
			CCompiler::GccError(NULL, 0, "Fatal Error: furrent file vanished (typedef)");
			YYABORT;
		}
		else
			pCurFile->AddTypedef($1);
	}
	| const_dcl semicolon
	{
		if (pCurFile == NULL)
		{
			CCompiler::GccError(NULL, 0, "Fatal Error: furrent file vanished (const)");
			YYABORT;
		}
		else
			pCurFile->AddConstant($1);
	}
	| except_dcl semicolon
	{
	}
	| interface semicolon
	{
		if ($1 != NULL) {
			if (pCurFile == NULL)
			{
				CCompiler::GccError(NULL, 0, "Fatal Error: furrent file vanished (interface)");
				YYABORT;
			}
			else
				pCurFile->AddInterface($1);
		}
		// else: was forward_dcl
	}
	| module semicolon
	{
		if (pCurFile == NULL)
		{
			CCompiler::GccError(NULL, 0, "Fatal Error: furrent file vanished (module)");
			YYABORT;
		}
		else
			pCurFile->AddLibrary($1);
	}
	| value semicolon
	;

module :
	  MODULE ID
	{
		// check if we can find a library with this name
		CFEFile *pRoot = pCurFile->GetRoot();
		ASSERT(pRoot);
		$<_library>$ = pRoot->FindLibrary($2);
	} LBRACE module_element_list rbrace
	{
		CFEAttribute *tmp = new CFEVersionAttribute(0,0);
		$$ = new CFELibrary(String($2), new Vector(RUNTIME_CLASS(CFEAttribute), 1, tmp), $5);
		$5->SetParentOfElements($$);
		tmp->SetParent($$);
		tmp->SetSourceLine(nLineNbCORBA);
		if ($<_library>3 != NULL)
			$$->AddSameLibrary($<_library>3);
	}
	;

module_element_list :
	  module_element_list module_element
	{
	        if ($2)
		        $1->Add($2);
		$$ = $1;
	}
	| module_element
	{
	        if ($1 != NULL)
		        $$ = new Vector(RUNTIME_CLASS(CFEFileComponent), 1, $1);
                else
		        $$ = new Vector(RUNTIME_CLASS(CFEFileComponent));
	}
	;

module_element :
	  type_dcl semicolon
	{ $$ = $1; }
	| const_dcl semicolon
	{ $$ = $1; }
	| except_dcl semicolon
	{ $$ = $1; }
	| interface semicolon
	{ $$ = $1; }
	| module semicolon
	{ $$ = $1; }
	| value semicolon
	{
	}
	;

interface :
	  interface_dcl
	{ $$ = $1; }
	| forward_dcl
	{ $$ = NULL; }
	;

interface_dcl	:
	  ABSTRACT INTERFACE ID interface_inheritance_spec
	{
		// test base interfaces
		CFEFile *pRoot = pCurFile->GetRoot();
		ASSERT(pRoot);
		VectorElement *pCurrent;
		CFEIdentifier *pId;
		for (pCurrent = $4->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL) {
				pId = (CFEIdentifier*)(pCurrent->GetElement());
				if (pRoot->FindInterface(pId->GetName()) == NULL)
				{
					corbaerror2("Couldn't find base interface name \"%s\".", (const char*)pId->GetName());
					YYABORT;
				}
			}
		}
		if (pRoot->FindInterface($3) != NULL)
		{
			corbaerror2("Interface name \"%s\" already exists", (const char*)$3);
			YYABORT;
		}
	} LBRACE export_list rbrace
	{
		if ($7 == NULL) {
			corbaerror2("no empty interface specification supported.\n");
			YYABORT;
		}
		CFEAttribute *pAttr = new CFEAttribute(ATTR_ABSTRACT);
		pAttr->SetSourceLine(nLineNbCORBA);
		$$ = new CFEInterface(new Vector(RUNTIME_CLASS(CFEAttribute), 1, pAttr), String($3), $4, $7);
		// set parent relationship
		pAttr->SetParent($$);
		$7->SetParentOfElements($$);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| ABSTRACT INTERFACE ID
	{
		CFEFile *pRoot = pCurFile->GetRoot();
		ASSERT(pRoot);
		if (pRoot->FindInterface($3) != NULL)
		{
			// name already exists
			corbaerror2("Interface name \"%s\" already exists", (const char*)$3);
			YYABORT;
		}
	} LBRACE export_list rbrace
	{
		if ($6 == NULL) {
			corbaerror2("no empty interface specification supported.\n");
			YYABORT;
		}
		CFEAttribute *pAttr = new CFEAttribute(ATTR_ABSTRACT);
		pAttr->SetSourceLine(nLineNbCORBA);
		$$ = new CFEInterface(new Vector(RUNTIME_CLASS(CFEAttribute), 1, pAttr), String($3), NULL, $6);
		// set parent relationship
		pAttr->SetParent($$);
		$6->SetParentOfElements($$);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| INTERFACE ID interface_inheritance_spec
	{
		// test base interfaces
		CFEFile *pRoot = pCurFile->GetRoot();
		ASSERT(pRoot);
		VectorElement *pCurrent;
		CFEIdentifier *pId;
		for (pCurrent = $3->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext())
        {
			if (pCurrent->GetElement())
            {
				pId = (CFEIdentifier*)(pCurrent->GetElement());
				if (!(pRoot->FindInterface(pId->GetName())))
				{
					corbaerror2("Couldn't find base interface name \"%s\".", (const char*)pId->GetName());
					YYABORT;
				}
			}
		}
		if (pRoot->FindInterface($2) != NULL)
		{
			// name already exists
			corbaerror2("Interface name \"%s\" already exists", (const char*)$2);
			YYABORT;
		}
	} LBRACE export_list rbrace
	{
		if ($6 == NULL) {
			corbaerror2("no empty interface specification supported.\n");
			YYABORT;
		}
		CFEAttribute *pAttr = new CFEVersionAttribute(0,0);
		pAttr->SetSourceLine(nLineNbCORBA);
		$$ = new CFEInterface(new Vector(RUNTIME_CLASS(CFEAttribute), 1, pAttr), String($2), $3, $6);
		// set parent relationship
		pAttr->SetParent($$);
		$6->SetParentOfElements($$);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| INTERFACE ID
	{
		CFEFile *pRoot = pCurFile->GetRoot();
		ASSERT(pRoot);
		if (pRoot->FindInterface($2) != NULL)
		{
			// name already exists
			corbaerror2("Interface name \"%s\" already exists", (const char*)$2);
			YYABORT;
		}
	} LBRACE export_list rbrace
	{
		if ($5 == NULL) {
			corbaerror2("no empty interface specification supported.\n");
			YYABORT;
		}
		CFEAttribute *pAttr = new CFEVersionAttribute(0,0);
		pAttr->SetSourceLine(nLineNbCORBA);
		$$ = new CFEInterface(new Vector(RUNTIME_CLASS(CFEAttribute), 1, pAttr), String($2), NULL, $5);
		// set parent relationship
		pAttr->SetParent($$);
		$5->SetParentOfElements($$);
		$$->SetSourceLine(nLineNbCORBA);
	}
	;

forward_dcl :
	  ABSTRACT INTERFACE ID
	| INTERFACE ID
	;

export_list :
	  export_list export
	{
	        if ($2)
		        $1->Add($2);
		$$ = $1;
	}
    | export_list attr_dcl
    {
        if ($2)
            $1->Add($2);
        $$ = $1;
    }
	| export
	{
		$$ = new Vector(RUNTIME_CLASS(CFEInterfaceComponent), 1, $1);
	}
    | attr_dcl
    {
        $$ = $1;
    }
	;

export :
	  type_dcl semicolon
	{ $$ = $1; }
	| const_dcl semicolon
	{ $$ = $1; }
	| except_dcl semicolon
	{ $$ = $1; }
	| op_dcl semicolon
	{ $$ = $1; }
	;

interface_inheritance_spec :
	  COLON interface_name_list
	{
		$$ = $2;
	}
	;

interface_name_list :
	  interface_name_list COMMA interface_name
	{
	        if ($3)
		        $1->Add($3);
		$$ = $1;
	}
	| interface_name
	{
		$$ = new Vector(RUNTIME_CLASS(CFEIdentifier), 1, $1);
	}
	;

interface_name :
	  scoped_name
	{ $$ = $1; }
	;

scoped_name :
	  ID
	{
		$$ = new CFEIdentifier($1);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| SCOPE ID
	{
		$$ = new CFEIdentifier($2);
		$$->Prefix(String("::"));
		$$->SetSourceLine(nLineNbCORBA);
	}
	| scoped_name SCOPE ID
	{
		$$ = $1;
		$$->Suffix(String("::"));
		$$->Suffix(String($3));
	}
	;

value :
	  value_dcl
	| value_abs_dcl
	| value_box_dcl
	| value_forward_dcl
	;

value_forward_dcl :
	  ABSTRACT VALUETYPE ID
	| VALUETYPE ID
	;

value_box_dcl :
	  VALUETYPE ID type_spec
	;

value_abs_dcl :
	  ABSTRACT VALUETYPE ID value_inheritance_spec LBRACE export_list rbrace
	| ABSTRACT VALUETYPE ID value_inheritance_spec LBRACE RBRACE
	| ABSTRACT VALUETYPE ID LBRACE export_list rbrace
	| ABSTRACT VALUETYPE ID LBRACE RBRACE
	;

value_dcl :
	  value_header LBRACE value_element_list rbrace
	| value_header LBRACE RBRACE
	;

value_element_list :
	  value_element_list value_element
	| value_element
	;

value_header :
	  CUSTOM VALUETYPE ID value_inheritance_spec
	| CUSTOM VALUETYPE ID
	| VALUETYPE ID value_inheritance_spec
	| VALUETYPE ID
	;

value_inheritance_spec :
	  COLON TRUNCABLE value_name_list SUPPORTS interface_name_list
	| COLON value_name_list SUPPORTS interface_name_list
	| SUPPORTS interface_name_list
	| COLON TRUNCABLE value_name_list
	| COLON value_name_list
	;

value_name_list :
	  value_name_list COMMA value_name
	{
	        if ($3)
		        $1->Add($3);
		$$ = $1;
	}
	| value_name
	{
		$$ = new Vector(RUNTIME_CLASS(CFEIdentifier), 1, $1);
	}
	;

value_name :
	  scoped_name
	{ $$ = $1; }
	;

value_element	:
	  export
	{ }
	| state_member
	| init_dcl
	;

state_member :
	  PUBLIC type_spec declarators semicolon
	| PRIVATE type_spec declarators semicolon
	;

init_dcl :
	  FACTORY ID LPAREN init_param_decls rparen semicolon
	| FACTORY ID LPAREN RPAREN semicolon
	;

init_param_decls :
	  init_param_decls COMMA init_param_decl
	{
	        if ($3)
		        $1->Add($3);
		$$ = $1;
	}
	| init_param_decl
	{
		$$ = new Vector(RUNTIME_CLASS(CFETypedDeclarator), 1, $1);
	}
	;

init_param_decl :
	  IN param_type_spec simple_declarator
	{
		CFEAttribute *tmp = new CFEAttribute(ATTR_IN);
		tmp->SetSourceLine(nLineNbCORBA);
		$$ = new CFETypedDeclarator(TYPEDECL_PARAM, $2, new Vector(RUNTIME_CLASS(CFEDeclarator), 1, $3), new Vector(RUNTIME_CLASS(CFEAttribute), 1, tmp));
		// check if OUT and reference
		if ($$->FindAttribute(ATTR_OUT) != NULL) {
			// because CORBA knows no pointers, we have to add a reference for out parameters
			$3->SetStars(1);
		}
		// set parent relationship
		$2->SetParent($$);
		$3->SetParent($$);
		tmp->SetParent($$);
		$$->SetSourceLine(nLineNbCORBA);
	}
	;

const_dcl :
	  CONST const_type ID IS const_exp
	{
		CFETypeSpec *pType = $2;
	        while (pType && (pType->GetType() == TYPE_USER_DEFINED))
		{
		    String sTypeName = ((CFEUserDefinedType*)pType)->GetName();
		    CFETypedDeclarator *pTypedef = pCurFile->FindUserDefinedType(sTypeName);
		    if (!pTypedef)
		        corbaerror2("Cannot find type for \"%s\".\n", (const char*)sTypeName);
		    pType = pTypedef->GetType();
		}
		if (!( $5->IsOfType(pType->GetType()) )) {
			corbaerror2("Const type of \"%s\" does not match with expression.\n",$3);
			YYABORT;
		}
		$$ = new CFEConstDeclarator($2, String($3), $5);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$2->SetParent($$);
		$5->SetParent($$);
	}
	;

const_type :
	  integer_type
	{ $$ = $1; }
	| char_type
	{ $$ = $1; }
	| wide_char_type
	{ $$ = $1; }
	| boolean_type
	{ $$ = $1; }
	| floating_pt_type
	{ $$ = $1; }
	| string_type
	{ $$ = $1; }
	| wide_string_type
	{ $$ = $1; }
	| fixed_pt_const_type
	{ $$ = $1; }
	| scoped_name
	{
		$$ = new CFEUserDefinedType($1->GetName());
		$$->SetSourceLine(nLineNbCORBA);
	}
	| octet_type
	{ $$ = $1; }
	;

const_exp :
	  or_expr
	{ $$ = $1; }
	;

or_expr :
	  xor_expr
	{ $$ = $1; }
	| or_expr BITOR xor_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITOR, $3);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	;

xor_expr :
	  and_expr
	{ $$ = $1; }
	| xor_expr BITXOR and_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITXOR, $3);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	;

and_expr :
	  shift_expr
	{ $$ = $1; }
	| and_expr BITAND shift_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITAND, $3);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	;

shift_expr :
	  add_expr
	{ $$ = $1; }
	| shift_expr RSHIFT add_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_RSHIFT, $3);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	| shift_expr LSHIFT add_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LSHIFT, $3);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	;

add_expr :
	  mult_expr
	{ $$ = $1; }
	| add_expr PLUS mult_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_PLUS, $3);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	| add_expr MINUS mult_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MINUS, $3);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	;

mult_expr :
	  unary_expr
	{ $$ = $1; }
	| mult_expr MUL unary_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MUL, $3);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	| mult_expr DIV unary_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_DIV, $3);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	| mult_expr MOD unary_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MOD, $3);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	;

unary_expr :
	  MINUS primary_expr
	{
		$$ = new CFEUnaryExpression(EXPR_UNARY, EXPR_SMINUS, $2);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$2->SetParent($$);
	}
	| PLUS primary_expr
	{
		$$ = new CFEUnaryExpression(EXPR_UNARY, EXPR_SPLUS, $2);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$2->SetParent($$);
	}
	| BITNOT primary_expr
	{
		$$ = new CFEUnaryExpression(EXPR_UNARY, EXPR_TILDE, $2);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$2->SetParent($$);
	}
	| primary_expr
	{ $$ = $1; }
	;

primary_expr :
	  scoped_name
	{
		$$ = new CFEUserDefinedExpression($1->GetName());
		$$->SetSourceLine(nLineNbCORBA);
	}
	| LIT_INT
	{
		$$ = new CFEPrimaryExpression(EXPR_INT, $1);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| LIT_STR
	{
		$$ = new CFEExpression(EXPR_STRING, *$1);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| LIT_WSTR
	{
		$$ = new CFEExpression(EXPR_STRING, *$1);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| LIT_CHAR
	{
		$$ = new CFEExpression(EXPR_CHAR, $1);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| LIT_WCHAR
	{
		$$ = new CFEExpression(EXPR_CHAR, $1);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| LIT_FLOAT
	{
		$$ = new CFEPrimaryExpression(EXPR_FLOAT, $1);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| TRUE
	{
		$$ = new CFEExpression(EXPR_TRUE);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| FALSE
	{
		$$ = new CFEExpression(EXPR_FALSE);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| LPAREN const_exp rparen
	{
		$$ = new CFEPrimaryExpression(EXPR_PAREN, $2);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$2->SetParent($$);
	}
	;

positive_int_const :
	  const_exp
	{ $$ = $1; }
	;

type_dcl :
	  TYPEDEF type_spec declarators
	{
		// check if type_names already exist
		CFEFile *pRoot = pCurFile->GetRoot();
		ASSERT(pRoot);
		VectorElement *pIter;
		CFEDeclarator *pDecl;
		for (pIter = $3->GetFirst(); pIter != NULL; pIter = pIter->GetNext()) {
			pDecl = (CFEDeclarator*)(pIter->GetElement());
			if (pDecl != NULL) {
				if (pDecl->GetName() != NULL) {
					if (pRoot->FindUserDefinedType(pDecl->GetName()) != NULL)
					{
						corbaerror2("\"%s\" has already been defined as type.\n",(const char*)pDecl->GetName());
						YYABORT;
					}
				}
			}
		}
		$$ = new CFETypedDeclarator(TYPEDECL_TYPEDEF, $2, $3);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$2->SetParent($$);
		$3->SetParentOfElements($$);
	}
	| struct_type
	{
		// extract the name
		if ($1->IsKindOf(RUNTIME_CLASS(CFETaggedStructType))) {
			String sTag = ((CFETaggedStructType*)$1)->GetTag();
			CFEDeclarator *pDecl =  new CFEDeclarator(DECL_IDENTIFIER, "_struct_" + sTag);
			pDecl->SetSourceLine(nLineNbCORBA);
			// modify tag, so compiler won't warn
			Vector *pDecls = new Vector(RUNTIME_CLASS(CFEDeclarator), 1, pDecl);
			// create a new typed decl
			$$ = new CFETypedDeclarator(TYPEDECL_TYPEDEF, $1, pDecls);
			$$->SetSourceLine(nLineNbCORBA);
			$1->SetParent($$);
			pDecl->SetParent($$);
			// return
		} else {
			corbaerror2("A struct without any declarators is not permitted\n");
			YYABORT;
		}
	}
	| union_type
	{
		// extract the name
		// create a new typed decl
		// return
	}
	| enum_type
	{
		// extract the name
		// create a new typed decl
		// return
	}
	| NATIVE simple_declarator
	{
		// use the name as type name
		//$$ = new CFEUserDefinedType($2);
		#warning "native type translation to typedef?"
	}
	;

type_spec :
	  simple_type_spec
	{ $$ = $1; }
	| constr_type_spec
	{ $$ = $1; }
	;

simple_type_spec :
	  base_type_spec
	{ $$ = $1; }
	| template_type_spec
	{ $$ = $1; }
	| scoped_name
	{
		$$ = new CFEUserDefinedType($1->GetName());
		$$->SetSourceLine(nLineNbCORBA);
	}
	;

base_type_spec :
	  floating_pt_type
	{ $$ = $1; }
	| integer_type
	{ $$ = $1; }
	| char_type
	{ $$ = $1; }
	| wide_char_type
	{ $$ = $1; }
	| boolean_type
	{ $$ = $1; }
	| octet_type
	{ $$ = $1; }
	| any_type
	{ $$ = $1; }
	| object_type
	{ $$ = $1; }
	| value_base_type
	{ $$ = $1; }
	;

template_type_spec :
	  sequence_type
	{ $$ = $1; }
	| string_type
	{ $$ = $1; }
	| wide_string_type
	{ $$ = $1; }
	| fixed_pt_type
	{ $$ = $1; }
	;

constr_type_spec :
	  struct_type
	{ $$ = $1; }
	| union_type
	{ $$ = $1; }
	| enum_type
	{ $$ = $1; }
	;

declarators :
	  declarators COMMA declarator
	{
	        if ($3)
		        $1->Add($3);
		$$ = $1;
	}
	| declarator
	{
		$$ = new Vector(RUNTIME_CLASS(CFEDeclarator), 1, $1);
	}
	;

/* complex_declarator == array_declarator */
declarator :
	  simple_declarator
	{ $$ = $1; }
	| array_declarator
	{ $$ = $1; }
	;

array_declarator :
	  declarator LBRACKET positive_int_const rbracket
	{
		if ($1->GetType() == DECL_ARRAY)
			$$ = (CFEArrayDeclarator*)$1;
		else {
			$$ = new CFEArrayDeclarator($1);
			delete $1;
		}
		$$->AddBounds(0, $3);
		$3->SetParent($$);
		$$->SetSourceLine(nLineNbCORBA);
	}
	;

simple_declarator :
	  ID
	{
		$$ = new CFEDeclarator(DECL_IDENTIFIER, String($1));
		$$->SetSourceLine(nLineNbCORBA);
	}
	| error
	{
		corbaerror2("expected a declarator\n");
		YYABORT;
	}
	;

floating_pt_type :
	  FLOAT
	{
		$$ = new CFESimpleType(TYPE_FLOAT);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| DOUBLE
	{
		$$ = new CFESimpleType(TYPE_DOUBLE);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| LONG DOUBLE
	{
		$$ = new CFESimpleType(TYPE_LONG_DOUBLE);
		$$->SetSourceLine(nLineNbCORBA);
	}
	;

integer_type :
	  SHORT
	{
		$$ = new CFESimpleType(TYPE_INTEGER, false, true, 2/*value for SHORT*/, false);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| LONG
	{
		$$ = new CFESimpleType(TYPE_INTEGER, false, true, 4/*value for LONG*/, false);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| LONG LONG
	{
		$$ = new CFESimpleType(TYPE_INTEGER, false, true, 8/*value for LONG LONG*/, false);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| UNSIGNED SHORT
	{
		$$ = new CFESimpleType(TYPE_INTEGER, true, true, 2/*value for SHORT*/, false);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| UNSIGNED LONG
	{
		$$ = new CFESimpleType(TYPE_INTEGER, true, true, 4/*value for LONG*/, false);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| UNSIGNED LONG LONG
	{
		$$ = new CFESimpleType(TYPE_INTEGER, true, true, 8/*value for LONG LONG*/, false);
		$$->SetSourceLine(nLineNbCORBA);
	}
	;

char_type :
	  CHAR
	{
		$$ = new CFESimpleType(TYPE_CHAR);
		$$->SetSourceLine(nLineNbCORBA);
	}
	;

wide_char_type :
	  WCHAR
	{
		$$ = new CFESimpleType(TYPE_WCHAR);
		$$->SetSourceLine(nLineNbCORBA);
	}
	;

boolean_type :
	  BOOLEAN
	{
		$$ = new CFESimpleType(TYPE_BOOLEAN);
		$$->SetSourceLine(nLineNbCORBA);
	}
	;

octet_type :
	  OCTET
	{
		$$ = new CFESimpleType(TYPE_OCTET);
		$$->SetSourceLine(nLineNbCORBA);
	}
	;

any_type :
	  ANY
	{
		$$ = new CFESimpleType(TYPE_ANY);
		$$->SetSourceLine(nLineNbCORBA);
	}
	;

object_type :
	  OBJECT
	{
		$$ = new CFESimpleType(TYPE_OBJECT);
		$$->SetSourceLine(nLineNbCORBA);
	}
	;

struct_type :
	  STRUCT ID LBRACE member_list rbrace
	{
		$$ = new CFETaggedStructType(String($2), $4);
		$4->SetParentOfElements($$);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| STRUCT LBRACE member_list RBRACE
	{
		$$ = new CFEStructType($3);
		$$->SetSourceLine(nLineNbCORBA);
		$3->SetParentOfElements($$);
	}
	;

member_list :
	  member_list member
	{
	        if ($2)
		        $1->Add($2);
		$$ = $1;
	}
	| member
	{
		$$ = new Vector(RUNTIME_CLASS(CFETypedDeclarator), 1, $1);
	}
	;

member :
	  type_spec declarators semicolon
	{
		$$ = new CFETypedDeclarator(TYPEDECL_FIELD, $1, $2);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$1->SetParent($$);
		$2->SetParentOfElements($$);
	}
	;

union_type :
	  UNION ID SWITCH LPAREN switch_type_spec rparen LBRACE switch_body rbrace
	{
		CFEUnionType *tmp = new CFEUnionType($5, String(), $8);
		tmp->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$5->SetParent(tmp);
		$8->SetParentOfElements(tmp);
		tmp->SetCORBA();
		$$ = new CFETaggedUnionType(String($2), tmp);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		tmp->SetParent($$);
	}
	;

switch_type_spec :
	  integer_type
	{ $$ = $1; }
	| char_type
	{ $$ = $1; }
	| boolean_type
	{ $$ = $1; }
	| enum_type
	{ $$ = $1; }
	| scoped_name
	{
		$$ = new CFEUserDefinedType($1->GetName());
		$$->SetSourceLine(nLineNbCORBA);
	}
	;

switch_body :
	  switch_body case
	{
	        if ($2)
		        $1->Add($2);
		$$ = $1;
	}
	| case
	{
		$$ = new Vector(RUNTIME_CLASS(CFEUnionCase), 1, $1);
	}
	;

case :
	  case_label_list element_spec semicolon
	{
		$$ = new CFEUnionCase($2, $1);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$2->SetParent($$);
		$1->SetParentOfElements($$);
	}
	| DEFAULT colon element_spec semicolon
	{
		$$ = new CFEUnionCase($3);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$3->SetParent($$);
	}
	;

case_label_list :
	  case_label_list case_label
	{
	        if ($2)
		        $1->Add($2);
		$$ = $1;
	}
	| case_label
	{
		$$ = new Vector(RUNTIME_CLASS(CFEExpression), 1, $1);
	}
	;

case_label :
	  CASE const_exp colon
	{
		$$ = $2;
	}
	;

element_spec :
	  type_spec declarator
	{
		$$ = new CFETypedDeclarator(TYPEDECL_FIELD, $1, new Vector(RUNTIME_CLASS(CFEDeclarator), 1, $2));
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$1->SetParent($$);
		$2->SetParent($$);
	}
	;

enum_type :
	  ENUM ID LBRACE enumerator_list rbrace
	{
		$$ = new CFETaggedEnumType(String($2), $4);
		$$->SetSourceLine(nLineNbCORBA);
	}
	;

enumerator_list :
	  enumerator_list COMMA ID
	{
		CFEIdentifier *tmp = new CFEIdentifier($3);
		tmp->SetSourceLine(nLineNbCORBA);
		$1->Add(tmp);
		$$ = $1;
	}
	| ID
	{
		CFEIdentifier *tmp = new CFEIdentifier($1);
		tmp->SetSourceLine(nLineNbCORBA);
		$$ = new Vector(RUNTIME_CLASS(CFEIdentifier), 1, tmp);
	}
	;

sequence_type	:
	  SEQUENCE LT simple_type_spec COMMA positive_int_const GT
	{
		// specifies a "bounded" array of that type
		// can be nested
		$$ = new CFEArrayType($3, $5);
		$$->SetSourceLine(nLineNbCORBA);
		$3->SetParent($$);
	}
	| SEQUENCE LT simple_type_spec GT
	{
		// specifies an "unbounded" array of that type
		// can be nested
		$$ = new CFEArrayType($3);
		$$->SetSourceLine(nLineNbCORBA);
		$3->SetParent($$);
	}
	;

string_type :
	  STRING LT positive_int_const GT
	{
		// string as "bounded" array of bytes
		// == char var[int]
		CFESimpleType *tmp = new CFESimpleType(TYPE_STRING);
		$$ = new CFEArrayType(tmp, $3);
		tmp->SetParent($$);
		tmp->SetSourceLine(nLineNbCORBA);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| STRING
	{
		// string as "unbounded" array of bytes (string attribute?)
		CFESimpleType *tmp = new CFESimpleType(TYPE_STRING);
		$$ = new CFEArrayType(tmp);
		tmp->SetParent($$);
		tmp->SetSourceLine(nLineNbCORBA);
		$$->SetSourceLine(nLineNbCORBA);
	}
	;

wide_string_type :
	  WSTRING LT positive_int_const GT
	{
		// string as "bounded" array of two bytes  (wchar array + max_is)
		CFESimpleType *tmp = new CFESimpleType(TYPE_WSTRING);
		$$ = new CFEArrayType(tmp, $3);
		tmp->SetParent($$);
		tmp->SetSourceLine(nLineNbCORBA);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| WSTRING
	{
		// string as "unbounded" array of two bytes  (wchar array)
		CFESimpleType *tmp = new CFESimpleType(TYPE_WSTRING);
		$$ = new CFEArrayType(tmp);
		tmp->SetParent($$);
		tmp->SetSourceLine(nLineNbCORBA);
		$$->SetSourceLine(nLineNbCORBA);
	}
	;

attr_dcl :
	  READONLY ATTRIBUTE param_type_spec simple_declarator_list
	{
		// add function "<param_type_spec> [out]__get_<simple_declarator>();" for each decl
//        if ($4->GetSize() > 0)
//            $$ = new Vector(RUNTIME_CLASS(CFEOperation));
//        CFEOperation *pOp;
//        VectorElement *pIter;
//        for (pIter = $4->GetFirst(); pIter; pIter = pIter->GetNext())
//        {
//            if (pIter->GetElement())
//            {
//                CFEDeclarator *pName = (CFEDeclarator*)(pIter->GetElement());
//                pOp = new CFEOperation($3, String("__get_")+pName->GetName(), NULL);
//                $$->Add(pOp);
//            }
//        }
        $$ = NULL;
	}
	| ATTRIBUTE param_type_spec simple_declarator_list
	{
		// add function "void [in]__set_<simple_declarator>(<param_type_spec> 'first letter of <decl>');" for each decl
		// add function "<param_type_spec> [out]__get_<simple_declarator>();" for each decl
//        if ($3->GetSize() > 0)
//            $$ = new Vector(RUNTIME_CLASS(CFEOperation));
//        CFEOperation *pOp;
//        VectorElement *pIter;
//        for (pIter = $3->GetFirst(); pIter; pIter = pIter->GetNext())
//        {
//            if (pIter->GetElement())
//            {
//                CFEDeclarator *pName = (CFEDeclarator*)(pIter->GetElement());
//                pOp = new CFEOperation($2, String("__get_")+pName->GetName(), NULL);
//                $$->Add(pOp);
//
//                CFETypedDeclarator *pParam = new CFETypedDeclarator(TYPEDECL_PARAM, $2,
//                    new Vector(RUNTIME_CLASS(CFEDeclarator), 1, new CFEDeclarator(DECL_DECLARATOR,
//                        String(pName->GetName()[0]))),
//                    new Vector(RUNTIME_CLASS(CFEAttribute), 1, new CFEAttribute(ATTR_IN)));
//                Vector *pParams = new Vector(RUNTIME_CLASS(CFETypedDeclarator), 1, pParam);
//                CFETypeSpec *pType = new CFETypeSpec(TYPE_VOID);
//                pOp = new CFEOperation(pType, String("__set_")+pName->GetName(), pParams);
//                $$->Add(pOp);
//            }
//        }
        $$ = NULL;
	}
	;

simple_declarator_list :
	  simple_declarator_list COMMA simple_declarator
	{
	        if ($3)
		        $1->Add($3);
		$$ = $1;
	}
	| simple_declarator
	{
		$$ = new Vector(RUNTIME_CLASS(CFEDeclarator), 1, $1);
	}
	;

except_dcl :
	  EXCEPTION ID LBRACE member_list rbrace
	{
		// defines an exception with name ID and members ... (represent as struct?)
		$$ = NULL;
	}
	| EXCEPTION ID LBRACE RBRACE
	{
		// defines an exception with name ID and no members
		$$ = NULL;
	}
	;

op_dcl :
	  op_attribute op_type_spec ID parameter_dcls raises_expr context_expr
	{
		// ignore the context
        if ($1 == 0)
            $$ = new CFEOperation($2, String($3), $4, 0, $5);
        else
            $$ = new CFEOperation($2, String($3), $4, new Vector(RUNTIME_CLASS(CFEAttribute), 1, $1), $5);
		$$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$2->SetParent($$);
		if ($1 != 0)
			$1->SetParent($$);
		if ($4 != 0)
			$4->SetParentOfElements($$);
		$5->SetParentOfElements($$);
	}
	| op_attribute op_type_spec ID parameter_dcls context_expr
	{
		// ignore the context
        if ($1 == 0)
            $$ = new CFEOperation($2, String($3), $4);
        else
            $$ = new CFEOperation($2, String($3), $4, new Vector(RUNTIME_CLASS(CFEAttribute), 1, $1));
        $$->SetSourceLine(nLineNbCORBA);
		// set parent relationship
		$2->SetParent($$);
		if ($1 != 0)
			$1->SetParent($$);
		if ($4 != 0)
			$4->SetParentOfElements($$);
	}
	;

op_attribute :
	  ONEWAY
	{
		// we do not support oneway attribute -> represent appropriately (in)
		$$ = new CFEAttribute(ATTR_IN);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| /* is optional attribute */
	{
		$$ = 0;
	}
	;

op_type_spec :
	  param_type_spec
	{
		$$ = $1;
	}
	| VOID
	{
		$$ = new CFESimpleType(TYPE_VOID);
		$$->SetSourceLine(nLineNbCORBA);
	}
	;

parameter_dcls :
	  LPAREN param_dcl_list rparen
	{
		$$ = $2;
	}
	| LPAREN RPAREN
	{
		$$ = 0;
	}
	;

param_dcl_list :
	  param_dcl_list COMMA param_dcl
	{
	        if ($3)
		        $1->Add($3);
		$$ = $1;
	}
	| param_dcl
	{
		$$ = new Vector(RUNTIME_CLASS(CFETypedDeclarator), 1, $1);
	}
	;

param_dcl	:
	  param_attribute param_type_spec simple_declarator
	{
		$$ = new CFETypedDeclarator(TYPEDECL_PARAM, $2, new Vector(RUNTIME_CLASS(CFEDeclarator), 1, $3), $1);
		$$->SetSourceLine(nLineNbCORBA);
		// check if OUT and reference
		if ($$->FindAttribute(ATTR_OUT) != 0) {
			// because CORBA knows no pointers we have to add a reference
			$3->SetStars(1);
		}
		// set parent relationship
		$2->SetParent($$);
		$3->SetParent($$);
		$1->SetParentOfElements($$);
	}
	;

param_attribute :
	  IN
	{
		CFEAttribute *tmp = new CFEAttribute(ATTR_IN);
		tmp->SetSourceLine(nLineNbCORBA);
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, tmp);
	}
	| OUT
	{
		CFEAttribute *tmp = new CFEAttribute(ATTR_OUT);
		tmp->SetSourceLine(nLineNbCORBA);
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, tmp);
	}
	| INOUT
	{
		CFEAttribute *tmpI = new CFEAttribute(ATTR_IN);
		CFEAttribute *tmpO = new CFEAttribute(ATTR_OUT);
		tmpI->SetSourceLine(nLineNbCORBA);
		tmpO->SetSourceLine(nLineNbCORBA);
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, tmpI);
		$$->Add(tmpO);
	}
	| error
	{
		corbaerror2("expected parameter attribute, such as 'in', 'out', or 'inout'.\n");
		YYABORT;
	}
	;

raises_expr :
	  RAISES LPAREN scoped_name_list rparen
	{ $$ = $3; }
	;

scoped_name_list :
	  scoped_name_list COMMA scoped_name
	{
	        if ($3)
		        $1->Add($3);
		$$ = $1;
	}
	| scoped_name
	{
		$$ = new Vector(RUNTIME_CLASS(CFEIdentifier), 1, $1);
	}
	;

context_expr :
	  CONTEXT LPAREN string_literal_list rparen
	| /* empty context */
	;

string_literal_list :
	  string_literal_list COMMA LIT_STR
	{
		CFEIdentifier *tmp = new CFEIdentifier(*$3);
		tmp->SetSourceLine(nLineNbCORBA);
		$1->Add(tmp);
		$$ = $1;
	}
	| LIT_STR
	{
		CFEIdentifier *tmp = new CFEIdentifier(*$1);
		tmp->SetSourceLine(nLineNbCORBA);
		$$ = new Vector(RUNTIME_CLASS(CFEIdentifier), 1, tmp);
	}
	;

param_type_spec :
	  base_type_spec
	{ $$ = $1; }
	| string_type
	{ $$ = $1; }
	| wide_string_type
	{ $$ = $1; }
	| scoped_name
	{
		$$ = new CFEUserDefinedType($1->GetName());
		$$->SetSourceLine(nLineNbCORBA);
		//$1->SetParent($$);
        delete $1;
	}
	| predefined_type
	{ $$ = $1; }
	| error
	{
		corbaerror2("expected a parameter type\n");
	}
	;

predefined_type :
	  FPAGE
	{
		$$ = new CFESimpleType(TYPE_FLEXPAGE);
		$$->SetSourceLine(nLineNbCORBA);
	}
	| REFSTRING
	{
		$$ = new CFESimpleType(TYPE_REFSTRING);
		$$->SetSourceLine(nLineNbCORBA);
	}
	;

fixed_pt_type :
	  FIXED LT positive_int_const COMMA positive_int_const GT
	{
		// use a fixed point type
		// I can think of a more stupid implementation
		// see CORBA language mapping for exact translation
		$$ = new CFESimpleType(TYPE_FLOAT);
		$$->SetSourceLine(nLineNbCORBA);
	}
	;

fixed_pt_const_type :
	  FIXED
	{

	}
	;

value_base_type :
 	  VALUEBASE
	{
		// ???
	}
	;

semicolon
	: SEMICOLON
	| error 
	{  
		corbaerror2("expecting ';'"); 
		YYABORT;
	}
	;

rbrace
	: RBRACE
	| error 
	{ 
		corbaerror2("expecting '}'"); 
		YYABORT;
	}
	;

rbracket
	: RBRACKET
	| error 
	{ 
		corbaerror2("expecting ']'"); 
		YYABORT;
	}
	;

rparen
	: RPAREN
	| error 
	{ 
		corbaerror2("expecting ')'"); 
		YYABORT;
	}
	;

colon
	: COLON
	| error 
	{ 
		corbaerror2("expecting ':'"); 
		YYABORT;
	}
	;

%%


void
corbaerror(char* s)
{
	// ignore this function because it is called by bison code, which we cannot control
	nParseErrorCORBA = 1;
}

void
corbaerror2(char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	CCompiler::GccError(pCurFile, nLineNbCORBA, fmt, args);
	va_end(args);
	nParseErrorCORBA = 0;
	erroccured = 1;
	errcount++;
}

void
corbawarning(char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	CCompiler::GccWarning(pCurFile, nLineNbCORBA, fmt, args);
	va_end(args);
	nParseErrorCORBA = 0;
	warningcount++;
}

