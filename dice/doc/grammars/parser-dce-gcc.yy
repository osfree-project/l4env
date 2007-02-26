%{
/*
 * Copyright (C) 2001
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
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "Vector.h"
#include "fe/stdfe.h"

void dceerror(char *);
void dceerror2(char *fmt, ...);
int dcelex();

#define YYDEBUG	1

// collection for elements
extern CFEFile *pCurFile;
extern CFENameSpace* pCurNS;

// error stuff
extern int errcount;
extern int warningcount;
extern int erroccured;

// #include/import special treatment
extern int nIncludeLevelDCE;
extern char* sInFileName;

%}

%union {
  char			*_id;
  char			*_str;
  long			_int;
  char			_byte;
  char			_char;
  float			_float;
  long double	_double;
  int			_bool;

  version_t		_version;

  CFEExpression*			_expr;

  CFEIdentifier*			_identifier;
  CFEDeclarator*			_decl;
  CFEArrayDeclarator*		_array_decl;
  CFEConstDeclarator*		_const_decl;
  CFETypedDeclarator*		_typed_decl;
  CFEFunctionDeclarator*	_func_decl;

  CFETypeSpec*				_type_spec;
  CFESimpleType*			_simple_type;
  CFEConstructedType*		_constructed_type;
  CFETaggedStructType*		_tag_struct_type;
  CFETaggedUnionType*		_tag_union_type;
  CFEUnionType*				_union_type;
  CFEUnionCase*				_union_case;
  CFEEnumType*				_enum_type;

  CFEAttribute*				_attr;
  CFETypeAttribute*			_type_attr;
  CFEVersionAttribute*		_version_attr;
  CFEIsAttribute*			_is_attr;

  CFEInterface*				_interface;
  CFEInterfaceComponent*	_i_component;
  CFEOperation*				_operation;
  CFELibrary*				_library;
  CFEFileComponent*			_f_component;

  CFEPortSpec*				_port_spec;

  Vector*					_collection;

  enum EXPT_OPERATOR		_exp_operator;
}

%token	LBRACE		RBRACE		LBRACKET	RBRACKET	COLON		COMMA
%token	LPAREN		RPAREN		DOT			QUOT		ASTERISK	SINGLEQUOT
%token	QUESTION	BITOR		BITXOR		BITAND		LT			GT
%token	PLUS		MINUS		DIV			MOD			TILDE		EXCLAM
%token	SEMICOLON	LOGICALOR	LOGICALAND	EQUAL		NOTEQUAL	LTEQUAL
%token	GTEQUAL		LSHIFT		RSHIFT		DOTDOT		

%token	IS			BOOLEAN		BYTE		CASE		CHAR		CONST
%token	DEFAULT		DOUBLE		ENUM		FALSE		FLOAT		HANDLE_T
%token	HYPER		IMPORT		INT			INTERFACE	LONG		EXPNULL
%token	PIPE		SHORT		SMALL		STRUCT		SWITCH		TRUE
%token	TYPEDEF		UNION		UNSIGNED	SIGNED		VOID		ERROR_STATUS_T
%token	FLEXPAGE	REFSTRING	OBJECT		IID_IS		ISO_LATIN_1	ISO_MULTI_LINGUAL
%token	ISO_UCS

%token	BROADCAST	CONTEXT_HANDLE	ENDPOINT	EXCEPTIONS	FIRST_IS	HANDLE
%token	IDEMPOTENT	IGNORE			IN			LAST_IS		LENGTH_IS	LOCAL
%token	MAX_IS		MAYBE			MIN_IS		OUT			PTR			POINTER_DEFAULT
%token	REF			REFLECT_DELETIONS			SIZE_IS		STRING		SWITCH_IS
%token	SWITCH_TYPE	TRANSMIT_AS		UNIQUE		UUID		VERSION		RAISES
%token	EXCEPTION	LIBRARY			CONTROL		HELPCONTEXT	HELPFILE	HELPSTRING
%token	HIDDEN		LCID			RESTRICTED	AUTO_HANDLE	BINDING_CALLOUT
%token	CODE		COMM_STATUS		CS_CHAR		CS_DRTAG	CS_RTAG		CS_STAG
%token	TAG_RTN		ENABLE_ALLOCATE	EXTERN_EXCEPTIONS		EXPLICIT_HANDLE
%token	FAULT_STATUS				HEAP		IMPLICIT_HANDLE			NOCODE
%token	REPRESENT_AS				USER_MARSHAL			WITHOUT_USING_EXCEPTIONS

%token	<_id>		ID
%token	<_int>		LIT_INT
%token	<_char>		LIT_CHAR
%token	<_str>		LIT_STR
%token	<_str>		UUID_STR
%token	<_double>	LIT_FLOAT
%token	<_str>		FILENAME
%token	<_str>		PORTSPEC

%type <_expr>				additive_exp;
%type <_expr>				and_exp;
%type <_expr>				array_bound;
%type <_array_decl>			array_declarator;
%type <_decl>				attr_var;
%type <_collection>			attr_var_list
%type <_simple_type>		base_type_spec
%type <_simple_type>		boolean_type
%type <_simple_type>		char_type
%type <_expr>				conditional_exp
%type <_constructed_type>	constructed_type_spec
%type <_const_decl>			const_declarator
%type <_expr>				const_exp
%type <_collection>			const_exp_list
%type <_type_spec>			const_type_spec
%type <_decl>				declarator
%type <_collection>			declarator_list
%type <_attr>				directional_attribute
%type <_decl>				direct_declarator
%type <_enum_type>			enumeration_type
%type <_expr>				equality_exp
%type <_typed_decl>			exception_declarator
%type <_collection>	excep_name_list
%type <_expr>				exclusive_or_exp
%type <_i_component>		export
%type <_attr>				field_attribute
%type <_collection>			field_attributes
%type <_collection>			field_attribute_list
%type <_typed_decl>			field_declarator
%type <_simple_type>		floating_pt_type
%type <_func_decl>			function_declarator
%type <_collection>			identifier_list
%type <_collection>			import
%type <_collection>			import_list
%type <_expr>				inclusive_or_exp
%type <_int>				integer_size
%type <_simple_type>		integer_type
%type <_interface>			interface
%type <_attr>				interface_attribute
%type <_collection>			interface_attributes
%type <_collection>			interface_attribute_list
%type <_i_component>		interface_component
%type <_collection>			interface_component_list
%type <_attr>				lib_attribute
%type <_collection>			lib_attribute_list
%type <_f_component>		lib_definition
%type <_collection>			lib_definitions
%type <_library>			library
%type <_expr>				logical_and_exp
%type <_expr>				logical_or_exp
%type <_typed_decl>			member
%type <_collection>			member_list
%type <_expr>				multiplicative_exp
%type <_simple_type>		only_integer
%type <_attr>				operation_attribute
%type <_collection>			operation_attributes
%type <_collection>			operation_attribute_list
%type <_operation>			op_declarator
%type <_attr>				param_attribute
%type <_collection>			param_attributes
%type <_collection>			param_attribute_list
%type <_typed_decl>			param_declarator
%type <_collection>			param_declarators
%type <_collection>			param_declarator_list
%type <_int>				pointer
%type <_port_spec>			port_spec
%type <_collection>			port_specs
%type <_type_spec>			predefined_type_spec
%type <_expr>				primary_exp
%type <_simple_type>		primitive_integer_type
%type <_attr>				ptr_attr
%type <_collection>			raises_declarator
%type <_expr>				relational_exp
%type <_expr>				shift_exp
%type <_bool>				signed_or_unsigned
%type <_type_spec>			simple_type_spec
%type <_str>				string
%type <_type_spec>			switch_type_spec
%type <_str>				tag
%type <_constructed_type>	tagged_declarator
%type <_tag_struct_type>	tagged_struct_declarator
%type <_tag_union_type>		tagged_union_declarator
%type <_attr>				type_attribute
%type <_collection>			type_attributes
%type <_collection>			type_attribute_list
%type <_typed_decl>			type_declarator
%type <_type_spec>			type_spec
%type <_expr>				unary_exp
%type <_typed_decl>			union_arm
%type <_collection>			union_body
%type <_collection>			union_body_n_e
%type <_union_case>			union_case
%type <_expr>				union_case_label
%type <_collection>			union_case_label_list
%type <_union_case>			union_case_n_e
%type <_is_attr>			union_instance_switch_attr
%type <_union_type>			union_type
%type <_union_type>			union_type_header
%type <_type_attr>			union_type_switch_attr
%type <_attr>				usage_attribute
%type <_str>				uuid_rep
%type <_version>			version_rep
%type <_exp_operator>		unary_operator

/******************************************************************************
 * GCC specific tokens
 ******************************************************************************/

%token	INC_OP	DEC_OP		ELLIPSIS	IF		ELSE		WHILE	DO	FOR
%token	BREAK	CONTINUE	RETURN		GOTO	ASM_KEYWORD	TYPEOF	ALIGNOF
%token	ATTRIBUTE			EXTENSION	LABEL	SIZEOF

%token	RS_ASSIGN	LS_ASSIGN	ADD_ASSIGN	SUB_ASSIGN	MUL_ASSIGN	DIV_ASSIGN
%token	MOD_ASSIGN	AND_ASSIGN	XOR_ASSIGN	OR_ASSIGN	PTR_OP

%token	ITERATOR	AUTO		EXTERN		REGISTER	STATIC		ONEWAY 
%token	RESTRICT	VOLATILE	BYCOPY		BYREF		INOUT		INLINE	
%token	COMPLEX		REALPART	IMAGPART

/******************************************************************************
 * end GCC specific tokens
 ******************************************************************************/

%start file
%%

file:
	  file_component_list 
	;

file_component_list:
	  file_component_list file_component 
	| file_component  
	;

file_component:
	  interface 
	{
		pCurFile->AddInterface($1);
	}
	| library 
	{
		pCurFile->AddLibrary($1);
	}
	| type_declarator semicolon 
	{ 
		pCurFile->AddTypedef($1);
	}
	| const_declarator semicolon 
	{ 
		pCurFile->AddConstant($1);
	}
	| tagged_declarator semicolon 
	{ 
		pCurFile->AddTaggedDecl($1);
	}
	| gcc_program { }
	;

interface:
	  interface_attributes INTERFACE ID COLON identifier_list 
	 { 
		CFEIdentifier *i_name = new CFEIdentifier($3);
		if (!pCurNS) {
			dceerror("no Name Space definded");
		}
		// test base interfaces
		VectorElement *pCurrent;
		CFEIdentifier *pId;
		for (pCurrent = $5->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL) {
				pId = (CFEIdentifier*)(pCurrent->GetElement());
				if (!(pCurNS->CheckName(pId, NS_INTERFACE))) {
					dceerror2("Couldn't find base interface name \"%s\".", pId->GetName());
				}
			}
		}
		CFENameSpace *pNewNS = pCurNS->CheckAndAddName(i_name, NS_INTERFACE);
		if (pNewNS == NULL) {
			// name already exists
			dceerror("Interface name already exists");
		}
		pCurNS = pNewNS;
	} LBRACE import_list interface_component_list rbrace opt_semicolon
	{ 
		// append imported elements
		$9->Add($8);
		$$ = new CFEInterface($1, new CFEIdentifier($3), $5, $9);
		$1->SetParentOfElements($$);
		$9->SetParentOfElements($$);
	}
	| interface_attributes INTERFACE ID 
	{
		CFEIdentifier *i_name = new CFEIdentifier($3);
		if (!pCurNS) {
			dceerror("no Name Space definded");
		}
		CFENameSpace *pNewNS = pCurNS->CheckAndAddName(i_name, NS_INTERFACE);
		if (pNewNS == NULL) {
			// name already exists
			dceerror("Interface name already exists");
		}
		pCurNS = pNewNS;
	} LBRACE import_list interface_component_list rbrace opt_semicolon
	{ 
		// append imported elements to interface components
		$7->Add($6);
		$$ = new CFEInterface($1, new CFEIdentifier($3), NULL, $7);
		$1->SetParentOfElements($$);
		$7->SetParentOfElements($$);
	}
	;

