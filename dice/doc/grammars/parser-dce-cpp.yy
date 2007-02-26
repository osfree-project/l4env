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
 * C/C++ specific tokens
 ******************************************************************************/
/* This group is used by the C/C++ language parser */
%token AUTO            BREAK           ELSE            REGISTER
%token EXTERN          RETURN          CONTINUE        FOR             
%token GOTO            SIZEOF          VOLATILE	       DO              
%token IF              STATIC          WHILE

/* The following are used in C++ only.  ANSI C would call these IDENTIFIERs */
%token NEW             DELETE
%token THIS
%token OPERATOR
%token CLASS
%token PUBLIC          PROTECTED       PRIVATE
%token VIRTUAL         FRIEND
%token INLINE          OVERLOAD

/* Multi-Character operators */
%token  PTR_OP				/*    ->                              */
%token  INC_OP	DEC_OP		/*    ++      --                      */
%token  ELLIPSIS			/*    ...                             */
                 /* Following are used in C++, not ANSI C        */
%token  CLCL             /*    ::                              */
%token  DOTstar ARROWstar/*    .*       ->*                    */

/* modifying assignment operators */
%token MUL_ASSIGN	DIV_ASSIGN	MOD_ASSIGN		/*   *=      /=      %=      */
%token ADD_ASSIGN	SUB_ASSIGN					/*   +=      -=              */
%token LS_ASSIGN	RS_ASSIGN					/*   <<=     >>=             */
%token AND_ASSIGN	XOR_ASSIGN	OR_ASSIGN		/*   &=      ^=      |=      */

/*****************************************************************************************
 * end C/C++ specific tokens
 *****************************************************************************************/

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
	| cpp_external_definition { }
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
 	| cpp_external_definition { $$ = NULL; }
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
		$3->SetParentOfElements($$);
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
		$4->SetParentOfElements($$);
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
		$3->SetParentOfElements($$);
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
	| cpp_external_definition { $$ = NULL; }
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

/*******************************************************************************************
 * C/C++ specific rules
 *******************************************************************************************/

/*********************** CONSTANTS *********************************/
cpp_constant:
        LIT_INT {}
        | LIT_FLOAT {}
        | LIT_CHAR {}
        ;

cpp_string_literal_list:
          LIT_STR {}
        | cpp_string_literal_list LIT_STR {}
        ;

/************************* EXPRESSIONS ********************************/

cpp_paren_identifier_declarator:
        cpp_scope_opt_identifier
        | cpp_scope_opt_complex_name
        | LPAREN cpp_paren_identifier_declarator RPAREN
        ;

cpp_primary_expression:
        cpp_global_opt_scope_opt_identifier
        | cpp_global_opt_scope_opt_complex_name
        | THIS   /* C++, cpp_not ANSI C */
        | cpp_constant
        | cpp_string_literal_list
        | LPAREN cpp_comma_expression RPAREN
        ;

cpp_non_elaborating_type_specifier:
        cpp_sue_type_specifier
        | cpp_basic_type_specifier
        | cpp_typedef_type_specifier
        | cpp_basic_type_name
        | ID {}
        | cpp_global_or_scoped_typedefname
        ;

cpp_operator_function_name:
        OPERATOR cpp_any_operator
        | OPERATOR cpp_type_qualifier_list            cpp_operator_function_ptr_opt
        | OPERATOR cpp_non_elaborating_type_specifier cpp_operator_function_ptr_opt
        ;

cpp_operator_function_ptr_opt:
        /* nothing */
        | cpp_unary_modifier        cpp_operator_function_ptr_opt
        | cpp_asterisk_or_ampersand cpp_operator_function_ptr_opt
        ;

    /* List of operators we can overload */
cpp_any_operator:
        PLUS
        | MINUS
        | ASTERISK
        | DIV
        | MOD
        | BITXOR
        | BITAND
        | BITOR
        | TILDE
        | EXCLAM
        | LT
        | GT
        | LSHIFT
        | RSHIFT
        | LOGICALAND
        | LOGICALOR
        | PTR_OP
        | ARROWstar
        | DOT
        | DOTstar
        | INC_OP
        | DEC_OP
        | LTEQUAL
        | GTEQUAL
        | EQUAL
        | NOTEQUAL
        | cpp_assignment_operator
        | LPAREN RPAREN
        | LBRACKET RBRACKET
        | NEW
        | DELETE
        | COMMA
        ;

cpp_type_qualifier_list_opt:
        /* Nothing */
        | cpp_type_qualifier_list
        ;

cpp_postfix_expression:
        cpp_primary_expression
        | cpp_postfix_expression LBRACKET cpp_comma_expression RBRACKET
        | cpp_postfix_expression LPAREN RPAREN
        | cpp_postfix_expression LPAREN cpp_argument_expression_list RPAREN
        | cpp_postfix_expression DOT cpp_member_name
        | cpp_postfix_expression PTR_OP cpp_member_name
        | cpp_postfix_expression INC_OP
        | cpp_postfix_expression DEC_OP
/* The next 4 rules are the source of cast ambiguity */
        | ID                  LPAREN RPAREN {}
        | cpp_global_or_scoped_typedefname LPAREN RPAREN
        | ID                  LPAREN cpp_argument_expression_list RPAREN {}
        | cpp_global_or_scoped_typedefname LPAREN cpp_argument_expression_list RPAREN
        | cpp_basic_type_name LPAREN cpp_assignment_expression RPAREN
            /* If the following rule is added to the  grammar,  there 
            will  be 3 additional reduce-reduce conflicts.  They will 
            all be resolved in favor of NOT using the following rule, 
            so no harm will be done.   However,  since  the  rule  is 
            semantically  illegal  we  will  omit  it  until  we  are 
            enhancing the grammar for error recovery */
/*      | cpp_basic_type_name LPAREN RPAREN  /* Illegal: no such constructor*/
        ;

cpp_member_name:
        cpp_scope_opt_identifier
        | cpp_scope_opt_complex_name
        | cpp_basic_type_name CLCL TILDE cpp_basic_type_name  /* C++, not ANSI C */
        | cpp_declaration_qualifier_list  CLCL TILDE   cpp_declaration_qualifier_list
        | cpp_type_qualifier_list         CLCL TILDE   cpp_type_qualifier_list
        ;

cpp_argument_expression_list:
        cpp_assignment_expression
        | cpp_argument_expression_list COMMA cpp_assignment_expression
        ;

cpp_unary_expression:
        cpp_postfix_expression
        | INC_OP  cpp_unary_expression
        | DEC_OP cpp_unary_expression
        | cpp_asterisk_or_ampersand cpp_cast_expression
        | MINUS cpp_cast_expression
        | PLUS cpp_cast_expression
        | TILDE cpp_cast_expression
        | EXCLAM cpp_cast_expression
        | SIZEOF cpp_unary_expression
        | SIZEOF '(' cpp_type_name ')'
        | cpp_allocation_expression
        ;

       /* Following are C++, not ANSI C */
