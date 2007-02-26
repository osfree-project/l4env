%{
/*
 * Copyright (C) 2001-2003
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
#include "Compiler.h"
#include "CParser.h"

void dceerror(char *);
void dceerror2(const char *fmt, ...);
void dcewarning(const char *fmt, ...);

// necessary front-end classes
class CFEFileComponent;
class CFEPortSpec;

#include "fe/FEUnaryExpression.h" // for enum EXPT_OPERATOR

#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEFile.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEOperation.h"
#include "fe/FEUnionCase.h"

#include "fe/FEConstDeclarator.h"
#include "fe/FEEnumDeclarator.h"
#include "fe/FEFunctionDeclarator.h"
#include "fe/FEArrayDeclarator.h"

#include "fe/FETaggedStructType.h"
#include "fe/FETaggedUnionType.h"
#include "fe/FETaggedEnumType.h"
#include "fe/FEUserDefinedType.h"
#include "fe/FESimpleType.h"
#include "fe/FEPipeType.h"

#include "fe/FEIntAttribute.h"
#include "fe/FEVersionAttribute.h"
#include "fe/FEEndPointAttribute.h"
#include "fe/FEExceptionAttribute.h"
#include "fe/FEStringAttribute.h"
#include "fe/FEPtrDefaultAttribute.h"
#include "fe/FEIsAttribute.h"
#include "fe/FETypeAttribute.h"

#include "fe/FEConditionalExpression.h"
#include "fe/FESizeOfExpression.h"
#include "fe/FEUserDefinedExpression.h"

#include "parser.h"
int dcelex(YYSTYPE*);

#define YYDEBUG	1

// collection for elements
extern CFEFileComponent *pCurFileComponent;

// error stuff
extern int errcount;
extern int warningcount;
extern int erroccured;

extern int gLineNumber;

// #include/import special treatment
extern String sInFileName;

// indicate which TOKENS should be recognized as tokens
extern int c_inc;
extern int c_inc_old;

// import helper
int nParseErrorDCE = 0;

// import helper
int nImportMe = 0;

// helper functions
bool
DoInterfaceCheck(String *pIName, Vector *pBaseInterfaces, CFEInterface **pFEInterface);

bool
AddInterfaceToFileComponent(CFEInterface* pFEInterface, CFELibrary *pFELibrary);

%}

// we want a reentrant parser
%pure_parser

%union {
  String*		_id;
  String*		_str;
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
  CFEEnumDeclarator*		_enum_decl;

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

  enum EXPT_OPERATOR		_expr_operator;
}

%token	LBRACE		RBRACE		LBRACKET	RBRACKET	COLON		COMMA
%token	LPAREN		RPAREN		DOT			QUOT		ASTERISK	SINGLEQUOT
%token	QUESTION	BITOR		BITXOR		BITAND		LT			GT
%token	PLUS		MINUS		DIV			MOD			TILDE		EXCLAM
%token	SEMICOLON	LOGICALOR	LOGICALAND	EQUAL		NOTEQUAL	LTEQUAL
%token	GTEQUAL		LSHIFT		RSHIFT		DOTDOT		SCOPE

%token	IS			BOOLEAN		BYTE		CASE		CHAR		CONST
%token	DEFAULT		DOUBLE		ENUM		FALSE		FLOAT		HANDLE_T
%token	HYPER		IMPORT		INT			INTERFACE	LONG		EXPNULL
%token	PIPE		SHORT		SMALL		STRUCT		SWITCH		TRUE
%token	TYPEDEF		UNION		UNSIGNED	SIGNED		VOID		ERROR_STATUS_T
%token	FLEXPAGE	REFSTRING	OBJECT		IID_IS		ISO_LATIN_1	ISO_MULTI_LINGUAL
%token	ISO_UCS		CHAR_PTR	VOID_PTR	LONGLONG

%token	BROADCAST	CONTEXT_HANDLE	ENDPOINT	EXCEPTIONS	FIRST_IS	HANDLE
%token	IDEMPOTENT	IGNORE			IN			LAST_IS		LENGTH_IS	LOCAL
%token	MAX_IS		MAYBE			MIN_IS		OUT			PTR			POINTER_DEFAULT
%token	REF			REFLECT_DELETIONS			SIZE_IS		STRING		SWITCH_IS
%token	SWITCH_TYPE	TRANSMIT_AS		UNIQUE		UUID		VERSION_ATTR		RAISES
%token	EXCEPTION	LIBRARY			CONTROL		HELPCONTEXT	HELPFILE	HELPSTRING
%token	HIDDEN		LCID			RESTRICTED	AUTO_HANDLE	BINDING_CALLOUT
%token	CODE		COMM_STATUS		CS_CHAR		CS_DRTAG	CS_RTAG		CS_STAG
%token	TAG_RTN		ENABLE_ALLOCATE	EXTERN_EXCEPTIONS		EXPLICIT_HANDLE
%token	FAULT_STATUS				HEAP		IMPLICIT_HANDLE			NOCODE
%token	REPRESENT_AS				USER_MARSHAL			WITHOUT_USING_EXCEPTIONS
%token  DEFAULT_FUNCTION            ERROR_FUNCTION          SERVER_PARAMETER
%token  INIT_RCVSTRING              INIT_WITH_IN            ALLOW_REPLY_ONLY
%token  PREALLOC    ABSTRACT        INOUT       MODULE

/* dice specific token */
%token EOF_TOKEN

%token	<_id>		ID
%token	<_id>		TYPENAME
%token	<_int>		LIT_INT
%token	<_char>		LIT_CHAR
%token	<_str>		LIT_STR
%token	<_str>		UUID_STR
%token	<_double>	LIT_FLOAT
%token	<_str>		FILENAME
%token	<_str>		PORTSPEC
%token  <_str>      VERSION_STR

%type <_expr>				additive_expr
%type <_expr>				and_expr
%type <_expr>				array_bound
%type <_array_decl>			array_declarator
%type <_decl>				attr_var
%type <_collection>			attr_var_list
%type <_collection>         base_interface_list
%type <_id>                 base_interface_name
%type <_simple_type>		base_type_spec
%type <_simple_type>		boolean_type
%type <_expr>				cast_expr
%type <_simple_type>		char_type
%type <_expr>				conditional_expr
%type <_constructed_type>	constructed_type_spec
%type <_const_decl>			const_declarator
%type <_expr>				const_expr
%type <_collection>			const_expr_list
%type <_type_spec>			const_type_spec
%type <_decl>				declarator
%type <_collection>			declarator_list
%type <_type_spec>			declaration_specifier
%type <_type_spec>			declaration_specifiers
%type <_attr>				directional_attribute
%type <_decl>				direct_declarator
%type <_enum_decl>			enumeration_declarator
%type <_collection>			enumeration_declarator_list
%type <_enum_type>			enumeration_type
%type <_enum_type>			tagged_enumeration_type
%type <_expr>				equality_expr
%type <_typed_decl>			exception_declarator
%type <_collection>	        excep_name_list
%type <_expr>				exclusive_or_expr
%type <_i_component>		export
%type <_attr>				field_attribute
%type <_collection>			field_attributes
%type <_collection>			field_attribute_list
%type <_typed_decl>			field_declarator
%type <_simple_type>		floating_pt_type
%type <_func_decl>			function_declarator
%type <_collection>			identifier_list
%type <_str>                scoped_name
%type <_id>                 id_or_typename
%type <_expr>				inclusive_or_expr
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
%type <_expr>				logical_and_expr
%type <_expr>				logical_or_expr
%type <_typed_decl>			member
%type <_collection>			member_list
%type <_collection>			member_list_1
%type <_expr>				multiplicative_expr
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
%type <_collection>			port_specs
%type <_expr>				postfix_expr
%type <_type_spec>			predefined_type_spec
%type <_expr>				primary_expr
%type <_attr>				ptr_attr
%type <_collection>			raises_declarator
%type <_expr>				relational_expr
%type <_expr>				shift_expr
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
%type <_expr>				unary_expr
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
//%type <_str>				uuid_rep
%type <_version>   			version_rep
%type <_expr_operator>		unary_operator

/******************************************************************************
 * GCC specific tokens
 ******************************************************************************/

%token	INC_OP	DEC_OP		ELLIPSIS	IF		ELSE		WHILE	DO	FOR
%token	BREAK	CONTINUE	RETURN		GOTO	ASM_KEYWORD	TYPEOF	ALIGNOF
%token	ATTRIBUTE			EXTENSION	LABEL	SIZEOF

%token	RS_ASSIGN	LS_ASSIGN	ADD_ASSIGN	SUB_ASSIGN	MUL_ASSIGN	DIV_ASSIGN
%token	MOD_ASSIGN	AND_ASSIGN	XOR_ASSIGN	OR_ASSIGN	PTR_OP

%token	ITERATOR	AUTO		EXTERN		REGISTER	STATIC		ONEWAY
%token	RESTRICT	VOLATILE	BYCOPY		BYREF		INLINE
%token	COMPLEX		REALPART	IMAGPART	C_CONST		

/******************************************************************************
 * end GCC specific tokens
 ******************************************************************************/

%left COMMA SEMICOLON
%right PLUS MINUS 
%left LOGICALAND LOGICALOR BITAND BITOR
%left LSHIFT RSHIFT

%right LPAREN
%left RPAREN

%token IDENTIFIER

%token PREC_TOKEN

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
	  interface semicolon
	{
		// interface already added to current file
	}
	| library semicolon
	{
		// library is already added to current file
	}
	| type_declarator semicolon
	{
		if (!CParser::GetCurrentFile())
		{
			CCompiler::GccError(NULL, 0, "Fatal Error: current file vanished (typedef)");
			YYABORT;
		}
		if (pCurFileComponent)
		{
			if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFELibrary)))
				((CFELibrary*)pCurFileComponent)->AddTypedef($1);
			else if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFEInterface)))
				((CFEInterface*)pCurFileComponent)->AddTypedef($1);
			else
			{
				TRACE("current file component is unknown type: %s\n", (const char*)pCurFileComponent->GetClassName());
				assert(false);
			}
		}
		else
			CParser::GetCurrentFile()->AddTypedef($1);
	}
	| const_declarator semicolon
	{
		if (!CParser::GetCurrentFile())
		{
			CCompiler::GccError(NULL, 0, "Fatal Error: current file vanished (const)");
			YYABORT;
		}
		else
			CParser::GetCurrentFile()->AddConstant($1);
	}
	| tagged_declarator semicolon
	{
		if (!CParser::GetCurrentFile())
		{
			CCompiler::GccError(NULL, 0, "Fatal Error: current file vanished (struct/union)");
			YYABORT;
		}
		else
			CParser::GetCurrentFile()->AddTaggedDecl($1);
	}
//	| gcc_function
//	{
		/** cpp functions are ignored */
//	}
//	| gcc_declarator
//	{
	    /** cpp declarators (global variables) are ignored */
//	}
//	| import
	| SEMICOLON
	| error SEMICOLON
	{
		dceerror2("unexpected token(s) before ';' (maybe C code in IDL file?!)");
		YYABORT;
	}
	| EOF_TOKEN
	{
	    YYACCEPT;
	}
	;

import:
	  IMPORT import_files opt_semicolon
	{}
	;

import_files:
	  import_files COMMA FILENAME
	{
		// get current parser
		CParser *pCurParser = CParser::GetCurrentParser();
		if (pCurParser)
		{
			if (!pCurParser->Import(*$3))
			{
			    TRACE("Aborted to import \"%s\".\n", (const char*)*$3);
				delete $3;
				YYABORT;
			}
		}
		delete $3;
	}
	| FILENAME
	{
		// get current parser
		CParser *pCurParser = CParser::GetCurrentParser();
		if (pCurParser)
		{
			if (!pCurParser->Import(*$1))
			{
			    TRACE("Aborted to import \"%s\".\n", (const char*)*$1);
				delete $1;
				YYABORT;
			}
		}
		delete $1;
	}
	| SEMICOLON
	{}
	;

