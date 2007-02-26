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

// from dce scanner
extern FILE* dcein;
// C scanner file
extern FILE* cin;
// C parser functions and variables
extern int cdebug;
extern int cparse();

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
extern int nLineNbDCE;

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
  enum EXPT_OPERATOR		_exp_operator;

  CFEExpression*			_expr;

  CFEIdentifier*			_identifier;
  CFEDeclarator*			_decl;
  CFEArrayDeclarator*		_array_decl;
  CFEConstDeclarator*		_const_decl;
  CFETypedDeclarator*		_typed_decl;
  CFEFunctionDeclarator*	_func_decl;
  CFEEnumDeclarator*		_enum_decl;

  CFETypeSpec*				_type_spec;
  CFESimpleType*			_simple_type;
  CFEConstructedType*		_constructed_type;
  CFEStructType*			_struct_type;
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

}

%token	LBRACE
%token	RBRACE
%token	LBRACKET
%token	RBRACKET
%token	COLON
%token	COMMA
%token	LPAREN
%token	RPAREN
%token	DOT
%token	QUOT
%token	ASTERISK
%token	SINGLEQUOT
%token	QUESTION
%token	BITOR
%token	BITXOR
%token	BITAND
%token	LT
%token	GT
%token	PLUS
%token	MINUS
%token	DIV
%token	MOD
%token	TILDE
%token	EXCLAM
%token	SEMICOLON
%token	LOGICALOR
%token	LOGICALAND
%token	EQUAL
%token	NOTEQUAL
%token	LTEQUAL
%token	GTEQUAL
%token	LSHIFT
%token	RSHIFT
%token	DOTDOT

%token	IS
%token	BOOLEAN
%token	BYTE
%token	CASE
%token	CHAR
%token	CONST
%token	DEFAULT
%token	DOUBLE
%token	ENUM
%token	FALSE
%token	FLOAT
%token	HANDLE_T
%token	HYPER
%token	IMPORT
%token	INT
%token	INTERFACE
%token	LONG
%token	EXPNULL
%token	PIPE
%token	SHORT
%token	SMALL
%token	STRUCT
%token	SWITCH
%token	TRUE
%token	TYPEDEF
%token	UNION
%token	UNSIGNED
%token	SIGNED
%token	VOID
%token	ERROR_STATUS_T
%token	FLEXPAGE
%token	REFSTRING
%token	OBJECT
%token	IID_IS
%token	ISO_LATIN_1 
%token	ISO_MULTI_LINGUAL
%token	ISO_UCS


%token	BROADCAST
%token	CONTEXT_HANDLE
%token	ENDPOINT
%token	EXCEPTIONS
%token	FIRST_IS
%token	HANDLE
%token	IDEMPOTENT
%token	IGNORE
%token	IN
%token	LAST_IS
%token	LENGTH_IS
%token	LOCAL
%token	MAX_IS
%token	MAYBE

%token	MIN_IS
%token	OUT
%token	PTR
%token	POINTER_DEFAULT
%token	REF
%token	REFLECT_DELETIONS
%token	SIZE_IS
%token	STRING
%token	SWITCH_IS
%token	SWITCH_TYPE
%token	TRANSMIT_AS
%token	UNIQUE
%token	UUID
%token	VERSION

%token	RAISES
%token	EXCEPTION

%token	LIBRARY
%token	CONTROL
%token	HELPCONTEXT
%token	HELPFILE
%token	HELPSTRING
%token  DEFAULT_FUNCTION
%token	HIDDEN
%token	LCID
%token	RESTRICTED

%token	AUTO_HANDLE
%token	BINDING_CALLOUT
%token	CODE
%token	COMM_STATUS
%token	CS_CHAR
%token	CS_DRTAG
%token	CS_RTAG
%token	CS_STAG
%token	TAG_RTN
%token	ENABLE_ALLOCATE
%token	EXTERN_EXCEPTIONS
%token	EXPLICIT_HANDLE
%token	FAULT_STATUS
%token	HEAP
%token	IMPLICIT_HANDLE
%token	NOCODE
%token	REPRESENT_AS
%token	USER_MARSHAL