cpp_allocation_expression:
        cpp_global_opt_scope_opt_operator_new LPAREN cpp_type_name RPAREN cpp_operator_new_initializer_opt
        | cpp_global_opt_scope_opt_operator_new LPAREN cpp_argument_expression_list RPAREN RPAREN cpp_type_name RPAREN cpp_operator_new_initializer_opt
                /* next two rules are the source of * and & ambiguities */
        | cpp_global_opt_scope_opt_operator_new cpp_operator_new_type
        | cpp_global_opt_scope_opt_operator_new LPAREN cpp_argument_expression_list RPAREN cpp_operator_new_type
        ;

       /* Following are C++, not ANSI C */
cpp_global_opt_scope_opt_operator_new:
        NEW
        | cpp_global_or_scope NEW
        ;

cpp_operator_new_type:
        cpp_type_qualifier_list cpp_operator_new_declarator_opt cpp_operator_new_initializer_opt
        | cpp_non_elaborating_type_specifier cpp_operator_new_declarator_opt cpp_operator_new_initializer_opt
        ;

cpp_operator_new_declarator_opt:
        /* Nothing */
        | cpp_operator_new_array_declarator
        | cpp_asterisk_or_ampersand cpp_operator_new_declarator_opt
        | cpp_unary_modifier        cpp_operator_new_declarator_opt
        ;

cpp_operator_new_array_declarator:
        LBRACKET RBRACKET
        | LBRACKET cpp_comma_expression RBRACKET
        | cpp_operator_new_array_declarator LBRACKET cpp_comma_expression RBRACKET
        ;

cpp_operator_new_initializer_opt:
        /* Nothing */
        | LPAREN RPAREN
        | LPAREN cpp_argument_expression_list RPAREN
        ;

cpp_cast_expression:
        cpp_unary_expression
        | LPAREN cpp_type_name RPAREN cpp_cast_expression
		| LPAREN cpp_type_name RPAREN LBRACE gcc_initlist_maybe_comma RBRACE
        ;

/**********************************************************************
 * GCC specifics
 **********************************************************************/

gcc_initlist_maybe_comma:
	  /* empty */
	| gcc_initlist gcc_maybecomma
	;

gcc_maybecomma:
	/* empty */
	| COMMA
	;

gcc_initlist:
	  gcc_initelt
	| gcc_initlist COMMA gcc_initelt
	;

gcc_initelt:
	  gcc_designator_list IS gcc_initval
	| gcc_designator gcc_initval
	| ID COLON gcc_initval {}
	| gcc_initval
	;

gcc_initval:
	  LBRACE gcc_initlist_maybe_comma RBRACE
	| cpp_assignment_expression
	;

gcc_designator_list:
	  gcc_designator
	| gcc_designator_list gcc_designator
	;

gcc_designator:
	  DOT ID {}
	/* These are for labeled elements.  The syntax for an array element
	   initializer conflicts with the syntax for an Objective-C message,
	   so don't include these productions in the Objective-C grammar.  */
	| LBRACKET cpp_assignment_expression ELLIPSIS cpp_assignment_expression RBRACKET
	| LBRACKET cpp_assignment_expression RBRACKET
	;

/************************************************************************
 * end GCC specifics
 ************************************************************************/

    /* Following cpp_are C++, not ANSI C */
cpp_deallocation_expression:
        cpp_cast_expression
        | cpp_global_opt_scope_opt_delete cpp_deallocation_expression
        | cpp_global_opt_scope_opt_delete LBRACKET cpp_comma_expression RBRACKET cpp_deallocation_expression  /* cpp_archaic C++, what a concept */
        | cpp_global_opt_scope_opt_delete LBRACKET RBRACKET cpp_deallocation_expression
        ;


    /* Following cpp_are C++, not ANSI C */
cpp_global_opt_scope_opt_delete:
        DELETE
        | cpp_global_or_scope DELETE
        ;


    /* Following are C++, not ANSI C */
cpp_point_member_expression:
        cpp_deallocation_expression
        | cpp_point_member_expression DOTstar  cpp_deallocation_expression
        | cpp_point_member_expression ARROWstar  cpp_deallocation_expression
        ;

cpp_multiplicative_expression:
        cpp_point_member_expression
        | cpp_multiplicative_expression ASTERISK cpp_point_member_expression
        | cpp_multiplicative_expression DIV cpp_point_member_expression
        | cpp_multiplicative_expression MOD cpp_point_member_expression
        ;

cpp_additive_expression:
        cpp_multiplicative_expression
        | cpp_additive_expression PLUS cpp_multiplicative_expression
        | cpp_additive_expression MINUS cpp_multiplicative_expression
        ;

cpp_shift_expression:
        cpp_additive_expression
        | cpp_shift_expression LSHIFT cpp_additive_expression
        | cpp_shift_expression RSHIFT cpp_additive_expression
        ;

cpp_relational_expression:
        cpp_shift_expression
        | cpp_relational_expression LT cpp_shift_expression
        | cpp_relational_expression GT cpp_shift_expression
        | cpp_relational_expression LTEQUAL  cpp_shift_expression
        | cpp_relational_expression GTEQUAL  cpp_shift_expression
        ;

cpp_equality_expression:
        cpp_relational_expression
        | cpp_equality_expression EQUAL cpp_relational_expression
        | cpp_equality_expression NOTEQUAL cpp_relational_expression
        ;

cpp_AND_expression:
        cpp_equality_expression
        | cpp_AND_expression BITAND cpp_equality_expression
        ;

cpp_exclusive_OR_expression:
        cpp_AND_expression
        | cpp_exclusive_OR_expression BITXOR cpp_AND_expression
        ;

cpp_inclusive_OR_expression:
        cpp_exclusive_OR_expression
        | cpp_inclusive_OR_expression BITOR cpp_exclusive_OR_expression
        ;

cpp_logical_AND_expression:
        cpp_inclusive_OR_expression
        | cpp_logical_AND_expression LOGICALAND cpp_inclusive_OR_expression
        ;

cpp_logical_OR_expression:
        cpp_logical_AND_expression
        | cpp_logical_OR_expression LOGICALOR cpp_logical_AND_expression
        ;

cpp_conditional_expression:
        cpp_logical_OR_expression
        | cpp_logical_OR_expression QUESTION cpp_comma_expression COLON cpp_conditional_expression
        ;

cpp_assignment_expression:
        cpp_conditional_expression
        | cpp_unary_expression cpp_assignment_operator cpp_assignment_expression
        ;

cpp_assignment_operator:
        IS
        | MUL_ASSIGN
        | DIV_ASSIGN
        | MOD_ASSIGN
        | ADD_ASSIGN
        | SUB_ASSIGN
        | LS_ASSIGN
        | RS_ASSIGN
        | AND_ASSIGN
        | XOR_ASSIGN
        | OR_ASSIGN
        ;