interface_attributes :
	  LBRACKET interface_attribute_list rbracket { $$ = $2; }
	| LBRACKET RBRACKET
	{ 
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, new CFEVersionAttribute(0,0)); 
	}
	| /* no attributes */
	{ 
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, new CFEVersionAttribute(0,0)); 
	}
	;

interface_attribute_list :
	  interface_attribute_list COMMA interface_attribute
	{
		$1->Add($3);
		$$ = $1;
	}
	| interface_attribute 
	{
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, $1);
	}
	;

interface_attribute 
	: UUID LPAREN uuid_rep rparen 
	{ $$ = new CFEStringAttribute(ATTR_UUID, $3); }
	| VERSION LPAREN version_rep rparen
	{ $$ = new CFEVersionAttribute($3); }
	| ENDPOINT LPAREN port_specs rparen 
	{ $$ = new CFEEndPointAttribute($3); }
	| EXCEPTIONS LPAREN excep_name_list rparen 
	{ 
		$$ = new CFEExceptionAttribute($3);
		$3->SetParentOfElements($$);
	}
	| LOCAL 
	{ $$ = new CFEAttribute(ATTR_LOCAL); }
	| POINTER_DEFAULT LPAREN ptr_attr rparen 
	{ 
		$$ = new CFEPtrDefaultAttribute($3); 
		// set parent relationship
		$3->SetParent($$);
	}
	| OBJECT 
	{ $$ = new CFEAttribute(ATTR_OBJECT); }
	| error 
	{ 
		dceerror("Unknown interface attribute"); 
		YYABORT;
	}
	;

version_rep
	: LIT_INT DOT LIT_INT 
	{ version_t v; v.nMajor = $1; v.nMinor = $3; $$ = v; }
	| LIT_INT 
	{ version_t v; v.nMajor = $1; v.nMinor = 0; $$ = v; }
	| LIT_FLOAT 
	{ 
		version_t v; 
		v.nMajor = int($1);
		int d1, d2;
		// make string from rest of version
		char* str = fcvt($1 - int($1), 10, &d1, &d2);
		if (str != NULL) {
			// get string after '.'
			char* b = str+d1;
			// eliminate trailing (stuffed) '0's
			while(b[strlen(b)-1] == '0') b[strlen(b)-1] = 0;
			// convert back to int
			if (b != NULL) v.nMinor = atol(b);
			else v.nMinor = 0;
		} else v.nMinor = 0;
		$$ = v;
	}
	| error 
	{ 
		dceerror("[version] format is incorrect"); 
		YYABORT;
	}
	;


port_specs :
	  port_specs COMMA port_spec
	{
		$1->Add($3);
		$$ = $1;
	}
	| port_spec
	{ 
		$$ = new Vector(RUNTIME_CLASS(CFEPortSpec), 1, $1); 
	}
	| error
	{
		dceerror("expecting port specification\n");
		YYABORT;
	}
	;

port_spec 
	:  PORTSPEC
	{ $$ = new CFEPortSpec($1); }
	;

excep_name_list :
	  excep_name_list COMMA ID 
	{
		$1->Add(new CFEIdentifier($3));
		$$ = $1;
	}
	| ID 
	{
		$$ = new Vector(RUNTIME_CLASS(CFEIdentifier), 1, new CFEIdentifier($1));
	}
	| error
	{
		dceerror("invalid exception specifier");
		YYABORT;
	}
	;

import_list :
	  import_list import
	{
		$$ = $1;
		$$->Add($2);
	}
	| import
	{
		$$ = $1;
	}
	| 
	{
		$$ = NULL;
	}
	;

import :
	  IMPORT LBRACE interface_component_list rbrace opt_semicolon
	{
		VectorElement *pCurrent, *pDel;
		for (pCurrent = $3->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			// if component if something else than 
			// type_declarator, const_declarator or tagged_declarator
			// throw it away
			if (pCurrent->GetElement() != NULL) {
				if (!(pCurrent->GetElement())->IsKindOf(RUNTIME_CLASS(CFETypedDeclarator)) &&
					!(pCurrent->GetElement())->IsKindOf(RUNTIME_CLASS(CFEConstDeclarator)) &&
					!(pCurrent->GetElement())->IsKindOf(RUNTIME_CLASS(CFEConstructedType))) {
					pDel = pCurrent->GetNext();
					$3->Remove(pCurrent->GetElement());
					pCurrent = pDel; // is incremented at end of loop
				}
			}
		}
		$$ = $3;
	}
	| IMPORT LBRACE RBRACE opt_semicolon
	{
		$$ = NULL;
	}
	;

interface_component_list :
	  interface_component_list interface_component
	{
		$1->Add($2);
		$$ = $1;
	}
	| interface_component
	{
		$$ = new Vector(RUNTIME_CLASS(CFEInterfaceComponent), 1, $1);
	}
	;

interface_component :
	  export semicolon 
	{ 
		//$1->SetFile(pCurFile);
		$$ = $1; 
	}
	| op_declarator semicolon 
	{ 
		//$1->SetFile(pCurFile);
		$$ = $1; 
	}
 	| gcc_program { $$ = NULL; }
	;

export 
	: type_declarator { $$ = $1; }
	| const_declarator { $$ = $1; }
	| tagged_declarator { $$ = $1; }
	| exception_declarator { $$ = $1; }
	;

const_declarator 
	: CONST const_type_spec ID IS const_exp
	{ 
		if (!( $5->IsOfType($2->GetType()) )) {
			dceerror2("Const type of \"%s\" does not match with expression.\n",$3);
			YYABORT;
		}
		$$ = new CFEConstDeclarator($2, new CFEIdentifier($3), $5); 
		// set parent relationship
		$2->SetParent($$);
		$5->SetParent($$);
	}
	;

const_type_spec 
	: primitive_integer_type { $$ = $1; }
	| CHAR
	{ $$ = new CFESimpleType(TYPE_CHAR); }
	| BOOLEAN
	{ $$ = new CFESimpleType(TYPE_BOOLEAN); }
	| VOID ASTERISK
	{ $$ = new CFESimpleType(TYPE_VOID_ASTERISK); }
	| CHAR ASTERISK
	{ $$ = new CFESimpleType(TYPE_CHAR_ASTERISK); }
	;


const_exp_list :
	  const_exp_list COMMA const_exp
	{ 
		$1->Add($3);
		$$ = $1; 
	}
	| const_exp 
	{
		$$ = new Vector(RUNTIME_CLASS(CFEExpression), 1, $1);
	}
	;

const_exp 
	: conditional_exp  { $$ = $1; }
	| string 
	{ $$ = new CFEExpression(EXPR_STRING, $1); }
	| LIT_CHAR
	{ $$ = new CFEExpression(EXPR_CHAR, $1); }
	| EXPNULL 
	{ $$ = new CFEExpression(EXPR_NULL); }
	| TRUE 
	{ $$ = new CFEExpression(EXPR_TRUE); }
	| FALSE 
	{ $$ = new CFEExpression(EXPR_FALSE); }
	;

conditional_exp 
	: logical_or_exp { $$ = $1; }
	| logical_or_exp QUESTION conditional_exp COLON conditional_exp 
	{ 
		$$ = new CFEConditionalExpression($1, $3, $5); 
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
		$5->SetParent($$);
	}
	;

logical_or_exp 
	: logical_and_exp { $$ = $1; }
	| logical_or_exp LOGICALOR logical_and_exp 
	{ 
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LOGOR, $3); 
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	;

logical_and_exp 
	: inclusive_or_exp { $$ = $1; }
	| logical_and_exp LOGICALAND inclusive_or_exp
	{ 
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LOGAND, $3); 
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	;