interface:
	  interface_attributes INTERFACE ID COLON base_interface_list LBRACE
	{
		// test interfaces
		CFEInterface* pFEInterface;
		if (!DoInterfaceCheck($3, $5, &pFEInterface))
		    YYABORT;
		// create interface and set as file-component
        if (!pFEInterface)
            pFEInterface = new CFEInterface($1, *$3, $5, NULL);
        else
        {
            // add attributes, and base names
            pFEInterface->AddAttributes($1);
            pFEInterface->AddBaseInterfaceNames($5);
        }
		$1->SetParentOfElements(pFEInterface);
		pFEInterface->SetSourceLine(gLineNumber);
		if (!AddInterfaceToFileComponent(pFEInterface, NULL))
		    YYABORT;
		pCurFileComponent = pFEInterface;
	} interface_component_list RBRACE
	{
		// imported elements are already added to respective file object
		// adjust current file component
		if (pCurFileComponent &&
			pCurFileComponent->GetParent())
		{
			if (pCurFileComponent->GetParent()->IsKindOf(RUNTIME_CLASS(CFEFileComponent)))
				pCurFileComponent = (CFEFileComponent*)pCurFileComponent->GetParent();
			else
				pCurFileComponent = NULL;
		}
        delete $3;
		$$ = NULL;
	}
	| interface_attributes INTERFACE ID COLON base_interface_list LBRACE
	{
		// test interfaces
		CFEInterface* pFEInterface;
		if (!DoInterfaceCheck($3, $5, &pFEInterface))
		    YYABORT;
		// create interface and set as file-component
        if (!pFEInterface)
            pFEInterface = new CFEInterface($1, *$3, $5, NULL);
        else
        {
            // add attributes, and base names
            pFEInterface->AddAttributes($1);
            pFEInterface->AddBaseInterfaceNames($5);
        }
		$1->SetParentOfElements(pFEInterface);
		pFEInterface->SetSourceLine(gLineNumber);
		if (!AddInterfaceToFileComponent(pFEInterface, NULL))
		    YYABORT;
	} RBRACE
	{
		// imported elements are already added to respective file object
        delete $3;
		$$ = NULL;
	}
	| interface_attributes INTERFACE ID LBRACE
	{
	    CFEInterface* pFEInterface;
		if (!DoInterfaceCheck($3, NULL, &pFEInterface))
		    YYABORT;
		// create interface and set as file-component
        if (!pFEInterface)
            pFEInterface = new CFEInterface($1, *$3, NULL, NULL);
        else
        {
            // add attributes
            pFEInterface->AddAttributes($1);
        }
		$1->SetParentOfElements(pFEInterface);
		pFEInterface->SetSourceLine(gLineNumber);
		if (!AddInterfaceToFileComponent(pFEInterface, NULL))
		    YYABORT;
		pCurFileComponent = pFEInterface;
	} interface_component_list RBRACE
	{
		// imported elements already added to respective file objects
		// adjust current file component
		if (pCurFileComponent &&
			pCurFileComponent->GetParent())
		{
			if (pCurFileComponent->GetParent()->IsKindOf(RUNTIME_CLASS(CFEFileComponent)))
				pCurFileComponent = (CFEFileComponent*) pCurFileComponent->GetParent();
			else
				pCurFileComponent = NULL;
		}
		delete $3;
		$$ = NULL;
	}
	| interface_attributes INTERFACE ID LBRACE
	{
	    CFEInterface* pFEInterface;
		if (!DoInterfaceCheck($3, NULL, &pFEInterface))
		    YYABORT;
		// create interface and set as file-component
        if (!pFEInterface)
            pFEInterface = new CFEInterface($1, *$3, NULL, NULL);
        else
        {
            // add attributes
            pFEInterface->AddAttributes($1);
        }
		$1->SetParentOfElements(pFEInterface);
		pFEInterface->SetSourceLine(gLineNumber);
		if (!AddInterfaceToFileComponent(pFEInterface, NULL))
		    YYABORT;
	} RBRACE
	{
		// imported elements already added to respective file objects
		delete $3;
		$$ = NULL;
	}
    | interface_attributes INTERFACE scoped_name
    {
        // forward declaration
		CFEInterface* pFEInterface;
	    if (!DoInterfaceCheck($3, NULL, &pFEInterface))
		    YYABORT;
		// create interface but do not set as filecomponent: we only need it to be found
        // if scoped name: find libraries first
		CFEFile *pRoot = CParser::GetCurrentFile()->GetRoot();
        int nScopePos = 0;
        CFELibrary *pFELibrary = NULL;
        String sRest = *$3;
        while ((nScopePos = sRest.Find("::")) >= 0)
        {
            String sScope = sRest.Left(nScopePos);
            sRest = sRest.Right(sRest.GetLength()-nScopePos-2);
            if (!sScope.IsEmpty())
            {
                if (pFELibrary)
                    pFELibrary = pFELibrary->FindLibrary(sScope);
                else
                    pFELibrary = pRoot->FindLibrary(sScope);
                if (!pFELibrary)
                {
                    dceerror2("Cannot find library %s, which is used in forward declaration of %s.",(const char*)sScope, (const char*)(*$3));
                    YYABORT;
                }
            }
        }
		if (pFEInterface)
			delete pFEInterface;
		pFEInterface = new CFEInterface(NULL, sRest, NULL, NULL);
		pFEInterface->SetSourceLine(gLineNumber);
		if (!AddInterfaceToFileComponent(pFEInterface, pFELibrary))
		    YYABORT;
		delete $3;
        $$ = NULL;
    }
	| INTERFACE ID COLON base_interface_list LBRACE
	{
		// test interfaces
		CFEInterface* pFEInterface;
		if (!DoInterfaceCheck($2, $4, &pFEInterface))
		    YYABORT;
		// create interface and set as file-component
		CFEVersionAttribute *tmp = new CFEVersionAttribute(0,0);
		tmp->SetSourceLine(gLineNumber);
        Vector *vec = new Vector(RUNTIME_CLASS(CFEAttribute), 1, tmp);
        if (!pFEInterface)
        {
            pFEInterface = new CFEInterface(vec, *$2, $4, NULL);
        }
        else
        {
            // add attributes, and base names
            pFEInterface->AddAttributes(vec);
            pFEInterface->AddBaseInterfaceNames($4);
        }
        tmp->SetParent(pFEInterface);
		pFEInterface->SetSourceLine(gLineNumber);
		if (!AddInterfaceToFileComponent(pFEInterface, NULL))
		    YYABORT;
		pCurFileComponent = pFEInterface;
	} interface_component_list RBRACE
	{
		// imported elements are already added to respective file object
		// adjust current file component
		if (pCurFileComponent &&
			pCurFileComponent->GetParent())
		{
			if (pCurFileComponent->GetParent()->IsKindOf(RUNTIME_CLASS(CFEFileComponent)))
				pCurFileComponent = (CFEFileComponent*)pCurFileComponent->GetParent();
			else
				pCurFileComponent = NULL;
		}
        delete $2;
		$$ = NULL;
	}
	| INTERFACE ID LBRACE
	{
	    CFEInterface* pFEInterface;
		if (!DoInterfaceCheck($2, NULL, &pFEInterface))
		    YYABORT;
		// create interface and set as file-component
        CFEAttribute *tmp = new CFEVersionAttribute(0,0);
        tmp->SetSourceLine(gLineNumber);
        Vector *vec = new Vector(RUNTIME_CLASS(CFEAttribute), 1, tmp);
        if (!pFEInterface)
		{
            pFEInterface = new CFEInterface(vec, *$2, NULL, NULL);
		}
        else
        {
            // add attributes
            pFEInterface->AddAttributes(vec);
        }
        tmp->SetParent(pFEInterface);
		pFEInterface->SetSourceLine(gLineNumber);
		if (!AddInterfaceToFileComponent(pFEInterface, NULL))
		    YYABORT;
		pCurFileComponent = pFEInterface;
	} interface_component_list RBRACE
	{
		// imported elements already added to respective file objects
		// adjust current file component
		if (pCurFileComponent &&
			pCurFileComponent->GetParent())
		{
			if (pCurFileComponent->GetParent()->IsKindOf(RUNTIME_CLASS(CFEFileComponent)))
				pCurFileComponent = (CFEFileComponent*) pCurFileComponent->GetParent();
			else
				pCurFileComponent = NULL;
		}
		delete $2;
		$$ = NULL;
	}
	| INTERFACE ID COLON base_interface_list LBRACE
	{
		// test base interfaces
		CFEInterface* pFEInterface;
		if (!DoInterfaceCheck($2, $4, &pFEInterface))
		    YYABORT;
		// create interface and set as file-component
        if (!pFEInterface)
            pFEInterface = new CFEInterface(NULL, *$2, $4, NULL);
        else
        {
            // add attributes, and base names
            pFEInterface->AddBaseInterfaceNames($4);
        }
		pFEInterface->SetSourceLine(gLineNumber);
		if (!AddInterfaceToFileComponent(pFEInterface, NULL))
		    YYABORT;
	} RBRACE
	{
	    delete $2;
		$$ = NULL;
	}
	| INTERFACE ID LBRACE
	{
	    CFEInterface* pFEInterface;
		if (!DoInterfaceCheck($2, NULL, &pFEInterface))
		    YYABORT;
		// create interface and set as file-component
        CFEAttribute *tmp = new CFEVersionAttribute(0,0);
        tmp->SetSourceLine(gLineNumber);
        Vector *vec = new Vector(RUNTIME_CLASS(CFEAttribute), 1, tmp);
        if (!pFEInterface)
            pFEInterface = new CFEInterface(vec, *$2, NULL, NULL);
        else
        {
            // add attributes
            pFEInterface->AddAttributes(vec);
        }
        tmp->SetParent(pFEInterface);
		pFEInterface->SetSourceLine(gLineNumber);
		if (!AddInterfaceToFileComponent(pFEInterface, NULL))
		    YYABORT;
	} RBRACE
	{
		// imported elements already added to respective file objects
		delete $2;
		$$ = NULL;
	}
    | INTERFACE scoped_name
    {
        // forward declaration
		CFEInterface* pFEInterface;
		if (!DoInterfaceCheck($2, NULL, &pFEInterface))
		    YYABORT;
		// create interface but do not set as filecomponent: we only need it to be found
        // if scoped name: find libraries first
		CFEFile *pRoot = CParser::GetCurrentFile()->GetRoot();
        int nScopePos = 0;
        CFELibrary *pFELibrary = NULL;
        String sRest = *$2;
        while ((nScopePos = sRest.Find("::")) >= 0)
        {
            String sScope = sRest.Left(nScopePos);
            sRest = sRest.Right(sRest.GetLength()-nScopePos-2);
            if (!sScope.IsEmpty())
            {
                if (pFELibrary)
                    pFELibrary = pFELibrary->FindLibrary(sScope);
                else
                    pFELibrary = pRoot->FindLibrary(sScope);
                if (!pFELibrary)
                {
                    dceerror2("Cannot find library %s, which is used in forward declaration of %s.",(const char*)sScope, (const char*)(*$2));
                    YYABORT;
                }
            }
        }
		if (pFEInterface)
		    delete pFEInterface;
		pFEInterface = new CFEInterface(NULL, sRest, NULL, NULL);
		pFEInterface->SetSourceLine(gLineNumber);
		if (!AddInterfaceToFileComponent(pFEInterface, pFELibrary))
		    YYABORT;
		delete $2;
        $$ = NULL;
    }
	;

base_interface_list:
      base_interface_name
	{
		CFEIdentifier *tmp = new CFEIdentifier(*$1);
		tmp->SetSourceLine(gLineNumber);
		$$ = new Vector(RUNTIME_CLASS(CFEIdentifier), 1, tmp);
		delete $1;
	}
    | base_interface_list COMMA base_interface_name
	{
		CFEIdentifier *tmp = new CFEIdentifier(*$3);
		tmp->SetSourceLine(gLineNumber);
		$1->Add(tmp);
		$$ = $1;
		delete $3;
	}
	;

base_interface_name:
      scoped_name
    {
	    $$ = $1;
    }
	| TYPENAME
	{
	    $$ = $1;
	}
	;

interface_attributes :
	  { if (c_inc == 0) c_inc = 2; } LBRACKET interface_attribute_list { if (c_inc == 2) c_inc = 0; } RBRACKET { $$ = $3; }
	| { if (c_inc == 0) c_inc = 2; } LBRACKET { if (c_inc == 2) c_inc = 0; } RBRACKET
	{
		CFEVersionAttribute *tmp = new CFEVersionAttribute(0,0);
		tmp->SetSourceLine(gLineNumber);
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, tmp);
	}
	;

interface_attribute_list :
	  interface_attribute_list COMMA interface_attribute
	{
		if ($3)
			$1->Add($3);
		$$ = $1;
	}
	| interface_attribute
	{
		if ($1 != NULL)
			$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, $1);
		else
			$$ = new Vector(RUNTIME_CLASS(CFEAttribute));
	}
    | error
    {
		dceerror2("unknown attribute");
		YYABORT;
	}
	;