cpp_comma_expression:
        cpp_assignment_expression
        | cpp_comma_expression COMMA cpp_assignment_expression
        ;

cpp_constant_expression:
        cpp_conditional_expression
        ;

/* The following was used for clarity */
cpp_comma_expression_opt:
        /* Nothing */
        | cpp_comma_expression
        ;

/******************************* DECLARATIONS *********************************/

cpp_declaration:
        cpp_declaring_list SEMICOLON
        | cpp_default_declaring_list SEMICOLON
        | cpp_sue_declaration_specifier SEMICOLON 
        | cpp_sue_type_specifier SEMICOLON
        | cpp_sue_type_specifier_elaboration SEMICOLON
        ;

cpp_default_declaring_list:  /* Can't  redeclare typedef names */
        cpp_declaration_qualifier_list   cpp_identifier_declarator {} cpp_initializer_opt
        | cpp_type_qualifier_list        cpp_identifier_declarator {} cpp_initializer_opt
        | cpp_default_declaring_list COMMA cpp_identifier_declarator {} cpp_initializer_opt

        | cpp_declaration_qualifier_list cpp_constructed_identifier_declarator
        | cpp_type_qualifier_list        cpp_constructed_identifier_declarator
        | cpp_default_declaring_list COMMA cpp_constructed_identifier_declarator
        ;

cpp_declaring_list:
        cpp_declaration_specifier          cpp_declarator {} cpp_initializer_opt
        | cpp_type_specifier               cpp_declarator {} cpp_initializer_opt
        | cpp_basic_type_name              cpp_declarator {} cpp_initializer_opt
        | ID                  cpp_declarator cpp_initializer_opt {}
        | cpp_global_or_scoped_typedefname cpp_declarator {} cpp_initializer_opt
        | cpp_declaring_list COMMA           cpp_declarator {} cpp_initializer_opt

        | cpp_declaration_specifier        cpp_constructed_declarator
        | cpp_type_specifier               cpp_constructed_declarator
        | cpp_basic_type_name              cpp_constructed_declarator
        | ID                  cpp_constructed_declarator {}
        | cpp_global_or_scoped_typedefname cpp_constructed_declarator
        | cpp_declaring_list COMMA           cpp_constructed_declarator
        ;

cpp_constructed_declarator:
        cpp_nonunary_constructed_identifier_declarator
        | cpp_constructed_paren_typedef_declarator
        | cpp_simple_paren_typedef_declarator LPAREN cpp_argument_expression_list RPAREN
        | cpp_simple_paren_typedef_declarator cpp_postfixing_abstract_declarator LPAREN cpp_argument_expression_list RPAREN  /* cpp_constraint error */
        | cpp_constructed_parameter_typedef_declarator
        | cpp_asterisk_or_ampersand cpp_constructed_declarator
        | cpp_unary_modifier        cpp_constructed_declarator
        ;

cpp_constructed_paren_typedef_declarator:
        LPAREN cpp_paren_typedef_declarator RPAREN
                    LPAREN cpp_argument_expression_list RPAREN
        | LPAREN cpp_paren_typedef_declarator RPAREN cpp_postfixing_abstract_declarator
                   LPAREN cpp_argument_expression_list RPAREN
        | LPAREN cpp_simple_paren_typedef_declarator cpp_postfixing_abstract_declarator RPAREN
                   LPAREN cpp_argument_expression_list RPAREN
        | LPAREN ID cpp_postfixing_abstract_declarator RPAREN LPAREN cpp_argument_expression_list RPAREN {}
        ;


cpp_constructed_parameter_typedef_declarator:
        ID LPAREN cpp_argument_expression_list RPAREN {}
        | ID  cpp_postfixing_abstract_declarator LPAREN cpp_argument_expression_list RPAREN {} /* cpp_constraint error */
        | LPAREN cpp_clean_typedef_declarator RPAREN LPAREN cpp_argument_expression_list RPAREN
        | LPAREN cpp_clean_typedef_declarator RPAREN cpp_postfixing_abstract_declarator LPAREN cpp_argument_expression_list RPAREN
        ;

cpp_constructed_identifier_declarator:
        cpp_nonunary_constructed_identifier_declarator
        | cpp_asterisk_or_ampersand cpp_constructed_identifier_declarator
        | cpp_unary_modifier        cpp_constructed_identifier_declarator
        ;

cpp_nonunary_constructed_identifier_declarator:
        cpp_paren_identifier_declarator   LPAREN cpp_argument_expression_list RPAREN
        | cpp_paren_identifier_declarator cpp_postfixing_abstract_declarator
                       LPAREN cpp_argument_expression_list RPAREN  /* constraint error*/
        | LPAREN cpp_unary_identifier_declarator RPAREN
                       LPAREN cpp_argument_expression_list RPAREN
        | LPAREN cpp_unary_identifier_declarator RPAREN cpp_postfixing_abstract_declarator
                       LPAREN cpp_argument_expression_list RPAREN
        ;

cpp_declaration_specifier:
        cpp_basic_declaration_specifier          /* Arithmetic or void */
        | cpp_sue_declaration_specifier          /* struct/union/enum/class */
        | cpp_typedef_declaration_specifier      /* typedef*/
        ;

cpp_type_specifier:
        cpp_basic_type_specifier                 /* Arithmetic or void */
        | cpp_sue_type_specifier                 /* Struct/Union/Enum/Class */
        | cpp_sue_type_specifier_elaboration     /* elaborated Struct/Union/Enum/Class */
        | cpp_typedef_type_specifier             /* Typedef */
        ;

cpp_declaration_qualifier_list:  /* storage class and optional const/volatile */
        cpp_storage_class
        | cpp_type_qualifier_list cpp_storage_class
        | cpp_declaration_qualifier_list cpp_declaration_qualifier
        ;

cpp_type_qualifier_list:
        cpp_type_qualifier
        | cpp_type_qualifier_list cpp_type_qualifier
        ;

cpp_declaration_qualifier:
        cpp_storage_class
        | cpp_type_qualifier                  /* const or volatile */
        ;

cpp_type_qualifier:
        CONST
        | VOLATILE
        ;

cpp_basic_declaration_specifier:      /*Storage Class+Arithmetic or void*/
        cpp_declaration_qualifier_list    cpp_basic_type_name
        | cpp_basic_type_specifier        cpp_storage_class
        | cpp_basic_type_name             cpp_storage_class
        | cpp_basic_declaration_specifier cpp_declaration_qualifier
        | cpp_basic_declaration_specifier cpp_basic_type_name
        ;

cpp_basic_type_specifier:
        cpp_type_qualifier_list    cpp_basic_type_name /* Arithmetic or void */
        | cpp_basic_type_name      cpp_basic_type_name
        | cpp_basic_type_name      cpp_type_qualifier
        | cpp_basic_type_specifier cpp_type_qualifier
        | cpp_basic_type_specifier cpp_basic_type_name
        ;