%token	WITHOUT_USING_EXCEPTIONS

/* token to eliminate reduce/reduce conflicts */
%token	BOGUS_1

/***********************************************
 * C specific tokens
 ***********************************************/
%token TYPE_NAME

%token SIZEOF
%token PTR_OP 
%token INC_OP 
%token DEC_OP 
%token MUL_ASSIGN 
%token DIV_ASSIGN 
%token MOD_ASSIGN 
%token ADD_ASSIGN
%token SUB_ASSIGN 
%token LEFT_ASSIGN 
%token RIGHT_ASSIGN 
%token AND_ASSIGN
%token XOR_ASSIGN 
%token OR_ASSIGN 

%token EXTERN 
%token STATIC 
%token AUTO 
%token REGISTER
%token VOLATILE 
%token ELLIPSIS

%token IF 
%token ELSE 
%token WHILE 
%token DO 
%token FOR 
%token GOTO 
%token CONTINUE 
%token BREAK 
%token RETURN
/***********************************************
 * end C specific tokens
 ***********************************************/

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
%type <_type_spec>			constructed_type_spec
%type <_const_decl>			const_declarator
%type <_expr>				const_exp
%type <_collection>			const_exp_list
%type <_type_spec>			const_type_spec
%type <_decl>				declarator
%type <_collection>			declarator_list
%type <_attr>				directional_attribute
%type <_decl>				direct_declarator
%type <_type_spec>			enumeration_type
%type <_identifier>			enum_identifier
%type <_collection>			enum_identifier_list
%type <_expr>				equality_exp
%type <_typed_decl>			exception_declarator
%type <_collection>			excep_name_list
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
%type <_collection>			lib_attributes
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
%type <_struct_type>		struct_type
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

/********************************************
 * C specific types
 ********************************************/

%type <_collection>			c_translation_unit
%type <_expr>				c_expression
%type <_expr>				c_postfix_expression
%type <_expr>				c_cast_expression
%type <_expr>				c_assignment_expression
%type <_expr>				c_expression_statement
%type <_collection>			c_declaration_list
%type <_int>				c_pointer
%type <_collection>			c_external_declaration
%type <_collection>			c_declaration
%type <_collection>			c_declaration_specifiers
%type <_type_spec>			type_spec

/********************************************
 * end C specific types
 ********************************************/

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
	| type_declarator SEMICOLON 
	{ 
		pCurFile->AddTypedef($1);
	}
	| const_declarator SEMICOLON 
	{ 
		pCurFile->AddConstant($1);
	}
	| tagged_declarator SEMICOLON 
	{ 
		pCurFile->AddTaggedDecl($1);
	}
	| import opt_semicolon
	{
		// XXX add elements of import to file
	}
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
	} LBRACE import_list interface_component_list RBRACE opt_semicolon
	{ 
		// append imported elements
		$9->Add($8);
		$$ = new CFEInterface($1, new CFEIdentifier($3), $5, $9);
		$1->SetParentOfElements($$);
		$9->SetParentOfElements($$);
		// set parent relationship
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
	} LBRACE import_list interface_component_list RBRACE opt_semicolon
	{ 
		// append imported elements to interface components
		$7->Add($6);
		$$ = new CFEInterface($1, new CFEIdentifier($3), NULL, $7);
		$1->SetParentOfElements($$);
		$7->SetParentOfElements($$);
		// set parent relationship
	}
	;