interface_attribute :
/*	  UUID LPAREN uuid_rep rparen
	{
		$$ = new CFEStringAttribute(ATTR_UUID, *$3);
		delete $3;
	}
	|*/
	  UUID LPAREN LIT_INT RPAREN
	{
		$$ = new CFEIntAttribute(ATTR_UUID, $3);
		$$->SetSourceLine(gLineNumber);
	}
	| UUID LPAREN error RPAREN
	{
		dcewarning("[uuid] format is incorrect");
		yyclearin;
		yyerrok;
        $$ = NULL;
	}
	| VERSION_ATTR LPAREN version_rep RPAREN
	{
		$$ = new CFEVersionAttribute($3);
		$$->SetSourceLine(gLineNumber);
	}
	| VERSION_ATTR LPAREN error RPAREN
	{
		dcewarning("[version] format is incorrect");
		yyclearin;
		yyerrok;
		$$ = new CFEVersionAttribute(0,0);
		$$->SetSourceLine(gLineNumber);
	}
	| ENDPOINT LPAREN port_specs rparen
	{
		$$ = new CFEEndPointAttribute($3);
		$$->SetSourceLine(gLineNumber);
	}
	| ENDPOINT LPAREN error RPAREN
	{
		dcewarning("[endpoint] format is incorrect");
		yyclearin;
		yyerrok;
        $$ = NULL;
	}
	| EXCEPTIONS LPAREN excep_name_list rparen
	{
		$$ = new CFEExceptionAttribute($3);
		$3->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	| EXCEPTIONS LPAREN error RPAREN
	{
		dcewarning("[exceptions] format is incorrect");
		yyclearin;
		yyerrok;
		$$ = NULL;
	}
	| LOCAL
	{
		$$ = new CFEAttribute(ATTR_LOCAL);
		$$->SetSourceLine(gLineNumber);
	}
    | SERVER_PARAMETER
    {
        $$ = new CFEAttribute(ATTR_SERVER_PARAMETER);
        $$->SetSourceLine(gLineNumber);
    }
    | INIT_RCVSTRING
    {
        $$ = new CFEAttribute(ATTR_INIT_RCVSTRING);
        $$->SetSourceLine(gLineNumber);
    }
    | INIT_RCVSTRING LPAREN ID rparen
    {
        $$ = new CFEStringAttribute(ATTR_INIT_RCVSTRING, *$3);
        delete $3;
        $$->SetSourceLine(gLineNumber);
    }
    | DEFAULT_FUNCTION LPAREN ID rparen
    {
        $$ = new CFEStringAttribute(ATTR_DEFAULT_FUNCTION, *$3);
        delete $3;
        $$->SetSourceLine(gLineNumber);
    }
    | ERROR_FUNCTION LPAREN ID rparen
    {
        $$ = new CFEStringAttribute(ATTR_ERROR_FUNCTION, *$3);
        delete $3;
        $$->SetSourceLine(gLineNumber);
    }
	| POINTER_DEFAULT LPAREN ptr_attr rparen
	{
		$$ = new CFEPtrDefaultAttribute($3);
		// set parent relationship
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| POINTER_DEFAULT LPAREN error RPAREN
	{
		dcewarning("[pointer_default] format is incorrect");
		yyclearin;
		yyerrok;
		$$ = NULL;
	}
	| OBJECT
	{
		$$ = new CFEAttribute(ATTR_OBJECT);
		$$->SetSourceLine(gLineNumber);
	}
	| ABSTRACT
	{
	    $$ = new CFEAttribute(ATTR_ABSTRACT);
		$$->SetSourceLine(gLineNumber);
	}
	;

version_rep
	: LIT_INT DOT LIT_INT
	{
		$$.nMajor = $1;
		$$.nMinor = $3;
	}
	| LIT_INT
	{
	    $$.nMajor = $1;
		$$.nMinor = 0;
	}
	| VERSION_STR
	{
	    $$.nMajor = $1->Left($1->Find('.')).ToInt();
		$$.nMinor = $1->Right($1->GetLength()-$1->Find('.')-1).ToInt();
		delete $1;
	}
	;


port_specs :
	  port_specs COMMA PORTSPEC
	{
		CFEPortSpec *tmp = new CFEPortSpec(*$3);
		tmp->SetSourceLine(gLineNumber);
		$1->Add(tmp);
		$$ = $1;
		delete $3;
	}
	| PORTSPEC
	{
		CFEPortSpec *tmp = new CFEPortSpec(*$1);
		tmp->SetSourceLine(gLineNumber);
		$$ = new Vector(RUNTIME_CLASS(CFEPortSpec), 1, tmp);
		delete $1;
	}
	;

excep_name_list :
	  excep_name_list COMMA ID
	{
		CFEIdentifier *tmp = new CFEIdentifier(*$3);
		tmp->SetSourceLine(gLineNumber);
		$1->Add(tmp);
		$$ = $1;
		delete $3;
	}
	| ID
	{
		CFEIdentifier *tmp = new CFEIdentifier(*$1);
		tmp->SetSourceLine(gLineNumber);
		$$ = new Vector(RUNTIME_CLASS(CFEIdentifier), 1, tmp);
		delete $1;
	}
	;

interface_component_list :
	  interface_component_list interface_component
	{
        if ($2)
		    $1->Add($2);
		$$ = $1;
	}
	| interface_component_list import
	{
		$$ = $1;
	}
	| interface_component
	{
        if ($1)
            $$ = new Vector(RUNTIME_CLASS(CFEInterfaceComponent), 1, $1);
        else
            $$ = new Vector(RUNTIME_CLASS(CFEInterfaceComponent));
	}
	| import
	{
		$$ = new Vector(RUNTIME_CLASS(CFEInterfaceComponent));
	}
	;

interface_component :
	  export semicolon
	{
		$$ = $1;
	}
	| op_declarator semicolon
	{
		$$ = $1;
	}
//	| gcc_function
//	{
//	    $$ = NULL;
//	}
//	| gcc_declarator
//	{
//	    $$ = NULL;
//	}
	;

export
	: type_declarator
	{
		$$ = $1;
		if (pCurFileComponent)
		{
			if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFELibrary)))
				((CFELibrary*)pCurFileComponent)->AddTypedef($1);
			else if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFEInterface)))
				((CFEInterface*)pCurFileComponent)->AddTypedef($1);
			else
			{
				TRACE("current file component is unknown type: %s\n", (const char*)pCurFileComponent->GetClassName());
				assert(false);
			}
		}
		else
			CParser::GetCurrentFile()->AddTypedef($1);
	}
	| const_declarator
	{
		$$ = $1;
		if (pCurFileComponent)
		{
			if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFELibrary)))
				((CFELibrary*)pCurFileComponent)->AddConstant($1);
			else if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFEInterface)))
				((CFEInterface*)pCurFileComponent)->AddConstant($1);
			else
			{
				TRACE("current file component is unknown type: %s\n", (const char*)pCurFileComponent->GetClassName());
				assert(false);
			}
		}
		else
			CParser::GetCurrentFile()->AddConstant($1);
	}
	| tagged_declarator
	{
		$$ = $1;
		if (pCurFileComponent)
		{
			if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFELibrary)))
				((CFELibrary*)pCurFileComponent)->AddTaggedDecl($1);
			else if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFEInterface)))
				((CFEInterface*)pCurFileComponent)->AddTaggedDecl($1);
			else
			{
				TRACE("current file component is unknown type: %s\n", (const char*)pCurFileComponent->GetClassName());
				assert(false);
			}
		}
		else
			CParser::GetCurrentFile()->AddTaggedDecl($1);
	}
	| exception_declarator { $$ = $1; }
	;

const_declarator
	: CONST const_type_spec id_or_typename IS const_expr
	{
		CFETypeSpec *pType = $2;
	        while (pType && (pType->GetType() == TYPE_USER_DEFINED))
		{
		    String sTypeName = ((CFEUserDefinedType*)pType)->GetName();
		    CFETypedDeclarator *pTypedef = CParser::GetCurrentFile()->FindUserDefinedType(sTypeName);
		    if (!pTypedef)
		        dceerror2("Cannot find type for \"%s\".", (const char*)sTypeName);
		    pType = pTypedef->GetType();
		}
		if (!( $5->IsOfType($2->GetType()) )) {
			dceerror2("Const type of \"%s\" does not match with expression.", (const char*)(*$3));
			delete $3;
			YYABORT;
		}
		$$ = new CFEConstDeclarator($2, *$3, $5);
		// set parent relationship
		$2->SetParent($$);
		$5->SetParent($$);
		$$->SetSourceLine(gLineNumber);
		delete $3;
	}
	;

const_type_spec
	: integer_type { $$ = $1; }
	| CHAR
	{
		$$ = new CFESimpleType(TYPE_CHAR);
		$$->SetSourceLine(gLineNumber);
	}
	| BOOLEAN
	{
		$$ = new CFESimpleType(TYPE_BOOLEAN);
		$$->SetSourceLine(gLineNumber);
	}
	| VOID_PTR
	{
		$$ = new CFESimpleType(TYPE_VOID_ASTERISK);
		$$->SetSourceLine(gLineNumber);
	}
	| CHAR_PTR
	{
		$$ = new CFESimpleType(TYPE_CHAR_ASTERISK);
		$$->SetSourceLine(gLineNumber);
	}
	;


const_expr_list :
	  const_expr_list COMMA const_expr
	{
	        if ($3)
		        $1->Add($3);
		$$ = $1;
	}
	| const_expr
	{
		$$ = new Vector(RUNTIME_CLASS(CFEExpression), 1, $1);
	}
	;

const_expr
	: conditional_expr  { $$ = $1; }
	| string
	{
		$$ = new CFEExpression(EXPR_STRING, *$1);
		$$->SetSourceLine(gLineNumber);
		delete $1;
	}
	| LIT_CHAR
	{
		$$ = new CFEExpression(EXPR_CHAR, $1);
		$$->SetSourceLine(gLineNumber);
	}
	| EXPNULL
	{
		$$ = new CFEExpression(EXPR_NULL);
		$$->SetSourceLine(gLineNumber);
	}
	| TRUE
	{
		$$ = new CFEExpression(EXPR_TRUE);
		$$->SetSourceLine(gLineNumber);
	}
	| FALSE
	{
		$$ = new CFEExpression(EXPR_FALSE);
		$$->SetSourceLine(gLineNumber);
	}
	/* gcc specifics */
	| unary_expr assignment_operator const_expr
	{ $$ = NULL; }
	;

assignment_operator:
	  IS
	| RS_ASSIGN
	| LS_ASSIGN
	| ADD_ASSIGN
	| SUB_ASSIGN
	| MUL_ASSIGN
	| DIV_ASSIGN
	| MOD_ASSIGN
	| AND_ASSIGN
	| XOR_ASSIGN
	| OR_ASSIGN
	;

conditional_expr
	: logical_or_expr { $$ = $1; }
	/* gcc specifics */
	| logical_or_expr QUESTION const_expr_list COLON conditional_expr
	{
		// because the const_expr_list contains the conditional_expr
		// we have to evaluate the case, this can happend (and be IDL specific)
		// get first of const_expr_list
		CFEExpression *pExpr = ($3->GetFirst() != NULL) ? (CFEExpression*)($3->GetFirst()->GetElement()) : NULL;
		$$ = new CFEConditionalExpression($1, pExpr, $5);
		// set parent relationship
		$1->SetParent($$);
		$5->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| logical_or_expr QUESTION COLON conditional_expr
	{
		$$ = new CFEConditionalExpression($1, NULL, $4);
		// set parent relationship
		$1->SetParent($$);
		$4->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

logical_or_expr
	: logical_and_expr { $$ = $1; }
	| logical_or_expr LOGICALOR logical_and_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LOGOR, $3);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

logical_and_expr
	: inclusive_or_expr { $$ = $1; }
	| logical_and_expr LOGICALAND inclusive_or_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LOGAND, $3);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

inclusive_or_expr
	: exclusive_or_expr { $$ = $1; }
	| inclusive_or_expr BITOR exclusive_or_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITOR, $3);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

exclusive_or_expr
	: and_expr { $$ = $1; }
	| exclusive_or_expr BITXOR and_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITXOR, $3);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

and_expr
	: equality_expr { $$ = $1; }
	| and_expr BITAND equality_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITAND, $3);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

equality_expr
	: relational_expr { $$ = $1; }
	| equality_expr EQUAL relational_expr
	{
		if (($1 != NULL) && ($3 != NULL)) {
			$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_EQUALS, $3);
			// set parent relationship
			$1->SetParent($$);
			$3->SetParent($$);
			$$->SetSourceLine(gLineNumber);
		} else
			$$ = NULL;
	}
	| equality_expr NOTEQUAL relational_expr
	{
		if (($1 != NULL) && ($3 != NULL)) {
			$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_NOTEQUAL, $3);
			// set parent relationship
			$1->SetParent($$);
			$3->SetParent($$);
			$$->SetSourceLine(gLineNumber);
		} else
			$$ = NULL;
	}
	;