cpp_sue_declaration_specifier:          /* Storage Class + struct/union/enum/class */
        cpp_declaration_qualifier_list       cpp_elaborated_type_name
        | cpp_declaration_qualifier_list     cpp_elaborated_type_name_elaboration
        | cpp_sue_type_specifier             cpp_storage_class
        | cpp_sue_type_specifier_elaboration cpp_storage_class
        | cpp_sue_declaration_specifier      cpp_declaration_qualifier
        ;

cpp_sue_type_specifier_elaboration:
        cpp_elaborated_type_name_elaboration     /* elaborated struct/union/enum/class */
        | cpp_type_qualifier_list cpp_elaborated_type_name_elaboration
        | cpp_sue_type_specifier_elaboration cpp_type_qualifier
        ;

cpp_sue_type_specifier:
        cpp_elaborated_type_name              /* struct/union/enum/class */
        | cpp_type_qualifier_list cpp_elaborated_type_name
        | cpp_sue_type_specifier cpp_type_qualifier
        ;

cpp_typedef_declaration_specifier:       /*Storage Class + typedef types */
        cpp_declaration_qualifier_list ID {}
        | cpp_declaration_qualifier_list cpp_global_or_scoped_typedefname
        | cpp_typedef_type_specifier cpp_storage_class
        | ID cpp_storage_class {}
        | cpp_global_or_scoped_typedefname cpp_storage_class
        | cpp_typedef_declaration_specifier cpp_declaration_qualifier
        ;

cpp_typedef_type_specifier:              /* typedef types */
        cpp_type_qualifier_list ID {}
        | cpp_type_qualifier_list cpp_global_or_scoped_typedefname
        | ID cpp_type_qualifier {}
        | cpp_global_or_scoped_typedefname cpp_type_qualifier
        | cpp_typedef_type_specifier cpp_type_qualifier
        ;

cpp_storage_class:
        EXTERN
        | TYPEDEF
        | STATIC
        | AUTO
        | REGISTER
        | FRIEND   /* C++, cpp_not ANSI C */
        | OVERLOAD /* C++, not ANSI C */
        | INLINE   /* C++, not ANSI C */
        | VIRTUAL  /* C++, not ANSI C */
        ;

cpp_basic_type_name:
        INT
        | CHAR
        | SHORT
        | LONG
        | FLOAT
        | DOUBLE
        | SIGNED
        | UNSIGNED
        | VOID
        ;

cpp_elaborated_type_name_elaboration:
        cpp_aggregate_name_elaboration
        | cpp_enum_name_elaboration
        ;

cpp_elaborated_type_name:
        cpp_aggregate_name
        | cpp_enum_name
        ;

cpp_aggregate_name_elaboration:
        cpp_aggregate_name cpp_derivation_opt  LBRACE cpp_member_declaration_list_opt RBRACE
        | cpp_aggregate_key cpp_derivation_opt LBRACE cpp_member_declaration_list_opt RBRACE
        ;

cpp_aggregate_name:
         cpp_aggregate_key cpp_tag_name
        | cpp_global_scope cpp_scope cpp_aggregate_key cpp_tag_name
        | cpp_global_scope       cpp_aggregate_key cpp_tag_name
        | cpp_scope              cpp_aggregate_key cpp_tag_name
        ;

cpp_derivation_opt:
        /* nothing */
        | COLON cpp_derivation_list
        ;

cpp_derivation_list:
        cpp_parent_class
        | cpp_derivation_list COMMA cpp_parent_class
        ;

cpp_parent_class:
          cpp_global_opt_scope_opt_typedefname
        | VIRTUAL cpp_access_specifier_opt cpp_global_opt_scope_opt_typedefname
        | cpp_access_specifier cpp_virtual_opt cpp_global_opt_scope_opt_typedefname
        ;

cpp_virtual_opt:
        /* nothing */
        | VIRTUAL
        ;

cpp_access_specifier_opt:
        /* cpp_nothing */
        | cpp_access_specifier
        ;

cpp_access_specifier:
        PUBLIC
        | PRIVATE
        | PROTECTED
        ;

cpp_aggregate_key:
        STRUCT
        | UNION
        | CLASS /* C++, not ANSI C */
        ;

cpp_member_declaration_list_opt:
        /* nothing */
        | cpp_member_declaration_list_opt cpp_member_declaration
        ;

cpp_member_declaration:
        cpp_member_declaring_list SEMICOLON
        | cpp_member_default_declaring_list SEMICOLON
        | cpp_access_specifier COLON               /* C++, not ANSI C */
        | cpp_new_function_definition            /* C++, not ANSI C */
        | cpp_constructor_function_in_class      /* C++, not ANSI C */
        | cpp_sue_type_specifier             SEMICOLON /* C++, not ANSI C */
        | cpp_sue_type_specifier_elaboration SEMICOLON /* C++, not ANSI C */
        | cpp_identifier_declarator          SEMICOLON /* C++, not ANSI C
                                                access modification
                                                conversion functions,
                                                unscoped destructors */
        | cpp_typedef_declaration_specifier SEMICOLON /* friend T */       /* C++, not ANSI C */
        | cpp_sue_declaration_specifier SEMICOLON     /* friend class C*/  /* C++, not ANSI C */
        ;

cpp_member_default_declaring_list:        /* doesn't redeclare typedef*/
        cpp_type_qualifier_list cpp_identifier_declarator cpp_member_pure_opt
        | cpp_declaration_qualifier_list cpp_identifier_declarator cpp_member_pure_opt /* C++, not ANSI C */
        | cpp_member_default_declaring_list COMMA cpp_identifier_declarator cpp_member_pure_opt
        | cpp_type_qualifier_list cpp_bit_field_identifier_declarator
        | cpp_declaration_qualifier_list cpp_bit_field_identifier_declarator /* C++, not ANSI C */
        | cpp_member_default_declaring_list COMMA  cpp_bit_field_identifier_declarator
        ;

cpp_member_declaring_list:        /* Can possibly redeclare typedefs */
        cpp_type_specifier cpp_declarator cpp_member_pure_opt
        | cpp_basic_type_name cpp_declarator cpp_member_pure_opt
        | cpp_global_or_scoped_typedefname cpp_declarator cpp_member_pure_opt
        | cpp_member_conflict_declaring_item
        | cpp_member_declaring_list COMMA cpp_declarator cpp_member_pure_opt
        | cpp_type_specifier cpp_bit_field_declarator
        | cpp_basic_type_name cpp_bit_field_declarator
        | ID cpp_bit_field_declarator {}
        | cpp_global_or_scoped_typedefname cpp_bit_field_declarator
        | cpp_declaration_specifier cpp_bit_field_declarator /* constraint violation: storage class used */
        | cpp_member_declaring_list COMMA cpp_bit_field_declarator
        ;