inclusive_or_exp 
	: exclusive_or_exp { $$ = $1; }
	| inclusive_or_exp BITOR exclusive_or_exp
	{ 
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITOR, $3); 
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	;

exclusive_or_exp 
	: and_exp { $$ = $1; }
	| exclusive_or_exp BITXOR and_exp 
	{ 
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITXOR, $3); 
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	;

and_exp 
	: equality_exp { $$ = $1; }
	| and_exp BITAND equality_exp
	{ 
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITAND, $3); 
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	;

equality_exp 
	: relational_exp { $$ = $1; }
	| equality_exp EQUAL relational_exp
	{ 
		if (($1 != NULL) && ($3 != NULL)) {
			$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_EQUALS, $3); 
			// set parent relationship
			$1->SetParent($$);
			$3->SetParent($$);
		} else
			$$ = NULL;
	}
	| equality_exp NOTEQUAL relational_exp
	{ 
		if (($1 != NULL) && ($3 != NULL)) {
			$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_NOTEQUAL, $3); 
			// set parent relationship
			$1->SetParent($$);
			$3->SetParent($$);
		} else
			$$ = NULL;
	}
	;

relational_exp:
	  shift_exp { $$ = $1; }
	| relational_exp LTEQUAL shift_exp 
	{ 
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LTEQU, $3); 
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	| relational_exp GTEQUAL shift_exp 
	{ 
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_GTEQU, $3); 
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	| relational_exp LT shift_exp 
	{ 
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LT, $3); 
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	| relational_exp GT shift_exp 
	{ 
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_GT, $3); 
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	;

shift_exp 
	: additive_exp { $$ = $1; }
	| shift_exp LSHIFT additive_exp 
	{ 
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LSHIFT, $3); 
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	| shift_exp RSHIFT additive_exp 
	{ 
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_RSHIFT, $3); 
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	;

additive_exp 
	: multiplicative_exp { $$ = $1; }
	| additive_exp PLUS multiplicative_exp 
	{ 
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_PLUS, $3); 
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	| additive_exp MINUS multiplicative_exp
	{ 
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MINUS, $3); 
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	;

multiplicative_exp 
	: unary_exp { $$ = $1; }
	| multiplicative_exp ASTERISK unary_exp 
	{ 
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MUL, $3); 
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	| multiplicative_exp DIV unary_exp
	{ 
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_DIV, $3); 
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	| multiplicative_exp MOD unary_exp
	{ 
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MOD, $3); 
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
	}
	;

unary_exp 
	: primary_exp { $$ = $1; }
	| unary_operator primary_exp 
	{ 
		$$ = new CFEUnaryExpression(EXPR_UNARY, $1, $2); 
		// set parent relationship
		$2->SetParent($$);
	}
	;

unary_operator
	: PLUS 
	{ $$ = EXPR_SPLUS; }
	| MINUS
	{ $$ = EXPR_SMINUS; }
	| TILDE
	{ $$ = EXPR_TILDE; }
	| EXCLAM
	{ $$ = EXPR_EXCLAM; }
	;

primary_exp 
	: LIT_INT 
	{ $$ = new CFEPrimaryExpression(EXPR_INT, $1); }
	| LIT_FLOAT
	{ $$ = new CFEPrimaryExpression(EXPR_FLOAT, $1); }
	| ID 
	{ $$ = new CFEUserDefinedExpression(new CFEIdentifier($1)); }
	| LPAREN const_exp rparen 
	{ 
		$$ = new CFEPrimaryExpression(EXPR_PAREN, $2); 
		// set parent relationship
		$2->SetParent($$);
	}
	;

string 
	: LIT_STR { $$ = $1; }
	| QUOT QUOT { $$ = NULL; }
	;