relational_expr:
	  shift_expr { $$ = $1; }
	| relational_expr LTEQUAL shift_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LTEQU, $3);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| relational_expr GTEQUAL shift_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_GTEQU, $3);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| relational_expr LT shift_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LT, $3);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| relational_expr GT shift_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_GT, $3);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

shift_expr
	: additive_expr { $$ = $1; }
	| shift_expr LSHIFT additive_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LSHIFT, $3);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| shift_expr RSHIFT additive_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_RSHIFT, $3);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

additive_expr
	: multiplicative_expr { $$ = $1; }
	| additive_expr PLUS multiplicative_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_PLUS, $3);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| additive_expr MINUS multiplicative_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MINUS, $3);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

multiplicative_expr :
	/* unary_expr { $$ = $1; } */
	  cast_expr
	{ $$ = $1; }
	| multiplicative_expr ASTERISK unary_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MUL, $3);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| multiplicative_expr DIV unary_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_DIV, $3);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| multiplicative_expr MOD unary_expr
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MOD, $3);
		// set parent relationship
		$1->SetParent($$);
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

unary_expr :
	  postfix_expr
	{ $$ = $1; }
	| unary_operator cast_expr
	{
		$$ = new CFEUnaryExpression(EXPR_UNARY, $1, $2);
		// set parent relationship
		$2->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	/* gcc specifics */
	| EXTENSION cast_expr
	{ $$ = NULL; }
	| LOGICALAND ID
	{
	    $$ = NULL;
		delete $2;
	}
	| SIZEOF unary_expr
	{
	    $$ = new CFESizeOfExpression($2);
		$2->SetParent($$);
	    $$->SetSourceLine(gLineNumber);
	}
	| SIZEOF LPAREN TYPENAME RPAREN
	{
	    $$ = new CFESizeOfExpression(*$3);
		delete $3;
		$$->SetSourceLine(gLineNumber);
	}
	| SIZEOF LPAREN base_type_spec RPAREN
	{
	    $$ = new CFESizeOfExpression($3);
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| ALIGNOF unary_expr
	{ $$ = NULL; }
	| ALIGNOF LPAREN TYPENAME RPAREN
	{
	    $$ = NULL;
		delete $3;
	}
	| REALPART cast_expr
	{ $$ = NULL; }
	| IMAGPART cast_expr
	{ $$ = NULL; }
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

/* gcc specifics */
postfix_expr :
	  primary_expr
	{ $$ = $1; }
	| LPAREN compound_statement // closing RPAREN is eaten by compund rule
	{ $$ = NULL; }
	| postfix_expr LBRACKET const_expr_list RBRACKET
	{ $$ = NULL; }
	| postfix_expr LPAREN const_expr_list RPAREN
	{ $$ = NULL; }
	| postfix_expr LPAREN RPAREN
	{ $$ = NULL; }
	| postfix_expr DOT ID
	{
	    $$ = NULL;
		delete $3;
	}
	| postfix_expr PTR_OP ID
	{
	    $$ = NULL;
		delete $3;
	}
	| postfix_expr INC_OP
	{ $$ = NULL; }
	| postfix_expr DEC_OP
	{ $$ = NULL; }
	;

primary_expr
	: LIT_INT
	{
		$$ = new CFEPrimaryExpression(EXPR_INT, $1);
		$$->SetSourceLine(gLineNumber);
	}
	| LIT_FLOAT
	{
		$$ = new CFEPrimaryExpression(EXPR_FLOAT, $1);
		$$->SetSourceLine(gLineNumber);
	}
	| VERSION_STR
	{
	    $$ = new CFEPrimaryExpression(EXPR_FLOAT, $1->ToLongDouble());
		$$->SetSourceLine(gLineNumber);
		delete $1;
	}
	| ID
	{
		$$ = new CFEUserDefinedExpression(*$1);
		delete $1;
		$$->SetSourceLine(gLineNumber);
	}
	| LPAREN const_expr_list RPAREN
	{
		// extract first const expression if available
		CFEExpression *pExpr = ($2->GetFirst() != NULL) ? (CFEExpression*)($2->GetFirst()->GetElement()) : NULL;
		if (pExpr != NULL) {
			$$ = new CFEPrimaryExpression(EXPR_PAREN, pExpr);
			pExpr->SetParent($$);
			$$->SetSourceLine(gLineNumber);
		} else
			$$ = NULL;
	}
	;

string :
	  LIT_STR
	{ $$ = $1; }
	| QUOT QUOT
	{ $$ = NULL; }
	/* gcc specifics */
	| string LIT_STR
	{
		if ($1 != NULL)
		{
			$1->Concat(*$2);
			$$ = $1;
			delete $2;
		}
		else
			$$ = $2;
	}
	;

/* gcc specifics */
cast_expr:
	  unary_expr
	{ $$ = $1; }
	| LPAREN TYPENAME RPAREN cast_expr
	{
	    delete $2;
	    $$ = $4;
	}
	| LPAREN TYPENAME RPAREN LBRACE RBRACE
	{
	    delete $2;
	    $$ = NULL;
	}
	| LPAREN TYPENAME RPAREN LBRACE initializer_list RBRACE
	{
	    delete $2;
	    $$ = NULL;
	}
	| LPAREN TYPENAME RPAREN LBRACE initializer_list COMMA RBRACE
	{
	    delete $2;
	    $$ = NULL;
	}
	;

type_declarator
	: TYPEDEF type_attribute_list type_spec declarator_list
	{
		// check if type_names already exist
		CParser *pCurParser = CParser::GetCurrentParser();
		// we can only test the import scope
		CFEFile *pRoot = pCurParser->GetTopFileInScope();
		assert(pRoot);
		VectorElement *pIter;
		CFEDeclarator *pDecl;
		for (pIter = $4->GetFirst(); pIter != NULL; pIter = pIter->GetNext()) {
			pDecl = (CFEDeclarator*)(pIter->GetElement());
			if (pDecl != NULL) {
				if (pDecl->GetName() != NULL) {
					if (pRoot->FindUserDefinedType(pDecl->GetName()) != NULL)
					{
						dceerror2("\"%s\" has already been defined as type.",(const char*)pDecl->GetName());
						YYABORT;
					}
				}
			}
		}
		$$ = new CFETypedDeclarator(TYPEDECL_TYPEDEF, $3, $4, $2);
		$2->SetParentOfElements($$);
		$3->SetParent($$);
		$4->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	| TYPEDEF type_spec declarator_list
	{
		// check if type_names already exist
		CParser *pCurParser = CParser::GetCurrentParser();
		// we can only test the import scope
		CFEFile *pRoot = pCurParser->GetTopFileInScope();
		assert(pRoot);
		VectorElement *pIter;
		CFEDeclarator *pDecl;
		for (pIter = $3->GetFirst(); pIter != NULL; pIter = pIter->GetNext()) {
			pDecl = (CFEDeclarator*)(pIter->GetElement());
			if (pDecl != NULL) {
				if (pDecl->GetName() != NULL) {
					if (pRoot->FindUserDefinedType(pDecl->GetName()) != NULL)
					{
						dceerror2("\"%s\" has already been defined as type.",(const char*)pDecl->GetName());
						YYABORT;
					}
				}
			}
		}
		$$ = new CFETypedDeclarator(TYPEDECL_TYPEDEF, $2, $3);
		// set parent relationship
		$2->SetParent($$);
		$3->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	/* gcc specifics */
	| TYPEDEF type_qualifier_list type_spec declarator_list
	{
		// check if type_names already exist
		CParser *pCurParser = CParser::GetCurrentParser();
		// we can only test the import scope
		CFEFile *pRoot = pCurParser->GetTopFileInScope();
		assert(pRoot);
		VectorElement *pIter;
		CFEDeclarator *pDecl;
		for (pIter = $4->GetFirst(); pIter != NULL; pIter = pIter->GetNext()) {
			pDecl = (CFEDeclarator*)(pIter->GetElement());
			if (pDecl != NULL) {
				if (pDecl->GetName() != NULL) {
					if (pRoot->FindUserDefinedType(pDecl->GetName()) != NULL)
					{
						dceerror2("\"%s\" has already been defined as type.",(const char*)pDecl->GetName());
						YYABORT;
					}
				}
			}
		}
		$$ = new CFETypedDeclarator(TYPEDECL_TYPEDEF, $3, $4);
		// set parent relationship
		$3->SetParent($$);
		$4->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

type_attribute_list :
	  { if (c_inc == 0) c_inc = 2; } LBRACKET type_attributes { if (c_inc == 2) c_inc = 0; } rbracket { $$ = $3; }
	;

type_spec
	: simple_type_spec { $$ = $1; }
	| constructed_type_spec { $$ = $1; }
	;

simple_type_spec
	: base_type_spec { $$ = $1; }
	| predefined_type_spec { $$ = $1; }
	| TYPENAME
	{
		$$ = new CFEUserDefinedType(*$1);
		delete $1;
		$$->SetSourceLine(gLineNumber);
	}
    | STRING
    {
        dceerror2("\"string\" is not a valid data type. Please use \'[string] char*\' instead.");
        YYABORT;
    }
    | error
    {
        dceerror2("No valid type specified."); // we cannot evaluate the 'error' token
        YYABORT;
    }
	;

declarator_list :
	  declarator_list COMMA declarator
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

declarator
	: pointer attribute_list direct_declarator
	{
		$3->SetStars($1);
		$$ = $3;
	}
	| pointer attribute_list direct_declarator COLON const_expr
	{
		$3->SetStars($1);
		if ($5)
		{
			$3->SetBitfields($5->GetIntValue());
			delete $5;
		}
		$$ = $3;
	}
	| attribute_list direct_declarator { $$ = $2; }
	| attribute_list direct_declarator COLON const_expr
	{
		if ($4)
		{
			$2->SetBitfields($4->GetIntValue());
			delete $4;
		}
		$$ = $2;
	}
	;

direct_declarator
	: ID
	{
		$$ = new CFEDeclarator(DECL_IDENTIFIER, *$1);
		delete $1;
		$$->SetSourceLine(gLineNumber);
	}
	| LPAREN declarator rparen
	{ $$ = $2; }
	| array_declarator
	{ $$ = $1; }
	| function_declarator
	{ $$ = $1; }
	;

tagged_declarator
	: tagged_struct_declarator { $$ = $1; }
	| tagged_union_declarator { $$ = $1; }
    | tagged_enumeration_type { $$ = $1; }
	;

base_type_spec
	: floating_pt_type { $$ = $1; }
	| integer_type { $$ = $1; }
	| char_type { $$ = $1; }
	| boolean_type { $$ = $1; }
	| BYTE
	{
		$$ = new CFESimpleType(TYPE_BYTE);
		$$->SetSourceLine(gLineNumber);
	}
	| VOID
	{
		$$ = new CFESimpleType(TYPE_VOID);
		$$->SetSourceLine(gLineNumber);
	}
	| HANDLE_T
	{
		$$ = new CFESimpleType(TYPE_HANDLE_T);
		$$->SetSourceLine(gLineNumber);
	}
	/* GCC specifics */
	| COMPLEX
	{
		$$ = new CFESimpleType(TYPE_GCC);
		$$->SetSourceLine(gLineNumber);
	}
	| TYPEOF LPAREN const_expr_list RPAREN
	{
		if ($3 != NULL)
			delete $3;
		$$ = new CFESimpleType(TYPE_GCC);
		$$->SetSourceLine(gLineNumber);
	}
	| VOID_PTR
	{
		$$ = new CFESimpleType(TYPE_VOID_ASTERISK);
		$$->SetSourceLine(gLineNumber);
	}
	| CHAR_PTR
	{
		$$ = new CFESimpleType(TYPE_CHAR_ASTERISK);
		$$->SetSourceLine(gLineNumber);
	}
	;

floating_pt_type
	: FLOAT
	{
		$$ = new CFESimpleType(TYPE_FLOAT);
		$$->SetSourceLine(gLineNumber);
	}
	| DOUBLE
	{
		$$ = new CFESimpleType(TYPE_DOUBLE);
		$$->SetSourceLine(gLineNumber);
	}
    | LONG DOUBLE
    {
        $$ = new CFESimpleType(TYPE_LONG_DOUBLE);
        $$->SetSourceLine(gLineNumber);
    }
	;

integer_type :
	  SIGNED INT
	{
		$$ = new CFESimpleType(TYPE_INTEGER, false, false);
		$$->SetSourceLine(gLineNumber);
	}
	| SIGNED integer_size
	{
	    if ($2 == 4) // it was LONG
			$$ = new CFESimpleType(TYPE_LONG, false, false, $2, false);
	    else
		    $$ = new CFESimpleType(TYPE_INTEGER, false, false, $2, false);
		$$->SetSourceLine(gLineNumber);
	}
	| SIGNED integer_size INT
	{
	    if ($2 == 4) // It was LONG
			$$ = new CFESimpleType(TYPE_LONG, false, false, $2);
	    else
		    $$ = new CFESimpleType(TYPE_INTEGER, false, false, $2);
		$$->SetSourceLine(gLineNumber);
	}
	| UNSIGNED INT
	{
		$$ = new CFESimpleType(TYPE_INTEGER, true, false);
		$$->SetSourceLine(gLineNumber);
	}
	| UNSIGNED integer_size
	{
	    if ($2 == 4) // it was long
			$$ = new CFESimpleType(TYPE_LONG, true, false, $2, false);
	    else
		    $$ = new CFESimpleType(TYPE_INTEGER, true, false, $2, false);
		$$->SetSourceLine(gLineNumber);
	}
	| UNSIGNED integer_size INT
	{
	    if ($2 == 4) // it was LONG
			$$ = new CFESimpleType(TYPE_LONG, true, false, $2);
	    else
		    $$ = new CFESimpleType(TYPE_INTEGER, true, false, $2);
		$$->SetSourceLine(gLineNumber);
	}
	| INT
	{
		$$ = new CFESimpleType(TYPE_INTEGER, false, false);
		$$->SetSourceLine(gLineNumber);
	}
	| integer_size INT
	{
	    if ($1 == 4) // it was LONG
			$$ = new CFESimpleType(TYPE_LONG, false, false, $1);
	    else
		    $$ = new CFESimpleType(TYPE_INTEGER, false, false, $1);
		$$->SetSourceLine(gLineNumber);
	}
	| integer_size
	{
	    if ($1 == 4) // it was a long
			$$ = new CFESimpleType(TYPE_LONG, false, false, $1, false);
	    else
			$$ = new CFESimpleType(TYPE_INTEGER, false, false, $1, false);
		$$->SetSourceLine(gLineNumber);
	}
	| SIGNED
	{
		$$ = new CFESimpleType(TYPE_INTEGER, false);
		$$->SetSourceLine(gLineNumber);
	}
	| UNSIGNED
	{
		$$ = new CFESimpleType(TYPE_INTEGER, true);
		$$->SetSourceLine(gLineNumber);
	}
	;

integer_size
	: LONG { $$ = 4; }
	| LONGLONG { $$ = 8; }
	| SHORT { $$ = 2; }
	| SMALL { $$ = 1; }
	| HYPER { $$ = 8; }
	;

char_type
	: UNSIGNED CHAR
	{
		$$ = new CFESimpleType(TYPE_CHAR, true);
		$$->SetSourceLine(gLineNumber);
	}
	| UNSIGNED CHAR_PTR
	{
	    $$ = new CFESimpleType(TYPE_CHAR_ASTERISK, true);
		$$->SetSourceLine(gLineNumber);
	}
	| CHAR
	{
		$$ = new CFESimpleType(TYPE_CHAR);
		$$->SetSourceLine(gLineNumber);
	}
	| SIGNED CHAR
	{
		$$ = new CFESimpleType(TYPE_CHAR);
		$$->SetSourceLine(gLineNumber);
	}
	| SIGNED CHAR_PTR
	{
	    $$ = new CFESimpleType(TYPE_CHAR_ASTERISK);
		$$->SetSourceLine(gLineNumber);
	}
	;

boolean_type
	: BOOLEAN
	{
		$$ = new CFESimpleType(TYPE_BOOLEAN);
		$$->SetSourceLine(gLineNumber);
	}
	;

constructed_type_spec
	: attribute_list STRUCT LBRACE member_list RBRACE
	{
        // the attribute list is GCC specific
		$$ = new CFEStructType($4);
		$4->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	| union_type { $$ = $1; }
	| enumeration_type { $$ = $1; }
	| tagged_declarator { $$ = $1; }
	| PIPE type_spec
	{
		$$ = new CFEPipeType($2);
		// set parent relationship
		$2->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

tagged_struct_declarator
	: attribute_list STRUCT tag LBRACE member_list RBRACE
	{
		$$ = new CFETaggedStructType(*$3, $5);
		$5->SetParentOfElements($$);
		delete $3;
		$$->SetSourceLine(gLineNumber);
	}
	| attribute_list STRUCT tag
	{
		$$ = new CFETaggedStructType(*$3);
		delete $3;
		$$->SetSourceLine(gLineNumber);
	}
	;

tag
	: id_or_typename { $$ = $1; }
	;

member_list :
	  /* empty */
	{ $$ = 0; }
	| member_list_1
	{ $$ = $1; }
	| member_list_1 member /* no semicolon here; if semi: member SEMICOLON -> member_list_1 */
	{
	        if ($2)
		        $1->Add($2);
		$$ = $1;
	}
	;

member_list_1 :
	  member_list_1 member SEMICOLON
	{
	        if ($2)
		        $1->Add($2);
		$$ = $1;
	}
	| member SEMICOLON
	{
		$$ = new Vector(RUNTIME_CLASS(CFETypedDeclarator), 1, $1);
	}
	;

member
	: field_declarator { $$ = $1; }
	;

field_declarator
	: field_attribute_list type_spec declarator_list
	{
		$$ = new CFETypedDeclarator(TYPEDECL_FIELD, $2, $3, $1);
		$1->SetParentOfElements($$);
		$2->SetParent($$);
		$3->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	| type_spec declarator_list
	{
		$$ = new CFETypedDeclarator(TYPEDECL_FIELD, $1, $2);
		// set parent relationship
		$1->SetParent($$);
		$2->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	/* gcc specific rule */
	| type_qualifier_list type_spec declarator_list
	{
		$$ = new CFETypedDeclarator(TYPEDECL_FIELD, $2, $3);
		// set parent relationship
		$2->SetParent($$);
		$3->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

field_attribute_list :
	  { if (c_inc == 0) c_inc = 2; } LBRACKET field_attributes { if (c_inc == 2) c_inc = 0; } rbracket { $$ = $3; }
	;

tagged_union_declarator
	: UNION tag
	{
		$$ = new CFETaggedUnionType(*$2);
		delete $2;
		$$->SetSourceLine(gLineNumber);
	}
	| UNION tag union_type_header
	{
		$$ = new CFETaggedUnionType(*$2, $3);
		// set parent relationship
		$3->SetParent($$);
		delete $2;
		$$->SetSourceLine(gLineNumber);
	}
	;

union_type
	: UNION union_type_header { $$ = $2; }
	;

union_type_header
	: SWITCH LPAREN switch_type_spec id_or_typename rparen id_or_typename LBRACE union_body RBRACE
	{
		$$ = new CFEUnionType($3, *$4, $8, *$6);
		// set parent relationship
		$3->SetParent($$);
		$8->SetParentOfElements($$);
		delete $4;
		delete $6;
		$$->SetSourceLine(gLineNumber);
	}
	| SWITCH LPAREN switch_type_spec id_or_typename rparen LBRACE union_body RBRACE
	{
		$$ = new CFEUnionType($3, *$4, $7);
		// set parent relationship
		$3->SetParent($$);
		$7->SetParentOfElements($$);
		delete $4;
		$$->SetSourceLine(gLineNumber);
	}
	| LBRACE union_body_n_e RBRACE
	{
		$$ = new CFEUnionType($2);
		$2->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	;


switch_type_spec
	: integer_type { $$ = $1; }
	| char_type { $$ = $1; }
	| boolean_type { $$ = $1; }
	| ID
	{
		$$ = new CFEUserDefinedType(*$1);
		delete $1;
		$$->SetSourceLine(gLineNumber);
	}
	| enumeration_type { $$ = $1; }
    | tagged_enumeration_type { $$ = $1; }
	;

union_body :
	  union_body union_case
	{
	        if ($2)
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
	        if ($2)
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
		$$->SetSourceLine(gLineNumber);
	}
	| DEFAULT COLON union_arm
	{
		$$ = new CFEUnionCase($3);
		// set parent relationship
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

union_case_n_e
	: LBRACKET CASE LPAREN const_expr_list rparen rbracket union_arm
	{
		$$ = new CFEUnionCase($7, $4);
		// set parent relationship
		$7->SetParent($$);
		$4->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	| LBRACKET DEFAULT rbracket union_arm
	{
		$$ = new CFEUnionCase($4);
		// set parent relationship
		$4->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| union_arm
	{
		// this one is to support C/C++ unions
		$$ = new CFEUnionCase($1);
		$1->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;


union_case_label_list :
	  union_case_label_list union_case_label
	{
	        if ($2)
		        $1->Add($2);
		$$ = $1;
	}
	| union_case_label
	{
		$$ = new Vector(RUNTIME_CLASS(CFEExpression), 1, $1);
	}
	;

union_case_label
	: CASE const_expr colon { $$ = $2; }
	;

union_arm
	: field_declarator semicolon { $$ = $1; }
	| SEMICOLON
	{
		$$ = new CFETypedDeclarator(TYPEDECL_VOID, 0, 0);
		$$->SetSourceLine(gLineNumber);
	}
	;

union_type_switch_attr
	: SWITCH_TYPE LPAREN switch_type_spec rparen
	{
		$$ = new CFETypeAttribute(ATTR_SWITCH_TYPE, $3);
		// set parent relationship
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

union_instance_switch_attr
	: SWITCH_IS LPAREN attr_var rparen
	{
		$$ = new CFEIsAttribute(ATTR_SWITCH_IS, new Vector(RUNTIME_CLASS(CFEDeclarator), 1, $3)); 
		// set parent relationship
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

identifier_list :
	  identifier_list COMMA scoped_name
	{
		CFEIdentifier *tmp = new CFEIdentifier(*$3);
		tmp->SetSourceLine(gLineNumber);
		$1->Add(tmp);
		$$ = $1;
		delete $3;
	}
	| scoped_name
	{
		CFEIdentifier *tmp = new CFEIdentifier(*$1);
		tmp->SetSourceLine(gLineNumber);
		$$ = new Vector(RUNTIME_CLASS(CFEIdentifier), 1, tmp);
		delete $1;
	}
	;

scoped_name :
      scoped_name SCOPE ID
    {
        $1->Concat("::");
        $1->Concat(*$3);
        $$ = $1;
        delete $3;
    }
    | SCOPE ID
    {
        $$ = $2;
    }
    | ID
    {
        $$ = $1;
    }
    ;


enumeration_type :
	  ENUM LBRACE enumeration_declarator_list RBRACE
	{
		$$ = new CFEEnumType($3);
		$3->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
    ;

tagged_enumeration_type :
      ENUM id_or_typename LBRACE enumeration_declarator_list RBRACE
	{
	    $$ = new CFETaggedEnumType(*$2, $4);
	    delete $2;
	    $4->SetParentOfElements($$);
	    $$->SetSourceLine(gLineNumber);
	}
    | ENUM id_or_typename
    {
        $$ = new CFETaggedEnumType(*$2, NULL);
        delete $2;
        $$->SetSourceLine(gLineNumber);
    }
	;

enumeration_declarator_list :
	  enumeration_declarator
	{
		$$ = new Vector(RUNTIME_CLASS(CFEIdentifier), 1, $1);
	}
	| enumeration_declarator_list COMMA enumeration_declarator
	{
	        if ($3)
		        $1->Add($3);
		$$ = $1;
	}
	;

enumeration_declarator :
	  id_or_typename
	{
		$$ = new CFEEnumDeclarator(*$1);
		delete $1;
		$$->SetSourceLine(gLineNumber);
	}
	| id_or_typename IS const_expr
	{
		$$ = new CFEEnumDeclarator(*$1, $3);
		delete $1;
		$$->SetSourceLine(gLineNumber);
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
		$$->AddBounds(0, 0);
		$$->SetSourceLine(gLineNumber);
	}
	| direct_declarator LBRACKET array_bound rbracket
	{
		if ($1->GetType() == DECL_ARRAY)
			$$ = (CFEArrayDeclarator*)$1;
		else {
			$$ = new CFEArrayDeclarator($1);
			delete $1;
		}
		$$->AddBounds(0, $3);
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| direct_declarator LBRACKET array_bound DOTDOT rbracket
	{
		// "decl [ expr '..' ]" == "decl [ expr '..' ASTERISK ]"
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
		$$->SetSourceLine(gLineNumber);
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
		$$->SetSourceLine(gLineNumber);
	}
	;

array_bound
	: ASTERISK
	{
		$$ = new CFEExpression(EXPR_CHAR, '*');
		$$->SetSourceLine(gLineNumber);
	}
	| const_expr { $$ = $1; }
	;

type_attributes
	: type_attributes COMMA type_attribute
	{
	        if ($3)
		        $1->Add($3);
		$$ = $1;
	}
	| type_attribute
	{
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, $1);
	}
    | error
    {
		dceerror2("unknown attribute");
		YYABORT;
	}
	;

type_attribute
	: TRANSMIT_AS LPAREN simple_type_spec RPAREN
	{
		$$ = new CFETypeAttribute(ATTR_TRANSMIT_AS, $3);
		// set parent relationship
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| HANDLE
	{
		$$ = new CFEAttribute(ATTR_HANDLE);
		$$->SetSourceLine(gLineNumber);
	}
	| usage_attribute { $$ = $1; }
	| union_type_switch_attr { $$ = $1; }
	| ptr_attr { $$ = $1; }
	;

usage_attribute
	: STRING
	{
		$$ = new CFEAttribute(ATTR_STRING);
		$$->SetSourceLine(gLineNumber);
	}
	| CONTEXT_HANDLE
	{
		$$ = new CFEAttribute(ATTR_CONTEXT_HANDLE);
		$$->SetSourceLine(gLineNumber);
	}
	;

field_attributes :
	  field_attributes COMMA field_attribute 
	{ 
		if ($3)
			$1->Add($3);
		$$ = $1; 
	}
	| field_attribute 
	{ 
		if ($1 != 0)
			$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, $1);
		else
			$$ = new Vector(RUNTIME_CLASS(CFEAttribute));
	}
	;

field_attribute 
	: FIRST_IS LPAREN attr_var_list RPAREN 
	{ 
		$$ = new CFEIsAttribute(ATTR_FIRST_IS, $3); 
		$3->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	| FIRST_IS LPAREN LIT_INT RPAREN
	{
		$$ = new CFEIntAttribute(ATTR_FIRST_IS, $3);
		$$->SetSourceLine(gLineNumber);
	}
	| FIRST_IS LPAREN error RPAREN
	{
		dcewarning("wrong [first_is()] format");
		yyclearin;
		yyerrok;
		$$ = 0;
	}
	| LAST_IS LPAREN attr_var_list RPAREN 
	{ 
		$$ = new CFEIsAttribute(ATTR_LAST_IS, $3); 
		$3->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	| LAST_IS LPAREN LIT_INT RPAREN
	{
		$$ = new CFEIntAttribute(ATTR_LAST_IS, $3);
		$$->SetSourceLine(gLineNumber);
	}
	| LAST_IS LPAREN error RPAREN
	{
		dcewarning("wrong [last_is()] format");
		yyclearin;
		yyerrok;
		$$ = 0;
	}
	| LENGTH_IS LPAREN attr_var_list RPAREN
	{
		$$ = new CFEIsAttribute(ATTR_LENGTH_IS, $3);
		$3->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	| LENGTH_IS LPAREN LIT_INT RPAREN
	{
		$$ = new CFEIntAttribute(ATTR_LENGTH_IS, $3);
		$$->SetSourceLine(gLineNumber);
	}
	| LENGTH_IS LPAREN error RPAREN
	{
		dcewarning("wrong [length_is()] format");
		yyclearin;
		yyerrok;
		$$ = 0;
	}
	| MIN_IS LPAREN attr_var_list RPAREN
	{
		$$ = new CFEIsAttribute(ATTR_MIN_IS, $3);
		$3->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	| MIN_IS LPAREN LIT_INT RPAREN
	{
		$$ = new CFEIntAttribute(ATTR_MIN_IS, $3);
		$$->SetSourceLine(gLineNumber);
	}
	| MIN_IS LPAREN error RPAREN
	{
		dcewarning("wrong [min_is()] format");
		yyclearin;
		yyerrok;
		$$ = 0;
	}
	| MAX_IS LPAREN attr_var_list RPAREN
	{
		$$ = new CFEIsAttribute(ATTR_MAX_IS, $3);
		$3->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	| MAX_IS LPAREN LIT_INT RPAREN
	{
		$$ = new CFEIntAttribute(ATTR_MAX_IS, $3);
		$$->SetSourceLine(gLineNumber);
	}
	| MAX_IS LPAREN error RPAREN
	{
		dcewarning("wrong [max_is()] format");
		yyclearin;
		yyerrok;
		$$ = 0;
	}
	| SIZE_IS LPAREN attr_var_list RPAREN
	{
		$$ = new CFEIsAttribute(ATTR_SIZE_IS, $3);
		$3->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	| SIZE_IS LPAREN LIT_INT RPAREN
	{
		$$ = new CFEIntAttribute(ATTR_SIZE_IS, $3);
		$$->SetSourceLine(gLineNumber);
	}
	| SIZE_IS LPAREN error RPAREN
	{
		dcewarning("wrong [size_is()] format");
		yyclearin;
		yyerrok;
		$$ = 0;
	}
	| usage_attribute { $$ = $1; }
	| union_instance_switch_attr { $$ = $1; }
	| IGNORE
	{
		$$ = new CFEAttribute(ATTR_IGNORE);
		$$->SetSourceLine(gLineNumber);
	}
	| ptr_attr { $$ = $1; }
	;


attr_var_list :
	  attr_var_list COMMA attr_var
	{
	        if ($3)
		        $1->Add($3);
		$$ = $1;
	}
	| attr_var
	{
		$$ = new Vector(RUNTIME_CLASS(CFEDeclarator), 1, $1);
	}
	;

attr_var
	: ASTERISK id_or_typename
	{
		$$ = new CFEDeclarator(DECL_ATTR_VAR, *$2, 1);
		delete $2;
		$$->SetSourceLine(gLineNumber);
	}
	| id_or_typename
	{
		$$ = new CFEDeclarator(DECL_ATTR_VAR, *$1);
		delete $1;
		$$->SetSourceLine(gLineNumber);
	}
	|
	{
		$$ = new CFEDeclarator(DECL_VOID);
		$$->SetSourceLine(gLineNumber);
	}
	;

ptr_attr
	: REF
	{
		$$ = new CFEAttribute(ATTR_REF);
		$$->SetSourceLine(gLineNumber);
	}
	| UNIQUE
	{
		$$ = new CFEAttribute(ATTR_UNIQUE);
		$$->SetSourceLine(gLineNumber);
	}
	| PTR
	{
		$$ = new CFEAttribute(ATTR_PTR);
		$$->SetSourceLine(gLineNumber);
	}
	| IID_IS LPAREN attr_var_list rparen
	{
		$$ = new CFEIsAttribute(ATTR_IID_IS, $3);
		$3->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

pointer :
	  ASTERISK pointer
	{ $$ = $2 + 1; }
	| ASTERISK
	{ $$ = 1; }
	/* GCC specifics (ignore type qualifier) */
	| ASTERISK type_qualifier_list
	{ $$ = 1; }
	| ASTERISK type_qualifier_list pointer
	{ $$ = $3 + 1; }
	;

op_declarator
	: operation_attributes simple_type_spec id_or_typename LPAREN param_declarators RPAREN raises_declarator
	{
		$$ = new CFEOperation($2, *$3, $5, $1, $7);
		// set parent relationship
		$1->SetParentOfElements($$);
		$2->SetParent($$);
		if ($5 != 0) {
			$5->SetParentOfElements($$);
		}
		$7->SetParentOfElements($$);
		delete $3;
		$$->SetSourceLine(gLineNumber);
		if (pCurFileComponent)
		{
			if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFEInterface)))
				((CFEInterface*)pCurFileComponent)->AddOperation($$);
			else
			{
				TRACE("current file component is unknown type: %s\n", (const char*)pCurFileComponent->GetClassName());
				assert(false);
			}
		}
		else
		{
			TRACE("Cannot add operation to file without interface\n");
			YYABORT;
		}
	}
	| operation_attributes simple_type_spec id_or_typename LPAREN param_declarators RPAREN
	{
		$$ = new CFEOperation($2, *$3, $5, $1);
		// set parent relationship
		$1->SetParentOfElements($$);
		$2->SetParent($$);
		if ($5 != 0) {
			$5->SetParentOfElements($$);
		}
		delete $3;
		$$->SetSourceLine(gLineNumber);
		if (pCurFileComponent)
		{
			if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFEInterface)))
				((CFEInterface*)pCurFileComponent)->AddOperation($$);
			else
			{
				TRACE("current file component is unknown type: %s\n", (const char*)pCurFileComponent->GetClassName());
				assert(false);
			}
		}
		else
		{
			TRACE("Cannot add operation to file without interface\n");
			YYABORT;
		}
	}
	| simple_type_spec id_or_typename LPAREN param_declarators RPAREN raises_declarator
	{
		$$ = new CFEOperation($1, *$2, $4, 0, $6);
		// set parent relationship
		$1->SetParent($$);
		if ($4 != 0) {
			$4->SetParentOfElements($$);
		}
		$6->SetParentOfElements($$);
		delete $2;
		$$->SetSourceLine(gLineNumber);
		if (pCurFileComponent)
		{
			if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFEInterface)))
				((CFEInterface*)pCurFileComponent)->AddOperation($$);
			else
			{
				TRACE("current file component is unknown type: %s\n", (const char*)pCurFileComponent->GetClassName());
				assert(false);
			}
		}
		else
		{
			TRACE("Cannot add operation to file without interface\n");
			YYABORT;
		}
	}
	| simple_type_spec id_or_typename LPAREN param_declarators RPAREN
	{
		$$ = new CFEOperation($1, *$2, $4);
		// set parent relationship
		$1->SetParent($$);
		if ($4 != 0) {
			$4->SetParentOfElements($$);
		}
		delete $2;
		$$->SetSourceLine(gLineNumber);
		if (pCurFileComponent)
		{
			if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFEInterface)))
				((CFEInterface*)pCurFileComponent)->AddOperation($$);
			else
			{
				TRACE("current file component is unknown type: %s\n", (const char*)pCurFileComponent->GetClassName());
				assert(false);
			}
		}
		else
		{
			TRACE("Cannot add operation to file without interface\n");
			YYABORT;
		}
	}
	;

id_or_typename:
      ID
    {
	    $$ = $1;
    }
	| TYPENAME
	{
	    $$ = $1;
	}
	;

raises_declarator
	: RAISES LPAREN identifier_list rparen { $$ = $3; }
	;

operation_attributes
	: { if (c_inc == 0) c_inc = 2; } LBRACKET operation_attribute_list { if (c_inc == 2) c_inc = 0; } rbracket { $$ = $3; }
	;

operation_attribute_list :
	  operation_attribute_list COMMA operation_attribute
	{
	        if ($3)
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
	{
		$$ = new CFEAttribute(ATTR_IDEMPOTENT);
		$$->SetSourceLine(gLineNumber);
	}
	| BROADCAST
	{
		$$ = new CFEAttribute(ATTR_BROADCAST);
		$$->SetSourceLine(gLineNumber);
	}
	| MAYBE
	{
		$$ = new CFEAttribute(ATTR_MAYBE);
		$$->SetSourceLine(gLineNumber);
	}
	| REFLECT_DELETIONS
	{
		$$ = new CFEAttribute(ATTR_REFLECT_DELETIONS);
		$$->SetSourceLine(gLineNumber);
	}
    | UUID LPAREN LIT_INT RPAREN
	{
		$$ = new CFEIntAttribute(ATTR_UUID, $3);
		$$->SetSourceLine(gLineNumber);
	}
	| usage_attribute
	{ $$ = $1; }
	| ptr_attr
	{ $$ = $1; }
	| directional_attribute
	{ $$ = $1; }
    | ONEWAY
    {
        $$ = new CFEAttribute(ATTR_IN);
        $$->SetSourceLine(gLineNumber);
    }
	| ALLOW_REPLY_ONLY
	{
	    $$ = new CFEAttribute(ATTR_ALLOW_REPLY_ONLY);
		$$->SetSourceLine(gLineNumber);
	}
	;

param_declarators :
	  /* empty */
	{ $$ = 0; }
	| VOID RPAREN
	{
		// conflicts with declaration_specififers -> type_spec -> VOID
		yychar = RPAREN;
		$$ = 0;
	}
	| param_declarator_list
	{ $$ = $1; }
	;

param_declarator_list :
	  param_declarator
	{
		if ($1)
			$$ = new Vector(RUNTIME_CLASS(CFETypedDeclarator), 1, $1);
		else
		{
			dcewarning("invalid parameter (probably C style parameter)");
			$$ = new Vector(RUNTIME_CLASS(CFETypedDeclarator));
		}
	}
	| param_declarator_list COMMA param_declarator
	{
		if ($3)
			$1->Add($3);
		else
			dcewarning("invalid parameter (probably C style parameter)");
		$$ = $1;
	}
	;

param_declarator :
	  param_attributes declaration_specifiers declarator
	{
		if ($2 != 0)
		{
			$$ = new CFETypedDeclarator(TYPEDECL_PARAM, $2, new Vector(RUNTIME_CLASS(CFEDeclarator), 1, $3), $1);
			$1->SetParentOfElements($$);
			$2->SetParent($$);
			$3->SetParent($$);
			$$->SetSourceLine(gLineNumber);
		}
		else
			$$ = 0;
	}
	| declaration_specifiers declarator
	{
		if ($1 != 0)
		{
			CFEAttribute *tmpA = new CFEAttribute(ATTR_NONE);
			tmpA->SetSourceLine(gLineNumber);
			Vector *tmp = new Vector(RUNTIME_CLASS(CFEAttribute), 1, tmpA);
			$$ = new CFETypedDeclarator(TYPEDECL_PARAM, $1, new Vector(RUNTIME_CLASS(CFEDeclarator), 1, $2), tmp);
			tmp->SetParentOfElements($$);
			$1->SetParent($$);
			$2->SetParent($$);
			$$->SetSourceLine(gLineNumber);
		}
		else
			$$ = 0;
	}
	/* GCC specifics */
	| param_attributes declaration_specifiers
	{ $$ = 0; }
	| param_attributes declaration_specifiers abstract_declarator
	{ $$ = 0; }
	| declaration_specifiers
	{ $$ = 0; }
	| declaration_specifiers abstract_declarator
	{ $$ = 0; }
	;

param_attributes :
	  { if (c_inc == 0) c_inc = 2; } LBRACKET param_attribute_list { if (c_inc == 2) c_inc = 0; } rbracket { $$ = $3; }
	| { if (c_inc == 0) c_inc = 2; } LBRACKET { if (c_inc == 2) c_inc = 0; } RBRACKET
	{
		CFEAttribute *tmp = new CFEAttribute(ATTR_NONE);
		tmp->SetSourceLine(gLineNumber);
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, tmp);
	}
	;

param_attribute_list
	: param_attribute_list COMMA param_attribute
	{
		if ($3)
			$1->Add($3);
		$$ = $1;
	}
	| param_attribute_list COMMA INOUT
	{
	    // have to place it here, because we have to return
		// a vector of two attributes
		dcewarning("Usage of [inout] is discouraged. Use [in, out] instead.");
		// IN attribute
		CFEAttribute *pAttr =  new CFEAttribute(ATTR_IN);
		pAttr->SetSourceLine(gLineNumber);
		$1->Add(pAttr);
		// OUT attribute
		pAttr = new CFEAttribute(ATTR_OUT);
		pAttr->SetSourceLine(gLineNumber);
		$1->Add(pAttr);
        // return vector
		$$ = $1;
	}
	| param_attribute
	{
		if ($1 == 0)
			$$ = new Vector(RUNTIME_CLASS(CFEAttribute));
		else
			$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, $1);
	}
	| INOUT
	{
	    // have to place it here, because we have to return
		// a vector of two attributes
		dcewarning("Usage of [inout] is discouraged. Use [in, out] instead.");
		// IN attribute
		CFEAttribute *pAttr =  new CFEAttribute(ATTR_IN);
		pAttr->SetSourceLine(gLineNumber);
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, pAttr);
		// OUT attribute
		pAttr = new CFEAttribute(ATTR_OUT);
		pAttr->SetSourceLine(gLineNumber);
		$$->Add(pAttr);
	}
    | error
    {
		dceerror2("unknown attribute");
		YYABORT;
	}
	;

param_attribute
	: directional_attribute { $$ = $1; }
	| field_attribute { $$ = $1; }
	| INIT_WITH_IN
	{
	    $$ = new CFEAttribute(ATTR_PREALLOC);
		$$->SetSourceLine(gLineNumber);
	}
	| PREALLOC
	{
	    // is the IDL4 version of "init_with_in"
	    $$ = new CFEAttribute(ATTR_PREALLOC);
		$$->SetSourceLine(gLineNumber);
	}
	| TRANSMIT_AS LPAREN simple_type_spec RPAREN
	{
		$$ = new CFETypeAttribute(ATTR_TRANSMIT_AS, $3);
		// set parent relationship
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

directional_attribute
	: IN
	{
		$$ = new CFEAttribute(ATTR_IN);
		$$->SetSourceLine(gLineNumber);
	}
	| OUT
	{
		$$ = new CFEAttribute(ATTR_OUT);
		$$->SetSourceLine(gLineNumber);
	}
	;

function_declarator
	: direct_declarator LPAREN param_declarators RPAREN
	{
		$$ = new CFEFunctionDeclarator($1, $3);
		// set parent relationship
		$1->SetParent($$);
		if ($3 != 0) {
			$3->SetParentOfElements($$);
		}
		$$->SetSourceLine(gLineNumber);
	}
	/* GCC specific rules */
	| direct_declarator LPAREN identifier_list rparen
	{
		if ($3 != 0) {
			// identifiers are GCC specific and are usually only used
			// when calling a function, so DUMP IT
			CObject *pEl;
			while ((pEl = $3->RemoveFirst()) != 0)
				delete pEl;
			delete $3;
		}
		if ($1 != 0)
			delete $1;
		$$ = 0;
	}
	| direct_declarator LPAREN param_declarators COMMA ELLIPSIS RPAREN
	{
		$$ = new CFEFunctionDeclarator($1, $3);
		// set parent relationship
		$1->SetParent($$);
		if ($3 != 0) {
			$3->SetParentOfElements($$);
		}
		$$->SetSourceLine(gLineNumber);
	}
	;

predefined_type_spec
	: ERROR_STATUS_T
	{
		$$ = new CFESimpleType(TYPE_ERROR_STATUS_T);
		$$->SetSourceLine(gLineNumber);
	}
	| FLEXPAGE
	{
		$$ = new CFESimpleType(TYPE_FLEXPAGE);
		$$->SetSourceLine(gLineNumber);
	}
	| ISO_LATIN_1
	{
		$$ = new CFESimpleType(TYPE_ISO_LATIN_1);
		$$->SetSourceLine(gLineNumber);
	}	 
	| ISO_MULTI_LINGUAL 
	{
		$$ = new CFESimpleType(TYPE_ISO_MULTILINGUAL); 
		$$->SetSourceLine(gLineNumber);
	}
	| ISO_UCS 
	{ 
		$$ = new CFESimpleType(TYPE_ISO_UCS); 
		$$->SetSourceLine(gLineNumber);
	}
	| REFSTRING
	{
		dceerror2("refstring type not supported by DCE IDL (use \"[ref] char*\" instead)");
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
		$$->SetSourceLine(gLineNumber);
	}
	| EXCEPTION type_spec declarator_list
	{ 
		$$ = new CFETypedDeclarator(TYPEDECL_EXCEPTION, $2, $3); 
		// set parent relationship
		$2->SetParent($$);
		$3->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	| EXCEPTION error
	{
		dceerror2("exception declarator expecting");
		YYABORT;
	}
	;

library:
	  { if (c_inc == 0) c_inc = 2; } LBRACKET lib_attribute_list { if (c_inc == 2) c_inc = 0; } RBRACKET library_or_module id_or_typename
	{
		// check if we can find a library with this name
		CFEFile *pRoot = CParser::GetCurrentFile()->GetRoot();
		assert(pRoot);
		$<_library>$ = pRoot->FindLibrary(*$7);
		// create library and set as current file-component
		CFELibrary *pFELibrary = new CFELibrary(*$7, $3, 0);
		pFELibrary->SetSourceLine(gLineNumber);
		if ($<_library>$ != 0)
			pFELibrary->AddSameLibrary($<_library>$);
		$3->SetParentOfElements(pFELibrary);
		if (pCurFileComponent)
		{
			if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFELibrary)))
			{
				((CFELibrary*)pCurFileComponent)->AddLibrary(pFELibrary);
				pCurFileComponent = pFELibrary;
			}
			else if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFEInterface)))
			{
				dceerror2("Cannot nest library %s into interface %s.", (const char*)*$7, (const char*)((CFEInterface*)pCurFileComponent)->GetName());
				delete $7;
				YYABORT;
			}
		}
		else
		{
			pCurFileComponent = pFELibrary;
			CParser::GetCurrentFile()->AddLibrary(pFELibrary);
		}
	} LBRACE lib_definitions RBRACE
	{
		// adjust current file component
		if (pCurFileComponent &&
			pCurFileComponent->GetParent())
		{
			if (pCurFileComponent->GetParent()->IsKindOf(RUNTIME_CLASS(CFEFileComponent)))
				pCurFileComponent = (CFEFileComponent*) pCurFileComponent->GetParent();
			else
				pCurFileComponent = 0;
		}
		$$ = 0;
		delete $7;
	}
	| library_or_module id_or_typename
	{
		// check if we already have lib with this name
		CFEFile *pRoot = CParser::GetCurrentFile()->GetRoot();
		assert(pRoot);
		$<_library>$ = pRoot->FindLibrary(*$2);
		// create library and set as current file-component
		CFELibrary *pFELibrary = new CFELibrary(*$2, 0, 0);
		pFELibrary->SetSourceLine(gLineNumber);
		if ($<_library>$ != 0)
			pFELibrary->AddSameLibrary($<_library>$);
		if (pCurFileComponent)
		{
			if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFELibrary)))
			{
				((CFELibrary*)pCurFileComponent)->AddLibrary(pFELibrary);
				pCurFileComponent = pFELibrary;
			}
			else if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFEInterface)))
			{
				dceerror2("Cannot nest library %s into interface %s.", (const char*)*($2), (const char*)((CFEInterface*)pCurFileComponent)->GetName());
				delete $2;
				YYABORT;
			}
		}
		else
		{
			pCurFileComponent = pFELibrary;
			CParser::GetCurrentFile()->AddLibrary(pFELibrary);
		}
	} LBRACE lib_definitions RBRACE
	{
		// imported elements already belong to lib_definitions
		// adjust current file component
		if (pCurFileComponent &&
			pCurFileComponent->GetParent())
		{
			if (pCurFileComponent->GetParent()->IsKindOf(RUNTIME_CLASS(CFEFileComponent)))
				pCurFileComponent = (CFEFileComponent*)pCurFileComponent->GetParent();
			else
				pCurFileComponent = 0;
		}
		delete $2;
		$$ = 0;
	}
    | library_or_module scoped_name
    {
        // forward declaration -> we create a library, but it will have nothing in it
        // this way we find it as base library if we search for it.
        // check if we already have lib with this name
        CFEFile *pRoot = CParser::GetCurrentFile()->GetRoot();
        assert(pRoot);
        $<_library>$ = pRoot->FindLibrary(*$2);
        // create library but do net set as current, simply add it
        // if scoped name: find libraries first
        int nScopePos = 0;
        CFELibrary *pFEScopeLibrary = NULL;
        String sRest = *$2;
        while ((nScopePos = sRest.Find("::")) >= 0)
        {
            String sScope = sRest.Left(nScopePos);
            sRest = sRest.Right(sRest.GetLength()-nScopePos-2);
            if (!sScope.IsEmpty())
            {
                if (pFEScopeLibrary)
                    pFEScopeLibrary = pFEScopeLibrary->FindLibrary(sScope);
                else
                    pFEScopeLibrary = pRoot->FindLibrary(sScope);
                if (!pFEScopeLibrary)
                {
                    dceerror2("Cannot find library %s, which is used in forward declaration of %s.",(const char*)sScope, (const char*)(*$2));
					delete $2;
                    YYABORT;
                }
            }
        }
        CFELibrary *pFELibrary = new CFELibrary(sRest, 0, 0);
        pFELibrary->SetSourceLine(gLineNumber);
        if ($<_library>$ != 0)
            pFELibrary->AddSameLibrary($<_library>$);
        if (pCurFileComponent)
        {
            if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFELibrary)))
            {
                if (pFEScopeLibrary)
                    pFEScopeLibrary->AddLibrary(pFELibrary);
                else
                    ((CFELibrary*)pCurFileComponent)->AddLibrary(pFELibrary);
            }
            else if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFEInterface)))
            {
                dceerror2("Cannot declare library %s within interface %s.", (const char*)*($2), (const char*)((CFEInterface*)pCurFileComponent)->GetName());
				delete $2;
                YYABORT;
            }
        }
        else
        {
            if (pFEScopeLibrary)
                pFEScopeLibrary->AddLibrary(pFELibrary);
            else
                CParser::GetCurrentFile()->AddLibrary(pFELibrary);
        }
		delete $2;
        $$ = NULL;
    }
	;