cpp_member_conflict_declaring_item:
        ID cpp_identifier_declarator cpp_member_pure_opt {}
        | ID cpp_parameter_typedef_declarator cpp_member_pure_opt {}
        | ID cpp_simple_paren_typedef_declarator cpp_member_pure_opt {}
        | cpp_declaration_specifier cpp_identifier_declarator cpp_member_pure_opt
        | cpp_declaration_specifier cpp_parameter_typedef_declarator cpp_member_pure_opt
        | cpp_declaration_specifier cpp_simple_paren_typedef_declarator cpp_member_pure_opt
        | cpp_member_conflict_paren_declaring_item
        ;

cpp_member_conflict_paren_declaring_item:
        ID cpp_asterisk_or_ampersand LPAREN cpp_simple_paren_typedef_declarator RPAREN cpp_member_pure_opt {}
        | ID cpp_unary_modifier LPAREN cpp_simple_paren_typedef_declarator RPAREN cpp_member_pure_opt {}
        | ID cpp_asterisk_or_ampersand LPAREN ID RPAREN cpp_member_pure_opt {}
        | ID cpp_unary_modifier LPAREN ID RPAREN cpp_member_pure_opt {}
        | ID cpp_asterisk_or_ampersand cpp_paren_typedef_declarator cpp_member_pure_opt {}
        | ID cpp_unary_modifier cpp_paren_typedef_declarator cpp_member_pure_opt {}
        | cpp_declaration_specifier cpp_asterisk_or_ampersand LPAREN cpp_simple_paren_typedef_declarator RPAREN cpp_member_pure_opt
        | cpp_declaration_specifier cpp_unary_modifier LPAREN cpp_simple_paren_typedef_declarator RPAREN cpp_member_pure_opt
        | cpp_declaration_specifier cpp_asterisk_or_ampersand LPAREN ID RPAREN cpp_member_pure_opt {}
        | cpp_declaration_specifier cpp_unary_modifier LPAREN ID RPAREN cpp_member_pure_opt {}
        | cpp_declaration_specifier cpp_asterisk_or_ampersand cpp_paren_typedef_declarator cpp_member_pure_opt
        | cpp_declaration_specifier cpp_unary_modifier cpp_paren_typedef_declarator cpp_member_pure_opt
        | cpp_member_conflict_paren_postfix_declaring_item
        ;

cpp_member_conflict_paren_postfix_declaring_item:
        ID LPAREN cpp_paren_typedef_declarator RPAREN cpp_member_pure_opt {}
        | ID LPAREN cpp_simple_paren_typedef_declarator cpp_postfixing_abstract_declarator RPAREN cpp_member_pure_opt {}
        | ID LPAREN ID cpp_postfixing_abstract_declarator RPAREN cpp_member_pure_opt {}
        | ID LPAREN cpp_paren_typedef_declarator RPAREN cpp_postfixing_abstract_declarator cpp_member_pure_opt {}
        | cpp_declaration_specifier LPAREN cpp_paren_typedef_declarator RPAREN cpp_member_pure_opt
        | cpp_declaration_specifier LPAREN cpp_simple_paren_typedef_declarator cpp_postfixing_abstract_declarator RPAREN cpp_member_pure_opt
        | cpp_declaration_specifier LPAREN ID {} cpp_postfixing_abstract_declarator RPAREN cpp_member_pure_opt
        | cpp_declaration_specifier LPAREN cpp_paren_typedef_declarator RPAREN cpp_postfixing_abstract_declarator cpp_member_pure_opt
        ;

cpp_member_pure_opt:
        /* nothing */
        | IS LIT_CHAR /* C++, not ANSI C */ /* Pure function*/
        ;

cpp_bit_field_declarator:
        cpp_bit_field_identifier_declarator
        | ID COLON cpp_constant_expression {}
        ;

cpp_bit_field_identifier_declarator:
          COLON cpp_constant_expression
        | cpp_identifier_declarator COLON cpp_constant_expression
        ;

cpp_enum_name_elaboration:
        cpp_global_opt_scope_opt_enum_key LBRACE cpp_enumerator_list RBRACE
        | cpp_enum_name                   LBRACE cpp_enumerator_list RBRACE
        ;

cpp_enum_name:
        cpp_global_opt_scope_opt_enum_key cpp_tag_name
        ;

cpp_global_opt_scope_opt_enum_key:
        ENUM
        | cpp_global_or_scope ENUM
        ;

cpp_enumerator_list:
        cpp_enumerator_list_no_trailing_comma
        | cpp_enumerator_list_no_trailing_comma COMMA /* C++, cpp_not ANSI C */
        ;

cpp_enumerator_list_no_trailing_comma:
        cpp_enumerator_name cpp_enumerator_value_opt
        | cpp_enumerator_list_no_trailing_comma COMMA cpp_enumerator_name cpp_enumerator_value_opt
        ;

cpp_enumerator_name:
        ID {}
        ;

cpp_enumerator_value_opt:
        /* Nothing */
        | IS cpp_constant_expression
        ;

cpp_parameter_type_list:
        LPAREN RPAREN                             cpp_type_qualifier_list_opt
        | LPAREN cpp_type_name RPAREN                 cpp_type_qualifier_list_opt
        | LPAREN cpp_type_name cpp_initializer RPAREN     cpp_type_qualifier_list_opt /* C++, cpp_not ANSI C */
        | LPAREN cpp_named_parameter_type_list RPAREN cpp_type_qualifier_list_opt
        ;

cpp_old_parameter_type_list:
        LPAREN RPAREN
        | LPAREN cpp_type_name RPAREN
        | LPAREN cpp_type_name cpp_initializer RPAREN  /* C++, cpp_not ANSI C */
        | LPAREN cpp_named_parameter_type_list RPAREN
        ;

cpp_named_parameter_type_list:  /* WARNING: cpp_excludes cpp_lone cpp_type_name*/
        cpp_parameter_list
        | cpp_parameter_list cpp_comma_opt_ellipsis
        | cpp_type_name cpp_comma_opt_ellipsis
        | cpp_type_name cpp_initializer cpp_comma_opt_ellipsis  /* C++, not ANSI C */
        | ELLIPSIS /* C++, cpp_not ANSI C */
        ;

cpp_comma_opt_ellipsis:
        ELLIPSIS       /* C++, cpp_not ANSI C */
        | COMMA ELLIPSIS
        ;

cpp_parameter_list:
        cpp_non_casting_parameter_declaration
        | cpp_non_casting_parameter_declaration cpp_initializer /* C++, cpp_not ANSI C */
        | cpp_type_name             COMMA cpp_parameter_declaration
        | cpp_type_name cpp_initializer COMMA cpp_parameter_declaration  /* C++, cpp_not ANSI C */
        | cpp_parameter_list        COMMA cpp_parameter_declaration
        ;