type_declarator 
	: TYPEDEF type_attribute_list type_spec declarator_list
	{ 
		$$ = new CFETypedDeclarator(TYPEDECL_TYPEDEF, $3, $4, $2); 
		// set parent relationship
		VectorElement *pCurrent;
		for (pCurrent = $2->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
		$3->SetParent($$);
		for (pCurrent = $4->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	| TYPEDEF type_spec declarator_list
	{ 
		$$ = new CFETypedDeclarator(TYPEDECL_TYPEDEF, $2, $3); 
		// set parent relationship
		$2->SetParent($$);
		VectorElement *pCurrent;
		for (pCurrent = $3->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	| TYPEDEF type_attribute_list type_spec error 
	{ 
		dceerror("invalid type definition: invalid name(s) specified"); 
		YYABORT; 
	}
	| TYPEDEF type_spec error
	{ 
		dceerror("invalid type definition: invalid name(s) specified"); 
		YYABORT; 
	}
	| TYPEDEF type_attribute_list error declarator_list
	{
		dceerror("invalid type definition: invalid type used");
		YYABORT;
	}
	| TYPEDEF error declarator_list
	{
		dceerror("invalid type definition: invalid type used");
		YYABORT;
	}
	| TYPEDEF error type_spec declarator_list
	{
		dceerror("invalid type definition: invalid attribute list");
		YYABORT;
	}
	;

type_attribute_list 
	: LBRACKET type_attributes rbracket { $$ = $2; }
	;

type_spec 
	: simple_type_spec { $$ = $1 }
	| constructed_type_spec { $$ = $1 }
	;

simple_type_spec 
	: base_type_spec { $$ = $1; }
	| predefined_type_spec { $$ = $1; }
	| ID 
	{ 
		$$ = new CFEUserDefinedType(new CFEIdentifier($1));
	}
	;

declarator_list :
	  declarator_list COMMA declarator 
	{ 
		$1->Add($3);
		$$ = $1; 
	}
	| declarator 
	{
		$$ = new Vector(RUNTIME_CLASS(CFEDeclarator), 1, $1);
	}
	;

declarator 
	: pointer direct_declarator 
	{ 
		$2->SetStars($1); 
		$$ = $2; 
	}
	| pointer direct_declarator COLON const_exp
	{
		$2->SetStars($1); 
		if ($4 != NULL)
			$2->SetBitfields($4->GetIntValue());
		$$ = $2; 
	}
	| direct_declarator { $$ = $1; }
	| direct_declarator COLON const_exp
	{
		if ($3 != NULL)
			$1->SetBitfields($3->GetIntValue());
		$$ = $1;
	}
	;

direct_declarator 
	: ID 
	{ $$ = new CFEDeclarator(DECL_IDENTIFIER, $1); }
	| LPAREN declarator rparen { $$ = $2; }
	| array_declarator  { $$ = $1; }
	| function_declarator  { $$ = $1; }
	;

tagged_declarator 
	: tagged_struct_declarator { $$ = $1; }
	| tagged_union_declarator { $$ = $1; }
	;

base_type_spec 
	: floating_pt_type { $$ = $1; } 
	| integer_type { $$ = $1; }
	| char_type { $$ = $1; }
	| boolean_type { $$ = $1; }
	| BYTE 
	{ $$ = new CFESimpleType(TYPE_BYTE); }
	| VOID 
	{ $$ = new CFESimpleType(TYPE_VOID); }
	| HANDLE_T 
	{ $$ = new CFESimpleType(TYPE_HANDLE_T); }
	;

floating_pt_type 
	: FLOAT 
	{ $$ = new CFESimpleType(TYPE_FLOAT); }
	| DOUBLE 
	{ $$ = new CFESimpleType(TYPE_DOUBLE); }
	;


integer_type 
	: primitive_integer_type { $$ = $1; }
	| HYPER signed_or_unsigned INT
	{ $$ = new CFESimpleType(TYPE_INTEGER, $2, false, 8); }
	| HYPER signed_or_unsigned
	{ $$ = new CFESimpleType(TYPE_INTEGER, $2, false, 8); }
	| HYPER INT
	{ $$ = new CFESimpleType(TYPE_INTEGER, false, false, 8); }
	| HYPER
	{ $$ = new CFESimpleType(TYPE_INTEGER, false, false, 8); }
	| signed_or_unsigned HYPER INT
	{ $$ = new CFESimpleType(TYPE_INTEGER, $1, true, 8); }
	| signed_or_unsigned HYPER
	{ $$ = new CFESimpleType(TYPE_INTEGER, $1, true, 8); }
	;

signed_or_unsigned
	: SIGNED 
	{ $$ = false; }
	| UNSIGNED
	{ $$ = true; }
	;

primitive_integer_type :
	  signed_or_unsigned only_integer
	{
		$2->SetUnsigned($1);
		$$ = $2;
	}
	| only_integer
	{ $$ = $1; }
	| signed_or_unsigned
	{ $$ = new CFESimpleType(TYPE_INTEGER, $1); }
	| integer_size signed_or_unsigned
	{ $$ = new CFESimpleType(TYPE_INTEGER, $2, false, $1); }
	| integer_size signed_or_unsigned INT
	{ $$ = new CFESimpleType(TYPE_INTEGER, $2, false, $1); }
	;

only_integer :
	  INT
	{ $$ = new CFESimpleType(TYPE_INTEGER, false, false); }
	| integer_size INT 
	{ $$ = new CFESimpleType(TYPE_INTEGER, false, false, $1); }
	| integer_size 
	{ $$ = new CFESimpleType(TYPE_INTEGER, false, false, $1, false); }
	;

integer_size 
	: LONG { $$ = 4; }
	| LONG LONG { $$ = 8; }
	| SHORT { $$ = 2; }
	| SMALL { $$ = 1; }
	;

char_type 
	: UNSIGNED CHAR 
	{ $$ = new CFESimpleType(TYPE_CHAR, true); }
	| CHAR 
	{ $$ = new CFESimpleType(TYPE_CHAR); }
	| SIGNED CHAR
	{ $$ = new CFESimpleType(TYPE_CHAR); }
	;

boolean_type
	: BOOLEAN 
	{ $$ = new CFESimpleType(TYPE_BOOLEAN); }
	;

constructed_type_spec 
	: STRUCT LBRACE member_list rbrace 
	{ 
		$$ = new CFEStructType($3);
		// set parent relationship
		VectorElement *pCurrent;
		for (pCurrent = $3->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	| union_type { $$ = $1; }
	| enumeration_type { $$ = $1; }
	| tagged_declarator { $$ = $1; }
	| PIPE type_spec 
	{ 
		$$ = new CFEPipeType($2); 
		// set parent relationship
		$2->SetParent($$);
	}
	;

tagged_struct_declarator 
	: STRUCT tag LBRACE member_list rbrace 
	{ 
		$$ = new CFETaggedStructType(new CFEIdentifier($2), $4);
		// set parent relationship
		VectorElement *pCurrent;
		for (pCurrent = $4->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	| STRUCT tag
	{ $$ = new CFETaggedStructType(new CFEIdentifier($2)); }
	;

tag 
	: ID { $$ = $1; }
	;

member_list :
	  member_list member 
	{ 
		$1->Add($2);
		$$ = $1; 
	}
	| member 
	{
		$$ = new Vector(RUNTIME_CLASS(CFETypedDeclarator), 1, $1);
	}
	;

member 
	: field_declarator semicolon { $$ = $1; }
	;

field_declarator 
	: field_attribute_list type_spec declarator_list 
	{ 
		$$ = new CFETypedDeclarator(TYPEDECL_FIELD, $2, $3, $1);
		// set parent relationship
		VectorElement *pCurrent;
		for (pCurrent = $1->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
		$2->SetParent($$);
		for (pCurrent = $3->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	| type_spec declarator_list 
	{ 
		$$ = new CFETypedDeclarator(TYPEDECL_FIELD, $1, $2); 
		// set parent relationship
		$1->SetParent($$);
		VectorElement *pCurrent;
		for (pCurrent = $2->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	;

field_attribute_list 
	: LBRACKET field_attributes rbracket { $$ = $2; }
	;

tagged_union_declarator 
	: UNION tag 
	{ $$ = new CFETaggedUnionType(new CFEIdentifier($2)); }
	| UNION tag union_type_header 
	{ 
		$$ = new CFETaggedUnionType(new CFEIdentifier($2), $3); 
		// set parent relationship
		$3->SetParent($$);
	}
	;

union_type 
	: UNION union_type_header { $$ = $2; }
	;

union_type_header
	: SWITCH LPAREN switch_type_spec ID rparen ID LBRACE union_body rbrace 
	{ 
		$$ = new CFEUnionType($3, new CFEIdentifier($4), $8, new CFEIdentifier($6));
		// set parent relationship
		$3->SetParent($$);
		VectorElement *pCurrent;
		for (pCurrent = $8->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	| SWITCH LPAREN switch_type_spec ID rparen LBRACE union_body rbrace 
	{ 
		$$ = new CFEUnionType($3, new CFEIdentifier($4), $7); 
		// set parent relationship
		$3->SetParent($$);
		VectorElement *pCurrent;
		for (pCurrent = $7->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	| LBRACE union_body_n_e rbrace 
	{ 
		$$ = new CFEUnionType($2); 
		// set parent relationship
		VectorElement *pCurrent;
		for (pCurrent = $2->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	;


switch_type_spec 
	: primitive_integer_type { $$ = $1; }
	| char_type { $$ = $1; }
	| boolean_type { $$ = $1; }
	| ID 
	{ 
		$$ = new CFEUserDefinedType(new CFEIdentifier($1)); 
	}
	| enumeration_type { $$ = $1; }
	;

union_body :
	  union_body union_case 
	{ 
		$1->Add($2);
		$$ = $1; 
	}
	| union_case 
	{
		$$ = new Vector(RUNTIME_CLASS(CFEUnionCase), 1, $1);
	}
	;

union_body_n_e :
	  union_body_n_e union_case_n_e 
	{ 
		$1->Add($2);
		$$ = $1; 
	}
	| union_case_n_e 
	{
		$$ = new Vector(RUNTIME_CLASS(CFEUnionCase), 1, $1);
	}
	;


union_case 
	: union_case_label_list union_arm 
	{ 
		$$ = new CFEUnionCase($2, $1); 
		// set parent relationship
		$2->SetParent($$);
		$1->SetParentOfElements($$);
	}
	| DEFAULT COLON union_arm 
	{ 
		$$ = new CFEUnionCase($3); 
		// set parent relationship
		$3->SetParent($$);
	}
	;

union_case_n_e 
	: LBRACKET CASE LPAREN const_exp_list rparen rbracket union_arm 
	{ 
		$$ = new CFEUnionCase($7, $4); 
		// set parent relationship
		$7->SetParent($$);
		$4->SetParentOfElements($$);
	}
	| LBRACKET DEFAULT rbracket union_arm 
	{ 
		$$ = new CFEUnionCase($4); 
		// set parent relationship
		$4->SetParent($$);
	}
	| union_arm
	{
		// this one is to support C/C++ unions
		$$ = new CFEUnionCase($1);
		$1->SetParent($$);
	}
	;


union_case_label_list :
	  union_case_label_list union_case_label 
	{ 
		$1->Add($2);
		$$ = $1;
	}
	| union_case_label 
	{
		$$ = new Vector(RUNTIME_CLASS(CFEExpression), 1, $1);
	}
	;

union_case_label 
	: CASE const_exp colon { $$ = $2; }
	;

union_arm 
	: field_declarator semicolon { $$ = $1; }
	| semicolon 
	{ $$ = new CFETypedDeclarator(TYPEDECL_VOID, NULL, NULL); }
	;

union_type_switch_attr 
	: SWITCH_TYPE LPAREN switch_type_spec rparen 
	{ 
		$$ = new CFETypeAttribute(ATTR_SWITCH_TYPE, $3); 
		// set parent relationship
		$3->SetParent($$);
	}
	;

union_instance_switch_attr 
	: SWITCH_IS LPAREN attr_var rparen 
	{ 
		$$ = new CFEIsAttribute(ATTR_SWITCH_IS, new Vector(RUNTIME_CLASS(CFEDeclarator), 1, $3)); 
		// set parent relationship
		$3->SetParent($$);
	}
	;

identifier_list :
	  identifier_list COMMA ID 
	{ 
		$1->Add(new CFEIdentifier($3)); 
		$$ = $1; 
	}
	| ID 
	{
		$$ = new Vector(RUNTIME_CLASS(CFEIdentifier), 1, new CFEIdentifier($1));
	}
	;


enumeration_type 
	: ENUM LBRACE identifier_list rbrace 
	{ 
		$$ = new CFEEnumType($3); 
		// set parent relationship
		VectorElement *pCurrent;
		for (pCurrent = $3->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	;

array_declarator 
	: direct_declarator LBRACKET RBRACKET 
	{ 
		if ($1->GetType() == DECL_ARRAY)
			$$ = (CFEArrayDeclarator*)$1;
		else {
			$$ = new CFEArrayDeclarator($1); 
			delete $1; 
		}
		$$->AddBounds(NULL, NULL);
	}
	| direct_declarator LBRACKET array_bound rbracket 
	{ 
		if ($1->GetType() == DECL_ARRAY)
			$$ = (CFEArrayDeclarator*)$1;
		else {
			$$ = new CFEArrayDeclarator($1);
			delete $1; 
		}
		$$->AddBounds(NULL, $3);
		$3->SetParent($$);
	}
	| direct_declarator LBRACKET array_bound DOTDOT rbracket
	{
		// "decl [ expr '..' ]" == "decl [ expr '..' ASTERISK ]"
		CFEExpression *tmp = new CFEExpression(EXPR_CHAR, ASTERISK);
		if ($1->GetType() == DECL_ARRAY)
			$$ = (CFEArrayDeclarator*)$1;
		else {
			$$ = new CFEArrayDeclarator($1);
			delete $1;
		}
		$$->AddBounds($3, tmp);
		$3->SetParent($$);
		tmp->SetParent($$);
	}
	| direct_declarator LBRACKET array_bound DOTDOT array_bound rbracket 
	{ 
		if ($1->GetType() == DECL_ARRAY)
			$$ = (CFEArrayDeclarator*)$1;
		else {
			$$ = new CFEArrayDeclarator($1); 
			delete $1; 
		}
		$$->AddBounds($3, $5);
		$3->SetParent($$);
		$5->SetParent($$);
	}
	;

array_bound 
	: ASTERISK 
	{ $$ = new CFEExpression(EXPR_CHAR, ASTERISK); }
	| conditional_exp { $$ = $1; }
	;

type_attributes
	: type_attributes COMMA type_attribute 
	{ 
		$1->Add($3);
		$$ = $1; 
	}
	| type_attribute 
	{
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, $1);
	}
	;

type_attribute 
	: TRANSMIT_AS LPAREN simple_type_spec rparen 
	{ 
		$$ = new CFETypeAttribute(ATTR_TRANSMIT_AS, $3); 
		// set parent relationship
		$3->SetParent($$);
	}
	| HANDLE 
	{ $$ = new CFEAttribute(ATTR_HANDLE); }
	| usage_attribute { $$ = $1; }
	| union_type_switch_attr { $$ = $1; }
	| ptr_attr { $$ = $1; }
	;

usage_attribute 
	: STRING 
	{ $$ = new CFEAttribute(ATTR_STRING); }
	| CONTEXT_HANDLE 
	{ $$ = new CFEAttribute(ATTR_CONTEXT_HANDLE); }
	;

field_attributes :
	  field_attributes COMMA field_attribute 
	{ 
		$1->Add($3);
		$$ = $1; 
	}
	| field_attribute 
	{ 
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, $1);
	}
	;

field_attribute 
	: FIRST_IS LPAREN attr_var_list rparen 
	{ 
		$$ = new CFEIsAttribute(ATTR_FIRST_IS, $3); 
		// set parent relationship
		VectorElement *pCurrent;
		for (pCurrent = $3->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	| LAST_IS LPAREN attr_var_list rparen 
	{ 
		$$ = new CFEIsAttribute(ATTR_LAST_IS, $3); 
		// set parent relationship
		VectorElement *pCurrent;
		for (pCurrent = $3->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	| LENGTH_IS LPAREN attr_var_list rparen 
	{ 
		$$ = new CFEIsAttribute(ATTR_LENGTH_IS, $3); 
		// set parent relationship
		VectorElement *pCurrent;
		for (pCurrent = $3->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	| MIN_IS LPAREN attr_var_list rparen 
	{ 
		$$ = new CFEIsAttribute(ATTR_MIN_IS, $3); 
		// set parent relationship
		VectorElement *pCurrent;
		for (pCurrent = $3->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	| MAX_IS LPAREN attr_var_list rparen 
	{ 
		$$ = new CFEIsAttribute(ATTR_MAX_IS, $3); 
		// set parent relationship
		VectorElement *pCurrent;
		for (pCurrent = $3->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	| SIZE_IS LPAREN attr_var_list rparen 
	{ 
		$$ = new CFEIsAttribute(ATTR_SIZE_IS, $3); 
		// set parent relationship
		VectorElement *pCurrent;
		for (pCurrent = $3->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	| usage_attribute { $$ = $1; }
	| union_instance_switch_attr { $$ = $1; }
	| IGNORE 
	{ 
		$$ = new CFEAttribute(ATTR_IGNORE); 
	}
	| ptr_attr { $$ = $1; }
	;


attr_var_list :
	  attr_var_list COMMA attr_var 
	{
		$1->Add($3);
		$$ = $1; 
	}
	| attr_var 
	{
		$$ = new Vector(RUNTIME_CLASS(CFEDeclarator), 1, $1);
	}
	;

attr_var 
	: ASTERISK ID 
	{ $$ = new CFEDeclarator(DECL_ATTR_VAR, $2, 1); }
	| ID 
	{ $$ = new CFEDeclarator(DECL_ATTR_VAR, $1); }
	| { $$ = new CFEDeclarator(DECL_VOID); }
	;

ptr_attr 
	: REF 
	{ $$ = new CFEAttribute(ATTR_REF); }
	| UNIQUE 
	{ $$ = new CFEAttribute(ATTR_UNIQUE); }
	| PTR 
	{ $$ = new CFEAttribute(ATTR_PTR); }
	| IID_IS LPAREN attr_var_list rparen 
	{ 
		$$ = new CFEIsAttribute(ATTR_IID_IS, $3); 
		// set parent relationship
		VectorElement *pCurrent;
		for (pCurrent = $3->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	;

pointer :
	  pointer ASTERISK 
	{ $$ = $1 + 1; }
	| ASTERISK 
	{ $$ = 1; }
	;

op_declarator 
	: operation_attributes simple_type_spec ID param_declarators raises_declarator
	{ 
		$$ = new CFEOperation($2, new CFEIdentifier($3), $4, $1, $5); 
		// set parent relationship
		$2->SetParent($$);
		VectorElement *pCurrent;
		for (pCurrent = $1->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
		if ($4 != NULL) {
			for (pCurrent = $4->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
				if (pCurrent->GetElement() != NULL)
					pCurrent->GetElement()->SetParent($$);
			}
		}
		for (pCurrent = $5->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	| operation_attributes simple_type_spec ID param_declarators
	{ 
		$$ = new CFEOperation($2, new CFEIdentifier($3), $4, $1); 
		// set parent relationship
		$2->SetParent($$);
		VectorElement *pCurrent;
		for (pCurrent = $1->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
		if ($4 != NULL) {
			for (pCurrent = $4->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
				if (pCurrent->GetElement() != NULL)
					pCurrent->GetElement()->SetParent($$);
			}
		}
	}
	| simple_type_spec ID param_declarators raises_declarator
	{ 
		$$ = new CFEOperation($1, new CFEIdentifier($2), $3, NULL, $4); 
		// set parent relationship
		$1->SetParent($$);
		VectorElement *pCurrent;
		if ($3 != NULL) {
			for (pCurrent = $3->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
				if (pCurrent->GetElement() != NULL)
					pCurrent->GetElement()->SetParent($$);
			}
		}
		for (pCurrent = $4->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	| simple_type_spec ID param_declarators
	{ 
		$$ = new CFEOperation($1, new CFEIdentifier($2), $3); 
		// set parent relationship
		$1->SetParent($$);
		if ($3 != NULL) {
			VectorElement *pCurrent;
			for (pCurrent = $3->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
				if (pCurrent->GetElement() != NULL)
					pCurrent->GetElement()->SetParent($$);
			}
		}
	}
	;

raises_declarator
	: RAISES LPAREN identifier_list rparen { $$ = $3; }
	;

operation_attributes 
	: LBRACKET operation_attribute_list rbracket { $$ = $2; }
	;

operation_attribute_list :
	  operation_attribute_list COMMA operation_attribute 
	{ 
		$1->Add($3);
		$$ = $1; 
	}
	| operation_attribute 
	{
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, $1);
	}
	;

operation_attribute 
	: IDEMPOTENT 
	{ $$ = new CFEAttribute(ATTR_IDEMPOTENT); }
	| BROADCAST 
	{ $$ = new CFEAttribute(ATTR_BROADCAST); }
	| MAYBE 
	{ $$ = new CFEAttribute(ATTR_MAYBE); }
	| REFLECT_DELETIONS 
	{ $$ = new CFEAttribute(ATTR_REFLECT_DELETIONS); }
	| usage_attribute 
	{ $$ = $1; }
	| ptr_attr 
	{ $$ = $1; }
	| directional_attribute
	{ $$ = $1; }
	;

param_declarators 
	: LPAREN RPAREN 
	{ $$ = NULL; }
	| LPAREN VOID rparen 
	{ $$ = NULL; }
	| LPAREN param_declarator_list rparen 
	{ $$ = $2; }
	;

param_declarator_list :
	  param_declarator_list COMMA param_declarator 
	{
		$1->Add($3);
		$$ = $1; 
	}
	| param_declarator 
	{
		$$ = new Vector(RUNTIME_CLASS(CFETypedDeclarator), 1, $1);
	}
	;

param_declarator 
	: param_attributes type_spec declarator 
	{
		$$ = new CFETypedDeclarator(TYPEDECL_PARAM, $2, new Vector(RUNTIME_CLASS(CFEDeclarator), 1, $3), $1); 
		// check if OUT and reference
		if ($$->FindAttribute(ATTR_OUT) != NULL) {
			if (!($3->IsReference()))
				dceerror("[out] parameter must be reference");
		}
		// set parent relationship
		$2->SetParent($$);
		$3->SetParent($$);
		VectorElement *pCurrent;
		for (pCurrent = $1->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	;

param_attributes :
	  LBRACKET param_attribute_list rbracket { $$ = $2; }
	| LBRACKET RBRACKET
	{ 
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, new CFEAttribute(ATTR_NONE)); 
	}
	|
	{
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, new CFEAttribute(ATTR_NONE));
	}
	;

param_attribute_list
	: param_attribute_list COMMA param_attribute 
	{ 
		$1->Add($3);
		$$ = $1; 
	}
	| param_attribute 
	{
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, $1);
	}
	;

param_attribute 
	: directional_attribute { $$ = $1; }
	| field_attribute { $$ = $1; }
	;

directional_attribute 
	: IN  
	{ 
		$$ = new CFEAttribute(ATTR_IN); 
	}
	| OUT 
	{ 
		$$ = new CFEAttribute(ATTR_OUT); 
	}
	;

function_declarator 
	: direct_declarator param_declarators 
	{ 
		$$ = new CFEFunctionDeclarator($1, $2); 
		// set parent relationship
		$1->SetParent($$);
		if ($2 != NULL) {
			VectorElement *pCurrent;
			for (pCurrent = $2->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
				if (pCurrent->GetElement() != NULL)
					pCurrent->GetElement()->SetParent($$);
			}
		}
	}
	;

predefined_type_spec 
	: ERROR_STATUS_T 
	{ $$ = new CFESimpleType(TYPE_ERROR_STATUS_T); }
	| FLEXPAGE
	{ $$ = new CFESimpleType(TYPE_FLEXPAGE); }
	| ISO_LATIN_1 
	{ $$ = new CFESimpleType(TYPE_ISO_LATIN_1); } 
	| ISO_MULTI_LINGUAL 
	{ $$ = new CFESimpleType(TYPE_ISO_MULTILINGUAL); }
	| ISO_UCS 
	{ $$ = new CFESimpleType(TYPE_ISO_UCS); }
	| REFSTRING
	{
		dceerror("refstring type not supported by DCE IDL (use \"[ref] char*\" instead)");
		YYABORT;
	}
	| error
	{
		dceerror("expected a predefined type (such as \"fpage\" or similar)");
		YYABORT;
	}
	;

exception_declarator
	: EXCEPTION type_attribute_list type_spec declarator_list
	{ 
		$$ = new CFETypedDeclarator(TYPEDECL_EXCEPTION, $3, $4, $2); 
		// set parent relationship
		$3->SetParent($$);
		VectorElement *pCurrent;
		for (pCurrent = $4->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
		for (pCurrent = $2->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	| EXCEPTION type_spec declarator_list
	{ 
		$$ = new CFETypedDeclarator(TYPEDECL_EXCEPTION, $2, $3); 
		// set parent relationship
		$2->SetParent($$);
		VectorElement *pCurrent;
		for (pCurrent = $3->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	| error
	{
		dceerror("exception declarator expecting");
		YYABORT;
	}
	;

library
	: LBRACKET lib_attribute_list RBRACKET LIBRARY ID LBRACE lib_definitions RBRACE semicolon
	{ 
		$$ = new CFELibrary(new CFEIdentifier($5), $2, $7);
		VectorElement *pCurrent;
		for (pCurrent = $2->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
		for (pCurrent = $7->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
	}
	| LIBRARY ID LBRACE lib_definitions RBRACE semicolon
	{ 
		CFEAttribute *tmp = new CFEVersionAttribute(1,0);
		$$ = new CFELibrary(new CFEIdentifier($2), new Vector(RUNTIME_CLASS(CFEAttribute), 1, tmp), $4); 
		VectorElement *pCurrent;
		for (pCurrent = $4->GetFirst(); pCurrent != NULL; pCurrent = pCurrent->GetNext()) {
			if (pCurrent->GetElement() != NULL)
				pCurrent->GetElement()->SetParent($$);
		}
		tmp->SetParent($$);
	}
	;

lib_attribute_list :
	  lib_attribute_list COMMA lib_attribute
	{
		$1->Add($3);
		$$ = $1;
	}
	| lib_attribute 
	{
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, $1);
	}
	;

lib_attribute
	: UUID LPAREN uuid_rep rparen 
	{ 
		$$ = new CFEStringAttribute(ATTR_UUID, $3); 
	}
	| VERSION LPAREN version_rep rparen 
	{ 
		$$ = new CFEVersionAttribute($3); 
	}
	| CONTROL 
	{ 
		$$ = new CFEAttribute(ATTR_CONTROL); 
	}
	| HELPCONTEXT LPAREN LIT_INT rparen 
	{ 
		$$ = new CFEIntAttribute(ATTR_HELPCONTEXT, $3); 
	}
	| HELPFILE LPAREN string rparen 
	{ 
		$$ = new CFEStringAttribute(ATTR_HELPFILE, $3); 
	}
	| HELPSTRING LPAREN string rparen 
	{ 
		$$ = new CFEStringAttribute(ATTR_HELPSTRING, $3); 
	}
	| HIDDEN 
	{ 
		$$ = new CFEAttribute(ATTR_HIDDEN); 
	}
	| LCID LPAREN LIT_INT rparen 
	{ 
		$$ = new CFEIntAttribute(ATTR_LCID, $3); 
	}
	| RESTRICTED 
	{ 
		$$ = new CFEAttribute(ATTR_RESTRICTED); 
	}
	| error
	{
		dceerror("expected a library attribute");
		YYABORT;
	}
	;

lib_definitions :
	  lib_definitions lib_definition
	{
		if ($2 != NULL)
			$1->Add($2);
		$$ = $1;
	}
	| lib_definition
	{
		if ($1 != NULL)
			$$ = new Vector(RUNTIME_CLASS(CFEFileComponent), 1, $1);
		else
			$$ = new Vector(RUNTIME_CLASS(CFEFileComponent));
	}
	| error
	{
		dceerror("library element ecpected");
		YYABORT;
	}
	;

lib_definition :
	  interface { $$ = $1; } 
	| type_declarator semicolon  { $$ = $1; }
	| const_declarator semicolon  { $$ = $1; }
	| tagged_declarator semicolon  { $$ = $1; }
	| gcc_program { $$ = NULL; }
	;

/* helper definitions */
uuid_rep
	: QUOT UUID_STR QUOT 
	{ $$ = $2; }
	| UUID_STR 
	{ $$ = $1; }
	| error
	{
		dceerror("expected a UUID representation");
		YYABORT;
	}
	;

opt_semicolon :
	  SEMICOLON
	|
	;

semicolon
	: SEMICOLON
	| error 
	{  
		dceerror("expecting ';'"); 
		YYABORT;
	}
	;

rbrace
	: RBRACE
	| error 
	{ 
		dceerror("expecting '}'"); 
		YYABORT;
	}
	;

rbracket
	: RBRACKET
	| error 
	{ 
		dceerror("expecting ']'"); 
		YYABORT;
	}
	;

rparen
	: RPAREN
	| error 
	{ 
		dceerror("expecting ')'"); 
		YYABORT;
	}
	;

colon
	: COLON
	| error 
	{ 
		dceerror("expecting ':'"); 
		YYABORT;
	}
	;

/*********************************************************************************
 * start GCC specific rules
 *********************************************************************************/

gcc_program: /* empty */
	| gcc_extdefs
	;

gcc_extdefs:
	gcc_extdef
	| gcc_extdefs gcc_extdef
	;

gcc_extdef:
	gcc_fndef
	| gcc_datadef
	| ASM_KEYWORD LPAREN gcc_expr RPAREN SEMICOLON
	| gcc_extension gcc_extdef
	;

gcc_datadef:
	  gcc_setspecs gcc_notype_initdecls SEMICOLON
	| gcc_declmods gcc_setspecs gcc_notype_initdecls SEMICOLON
	| gcc_typed_declspecs gcc_setspecs gcc_initdecls SEMICOLON
    | gcc_declmods SEMICOLON
	| gcc_typed_declspecs SEMICOLON
	| error SEMICOLON
	| error RBRACE
	| SEMICOLON
	;

gcc_fndef:
	  gcc_typed_declspecs gcc_setspecs gcc_declarator gcc_old_style_parm_decls gcc_compstmt_or_error
	| gcc_typed_declspecs gcc_setspecs gcc_declarator error		
	| gcc_declmods gcc_setspecs gcc_notype_declarator gcc_old_style_parm_decls gcc_compstmt_or_error
	| gcc_declmods gcc_setspecs gcc_notype_declarator error
	| gcc_setspecs gcc_notype_declarator gcc_old_style_parm_decls gcc_compstmt_or_error
	| gcc_setspecs gcc_notype_declarator error
	;

gcc_identifier:
	ID {}
	;

gcc_unop: BITAND
	| MINUS
	| PLUS
	| INC_OP
	| DEC_OP
	| TILDE
	| EXCLAM
	;

gcc_expr:	gcc_nonnull_exprlist
	;

gcc_exprlist:
	  /* empty */
	| gcc_nonnull_exprlist
	;

gcc_nonnull_exprlist:
	gcc_expr_no_commas
	| gcc_nonnull_exprlist COMMA gcc_expr_no_commas
	;

gcc_unary_expr:
	gcc_primary
	| ASTERISK gcc_cast_expr   
	| gcc_extension gcc_cast_expr	  
	| gcc_unop gcc_cast_expr  
	| LOGICALAND gcc_identifier
	| gcc_sizeof gcc_unary_expr  
	| gcc_sizeof LPAREN gcc_typename RPAREN
	| gcc_alignof gcc_unary_expr  
	| gcc_alignof LPAREN gcc_typename RPAREN
	| REALPART gcc_cast_expr 
	| IMAGPART gcc_cast_expr 
	;

gcc_sizeof:
	SIZEOF
	;

gcc_alignof:
	ALIGNOF
	;

gcc_cast_expr:
	gcc_unary_expr
	| LPAREN gcc_typename RPAREN gcc_cast_expr  
	| LPAREN gcc_typename RPAREN LBRACE gcc_initlist_maybe_comma RBRACE  
	;

gcc_expr_no_commas:
	  gcc_cast_expr
	| gcc_expr_no_commas PLUS gcc_expr_no_commas
	| gcc_expr_no_commas MINUS gcc_expr_no_commas
	| gcc_expr_no_commas ASTERISK gcc_expr_no_commas
	| gcc_expr_no_commas DIV gcc_expr_no_commas
	| gcc_expr_no_commas MOD gcc_expr_no_commas
	| gcc_expr_no_commas LSHIFT gcc_expr_no_commas
	| gcc_expr_no_commas RSHIFT gcc_expr_no_commas
	| gcc_expr_no_commas LTEQUAL gcc_expr_no_commas
	| gcc_expr_no_commas GTEQUAL gcc_expr_no_commas
	| gcc_expr_no_commas EQUAL gcc_expr_no_commas
	| gcc_expr_no_commas NOTEQUAL gcc_expr_no_commas
	| gcc_expr_no_commas BITAND gcc_expr_no_commas
	| gcc_expr_no_commas BITOR gcc_expr_no_commas
	| gcc_expr_no_commas BITXOR gcc_expr_no_commas
	| gcc_expr_no_commas LOGICALAND gcc_expr_no_commas
	| gcc_expr_no_commas LOGICALOR gcc_expr_no_commas
	| gcc_expr_no_commas QUESTION gcc_expr COLON gcc_expr_no_commas
	| gcc_expr_no_commas QUESTION COLON gcc_expr_no_commas
	| gcc_expr_no_commas IS gcc_expr_no_commas
	| gcc_expr_no_commas RS_ASSIGN gcc_expr_no_commas
	| gcc_expr_no_commas LS_ASSIGN gcc_expr_no_commas
	| gcc_expr_no_commas ADD_ASSIGN gcc_expr_no_commas
	| gcc_expr_no_commas SUB_ASSIGN gcc_expr_no_commas
	| gcc_expr_no_commas MUL_ASSIGN gcc_expr_no_commas
	| gcc_expr_no_commas DIV_ASSIGN gcc_expr_no_commas
	| gcc_expr_no_commas MOD_ASSIGN gcc_expr_no_commas
	| gcc_expr_no_commas AND_ASSIGN gcc_expr_no_commas
	| gcc_expr_no_commas XOR_ASSIGN gcc_expr_no_commas
	| gcc_expr_no_commas OR_ASSIGN gcc_expr_no_commas
	;

gcc_primary:
	ID {}
	| LIT_INT {}
	| LIT_FLOAT {}
	| gcc_string
	| LPAREN gcc_expr RPAREN
	| LPAREN error RPAREN
	| LPAREN gcc_compstmt RPAREN
	| gcc_primary LPAREN gcc_exprlist RPAREN   %prec DOT
	| gcc_primary LBRACKET gcc_expr RBRACKET   %prec DOT
	| gcc_primary DOT gcc_identifier
	| gcc_primary PTR_OP gcc_identifier
	| gcc_primary INC_OP
	| gcc_primary DEC_OP
	;

gcc_string:
	  STRING
	| gcc_string STRING
	;


gcc_old_style_parm_decls:
	/* gcc_empty */
	| gcc_datadecls
	| gcc_datadecls ELLIPSIS
	;

gcc_lineno_datadecl:
	  gcc_save_filename gcc_save_lineno gcc_datadecl
	;

gcc_datadecls:
	gcc_lineno_datadecl
	| gcc_errstmt
	| gcc_datadecls gcc_lineno_datadecl
	| gcc_lineno_datadecl gcc_errstmt
	;

gcc_datadecl:
	gcc_typed_declspecs_no_prefix_attr gcc_setspecs gcc_initdecls SEMICOLON
	| gcc_declmods_no_prefix_attr gcc_setspecs gcc_notype_initdecls SEMICOLON
	| gcc_typed_declspecs_no_prefix_attr SEMICOLON
	| gcc_declmods_no_prefix_attr SEMICOLON
	;

gcc_lineno_decl:
	  gcc_save_filename gcc_save_lineno gcc_decl
	;

gcc_decls:
	gcc_lineno_decl
	| gcc_errstmt
	| gcc_decls gcc_lineno_decl
	| gcc_lineno_decl gcc_errstmt
	;

gcc_setspecs: /* empty */
	;

gcc_setattrs: /* empty */
	;

gcc_decl:
	gcc_typed_declspecs gcc_setspecs gcc_initdecls SEMICOLON
	| gcc_declmods gcc_setspecs gcc_notype_initdecls SEMICOLON
	| gcc_typed_declspecs gcc_setspecs gcc_nested_function
	| gcc_declmods gcc_setspecs gcc_notype_nested_function
	| gcc_typed_declspecs SEMICOLON
	| gcc_declmods SEMICOLON
	| gcc_extension gcc_decl
	;

gcc_typed_declspecs:
	  gcc_typespec gcc_reserved_declspecs
	| gcc_declmods gcc_typespec gcc_reserved_declspecs
	;

gcc_reserved_declspecs:  /* empty */
	| gcc_reserved_declspecs gcc_typespecqual_reserved
	| gcc_reserved_declspecs gcc_scspec
	| gcc_reserved_declspecs gcc_attributes
	;

gcc_typed_declspecs_no_prefix_attr:
	  gcc_typespec gcc_reserved_declspecs_no_prefix_attr
	| gcc_declmods_no_prefix_attr gcc_typespec gcc_reserved_declspecs_no_prefix_attr
	;

gcc_reserved_declspecs_no_prefix_attr:
	  /* gcc_empty */
	| gcc_reserved_declspecs_no_prefix_attr gcc_typespecqual_reserved
	| gcc_reserved_declspecs_no_prefix_attr gcc_scspec
	;

gcc_declmods:
	  gcc_declmods_no_prefix_attr
	| gcc_attributes
	| gcc_declmods gcc_declmods_no_prefix_attr
	| gcc_declmods gcc_attributes
	;

gcc_declmods_no_prefix_attr:
	  gcc_type_qual
	| gcc_scspec
	| gcc_declmods_no_prefix_attr gcc_type_qual
	| gcc_declmods_no_prefix_attr gcc_scspec
	;

gcc_typed_typespecs:
	  gcc_typespec gcc_reserved_typespecquals
	| gcc_nonempty_type_quals gcc_typespec gcc_reserved_typespecquals
	;

gcc_reserved_typespecquals:  /* empty */
	| gcc_reserved_typespecquals gcc_typespecqual_reserved
	;

gcc_typespec: 
	| gcc_structsp
	| ID {}
	| TYPEOF LPAREN gcc_expr RPAREN
	| TYPEOF LPAREN gcc_typename RPAREN
	;

gcc_typespecqual_reserved:
	  gcc_typespec
	| gcc_type_qual
	;

gcc_initdecls:
	gcc_initdcl
	| gcc_initdecls COMMA gcc_initdcl
	;

gcc_notype_initdecls:
	gcc_notype_initdcl
	| gcc_notype_initdecls COMMA gcc_initdcl
	;

gcc_maybeasm:
	  /* empty */
	| ASM_KEYWORD LPAREN gcc_string RPAREN
	;

gcc_initdcl:
	  gcc_declarator gcc_maybeasm gcc_maybe_attribute IS gcc_init
	| gcc_declarator gcc_maybeasm gcc_maybe_attribute
	;

gcc_notype_initdcl:
	  gcc_notype_declarator gcc_maybeasm gcc_maybe_attribute IS gcc_init
	| gcc_notype_declarator gcc_maybeasm gcc_maybe_attribute
	;

gcc_maybe_attribute:
      /* empty */
	| gcc_attributes
	;
 
gcc_attributes:
      gcc_attribute
	| gcc_attributes gcc_attribute
	;

gcc_attribute:
      ATTRIBUTE LPAREN LPAREN gcc_attribute_list RPAREN RPAREN
	;

gcc_attribute_list:
      gcc_attrib
	| gcc_attribute_list COMMA gcc_attrib
	;
 
gcc_attrib:
    /* empty */
	| gcc_any_word
	| gcc_any_word LPAREN ID RPAREN {}
	| gcc_any_word LPAREN ID COMMA gcc_nonnull_exprlist RPAREN {}
	| gcc_any_word LPAREN gcc_exprlist RPAREN
	;

gcc_any_word:
	  gcc_identifier
	| gcc_scspec
	| gcc_typespec
	| gcc_type_qual
	;

gcc_scspec:
	  INLINE
	| ITERATOR
	| AUTO
	| EXTERN
	| REGISTER
	| STATIC
	| TYPEDEF
	;

gcc_type_qual:
	  CONST
	| RESTRICT
	| VOLATILE
	| BYCOPY
	| BYREF
	| IN
	| INOUT
	| ONEWAY
	| OUT
	;

gcc_typespec:
	  COMPLEX
	| SIGNED
	| CHAR
	| DOUBLE
	| FLOAT
	| INT
	| LONG
	| SHORT
	| UNSIGNED
	| VOID
	;

gcc_init:
	gcc_expr_no_commas
	| LBRACE gcc_initlist_maybe_comma RBRACE
	| error
	;

gcc_initlist_maybe_comma:
	  /* empty */
	| gcc_initlist1 gcc_maybecomma
	;

gcc_initlist1:
	  gcc_initelt
	| gcc_initlist1 COMMA gcc_initelt
	;

gcc_initelt:
	  gcc_designator_list IS gcc_initval
	| gcc_designator gcc_initval
	| gcc_identifier COLON gcc_initval
	| gcc_initval
	;

gcc_initval:
	  LBRACE gcc_initlist_maybe_comma RBRACE
	| gcc_expr_no_commas
	| error
	;

gcc_designator_list:
	  gcc_designator
	| gcc_designator_list gcc_designator
	;

gcc_designator:
	  DOT gcc_identifier
	| LBRACKET gcc_expr_no_commas ELLIPSIS gcc_expr_no_commas RBRACKET
	| LBRACKET gcc_expr_no_commas RBRACKET
	;

gcc_nested_function:
	  gcc_declarator gcc_old_style_parm_decls gcc_compstmt
	;

gcc_notype_nested_function:
	  gcc_notype_declarator gcc_old_style_parm_decls gcc_compstmt
	;

gcc_declarator:
	  LPAREN gcc_declarator RPAREN
	| gcc_declarator LPAREN gcc_parmlist_or_identifiers  %prec DOT
	| gcc_attributes gcc_setattrs gcc_declarator
	| gcc_notype_declarator
	;

gcc_notype_declarator:
	  gcc_notype_declarator LPAREN gcc_parmlist_or_identifiers  %prec DOT
	| LPAREN gcc_notype_declarator RPAREN
	| ASTERISK gcc_type_quals gcc_notype_declarator  
	| gcc_notype_declarator LBRACKET ASTERISK RBRACKET  %prec DOT
	| gcc_notype_declarator LBRACKET gcc_expr RBRACKET  %prec DOT
	| gcc_notype_declarator LBRACKET RBRACKET  %prec DOT
	| gcc_attributes gcc_setattrs gcc_notype_declarator
	| ID {}
	;

gcc_parm_declarator:
	  gcc_parm_declarator LPAREN gcc_parmlist_or_identifiers  %prec DOT
	| gcc_parm_declarator LBRACKET ASTERISK RBRACKET  %prec DOT
	| gcc_parm_declarator LBRACKET gcc_expr RBRACKET  %prec DOT
	| gcc_parm_declarator LBRACKET RBRACKET  %prec DOT
	| ASTERISK gcc_type_quals gcc_parm_declarator  
	| gcc_attributes gcc_setattrs gcc_parm_declarator
	| ID {}
	;

gcc_struct_head:
	  STRUCT
	| STRUCT gcc_attributes
	;

gcc_union_head:
	  UNION
	| UNION gcc_attributes
	;

gcc_enum_head:
	  ENUM
	| ENUM gcc_attributes
	;

gcc_structsp:
	  gcc_struct_head gcc_identifier LBRACE gcc_component_decl_list RBRACE gcc_maybe_attribute 
	| gcc_struct_head LBRACE gcc_component_decl_list RBRACE gcc_maybe_attribute
	| gcc_struct_head gcc_identifier
	| gcc_union_head gcc_identifier LBRACE gcc_component_decl_list RBRACE gcc_maybe_attribute
	| gcc_union_head LBRACE gcc_component_decl_list RBRACE gcc_maybe_attribute
	| gcc_union_head gcc_identifier
	| gcc_enum_head gcc_identifier LBRACE gcc_enumlist gcc_maybecomma_warn RBRACE gcc_maybe_attribute
	| gcc_enum_head LBRACE gcc_enumlist gcc_maybecomma_warn RBRACE gcc_maybe_attribute
	| gcc_enum_head gcc_identifier
	;

gcc_maybecomma:
	  /* empty */
	| COMMA
	;

gcc_maybecomma_warn:
	  /* empty */
	| COMMA
	;

gcc_component_decl_list:
	  gcc_component_decl_list2
	| gcc_component_decl_list2 gcc_component_decl
	;

gcc_component_decl_list2:	/* empty */
	| gcc_component_decl_list2 gcc_component_decl SEMICOLON
	| gcc_component_decl_list2 SEMICOLON
	;

gcc_component_decl:
	  gcc_typed_typespecs gcc_setspecs gcc_components
	| gcc_typed_typespecs
	| gcc_nonempty_type_quals gcc_setspecs gcc_components
	| gcc_nonempty_type_quals
	| error
	| gcc_extension gcc_component_decl
	;

gcc_components:
	  gcc_component_declarator
	| gcc_components COMMA gcc_component_declarator
	;

gcc_component_declarator:
	  gcc_save_filename gcc_save_lineno gcc_declarator gcc_maybe_attribute
	| gcc_save_filename gcc_save_lineno gcc_declarator COLON gcc_expr_no_commas gcc_maybe_attribute
	| gcc_save_filename gcc_save_lineno COLON gcc_expr_no_commas gcc_maybe_attribute
	;

gcc_enumlist:
	  gcc_enumerator
	| gcc_enumlist COMMA gcc_enumerator
	| error
	;

gcc_enumerator:
	  gcc_identifier
	| gcc_identifier IS gcc_expr_no_commas
	;

gcc_typename:
	gcc_typed_typespecs gcc_absdcl
	| gcc_nonempty_type_quals gcc_absdcl
	;

gcc_absdcl:   /* gcc_an gcc_absolute gcc_declarator */
	/* empty */
	| gcc_absdcl1
	;

gcc_nonempty_type_quals:
	  gcc_type_qual
	| gcc_nonempty_type_quals gcc_type_qual
	;

gcc_type_quals:
	  /* empty */
	| gcc_type_quals gcc_type_qual
	;

gcc_absdcl1:  /* gcc_a gcc_nonempty gcc_absolute gcc_declarator */
	  LPAREN gcc_absdcl1 RPAREN
	| ASTERISK gcc_type_quals gcc_absdcl1  
	| ASTERISK gcc_type_quals  
	| gcc_absdcl1 LPAREN gcc_parmlist  %prec DOT
	| gcc_absdcl1 LBRACKET gcc_expr RBRACKET  %prec DOT
	| gcc_absdcl1 LBRACKET RBRACKET  %prec DOT
	| LPAREN gcc_parmlist  %prec DOT
	| LBRACKET gcc_expr RBRACKET  %prec DOT
	| LBRACKET RBRACKET  %prec DOT
	| gcc_attributes gcc_setattrs gcc_absdcl1
	;

gcc_stmts:
	gcc_lineno_stmt_or_labels
	;

gcc_lineno_stmt_or_labels:
	  gcc_lineno_stmt_or_label
	| gcc_lineno_stmt_or_labels gcc_lineno_stmt_or_label
	| gcc_lineno_stmt_or_labels gcc_errstmt
	;

gcc_xstmts:
	/* empty */
	| gcc_stmts
	;

gcc_errstmt:  error SEMICOLON
	;

gcc_pushlevel:  /* empty */
	;

gcc_maybe_label_decls:
	  /* empty */
	| gcc_label_decls
	;

gcc_label_decls:
	  gcc_label_decl
	| gcc_label_decls gcc_label_decl
	;

gcc_label_decl:
	  LABEL gcc_identifiers_or_typenames SEMICOLON
	;

gcc_compstmt_or_error:
	  gcc_compstmt
	| error gcc_compstmt
	;

gcc_compstmt_start: LBRACE 

gcc_compstmt: gcc_compstmt_start RBRACE
	| gcc_compstmt_start gcc_pushlevel gcc_maybe_label_decls gcc_decls gcc_xstmts RBRACE
	| gcc_compstmt_start gcc_pushlevel gcc_maybe_label_decls error RBRACE
	| gcc_compstmt_start gcc_pushlevel gcc_maybe_label_decls gcc_stmts RBRACE
	;

gcc_simple_if:
	  gcc_if_prefix gcc_lineno_labeled_stmt
	| gcc_if_prefix error
	;

gcc_if_prefix:
	  IF LPAREN gcc_expr RPAREN
	;

gcc_do_stmt_start:
	  DO
	  gcc_lineno_labeled_stmt WHILE
	;

gcc_save_filename:
	;

gcc_save_lineno:
	;

gcc_lineno_labeled_stmt:
	  gcc_save_filename gcc_save_lineno gcc_stmt
	| gcc_save_filename gcc_save_lineno gcc_label gcc_lineno_labeled_stmt
	;

gcc_lineno_stmt_or_label:
	  gcc_save_filename gcc_save_lineno gcc_stmt_or_label
	;

gcc_stmt_or_label:
	  gcc_stmt
	| gcc_label
	;

gcc_stmt:
	  gcc_compstmt
    | gcc_all_iter_stmt 
	| gcc_expr SEMICOLON
	| gcc_simple_if ELSE gcc_lineno_labeled_stmt
	| gcc_simple_if %prec IF
	| gcc_simple_if ELSE error
	| WHILE LPAREN gcc_expr RPAREN gcc_lineno_labeled_stmt
	| gcc_do_stmt_start LPAREN gcc_expr RPAREN SEMICOLON
	| gcc_do_stmt_start error
	| FOR LPAREN gcc_xexpr SEMICOLON gcc_xexpr SEMICOLON gcc_xexpr RPAREN gcc_lineno_labeled_stmt
	| SWITCH LPAREN gcc_expr RPAREN gcc_lineno_labeled_stmt
	| BREAK SEMICOLON
	| CONTINUE SEMICOLON
	| RETURN SEMICOLON
	| RETURN gcc_expr SEMICOLON
	| ASM_KEYWORD gcc_maybe_type_qual LPAREN gcc_expr RPAREN SEMICOLON
	| ASM_KEYWORD gcc_maybe_type_qual LPAREN gcc_expr COLON gcc_asm_operands RPAREN SEMICOLON
	| ASM_KEYWORD gcc_maybe_type_qual LPAREN gcc_expr COLON gcc_asm_operands COLON gcc_asm_operands RPAREN SEMICOLON
	| ASM_KEYWORD gcc_maybe_type_qual LPAREN gcc_expr COLON gcc_asm_operands COLON gcc_asm_operands COLON gcc_asm_clobbers RPAREN SEMICOLON
	| GOTO gcc_identifier SEMICOLON
	| GOTO ASTERISK gcc_expr SEMICOLON
	| SEMICOLON
	;

gcc_all_iter_stmt:
	  gcc_all_iter_stmt_simple
	;

gcc_all_iter_stmt_simple:
	  FOR LPAREN gcc_primary RPAREN gcc_lineno_labeled_stmt

gcc_label:	  CASE gcc_expr_no_commas COLON
	| CASE gcc_expr_no_commas ELLIPSIS gcc_expr_no_commas COLON
	| DEFAULT COLON
	| gcc_identifier COLON gcc_maybe_attribute
	;

gcc_maybe_type_qual:
	/* empty */
	| gcc_type_qual
	;

gcc_xexpr:
	/* empty */
	| gcc_expr
	;

gcc_asm_operands: /* empty */
	| gcc_nonnull_asm_operands
	;

gcc_nonnull_asm_operands:
	  gcc_asm_operand
	| gcc_nonnull_asm_operands COMMA gcc_asm_operand
	;

gcc_asm_operand:
	  STRING LPAREN gcc_expr RPAREN
	;

gcc_asm_clobbers:
	  gcc_string
	| gcc_asm_clobbers COMMA gcc_string
	;

gcc_parmlist:
	  gcc_parmlist_1
	;

gcc_parmlist_1:
	  gcc_parmlist_2 RPAREN
	| gcc_parms SEMICOLON gcc_parmlist_1
	| error RPAREN
	;

gcc_parmlist_2:  /* empty */
	| ELLIPSIS
	| gcc_parms
	| gcc_parms COMMA ELLIPSIS
	;

gcc_parms:
	gcc_parm
	| gcc_parms COMMA gcc_parm
	;

gcc_parm:
	  gcc_typed_declspecs gcc_setspecs gcc_parm_declarator gcc_maybe_attribute
	| gcc_typed_declspecs gcc_setspecs gcc_notype_declarator gcc_maybe_attribute
	| gcc_typed_declspecs gcc_setspecs gcc_absdcl gcc_maybe_attribute
	| gcc_declmods gcc_setspecs gcc_notype_declarator gcc_maybe_attribute
	| gcc_declmods gcc_setspecs gcc_absdcl gcc_maybe_attribute
	;

gcc_parmlist_or_identifiers:
	  gcc_parmlist_or_identifiers_1
	;

gcc_parmlist_or_identifiers_1:
	  gcc_parmlist_1
	| gcc_identifiers RPAREN
	;

gcc_identifiers:
	ID {}
	| gcc_identifiers COMMA ID {}
	;

gcc_identifiers_or_typenames:
	gcc_identifier
	| gcc_identifiers_or_typenames COMMA gcc_identifier
	;

gcc_extension:
	EXTENSION
	;

/*********************************************************************************
 * end GCC specific rules
 *********************************************************************************/

%%

extern int nLineNbDCE;

void
dceerror(char* s)
{
	fprintf(stderr, "(DICE Parser): Error in %s, line %d: %s.\n", sInFileName, nLineNbDCE, s);
	erroccured = 1;
	errcount++;
}

void
dceerror2(char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	erroccured = 1;
	errcount++;
}