library_or_module :
      LIBRARY
    | MODULE
	{
	    dcewarning("Usage of the keyword 'module' is discouraged. Use 'library' instead.");
	}
	;

lib_attribute_list :
	  lib_attribute_list COMMA lib_attribute
	{
	        if ($3)
		        $1->Add($3);
		$$ = $1;
	}
	| lib_attribute
	{
		$$ = new Vector(RUNTIME_CLASS(CFEAttribute), 1, $1);
	}
    | error
    {
		dceerror2("unknown attribute");
		YYABORT;
	}
	;

lib_attribute :
/*	  UUID LPAREN uuid_rep rparen
	{
		$$ = new CFEStringAttribute(ATTR_UUID, *$3);
		delete $3;
		$$->SetSourceLine(gLineNumber);
	}
	|*/ VERSION_ATTR LPAREN version_rep RPAREN
	{
		$$ = new CFEVersionAttribute($3);
		$$->SetSourceLine(gLineNumber);
	}
	| VERSION_ATTR LPAREN error RPAREN
	{
		dcewarning("[version] format is incorrect");
		yyclearin;
		yyerrok;
	}
	| CONTROL
	{
		$$ = new CFEAttribute(ATTR_CONTROL);
		$$->SetSourceLine(gLineNumber);
	}
	| HELPCONTEXT LPAREN LIT_INT rparen
	{
		$$ = new CFEIntAttribute(ATTR_HELPCONTEXT, $3);
		$$->SetSourceLine(gLineNumber);
	}
	| HELPFILE LPAREN string rparen
	{
		$$ = new CFEStringAttribute(ATTR_HELPFILE, *$3);
		delete $3;
		$$->SetSourceLine(gLineNumber);
	}
	| HELPSTRING LPAREN string rparen
	{
		$$ = new CFEStringAttribute(ATTR_HELPSTRING, *$3);
		delete $3;
		$$->SetSourceLine(gLineNumber);
	}
	| HIDDEN
	{
		$$ = new CFEAttribute(ATTR_HIDDEN);
		$$->SetSourceLine(gLineNumber);
	}
	| LCID LPAREN LIT_INT rparen
	{
		$$ = new CFEIntAttribute(ATTR_LCID, $3);
		$$->SetSourceLine(gLineNumber);
	}
	| RESTRICTED
	{
		$$ = new CFEAttribute(ATTR_RESTRICTED);
		$$->SetSourceLine(gLineNumber);
	}
	;