cpp_parameter_declaration:
        cpp_type_name
        | cpp_type_name                         cpp_initializer  /* C++, cpp_not ANSI C */
        | cpp_non_casting_parameter_declaration
        | cpp_non_casting_parameter_declaration cpp_initializer /* C++, not ANSI C */
        ;

cpp_non_casting_parameter_declaration: /*have names or storage classes */
        cpp_declaration_specifier
        | cpp_declaration_specifier cpp_abstract_declarator
        | cpp_declaration_specifier cpp_identifier_declarator
        | cpp_declaration_specifier cpp_parameter_typedef_declarator
        | cpp_declaration_qualifier_list
        | cpp_declaration_qualifier_list cpp_abstract_declarator
        | cpp_declaration_qualifier_list cpp_identifier_declarator
        | cpp_type_specifier cpp_identifier_declarator
        | cpp_type_specifier cpp_parameter_typedef_declarator
        | cpp_basic_type_name cpp_identifier_declarator
        | cpp_basic_type_name cpp_parameter_typedef_declarator
        | ID                   cpp_identifier_declarator {}
        | ID                   cpp_parameter_typedef_declarator {}
        | cpp_global_or_scoped_typedefname  cpp_identifier_declarator
        | cpp_global_or_scoped_typedefname  cpp_parameter_typedef_declarator
        | cpp_type_qualifier_list cpp_identifier_declarator
        ;

cpp_type_name:
        cpp_type_specifier
        | cpp_basic_type_name
        | ID {}
        | cpp_global_or_scoped_typedefname
        | cpp_type_qualifier_list
        | cpp_type_specifier               cpp_abstract_declarator
        | cpp_basic_type_name              cpp_abstract_declarator
        | ID				               cpp_abstract_declarator {}
        | cpp_global_or_scoped_typedefname cpp_abstract_declarator
        | cpp_type_qualifier_list          cpp_abstract_declarator
        ;

cpp_initializer_opt:
        /* nothing */
        | cpp_initializer
        ;

cpp_initializer:
        IS cpp_initializer_group
        ;

cpp_initializer_group:
        LBRACE cpp_initializer_list RBRACE
        | LBRACE cpp_initializer_list COMMA RBRACE
        | cpp_assignment_expression
        ;

cpp_initializer_list:
        cpp_initializer_group
        | cpp_initializer_list COMMA cpp_initializer_group
        ;

/*************************** STATEMENTS *******************************/

cpp_statement:
        cpp_labeled_statement
        | cpp_compound_statement
        | cpp_expression_statement
        | cpp_selection_statement
        | cpp_iteration_statement
        | cpp_jump_statement
        | cpp_declaration /* C++, not ANSI C */
        ;

cpp_labeled_statement:
        cpp_label                      COLON cpp_statement
        | CASE cpp_constant_expression COLON cpp_statement
        | DEFAULT                  COLON cpp_statement
        ;

cpp_compound_statement:
        LBRACE cpp_statement_list_opt RBRACE
        ;

cpp_declaration_list:
        cpp_declaration
        | cpp_declaration_list cpp_declaration
        ;

cpp_statement_list_opt:
        /* nothing */
        | cpp_statement_list_opt cpp_statement
        ;

cpp_expression_statement:
        cpp_comma_expression_opt SEMICOLON
        ;

cpp_selection_statement:
          IF LPAREN cpp_comma_expression RPAREN cpp_statement
        | IF LPAREN cpp_comma_expression RPAREN cpp_statement ELSE cpp_statement
        | SWITCH LPAREN cpp_comma_expression RPAREN cpp_statement
        ;

cpp_iteration_statement:
        WHILE LPAREN cpp_comma_expression_opt RPAREN cpp_statement
        | DO cpp_statement WHILE LPAREN cpp_comma_expression RPAREN SEMICOLON
        | FOR LPAREN cpp_comma_expression_opt SEMICOLON cpp_comma_expression_opt SEMICOLON
                cpp_comma_expression_opt RPAREN cpp_statement
        | FOR LPAREN cpp_declaration        cpp_comma_expression_opt SEMICOLON
                cpp_comma_expression_opt RPAREN cpp_statement  /* C++, not ANSI C */
        ;

cpp_jump_statement:
        GOTO cpp_label                     SEMICOLON
        | CONTINUE                     SEMICOLON
        | BREAK                        SEMICOLON
        | RETURN cpp_comma_expression_opt  SEMICOLON
        ;

cpp_label:
        ID {}
        ;

/***************************** EXTERNAL DEFINITIONS *****************************/

cpp_translation_unit:
        /* nothing */
        | cpp_translation_unit cpp_external_definition
        ;

cpp_external_definition:
        cpp_function_declaration                         /* C++, cpp_not ANSI C*/
        | cpp_function_definition
        | cpp_declaration
        | cpp_linkage_specifier cpp_function_declaration     /* C++, not ANSI C*/
        | cpp_linkage_specifier cpp_function_definition      /* C++, not ANSI C*/
        | cpp_linkage_specifier cpp_declaration              /* C++, not ANSI C*/
        | cpp_linkage_specifier LBRACE cpp_translation_unit RBRACE /* C++, not ANSI C*/
        ;

cpp_linkage_specifier:
        EXTERN LIT_STR
        ;

cpp_function_declaration:
        cpp_identifier_declarator SEMICOLON   /*  semantically  verify  it is a 
                                    function, and (if ANSI says  it's 
                                    the  law for C++ also...) that it 
                                    is something that  can't  have  a 
                                    return  type  (like  a conversion 
                                    function, or a destructor */

        | cpp_constructor_function_declaration SEMICOLON
        ;

cpp_function_definition:
        cpp_new_function_definition
        | cpp_old_function_definition
        | cpp_constructor_function_definition
        ;

cpp_new_function_definition:
										cpp_identifier_declarator cpp_compound_statement
        | cpp_declaration_specifier     cpp_declarator cpp_compound_statement /* partially C++ only */
        | cpp_type_specifier            cpp_declarator cpp_compound_statement /* partially C++ only */
        | cpp_basic_type_name           cpp_declarator cpp_compound_statement /* partially C++ only */
        | ID                            cpp_declarator cpp_compound_statement {} /* partially C++ only */
        | cpp_global_or_scoped_typedefname	cpp_declarator cpp_compound_statement /* partially C++ only */
        | cpp_declaration_qualifier_list	cpp_identifier_declarator cpp_compound_statement
        | cpp_type_qualifier_list       cpp_identifier_declarator cpp_compound_statement
        ;