interface_attributes :
	  LBRACKET interface_attribute_list rbracket { $$ = $2; }
	| LBRACKET rbracket
	{
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, new CFEVersionAttribute(0,0)); 
	}
	| /* no attributes at all */
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
    | DEFAULT_FUNCTION LPAREN ID rparen
    { $$ = new CFEStringAttribute(ATTR_DEFAULT_FUNCTION, $3);}
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
	  IMPORT LBRACE c_translation_unit RBRACE opt_semicolon
	{
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
	  export SEMICOLON 
	{ 
		//$1->SetFile(pCurFile);
		$$ = $1; 
	}
	| op_declarator SEMICOLON 
	{ 
		//$1->SetFile(pCurFile);
		$$ = $1; 
	}
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
	| logical_or_exp QUESTION c_expression COLON conditional_exp 
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
	: c_cast_expression	{ $$ = $1; /* contains unary_exp */ }
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

/* was: 
 * unary_exp: unary_operator primary_exp | primary_exp ;
 * but had to be extended for C
 */ 
unary_exp 
	: c_postfix_expression
	{ $$ = $1; }
	| unary_operator c_cast_expression
	{ 
		$$ = new CFEUnaryExpression(EXPR_UNARY, $1, $2); 
		// set parent relationship
		$2->SetParent($$);
	}
/* C specific declarations */
	| INC_OP unary_exp
	{ $$ = NULL; }
	| DEC_OP unary_exp
	{ $$ = NULL; }
	| SIZEOF unary_exp
	{ $$ = NULL; }
	| SIZEOF LPAREN c_type_name RPAREN
	{ $$ = NULL; }
/* C++ specific declarations */
	| c_new_expression
	{ $$ = NULL; }
	;

unary_operator
	: ASTERISK
	{ $$ = EXPR_MUL; }
	| BITAND
	{ $$ = EXPR_BITAND; }
	| EXCLAM
	{ $$ = EXPR_EXCLAM; }
	| TILDE
	{ $$ = EXPR_TILDE; }
	| MINUS
	{ $$ = EXPR_MINUS; }
	| PLUS 
	{ $$ = EXPR_SPLUS; }
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
		$2->SetParentOfElements($$);
		$3->SetParent($$);
		$4->SetParentOfElements($$);
	}
	| TYPEDEF type_spec declarator_list
	{ 
		$$ = new CFETypedDeclarator(TYPEDECL_TYPEDEF, $2, $3); 
		// set parent relationship
		$2->SetParent($$);
		$3->SetParentOfElements($$);
	}
	| TYPEDEF type_attribute_list ID declarator_list
	{
		// user defined type redefined
		CFEUserDefinedType *tmp = new CFEUserDefinedType(new CFEIdentifier($3));
		$$ = new CFETypedDeclarator(TYPEDECL_TYPEDEF, tmp, $4, $2);
		tmp->SetParent($$);
		$4->SetParentOfElements($$);
		$2->SetParentOfElements($$);
	}
	| TYPEDEF ID declarator_list
	{
		// user defined type redefined
		CFEUserDefinedType *tmp = new CFEUserDefinedType(new CFEIdentifier($2));
		$$ = new CFETypedDeclarator(TYPEDECL_TYPEDEF, tmp, $3);
		tmp->SetParent($$);
		$3->SetParentOfElements($$);
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
	: c_pointer
	{
		// ignore free-standing pointers
		$$ = NULL;
	}
	| c_pointer direct_declarator 
	{ 
		if ($2 != NULL)
			$2->SetStars($1); 
		$$ = $2; 
	}
	| c_pointer direct_declarator COLON const_exp
	{
		if ($2 != NULL) {
			$2->SetStars($1); 
			if ($4 != NULL)
				$2->SetBitfields($4->GetIntValue());
		}
		$$ = $2; 
	}
	| direct_declarator { $$ = $1; }
	| direct_declarator COLON const_exp
	{
		if (($3 != NULL) && ($1 != NULL))
			$1->SetBitfields($3->GetIntValue());
		$$ = $1;
	}
	| COLON const_exp
	{
		dceerror("struct member needs name (no bitfield without name)\n");
		YYABORT;
	}
	;

direct_declarator 
	: ID 
	{ $$ = new CFEDeclarator(DECL_IDENTIFIER, $1); }
	| LPAREN declarator rparen { $$ = $2; }
	| array_declarator  { $$ = $1; }
	| function_declarator  { $$ = $1; }
/* the C specific part starts here (direct_abstract_declarator) */
	| LBRACKET RBRACKET
	{ $$ = NULL; }
	| LBRACKET const_exp RBRACKET
	{ $$ = NULL; }
	| LPAREN RPAREN
	{ $$ = NULL; }
	| LPAREN c_parameter_type_list RPAREN
	{ $$ = NULL; }
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
	: struct_type { $$ = $1; }
	| union_type { $$ = $1; }
	| enumeration_type { $$ = $1; }
	| PIPE type_spec 
	{ 
		$$ = new CFEPipeType($2); 
		// set parent relationship
		$2->SetParent($$);
	}
	;

struct_type
	: STRUCT LBRACE member_list rbrace 
	{ 
		$$ = new CFEStructType($3);
		$3->SetParentOfElements($$);
	}
	| tagged_struct_declarator 
	{ $$ = $1; }
	;

tagged_struct_declarator 
	: STRUCT tag LBRACE member_list rbrace 
	{ 
		$$ = new CFETaggedStructType(new CFEIdentifier($2), $4);
		$4->SetParentOfElements($$);
	}
	| STRUCT tag
	{ 
		$$ = new CFETaggedStructType(new CFEIdentifier($2)); 
	}
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
	: field_declarator SEMICOLON { $$ = $1; }
	;

field_declarator 
	: field_attribute_list type_spec declarator_list 
	{ 
		$$ = new CFETypedDeclarator(TYPEDECL_FIELD, $2, $3, $1);
		// set parent relationship
		$1->SetParentOfElements($$);
		$2->SetParent($$);
		$3->SetParentOfElements($$);
	}
	| type_spec declarator_list 
	{ 
		$$ = new CFETypedDeclarator(TYPEDECL_FIELD, $1, $2); 
		// set parent relationship
		$1->SetParent($$);
		$2->SetParentOfElements($$);
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
	| tagged_union_declarator { $$ = $1; }
	;

union_type_header
	: SWITCH LPAREN switch_type_spec ID rparen ID LBRACE union_body rbrace 
	{ 
		$$ = new CFEUnionType($3, new CFEIdentifier($4), $8, new CFEIdentifier($6));
		// set parent relationship
		$3->SetParent($$);
		$8->SetParentOfElements($$);
	}
	| SWITCH LPAREN switch_type_spec ID rparen LBRACE union_body rbrace 
	{ 
		$$ = new CFEUnionType($3, new CFEIdentifier($4), $7); 
		// set parent relationship
		$3->SetParent($$);
		$7->SetParentOfElements($$);
	}
	| LBRACE union_body_n_e rbrace 
	{ 
		$$ = new CFEUnionType($2); 
		$2->SetParentOfElements($$);
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
		$1->SetParentOfElements($$);
		$2->SetParent($$);
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
		// this one is to support C unions
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
	: field_declarator SEMICOLON { $$ = $1; }
	| SEMICOLON 
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

enum_identifier_list :
	  enum_identifier
	{
		$$ = new Vector(RUNTIME_CLASS(CFEIdentifier), 1, $1);

	}
	| enum_identifier_list COMMA enum_identifier
	{
		$1->Add($3);
		$$ = $1;
	}
	;

enum_identifier :
	  ID
	{ 
		$$ = new CFEIdentifier($1);
	}
	| ID IS const_exp
	{
		$$ = new CFEEnumDeclarator($1, $3);
	}
	;

enumeration_type 
	: ENUM LBRACE enum_identifier_list RBRACE 
	{ 
		$$ = new CFEEnumType($3);
		$3->SetParentOfElements($$);
	}
	| ENUM ID LBRACE enum_identifier_list RBRACE
	{
		$$ = new CFETaggedEnumType(new CFEIdentifier($2), $4);
		$4->SetParentOfElements($$);
	}
	| ENUM ID
	{
		// this form is only used if the type is used, not declared, so 
		// use a user-defined type here
		$$ = new CFEUserDefinedType(new CFEIdentifier($2));
	}
	;

array_declarator 
	: direct_declarator LBRACKET RBRACKET 
	{ 
		if ($1 != NULL) {
			if ($1->GetType() == DECL_ARRAY)
				$$ = (CFEArrayDeclarator*)$1;
			else {
				$$ = new CFEArrayDeclarator($1); 
				delete $1; 
			}
			$$->AddBounds(NULL, NULL);
		} else
			$$ = NULL;
	}
	| direct_declarator LBRACKET array_bound rbracket 
	{ 
		if ($1 != NULL) {
			if ($1->GetType() == DECL_ARRAY)
				$$ = (CFEArrayDeclarator*)$1;
			else {
				$$ = new CFEArrayDeclarator($1);
				delete $1; 
			}
			$$->AddBounds(NULL, $3);
			$3->SetParent($$);
		} else
			$$ = NULL;
	}
	| direct_declarator LBRACKET array_bound DOTDOT rbracket
	{
		if ($1 != NULL) {
			// "decl [ expr '..' ]" == "decl [ expr '..' '*' ]"
			CFEExpression *tmp = new CFEExpression(EXPR_CHAR, '*');
			if ($1->GetType() == DECL_ARRAY)
				$$ = (CFEArrayDeclarator*)$1;
			else {
				$$ = new CFEArrayDeclarator($1);
				delete $1;
			}
			$$->AddBounds($3, tmp);
			$3->SetParent($$);
			tmp->SetParent($$);
		} else
			$$ = NULL;
	}
	| direct_declarator LBRACKET array_bound DOTDOT array_bound rbracket 
	{ 
		if ($1 != NULL) {
			if ($1->GetType() == DECL_ARRAY)
				$$ = (CFEArrayDeclarator*)$1;
			else {
				$$ = new CFEArrayDeclarator($1); 
				delete $1; 
			}
			$$->AddBounds($3, $5);
			$3->SetParent($$);
			$5->SetParent($$);
		} else
			$$ = NULL;
	}
	;

array_bound 
	: ASTERISK 
	{ $$ = new CFEExpression(EXPR_CHAR, '*'); }
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
		$3->SetParentOfElements($$);
	}
	| LAST_IS LPAREN attr_var_list rparen 
	{ 
		$$ = new CFEIsAttribute(ATTR_LAST_IS, $3); 
		$3->SetParentOfElements($$);
	}
	| LENGTH_IS LPAREN attr_var_list rparen 
	{ 
		$$ = new CFEIsAttribute(ATTR_LENGTH_IS, $3); 
		$3->SetParentOfElements($$);
	}
	| MIN_IS LPAREN attr_var_list rparen 
	{ 
		$$ = new CFEIsAttribute(ATTR_MIN_IS, $3); 
		$3->SetParentOfElements($$);
	}
	| MAX_IS LPAREN attr_var_list rparen 
	{ 
		$$ = new CFEIsAttribute(ATTR_MAX_IS, $3); 
		$3->SetParentOfElements($$);
	}
	| SIZE_IS LPAREN attr_var_list rparen 
	{ 
		$$ = new CFEIsAttribute(ATTR_SIZE_IS, $3); 
		$3->SetParentOfElements($$);
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
		$3->SetParentOfElements($$);
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
		$1->SetParentOfElements($$);
		if ($4 != NULL) {
			$4->SetParentOfElements($$);
		}
		$5->SetParentOfElements($$);
	}

	| operation_attributes simple_type_spec ID param_declarators
	{ 
		$$ = new CFEOperation($2, new CFEIdentifier($3), $4, $1); 
		// set parent relationship
		$2->SetParent($$);
		$1->SetParentOfElements($$);
		if ($4 != NULL) {
			$4->SetParentOfElements($$);
		}
	}
	| simple_type_spec ID param_declarators raises_declarator
	{ 
		$$ = new CFEOperation($1, new CFEIdentifier($2), $3, NULL, $4); 
		// set parent relationship
		$1->SetParent($$);
		VectorElement *pCurrent;
		if ($3 != NULL) {
			$3->SetParentOfElements($$);
		}
		$4->SetParentOfElements($$);
	}
	| simple_type_spec ID param_declarators
	{ 
		$$ = new CFEOperation($1, new CFEIdentifier($2), $3); 
		// set parent relationship
		$1->SetParent($$);
		if ($3 != NULL) {
			$3->SetParentOfElements($$);
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
		$1->SetParentOfElements($$);
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

/* function declarators are used to reference functions (e.g. as parameters) */
function_declarator 
	: direct_declarator param_declarators 
	{ 
		if ($1 != NULL) {
			$$ = new CFEFunctionDeclarator($1, $2); 
			// set parent relationship
			$1->SetParent($$);
			if ($2 != NULL) {
				$2->SetParentOfElements($$);
			}
		} else
			$$ = NULL;
	}
/* C specific function declarations start here */
	| direct_declarator LPAREN identifier_list RPAREN
	{ $$ = NULL; }
	| direct_declarator LPAREN c_parameter_type_list RPAREN
	{ $$ = NULL; }
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
	;

exception_declarator
	: EXCEPTION type_attribute_list type_spec declarator_list
	{ 
		$$ = new CFETypedDeclarator(TYPEDECL_EXCEPTION, $3, $4, $2); 
		// set parent relationship
		$3->SetParent($$);
		$4->SetParentOfElements($$);
		$2->SetParentOfElements($$);
	}
	| EXCEPTION type_spec declarator_list
	{ 
		$$ = new CFETypedDeclarator(TYPEDECL_EXCEPTION, $2, $3); 
		// set parent relationship
		$2->SetParent($$);
		$3->SetParentOfElements($$);
	}
	| error
	{
		dceerror("exception declarator expecting");
		YYABORT;
	}
	;

library
	: lib_attributes LIBRARY ID LBRACE import_list lib_definitions RBRACE SEMICOLON
	{ 
		$$ = new CFELibrary(new CFEIdentifier($3), $1, $6);
		$1->SetParentOfElements($$);
		$6->SetParentOfElements($$);
		//  XXXX add elements of import_list to library (if any)
	}
	;

lib_attributes :
	  LBRACKET lib_attribute_list rbracket
	{
		$$ = $2;
	}
	| LBRACKET RBRACKET
	{
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, new CFEVersionAttribute(0,0));
	}
	| /* no attributes at all */
	{
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, new CFEVersionAttribute(0,0));
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
	;

lib_definitions :
	  lib_definitions lib_definition
	{
		$1->Add($2);
		$$ = $1;
	}
	| lib_definition
	{
		$$ = new Vector(RUNTIME_CLASS(CFEFileComponent), 1, $1);
	}
	;

lib_definition :
	  interface { $$ = $1; } 
	| type_declarator SEMICOLON  { $$ = $1; }
	| const_declarator SEMICOLON  { $$ = $1; }
	| tagged_declarator SEMICOLON  { $$ = $1; }
	;

/***********************************************
 * C specific rules
 ***********************************************/

c_postfix_expression
	: primary_exp
	{ $$ = $1; }
	| c_postfix_expression LBRACKET c_expression RBRACKET
	{ 
		if ($3 != NULL)
			delete $3;
		$$ = NULL;
	}
	| c_postfix_expression LPAREN RPAREN
	{ $$ = NULL; }
	| c_postfix_expression LPAREN c_argument_expression_list RPAREN
	{ $$ = NULL; }
	| c_postfix_expression DOT ID
	{ $$ = NULL; }
	| c_postfix_expression PTR_OP ID
	{ $$ = NULL; }
	| c_postfix_expression INC_OP
	{ $$ = NULL; }
	| c_postfix_expression DEC_OP
	{ $$ = NULL; }
	;

c_argument_expression_list
	: c_assignment_expression
	{
		if ($1 != NULL) delete $1; 
	}
	| c_argument_expression_list COMMA c_assignment_expression
	{ 
		if ($3 != NULL) delete $3;
	}
	;

c_cast_expression
	: unary_exp
	{ $$ = $1; }
	| LPAREN c_type_name RPAREN c_cast_expression
	{ $$ = NULL; }
	;

c_assignment_expression
	: conditional_exp
	{ $$ = $1; }
	| unary_exp c_assignment_operator c_assignment_expression
	{ $$ = NULL; }
	;

c_assignment_operator
	: IS
	| MUL_ASSIGN
	| DIV_ASSIGN
	| MOD_ASSIGN
	| ADD_ASSIGN
	| SUB_ASSIGN
	| LEFT_ASSIGN
	| RIGHT_ASSIGN
	| AND_ASSIGN
	| XOR_ASSIGN
	| OR_ASSIGN
	;

c_expression
	: c_assignment_expression
	{ $$ = $1; }
	| c_expression COMMA c_assignment_expression
	{ $$ = NULL; }
	;

c_declaration
	: c_declaration_specifiers SEMICOLON
	{ $$ = $1; }
	| c_declaration_specifiers c_init_declarator_list SEMICOLON
	{
		// ignore initialization
		$$ = $1; 
	}
	;

c_declaration_specifiers
	: c_storage_class_specifier
	{ $$ = NULL; }
	| c_storage_class_specifier c_declaration_specifiers
	{ $$ = $2; }
	| type_spec
	{
		if ($1 != NULL)
			$$ = new Vector(RUNTIME_CLASS(CFETypeSpec), 1, $1);
		else
			$$ = new Vector(RUNTIME_CLASS(CFETypeSpec));
	}
	| type_spec c_declaration_specifiers
	{
		if ($1 != NULL) {
			if ($2 != NULL) {
				$2->Add($1);
				$$ = $2;
			} else
				$$ = new Vector(RUNTIME_CLASS(CFETypeSpec), 1, $1);
		} else
			$$ = $2;
	}
	| c_type_qualifier
	{ $$ = NULL; }
	| c_type_qualifier c_declaration_specifiers
	{ $$ = $2; }
/*	| type_declarator
	{ $$ = $1; }*/
	;

/* ignore all storage class specifier in IDL (except TYPEDEF...) */
c_storage_class_specifier
	: TYPEDEF
	| EXTERN
	| STATIC
	| AUTO
	| REGISTER
	;

/* ignore initialization declarators */
c_init_declarator_list
	: c_init_declarator
	| c_init_declarator_list COMMA c_init_declarator
	;

c_init_declarator
	: declarator
	{
		if ($1 != NULL) delete $1;
	}
	| declarator IS c_initializer
	{
		if ($1 != NULL) delete $1;
	}
	;

c_pointer
	: pointer
	{ $$ = $1; }
	| pointer c_type_qualifier_list
	{ $$ = $1; }
	| ASTERISK c_type_qualifier_list c_pointer
	{ $$ = $3 + 1; }
	;

/* type qualifier are of no interest for the IDL */
c_type_qualifier_list
	: c_type_qualifier
	| c_type_qualifier_list c_type_qualifier
	;

c_type_qualifier
	: CONST
	| VOLATILE
	;

/* ignore parameters of C functions in IDL */
c_parameter_type_list
	: c_parameter_list
	| c_parameter_list COMMA ELLIPSIS
	;

c_parameter_list
	: c_parameter_declaration
	| c_parameter_list COMMA c_parameter_declaration
	;

c_parameter_declaration
	: c_declaration_specifiers declarator
	{
		if ($1 != NULL) delete $1;
		if ($2 != NULL) delete $2;
	}
	| c_declaration_specifiers
	{
		if ($1 != NULL) delete $1;
	}
	;

/* type name can be ignored (only used for sizeof) */
c_type_name
	: c_specifier_qualifier_list

	| c_specifier_qualifier_list declarator
	{
		if ($2 != NULL) delete $2;
	}
	;

/* specifier list only used with type name -> can be ignored */
c_specifier_qualifier_list
	: type_spec c_specifier_qualifier_list
	{ 
		if ($1 != NULL)
			delete $1; 
	}
	| type_spec
	{ 
		if ($1 != NULL)
			delete $1; 
	}
	| c_type_qualifier c_specifier_qualifier_list
	| c_type_qualifier
	;

/* ignore initializers in IDL */
c_initializer
	: c_assignment_expression
	{ 
		if ($1 != NULL)
			delete $1; 
	}
	| LBRACE c_initializer_list RBRACE
	| LBRACE c_initializer_list COMMA RBRACE
	;

c_initializer_list
	: c_initializer
	| c_initializer_list COMMA c_initializer
	;

/* ignore all statements in IDL */
c_statement
	: c_labeled_statement
	| c_compound_statement
	| c_expression_statement
	{
		if ($1 != NULL)
			delete $1;
	}
	| c_selection_statement
	| c_iteration_statement
	| c_jump_statement
	;

/* ignore all statements in IDL */
c_labeled_statement
	: ID COLON c_statement
	{
		// ignore ID
	}
	| CASE const_exp COLON c_statement
	| DEFAULT COLON c_statement
	;

/* this is a block of statements (ignore in IDL) */
c_compound_statement
	: LBRACE RBRACE
	| LBRACE c_statement_list RBRACE
	| LBRACE c_declaration_list RBRACE
	| LBRACE c_declaration_list c_statement_list RBRACE
	;

c_declaration_list
	: c_declaration
	{
		if ($1 != NULL)
			$$ = $1;
		else
			$$ = new Vector(RUNTIME_CLASS(CFEFileComponent));
	}
	| c_declaration_list c_declaration
	{
		if ($2 != NULL)
			$1->Add($2);
		$$ = $1;
	}
	;

/* ignore all statements in IDL */
c_statement_list
	: c_statement
	| c_statement_list c_statement
	;

c_expression_statement
	: SEMICOLON
	{ $$ = NULL; }
	| c_expression SEMICOLON
	{ $$ = $1; }
	;

c_selection_statement
	: IF LPAREN c_expression RPAREN c_statement
	| IF LPAREN c_expression RPAREN c_statement ELSE c_statement
	| SWITCH LPAREN c_expression RPAREN c_statement
	;

c_iteration_statement
	: WHILE LPAREN c_expression RPAREN c_statement
	| DO c_statement WHILE LPAREN c_expression RPAREN SEMICOLON
	| FOR LPAREN c_expression_statement c_expression_statement RPAREN c_statement
	| FOR LPAREN c_expression_statement c_expression_statement c_expression RPAREN c_statement
	;

c_jump_statement
	: GOTO ID SEMICOLON
	| CONTINUE SEMICOLON
	| BREAK SEMICOLON
	| RETURN SEMICOLON
	| RETURN c_expression SEMICOLON
	;

/* start for C header declarations */
c_translation_unit
	: c_external_declaration
	{
		if ($1 != NULL)
			$$ = $1;
		else
			$$ = new Vector(RUNTIME_CLASS(CFEFileComponent));
	}
	| c_translation_unit c_external_declaration
	{
		if ($2 != NULL)
			$1->Add($2);
		$$ = $1;
	}
	;

c_external_declaration
	: c_function_definition
	{ $$ = NULL; }
	| c_declaration
	{ $$ = $1; }
	;

/* ignore all functions in IDL */
c_function_definition
	: c_declaration_specifiers declarator c_declaration_list c_compound_statement
	{
		if ($1 != NULL) delete $1;
		if ($2 != NULL) delete $2;
		if ($3 != NULL) delete $3;
	}
	| c_declaration_specifiers declarator c_compound_statement
	{
		if ($1 != NULL) delete $1;
		if ($2 != NULL) delete $2;
	}
	| declarator c_declaration_list c_compound_statement
	{
		if ($1 != NULL) delete $1;
		if ($2 != NULL) delete $2;
	}
	| declarator c_compound_statement
	{
		if ($1 != NULL) delete $1;
	}
	;

/***********************************************
 * end C specific rules
 ***********************************************/

/***********************************************
 * C++ specific rules
 ***********************************************/

c_new_expression
	: LPAREN c_expression_list RPAREN c_new_type_id c_new_initializer
	| c_new_type_id c_new_initializer
	| LPAREN c_expression_list RPAREN c_new_type_id 
	| c_new_type_id 
	| LPAREN c_expression_list RPAREN LPAREN c_type_name RPAREN c_new_initializer
	| LPAREN c_type_name RPAREN c_new_initializer
	| LPAREN c_expression_list RPAREN LPAREN c_type_name RPAREN 
	| LPAREN c_type_name RPAREN 
	;

c_new_type_id

	: declaration_specifiers new_declarator
	| declaration_specifiers
	;


/***********************************************
 * end C++ specific rules
 ***********************************************/

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

%%


void
dceerror(char* s)
{
	fprintf(stderr, "%s:%d: %s.\n", sInFileName, nLineNbDCE, s);
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