lib_definitions :
	  lib_definitions lib_definition
	{
		if ($2)
			$1->Add($2);
		$$ = $1;
	}
	| lib_definitions import
	{
		/*if ($2) {
			$1->Add($2);
			// delete Vector
			delete $2;
		}*/
		$$ = $1;
	}
	| lib_definition
	{
		if ($1 != 0)
			$$ = new Vector(RUNTIME_CLASS(CFEFileComponent), 1, $1);
		else
			$$ = new Vector(RUNTIME_CLASS(CFEFileComponent));
	}
	| import
	{
		/*if ($1 != 0)
			$$ = $1;
		else*/
			$$ = new Vector(RUNTIME_CLASS(CFEFileComponent));
	}
	;

lib_definition :
	  interface semicolon
	{
		$$ = 0;
		// interface already added to current file component
	}
	| type_declarator semicolon
	{
		$$ = 0;
		if (pCurFileComponent)
		{
			if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFELibrary)))
				((CFELibrary*)pCurFileComponent)->AddTypedef($1);
			else if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFEInterface)))
				((CFEInterface*)pCurFileComponent)->AddTypedef($1);
			else
			{
				TRACE("current file component is unknown type: %s\n", (const char*)pCurFileComponent->GetClassName());
				assert(false);
			}
		}
		else
			CParser::GetCurrentFile()->AddTypedef($1); // should be error, since current-file component should exist
	}
	| const_declarator semicolon
	{
		$$ = 0;
		if (pCurFileComponent)
		{
			if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFELibrary)))
				((CFELibrary*)pCurFileComponent)->AddConstant($1);
			else if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFEInterface)))
				((CFEInterface*)pCurFileComponent)->AddConstant($1);
			else
			{
				TRACE("current file component is unknown type: %s\n", (const char*)pCurFileComponent->GetClassName());
				assert(false);
			}
		}
	}
	| tagged_declarator semicolon
	{
		$$ = NULL;
		if (pCurFileComponent)
		{
			if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFELibrary)))
				((CFELibrary*)pCurFileComponent)->AddTaggedDecl($1);
			else if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFEInterface)))
				((CFEInterface*)pCurFileComponent)->AddTaggedDecl($1);
			else
			{
				TRACE("current file component is unknown type: %s\n", (const char*)pCurFileComponent->GetClassName());
				assert(false);
			}
		}
	}
    | library semicolon
    {
        $$ = NULL;
        // library has been added already to current file component
	}
	;