cpp_old_function_definition:
											cpp_old_function_declarator cpp_old_function_body
        | cpp_declaration_specifier			cpp_old_function_declarator cpp_old_function_body
        | cpp_type_specifier				cpp_old_function_declarator cpp_old_function_body
        | cpp_basic_type_name				cpp_old_function_declarator cpp_old_function_body
        | ID								cpp_old_function_declarator cpp_old_function_body {}
        | cpp_global_or_scoped_typedefname	cpp_old_function_declarator cpp_old_function_body
        | cpp_declaration_qualifier_list	cpp_old_function_declarator cpp_old_function_body
        | cpp_type_qualifier_list			cpp_old_function_declarator cpp_old_function_body
        ;

cpp_old_function_body:
        cpp_declaration_list cpp_compound_statement
        | cpp_compound_statement
        ;

cpp_constructor_function_definition:
        cpp_global_or_scoped_typedefname cpp_parameter_type_list cpp_constructor_init_list_opt cpp_compound_statement
        | cpp_declaration_specifier cpp_parameter_type_list cpp_constructor_init_list_opt cpp_compound_statement
        ;

cpp_constructor_function_declaration:
        cpp_global_or_scoped_typedefname cpp_parameter_type_list  /* wasteful redeclaration; used for friend decls.  */
        | cpp_declaration_specifier      cpp_parameter_type_list  /* request to inline, no definition */
        ;

cpp_constructor_function_in_class:
        cpp_declaration_specifier cpp_constructor_parameter_list_and_body
        | ID cpp_constructor_parameter_list_and_body {}
        ;

    /* C++, not ANSI C */
cpp_constructor_parameter_list_and_body:
          LPAREN RPAREN cpp_type_qualifier_list_opt SEMICOLON
        | LPAREN cpp_type_name cpp_initializer RPAREN cpp_type_qualifier_list_opt SEMICOLON 
        | LPAREN cpp_named_parameter_type_list RPAREN cpp_type_qualifier_list_opt SEMICOLON
        | LPAREN RPAREN cpp_type_qualifier_list_opt cpp_constructor_init_list_opt cpp_compound_statement
        | LPAREN cpp_type_name cpp_initializer RPAREN cpp_type_qualifier_list_opt cpp_constructor_init_list_opt cpp_compound_statement
        | LPAREN cpp_named_parameter_type_list RPAREN cpp_type_qualifier_list_opt cpp_constructor_init_list_opt cpp_compound_statement
        | cpp_constructor_conflicting_parameter_list_and_body
        ;

cpp_constructor_conflicting_parameter_list_and_body:
        LPAREN cpp_type_specifier RPAREN cpp_type_qualifier_list_opt SEMICOLON
        | LPAREN cpp_basic_type_name RPAREN cpp_type_qualifier_list_opt SEMICOLON
        | LPAREN ID RPAREN cpp_type_qualifier_list_opt SEMICOLON {}
        | LPAREN cpp_global_or_scoped_typedefname RPAREN cpp_type_qualifier_list_opt SEMICOLON
        | LPAREN cpp_type_qualifier_list RPAREN cpp_type_qualifier_list_opt SEMICOLON
        | LPAREN cpp_type_specifier cpp_abstract_declarator RPAREN cpp_type_qualifier_list_opt SEMICOLON
        | LPAREN cpp_basic_type_name cpp_abstract_declarator RPAREN cpp_type_qualifier_list_opt SEMICOLON
        /* cpp_missing cpp_entry cpp_posted cpp_below */
        | LPAREN cpp_global_or_scoped_typedefname cpp_abstract_declarator RPAREN cpp_type_qualifier_list_opt SEMICOLON
        | LPAREN cpp_type_qualifier_list cpp_abstract_declarator RPAREN cpp_type_qualifier_list_opt SEMICOLON
        | LPAREN cpp_type_specifier RPAREN cpp_type_qualifier_list_opt cpp_constructor_init_list_opt cpp_compound_statement
        | LPAREN cpp_basic_type_name RPAREN cpp_type_qualifier_list_opt cpp_constructor_init_list_opt cpp_compound_statement
        | LPAREN ID RPAREN cpp_type_qualifier_list_opt cpp_constructor_init_list_opt cpp_compound_statement {}
        | LPAREN cpp_global_or_scoped_typedefname RPAREN cpp_type_qualifier_list_opt cpp_constructor_init_list_opt cpp_compound_statement
        | LPAREN cpp_type_qualifier_list RPAREN cpp_type_qualifier_list_opt cpp_constructor_init_list_opt cpp_compound_statement
        | LPAREN cpp_type_specifier  cpp_abstract_declarator RPAREN cpp_type_qualifier_list_opt cpp_constructor_init_list_opt cpp_compound_statement
        | LPAREN cpp_basic_type_name cpp_abstract_declarator RPAREN cpp_type_qualifier_list_opt cpp_constructor_init_list_opt cpp_compound_statement
        /* cpp_missing cpp_entry posted below */
        | LPAREN cpp_global_or_scoped_typedefname cpp_abstract_declarator RPAREN cpp_type_qualifier_list_opt cpp_constructor_init_list_opt cpp_compound_statement
        | LPAREN cpp_type_qualifier_list cpp_abstract_declarator RPAREN cpp_type_qualifier_list_opt cpp_constructor_init_list_opt cpp_compound_statement
        | cpp_constructor_conflicting_typedef_declarator
        ;

cpp_constructor_conflicting_typedef_declarator:
        LPAREN ID cpp_unary_abstract_declarator RPAREN cpp_type_qualifier_list_opt SEMICOLON {}
        | LPAREN ID cpp_unary_abstract_declarator RPAREN cpp_type_qualifier_list_opt cpp_constructor_init_list_opt cpp_compound_statement {}
        | LPAREN ID cpp_postfix_abstract_declarator RPAREN cpp_type_qualifier_list_opt SEMICOLON {}
        | LPAREN ID cpp_postfix_abstract_declarator RPAREN cpp_type_qualifier_list_opt cpp_constructor_init_list_opt cpp_compound_statement {}
        | LPAREN ID cpp_postfixing_abstract_declarator RPAREN cpp_type_qualifier_list_opt SEMICOLON {}
        | LPAREN ID cpp_postfixing_abstract_declarator  RPAREN cpp_type_qualifier_list_opt cpp_constructor_init_list_opt cpp_compound_statement {}
        ;

cpp_constructor_init_list_opt:
        /* nothing */
        | cpp_constructor_init_list
        ;

cpp_constructor_init_list:
        COLON cpp_constructor_init
        | cpp_constructor_init_list COMMA cpp_constructor_init
        ;

cpp_constructor_init:
          ID LPAREN cpp_argument_expression_list RPAREN {}
        | ID LPAREN RPAREN {}
        | cpp_global_or_scoped_typedefname LPAREN cpp_argument_expression_list RPAREN
        | cpp_global_or_scoped_typedefname LPAREN RPAREN
        | LPAREN cpp_argument_expression_list RPAREN /* Single inheritance ONLY*/
        | LPAREN RPAREN /* Is cpp_this legal? It might be default! */
        ;

cpp_declarator:
        cpp_identifier_declarator
        | cpp_typedef_declarator
        ;