/* helper definitions */
/*uuid_rep
	: QUOT UUID_STR QUOT
	{
		$$ = $2;
	}
	| UUID_STR
	{
		$$ = $1;
	}
	| error
	{
		dceerror2("expected a UUID representation");
		YYABORT;
	}
	;
*/
opt_semicolon :
	  SEMICOLON
	|
	;

semicolon
	: SEMICOLON
	| error
	{
		dceerror2("expecting ';'");
//		yyclearin;
//		YYBACKUP(SEMICOLON, yylval);
//		dcelex(&yylval);
		YYABORT;
	}
	;

rbracket
	: RBRACKET
	| error
	{
		dceerror2("expecting ']'");
		YYABORT;
	}
	;

rparen
	: RPAREN
	| error
	{
		dceerror2("expecting ')'");
		YYABORT;
	}
	;

colon
	: COLON
	| error
	{
		dceerror2("expecting ':'");
		YYABORT;
	}
	;

/*********************************************************************************
 * start GCC specific rules
 *********************************************************************************/

type_qualifier_list:
	  type_qualifier
	| type_qualifier_list type_qualifier
	;

type_qualifier:
	  C_CONST
	| VOLATILE
	| RESTRICT
/*	| BYCOPY
	| BYREF
	| IN
	| OUT
	| INOUT
	| ONEWAY*/
	;

storage_class_specifier:
	  AUTO
	| EXTERN
	| REGISTER
	| STATIC
/*	| TYPEDEF */
	| INLINE
	;

declaration_specifiers:
	  declaration_specifier
	{ $$ = $1; }
	| declaration_specifiers declaration_specifier
	{
		if ($1 == 0)
			$$ = $2;
		else
			$$ = $1;
	}
	;

declaration_specifier:
	  storage_class_specifier
	{ $$ = 0; }
	|type_spec
	{
	    $$ = $1;
	}
	| type_qualifier
	{ $$ = 0; }
	;

abstract_declarator:
	  pointer {}
	|		  attribute_list direct_abstract_declarator
	| pointer attribute_list direct_abstract_declarator {}
	;

direct_abstract_declarator:
      IDENTIFIER {}
	| LPAREN abstract_declarator RPAREN
	| LBRACKET RBRACKET
	| LBRACKET const_expr RBRACKET
	| LPAREN param_declarators RPAREN
	| LPAREN param_declarators COMMA ELLIPSIS RPAREN
	| direct_abstract_declarator LBRACKET RBRACKET
	| direct_abstract_declarator LBRACKET const_expr RBRACKET
	| direct_abstract_declarator LPAREN param_declarators RPAREN
	| direct_abstract_declarator LPAREN param_declarators COMMA ELLIPSIS RPAREN
	;

/* function declaration and definition */
gcc_function:
	  function_specifier compound_statement opt_semicolon
	| function_specifier attribute_list SEMICOLON
	| op_declarator attribute_list SEMICOLON
	{ delete $1; }
	| declaration_specifiers direct_declarator SEMICOLON {}
	;

gcc_declarator:
      declaration_specifiers abstract_declarator
    {
	    /* a gcc declarator is ignored */
    }
	;

function_specifier:
							 function_declarator				  {}
	|						 function_declarator declaration_list {}
	| declaration_specifiers function_declarator				  {}
	| declaration_specifiers function_declarator declaration_list {}
	;

declaration_list :
	  declaration
	| declaration_list declaration
	;

declaration:
	  declaration_specifiers initialized_declarator_list SEMICOLON
	{ }
	;

initialized_declarator_list:
	  initialized_declarator
	| initialized_declarator_list COMMA initialized_declarator
	;

initialized_declarator:
	  declarator {}
	| declarator IS initializer {}
	;

initializer:
	  const_expr
	{
		if ($1 != 0) delete $1;
	}
	| LBRACE initializer_list RBRACE
	| LBRACE initializer_list COMMA RBRACE
	;

initializer_list:
	  initializer
	| initializer_list COMMA initializer
	;

compound_statement:
	  LBRACE
	{
		/* scan to next matching RBRACE */
		int nLevel = 1, nChar;
		while (nLevel > 0) {
			nChar = dcelex(&yylval);
			if (nChar == LBRACE) nLevel++;
			if (nChar == RBRACE) nLevel--;
		}
	}
	;

attribute_list:
	  /* empty */
	| attribute_list attribute
	;

attribute:
      ATTRIBUTE LPAREN LPAREN
	{
		// attributes_attribute_list RPAREN RPAREN
		// scan to next matching RPAREN RPAREN
		int nLevel = 2, nChar;
		while (nLevel > 0) {
			nChar = dcelex(&yylval);
			if (nChar == LPAREN) nLevel++;
			if (nChar == RPAREN) nLevel--;
		}
	}
	;

/*********************************************************************************
 * end GCC specific rules
 *********************************************************************************/

%%

extern int gLineNumber;

void
dceerror(char* s)
{
	// ignore this functions -> it's called by bison code, which is not controlled by us
	nParseErrorDCE = 1;
}

void
dceerror2(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	CCompiler::GccErrorVL(CParser::GetCurrentFile(), gLineNumber, fmt, args);
	va_end(args);
	nParseErrorDCE = 0;
	erroccured = 1;
	errcount++;
}

void
dcewarning(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	CCompiler::GccWarningVL(CParser::GetCurrentFile(), gLineNumber, fmt, args);
	va_end(args);
	nParseErrorDCE = 0;
}

bool
DoInterfaceCheck(String *pIName, Vector *pBaseInterfaces, CFEInterface **pFEInterface)
{
	// test base interfaces
	CFEFile *pRoot = CParser::GetCurrentFile()->GetRoot();
	assert(pRoot);
	if (pBaseInterfaces)
	{
		VectorElement *pCurrent;
		CFEIdentifier *pId;
		for (pCurrent = pBaseInterfaces->GetFirst(); pCurrent; pCurrent = pCurrent->GetNext())
		{
			if (pCurrent->GetElement())
			{
				pId = (CFEIdentifier*)(pCurrent->GetElement());
				if (pRoot->FindInterface(pId->GetName()) == NULL)
				{
					dceerror2("Couldn't find base interface name \"%s\".", (const char*)(pId->GetName()));
					return false;
				}
			}
		}
	}
	// test new interface
	*pFEInterface = NULL;
	if ((*pFEInterface = pRoot->FindInterface(*pIName)) != NULL)
	{
		// name already exists -> test if forward declaration (has no elements)
		if (!((*pFEInterface)->IsForward()))
		{
			dceerror2("Interface \"%s\" already exists", (const char*)(*pIName));
			return false;
		}
	}
	return true;
}

bool
AddInterfaceToFileComponent(CFEInterface* pFEInterface, CFELibrary *pFELibrary)
{
	if (pCurFileComponent)
	{
		if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFELibrary)))
		{
			if (pFELibrary)
				pFELibrary->AddInterface(pFEInterface);
			else
				((CFELibrary*)pCurFileComponent)->AddInterface(pFEInterface);
		}
		else if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFEInterface)))
		{
			dceerror2("Cannot nest interface %s into interface %s\n",
			    (const char*)pFEInterface->GetName(),
				(const char*)((CFEInterface*)pCurFileComponent)->GetName());
			return false;
		}
	}
	else
	{
		if (pFELibrary)
			pFELibrary->AddInterface(pFEInterface);
		else
			CParser::GetCurrentFile()->AddInterface(pFEInterface);
	}
	return true;
}