cpp_typedef_declarator:
        cpp_paren_typedef_declarator          /* would be ambiguous as parameter*/
        | cpp_simple_paren_typedef_declarator /* also ambiguous */
        | cpp_parameter_typedef_declarator    /* not ambiguous as parameter*/
        ;

cpp_parameter_typedef_declarator:
        ID {}
        | ID cpp_postfixing_abstract_declarator {}
        | cpp_clean_typedef_declarator
        ;

cpp_clean_typedef_declarator:
        cpp_clean_postfix_typedef_declarator
        | cpp_asterisk_or_ampersand cpp_parameter_typedef_declarator
        | cpp_unary_modifier        cpp_parameter_typedef_declarator
        ;

cpp_clean_postfix_typedef_declarator:
        LPAREN cpp_clean_typedef_declarator RPAREN
        | LPAREN cpp_clean_typedef_declarator RPAREN cpp_postfixing_abstract_declarator
        ;

cpp_paren_typedef_declarator:
        cpp_postfix_paren_typedef_declarator
        | cpp_asterisk_or_ampersand LPAREN cpp_simple_paren_typedef_declarator RPAREN
        | cpp_unary_modifier        LPAREN cpp_simple_paren_typedef_declarator RPAREN
        | cpp_asterisk_or_ampersand LPAREN ID RPAREN /* redundant paren */ {}
        | cpp_unary_modifier        LPAREN ID RPAREN /* redundant paren */ {}
        | cpp_asterisk_or_ampersand cpp_paren_typedef_declarator
        | cpp_unary_modifier        cpp_paren_typedef_declarator
        ;

cpp_postfix_paren_typedef_declarator:
        LPAREN cpp_paren_typedef_declarator RPAREN
        | LPAREN cpp_simple_paren_typedef_declarator cpp_postfixing_abstract_declarator RPAREN
        | LPAREN ID cpp_postfixing_abstract_declarator RPAREN {} /* redundant paren */
        | LPAREN cpp_paren_typedef_declarator RPAREN cpp_postfixing_abstract_declarator
        ;

cpp_simple_paren_typedef_declarator:
        LPAREN ID RPAREN {}
        | LPAREN cpp_simple_paren_typedef_declarator RPAREN
        ;

cpp_identifier_declarator:
        cpp_unary_identifier_declarator
        | cpp_paren_identifier_declarator
        ;

cpp_unary_identifier_declarator:
        cpp_postfix_identifier_declarator
        | cpp_asterisk_or_ampersand cpp_identifier_declarator
        | cpp_unary_modifier cpp_identifier_declarator
        ;

cpp_postfix_identifier_declarator:
        cpp_paren_identifier_declarator cpp_postfixing_abstract_declarator
        | LPAREN cpp_unary_identifier_declarator RPAREN
        | LPAREN cpp_unary_identifier_declarator RPAREN cpp_postfixing_abstract_declarator
        ;

cpp_old_function_declarator:
        cpp_postfix_old_function_declarator
        | cpp_asterisk_or_ampersand cpp_old_function_declarator
        | cpp_unary_modifier cpp_old_function_declarator
        ;

cpp_postfix_old_function_declarator:
        cpp_paren_identifier_declarator LPAREN cpp_argument_expression_list RPAREN
        | LPAREN cpp_old_function_declarator RPAREN
        | LPAREN cpp_old_function_declarator RPAREN cpp_old_postfixing_abstract_declarator
        ;

cpp_old_postfixing_abstract_declarator:
        cpp_array_abstract_declarator /* array modifiers */
        | cpp_old_parameter_type_list  /* function returning modifiers */
        ;

cpp_abstract_declarator:
        cpp_unary_abstract_declarator
        | cpp_postfix_abstract_declarator
        | cpp_postfixing_abstract_declarator
        ;

cpp_postfixing_abstract_declarator:
        cpp_array_abstract_declarator
        | cpp_parameter_type_list
        ;

cpp_array_abstract_declarator:
        LBRACKET RBRACKET
        | LBRACKET cpp_constant_expression RBRACKET
        | cpp_array_abstract_declarator LBRACKET cpp_constant_expression RBRACKET
        ;

cpp_unary_abstract_declarator:
        cpp_asterisk_or_ampersand
        | cpp_unary_modifier
        | cpp_asterisk_or_ampersand cpp_abstract_declarator
        | cpp_unary_modifier cpp_abstract_declarator
        ;

cpp_postfix_abstract_declarator:
        LPAREN cpp_unary_abstract_declarator RPAREN
        | LPAREN cpp_postfix_abstract_declarator RPAREN
        | LPAREN cpp_postfixing_abstract_declarator RPAREN
        | LPAREN cpp_unary_abstract_declarator RPAREN cpp_postfixing_abstract_declarator
        ;

cpp_asterisk_or_ampersand:
        ASTERISK
        | BITAND
        ;

cpp_unary_modifier:
        cpp_scope ASTERISK cpp_type_qualifier_list_opt
        | cpp_asterisk_or_ampersand cpp_type_qualifier_list
        ;

/************************* NESTED SCOPE SUPPORT ******************************/

cpp_scoping_name:
        cpp_tag_name
        | cpp_aggregate_key cpp_tag_name /* also update symbol table here by notifying it about a (possibly) new tag*/
        ;

cpp_scope:
        cpp_scoping_name CLCL
        | cpp_scope cpp_scoping_name  CLCL
        ;

cpp_tag_name:
        ID {}
        ;

cpp_global_scope:
        { /*scan for upcoming name in file scope */ } CLCL
        ;

cpp_global_or_scope:
        cpp_global_scope
        | cpp_scope
        | cpp_global_scope cpp_scope
        ;

cpp_scope_opt_identifier:
          ID {}
        | cpp_scoped_typedefname /* was: cpp_scope ID {}*/ /* C++ not ANSI C */
        ;

cpp_scope_opt_complex_name:
          cpp_complex_name
        | cpp_scope cpp_complex_name
        ;

cpp_complex_name:
        TILDE ID {}
        | cpp_operator_function_name
        ;

cpp_global_opt_scope_opt_identifier:
        cpp_global_scope cpp_scope_opt_identifier
        | cpp_scope_opt_identifier
        ;

cpp_global_opt_scope_opt_complex_name:
        cpp_global_scope cpp_scope_opt_complex_name
        | cpp_scope_opt_complex_name
        ;

cpp_scoped_typedefname:
        cpp_scope ID {}
        ;

cpp_global_or_scoped_typedefname:
          cpp_scoped_typedefname
        | cpp_global_scope cpp_scoped_typedefname
        | cpp_global_scope ID {}
        ;

cpp_global_opt_scope_opt_typedefname:
        ID {}
        | cpp_global_or_scoped_typedefname
        ;

/************************************************************************************
 * end C/C++ specific rules
 ************************************************************************************/
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