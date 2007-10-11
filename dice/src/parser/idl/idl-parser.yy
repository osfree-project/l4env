%skeleton "lalr1.cc"
%require "2.1a"
%defines
%define "parser_class_name" "idl_parser"

%{
#include "fe/FESimpleType.h"
#include "fe/FEIdentifier.h"
#include "fe/FEInterface.h"
#include "fe/FELibrary.h"
#include "fe/FEFile.h"
#include "fe/FEArrayType.h"
#include "fe/FEEnumType.h"
#include "fe/FEStructType.h"
#include "fe/FEIDLUnionType.h"
#include "fe/FEUserDefinedType.h"
#include "fe/FETypeOfType.h"
#include "fe/FEPipeType.h"
#include "fe/FEOperation.h"
#include "fe/FEIsAttribute.h"
#include "fe/FEExceptionAttribute.h"
#include "fe/FEEndPointAttribute.h"
#include "fe/FEIntAttribute.h"
#include "fe/FERangeAttribute.h"
#include "fe/FEVersionAttribute.h"
#include "fe/FEStringAttribute.h"
#include "fe/FETypeAttribute.h"
#include "fe/FEPtrDefaultAttribute.h"
#include "fe/FEConditionalExpression.h"
#include "fe/FEUserDefinedExpression.h"
#include "fe/FESizeOfExpression.h"
#include "fe/FEConstDeclarator.h"
#include "fe/FEArrayDeclarator.h"
#include "fe/FEEnumDeclarator.h"
#include "fe/FEFunctionDeclarator.h"
#include "fe/FEAttributeDeclarator.h"
#include <vector>
using std::vector;
#include <cassert>

class idl_parser_driver;

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif
%}

// The parsing context.
%parse-param { idl_parser_driver& driver }
%lex-param   { idl_parser_driver& driver }

%locations
%initial-action
{
  // Initialize the initial location.
  @$.begin.filename = @$.end.filename = &driver.current_file;
};

%debug
%error-verbose

// Symbols.
%union
{
  int				ival;
  long				lval;
  unsigned long			ulval;
#if SIZEOF_LONG_LONG > 0
  long long			llval;
  unsigned long long		ullval;
#endif
  double			dval;
  signed char			cval;
  signed short			wcval;
  std::string			*sval;
  enum EXPT_OPERATOR		uop;

  CFETypeSpec*			type;
  CFEAttribute*			attr;
  CFEExpression*		expr;
  CFEDeclarator*		decl;
  CFETypedDeclarator*		tDecl;
  CFEConstDeclarator*		cdecl;
  CFEUnionCase*			ucase;
  CFEConstructedType*		ctype;
  CFEUnionType*			utype;
  CFEEnumType*			etype;
  CFEStructType*		stype;
  CFESimpleType*		simpleType;
  CFEOperation*			op;
  CFEInterfaceComponent*	ifComp;
  CFEFileComponent*		fComp;
  CFEInterface*			iface;
  CFELibrary*			lib;
  CFEArrayDeclarator*		adecl;

  vector<CFEIdentifier*>*	vecIdent;
  vector<CFEAttribute*>*	vecAttr;
  vector<CFEExpression*>*	vecExpr;
  vector<CFEDeclarator*>*	vecDecl;
  vector<CFEUnionCase*>*	vecUcase;
  vector<CFETypedDeclarator*>*	vecTDecl;
  vector<CFEInterfaceComponent*>*	vecIfComp;
  vector<CFEFileComponent*>*	vecFComp;
  vector<PortSpec>*		pSpecs;
};

%{
#include "idl-parser-driver.hh"
%}

%token		EOF_TOKEN	0 "end of file"
%token		INVALID		1 "invalid token"
%token <sval>	ID		"identifier"

%token		ADD_ASSIGN	"+="
%token		AND_ASSIGN	"&="
%token		ASTERISK	"*"
%token		ATTRIBUTE	"attribute"
%token		BITAND		"&"
%token		BITOR		"|"
%token		BITXOR		"^"
%token		BOOLEAN		"boolean"
%token		BYTE		"byte"
%token		CASE		"case"
%token		CHAR		"char"
%token		CHAR_PTR	"char*"
%token		COLON		":"
%token		COMMA		","
%token		CONST		"const"
%token		CONTROL		"control"
%token		DEC_OP		"--"
%token		DEFAULT		"default"
%token		DIV		"/"
%token		DIV_ASSIGN	"/="
%token		DOT		"."
%token		DOTDOT		".."
%token		DOUBLE		"double"
%token		ENUM		"enum"
%token		EQUAL		"=="
%token		ERROR_STATUS_T	"error_status_t"
%token		EXCEPTION	"exception"
%token		EXCLAM		"!"
%token		EXPNULL		"null"
%token		FALSE		"false"
%token	<sval>	FILENAME
%token		FLEXPAGE	"flexpage"
%token		FLOAT		"float"
%token		GT		">"
%token		GTEQUAL		">="
%token		HANDLE_T	"handle_t"
%token		HYPER		"hyper"
%token		IMPORT		"import"
%token		IN		"in"
%token		INC_OP		"++"
%token		INOUT		"inout"
%token		INT		"int"
%token		INTERFACE	"interface"
%token		IS		"="
%token		ISO_LATIN_1	"iso_latin_1"
%token		ISO_MULTI_LINGUAL	"iso_multi_lingual"
%token		ISO_UCS		"iso_ucs"
%token		LBRACE		"{"
%token		LBRACKET	"["
%token		LIBRARY		"library"
%token	<cval>	LIT_CHAR	"literal character"
%token	<dval>	LIT_FLOAT	"float value"
%token	<ival>	LIT_INT		"integer value"
%token	<lval>  LIT_LONG	"long value"
%token	<ulval>	LIT_ULONG	"unsigned long value"
%token	<llval>	LIT_LLONG	"long long value"
%token	<ullval> LIT_ULLONG	"unsigned long long value"
%token	<sval>	LIT_STR		"literal string"
%token	<wcval>	LIT_WCHAR	"literal wide-character"
%token	<sval>	LIT_WSTR	"literal wide-char string"
%token		LOGICALAND	"&&"
%token		LOGICALOR	"||"
%token		LONG		"long"
%token		LONGLONG	"long long"
%token		LPAREN		"("
%token		LS_ASSIGN	"<<="
%token		LSHIFT		"<<"
%token		LT		"<"
%token		LTEQUAL		"<="
%token		MINUS		"-"
%token		MOD		"%"
%token		MOD_ASSIGN	"%="
%token		MODULE		"module"
%token		MUL_ASSIGN	"*="
%token		NOTEQUAL	"!="
%token		OR_ASSIGN	"|="
%token		OUT		"out"
%token		PIPE		"pipe"
%token		PLUS		"+"
%token	<pSpecs>	PORTSPEC
%token		PTR_OP		"->"
%token		QUESTION	"?"
%token		QUOT		"\""
%token		RAISES		"raises"
%token		RBRACE		"}"
%token		RBRACKET	"]"
%token		REFSTRING	"refstring"
%token		RPAREN		")"
%token		RS_ASSIGN	">>="
%token		RSHIFT		">>"
%token		SCOPE		"::"
%token		SEMICOLON	";"
%token		SHORT		"short"
%token		SIGNED		"signed"
%token		SINGLEQUOT	"\'"
%token		SIZEOF		"sizeof"
%token		SMALL		"small"
%token	<sval>	STD_FILENAME
%token		STRUCT		"struct"
%token		SUB_ASSIGN	"-="
%token		SWITCH		"switch"
%token		TILDE		"~"
%token		TRUE		"true"
%token		TYPEDEF		"typedef"
%token	<sval>	TYPENAME
%token		UNION		"union"
%token		UNSIGNED	"unsigned"
%token	<sval>	UUID_STR	"uuid specification"
%token	<sval>	VERSION_STR	"version string"
%token		VOID		"void"
%token		VOID_PTR	"void*"
%token		XOR_ASSIGN	"^="

%token		ABS			"abs"
%token		ABSTRACT	"abstract"
%token		ANY		"any"
%token		ALLOW_REPLY_ONLY	"allow_reply_only"
%token		AUTO_HANDLE	"auto_handle"
%token		BROADCAST	"broadcast"
%token		CALLBACK	"callback"
%token		CONTEXT		"context"
%token		CONTEXT_HANDLE	"context_handle"
%token		CUSTOM		"custom"
%token		DEDICATED_PARTNER	"dedicated_partner"
%token		DEFAULT_FUNCTION	"default_function"
%token		DEFAULT_TIMEOUT	"default_timeout"
%token		ENDPOINT	"endpoint"
%token		ERROR_FUNCTION	"error_function"
%token		ERROR_FUNCTION_CLIENT	"error_function_client"
%token		ERROR_FUNCTION_SERVER	"error_function_server"
%token		EXCEPTIONS	"exceptions"
%token		FACTORY		"factory"
%token		FIRST_IS	"first_is"
%token		FIXED		"fixed"
%token		HANDLE		"handle"
%token		HELPCONTEXT	"helpcontext"
%token		HELPFILE	"helpfile"
%token		HELPSTRING	"helpstring"
%token		HIDDEN		"hidden"
%token		IDEMPOTENT	"idempotent"
%token		IGNORE		"ignore"
%token		IID_IS		"iid_is"
%token		INIT_RCVSTRING	"init_rcvstring"
%token		INIT_RCVSTRING_CLIENT	"init_rcvstring_client"
%token		INIT_RCVSTRING_SERVER	"init_rcvstring_server"
%token		LAST_IS		"last_is"
%token		LCID		"lcid"
%token		LENGTH_IS	"length_is"
%token		LOCAL		"local"
%token		MAX_IS		"max_is"
%token		MAYBE		"maybe"
%token		MIN_IS		"min_is"
%token		NATIVE		"native"
%token		NOEXCEPTIONS	"noexceptions"
%token		NOOPCODE	"noopcode"
%token		OBJECT		"object"
%token		OCTET		"octet"
%token		ONEWAY		"oneway"
%token		POINTER_DEFAULT	"pointer_default"
%token		PREALLOC_CLIENT	"prealloc_client"
%token		PREALLOC_SERVER	"prealloc_server"
%token		PRIVATE		"private"
%token		PTR		"ptr"
%token		PUBLIC		"public"
%token		READONLY	"readonly"
%token		REF		"ref"
%token		REFLECT_DELETIONS	"reflect_deletions"
%token		RESTRICTED	"restricted"
%token		SCHED_DONATE	"sched_donate"
%token		SEQUENCE	"sequence"
%token		SIZE_IS		"size_is"
%token		STRING		"string"
%token		SUPPORTS	"supports"
%token		SWITCH_IS	"switch_is"
%token		SWITCH_TYPE	"switch_type"
%token		TRANSMIT_AS	"transmit_as"
%token		TRUNCABLE	"truncable"
%token		TYPEOF		"typeof"
%token		UNIQUE		"unique"
%token		UUID		"uuid"
%token		VERSION_ATTR	"version"
%token		VALUETYPE	"valuetype"
%token		WCHAR		"wchar"
%token		WSTRING		"wstring"

%printer    { debug_stream () << *$$; } "identifier" "uuid specification" "version string" "literal string"
%destructor { delete $$; } "identifier" "uuid specification" "version string" "literal string"

%printer    { debug_stream () << $$; } "literal character" "float value" "integer value"

%left COMMA SEMICOLON
%right PLUS MINUS
%left LOGICALAND LOGICALOR BITAND BITOR
%left LSHIFT RSHIFT

%right LPAREN
%left RPAREN

/* intermediate token */
%type	<expr>		additive_expr
%type	<expr>		and_expr
%type	<expr>		array_bound
%type	<adecl>		array_declarator
%type	<uop>		assignment_operator
%type	<sval>		attribute_keyword
%type	<vecAttr>	attribute
%type	<vecAttr>	attribute_list
%type	<attr>		attributes
%type	<vecAttr>	attributes_list
%type	<tDecl>		attr_declarator
%type	<decl>		attr_var
%type	<vecDecl>	attr_var_list
%type	<sval>		base_interface
%type	<vecIdent>	base_interface_list
%type	<type>		base_type_spec
%type	<type>		boolean_type
%type	<expr>		cast_expr
%type	<simpleType>	char_base
%type	<simpleType>	char_type
%type	<expr>		conditional_expr
%type	<cdecl>		const_declarator
%type	<expr>		const_expr
%type	<vecExpr>	const_expr_list
%type	<type>		const_type_spec
%type	<ctype>		constructed_type_spec
%type	<decl>		declarator
%type	<vecDecl>	declarator_list
%type	<decl>		direct_declarator
%type	<attr>		directional_attribute
%type	<decl>		enumeration_declarator
%type	<vecDecl>	enumeration_declarator_list
%type	<etype>		enumeration_type
%type	<expr>		equality_expr
%type	<tDecl>		exception_declarator
%type	<vecIdent>	excep_name_list
%type	<expr>		exclusive_or_expr
%type	<attr>		field_attribute
%type	<vecAttr>	field_attribute_list
%type	<vecAttr>	field_attributes
%type	<tDecl>		field_declarator
%type	<type>		fixed_type
%type	<type>		floating_pt_type
%type	<decl>		function_declarator
%type	<vecIdent>	identifier_list
%type	<vecAttr>	if_or_lib_attributes
%type	<vecAttr>	if_or_lib_attribute_list
%type	<expr>		inclusive_or_expr
%type	<vecIdent>	inheritance_spec
%type	<lval>		integer_size
%type	<type>		integer_type
%type	<iface>		interface
%type	<attr>		interface_attribute
%type	<ifComp>	interface_component
%type	<vecIfComp>	interface_component_list
%type	<fComp>		lib_definition
%type	<vecFComp>	lib_definition_list
%type	<lib>		library
%type	<expr>		logical_and_expr
%type	<expr>		logical_or_expr
%type	<tDecl>		member
%type	<vecTDecl>	member_list
%type	<vecTDecl>	member_list_1
%type	<expr>		multiplicative_expr
%type	<op>		op_declarator
%type	<attr>		operation_attribute
%type	<vecAttr>	operation_attribute_list
%type	<vecAttr>	operation_attributes
%type	<attr>		param_attribute
%type	<vecAttr>	param_attribute_list
%type	<vecAttr>	param_attributes
%type	<tDecl>		param_declarator
%type	<vecTDecl>	param_declarator_list
%type	<vecTDecl>	param_declarators
%type	<ival>		pointer
%type	<expr>		postfix_expr
%type	<type>		predefined_type_spec
%type	<expr>		primary_expr
%type	<attr>		ptr_attr
%type	<vecIdent>	raises_declarator
%type	<expr>		relational_expr
%type	<type>		sequence_type
%type	<sval>		scoped_name
%type	<expr>		shift_expr
%type	<type>		simple_type_spec
%type	<sval>		string
%type	<type>		string_type
%type	<type>		switch_type_spec
%type	<sval>		tag
%type	<ctype>		tagged_declarator
%type	<etype>		tagged_enumeration_type
%type	<stype>		tagged_struct_declarator
%type	<utype>		tagged_union_declarator
%type	<type>		template_type_spec
%type	<attr>		type_attribute
%type	<vecAttr>	type_attribute_list
%type	<vecAttr>	type_attributes
%type	<tDecl>		type_declarator
%type	<type>		type_spec
%type	<expr>		unary_expr
%type	<uop>		unary_operator
%type	<tDecl>		union_arm
%type	<vecUcase>	union_body
%type	<vecUcase>	union_body_n_e
%type	<expr>		union_case_label
%type	<vecExpr>	union_case_label_list
%type	<ucase>		union_case
%type	<ucase>		union_case_n_e
%type	<attr>		union_instance_switch_attr
%type	<utype>		union_type
%type	<utype>		union_type_header
%type	<attr>		union_type_switch_attr
%type	<attr>		usage_attribute
%type	<ival>		uuid_absolute

%start file

%%

/* In some places you will discover that the default rule $$ = $1; is
 * explicetly given. This is due to the fact that the type of $$ is a pointer
 * to a superclass of $1. The types in the union do not match <type> !=
 * <ctype>, therefore the rule has to be given explicetly.
 */

file :
	  file file_component
	| /* a totally empty file */
	;

file_component :
	  interface SEMICOLON
	{
	    if ($1)
	    {
		// add interface to current context
		CFEBase *pContext = driver.getCurrentContext();
		CFEFile *pFile = dynamic_cast<CFEFile*>(pContext);
		CFELibrary *pLib = dynamic_cast<CFELibrary*>(pContext);
		if (pFile)
		{
		    pFile->m_Interfaces.Add($1);
		    pFile->m_sourceLoc += @1 + @2;
		}
		else if (pLib)
		{
		    pLib->m_Interfaces.Add($1);
		    pLib->m_sourceLoc += @1 + @2;
		}
		else
		{
		    // something else is context, which is not possible: throw error
		    driver.error(@1, "Interface \"" + $1->GetName() + "\" declared in invalid context.");
		    YYABORT;
		}
	    }
	}
	| library SEMICOLON
	{
	    if ($1)
	    {
		// add library to current context
		CFEBase *pContext = driver.getCurrentContext();
		CFEFile *pFile = dynamic_cast<CFEFile*>(pContext);
		CFELibrary *pLib = dynamic_cast<CFELibrary*>(pContext);
		if (pFile)
		{
		    pFile->m_Libraries.Add($1);
		    pFile->m_sourceLoc += @1 + @2;
		}
		else if (pLib)
		{
		    pLib->m_Libraries.Add($1);
		    pLib->m_sourceLoc += @1 + @2;
		}
		else
		{
		    // something else is context, which is not possible: throw error
		    driver.error(@1, "Library \"" + $1->GetName() + "\" declared in invalid context.");
		    YYABORT;
		}
	    }
	}
	| export
	{
	    // export is already added to context
	}
	| SEMICOLON
	/* ignore CORBA's value declaration */
	;

export :
	  type_declarator SEMICOLON
	{
	    if ($1)
	    {
		// add typedef to current context
		CFEBase *pContext = driver.getCurrentContext();
		CFEFile *pFile = dynamic_cast<CFEFile*>(pContext);
		CFELibrary *pLib = dynamic_cast<CFELibrary*>(pContext);
		CFEInterface *pInterface = dynamic_cast<CFEInterface*>(pContext);
		if (pFile)
		{
		    pFile->m_Typedefs.Add($1);
		    pFile->m_sourceLoc += @1 + @2;
		}
		else if (pLib)
		{
		    pLib->m_Typedefs.Add($1);
		    pLib->m_sourceLoc += @1 + @2;
		}
		else if (pInterface)
		{
		    pInterface->m_Typedefs.Add($1);
		    pInterface->m_sourceLoc += @1 + @2;
		}
		else
		{
		    // something else is context, which is not possible: throw error
		    driver.error(@1, "Typedef declared in invalid context.");
		    YYABORT;
		}
	    }
	}
	| const_declarator SEMICOLON
	{
	    if ($1)
	    {
		// add constant to current context
		CFEBase *pContext = driver.getCurrentContext();
		CFEFile *pFile = dynamic_cast<CFEFile*>(pContext);
		CFELibrary *pLib = dynamic_cast<CFELibrary*>(pContext);
		CFEInterface *pInterface = dynamic_cast<CFEInterface*>(pContext);
		if (pFile)
		{
		    pFile->m_Constants.Add($1);
		    pFile->m_sourceLoc += @1 + @2;
		}
		else if (pLib)
		{
		    pLib->m_Constants.Add($1);
		    pLib->m_sourceLoc += @1 + @2;
		}
		else if (pInterface)
		{
		    pInterface->m_Constants.Add($1);
		    pInterface->m_sourceLoc += @1 + @2;
		}
		else
		{
		    // something else is context, which is not possible: throw error
		    driver.error(@1, "Constant declared in invalid context.");
		    YYABORT;
		}
	    }
	}
	| tagged_declarator SEMICOLON
	{
	    if ($1)
	    {
		// add type declaration to current context
		CFEBase *pContext = driver.getCurrentContext();
		CFEFile *pFile = dynamic_cast<CFEFile*>(pContext);
		CFELibrary *pLib = dynamic_cast<CFELibrary*>(pContext);
		CFEInterface *pInterface = dynamic_cast<CFEInterface*>(pContext);
		if (pFile)
		{
		    pFile->m_TaggedDeclarators.Add($1);
		    pFile->m_sourceLoc += @1 + @2;
		}
		else if (pLib)
		{
		    pLib->m_TaggedDeclarators.Add($1);
		    pLib->m_sourceLoc += @1 + @2;
		}
		else if (pInterface)
		{
		    pInterface->m_TaggedDeclarators.Add($1);
		    pInterface->m_sourceLoc += @1 + @2;
		}
		else
		{
		    // something else is context, which is not possible: throw error
		    driver.error(@1, "Type declared in invalid context.");
		    YYABORT;
		}
	    }
	}
	| exception_declarator SEMICOLON
	{
	    if ($1)
	    {
		// add type declaration to current context
		CFEBase *pContext = driver.getCurrentContext();
		CFEInterface *pInterface = dynamic_cast<CFEInterface*>(pContext);
		if (pInterface)
		{
		    pInterface->m_Exceptions.Add($1);
		    pInterface->m_sourceLoc += @1 + @2;
		}
		else
		{
		    // something else is context, which is not possible: throw error
		    driver.error(@1, "Exception declared in invalid context.");
		    YYABORT;
		}
	    }
	}
	| import
	{ }
	;

import :
	  IMPORT import_list
	;

import_list :
	  import_list COMMA FILENAME
	{
	    driver.import(*$3, false);
	}
	| import_list COMMA LIT_STR
	{
	    driver.import(*$3, false);
	}
	| import_list COMMA STD_FILENAME
	{
	    driver.import(*$3, true);
	}
	| FILENAME
	{
	    driver.import(*$1, false);
	}
	| LIT_STR
	{
	    driver.import(*$1, false);
	}
	| STD_FILENAME
	{
	    driver.import(*$1, true);
	}
	;

library :
	  if_or_lib_attributes LIBRARY scoped_name LBRACE
	{
		if (driver.trace_scanning)
			std::cerr << "C-Parser: LIBRARY checking " << *$3 << std::endl;

		$<lib>$ = new CFELibrary(*$3, $1, driver.getCurrentContext());
		// try to find another library with the same name
		CFEFile *pFile = driver.getCurrentContext()->GetRoot();
		assert(pFile);
		CFELibrary *pSameLib = pFile->FindLibrary(*$3);
		if (pSameLib)
			pSameLib->AddSameLibrary($<lib>$);
		// always add new lib to current scope
		driver.add_token(*$3, dice::parser::CSymbolTable::NAMESPACE, (CFEBase*)0,
			*@3.begin.filename, @3.begin.line, @3.begin.column);

		driver.setCurrentContext($<lib>$);
	} lib_definition_list RBRACE
	{
	    $$ = $<lib>5;
	    $$->AddComponents($6);
	    $$->m_sourceLoc = @$;
	    driver.leaveCurrentContext();
	}
	| LIBRARY scoped_name LBRACE
	{
	    if (driver.trace_scanning)
		std::cerr << "C-Parser: LIBRARY checking " << *$2 << "\n";

	    $<lib>$ = new CFELibrary(*$2, NULL, driver.getCurrentContext());
	    // try to find another library with the same name
	    CFEFile *pFile = driver.getCurrentContext()->GetRoot();
	    assert(pFile);
	    CFELibrary *pSameLib = pFile->FindLibrary(*$2);
	    if (pSameLib)
		pSameLib->AddSameLibrary($<lib>$);
	    // set new lib
	    driver.add_token(*$2, dice::parser::CSymbolTable::NAMESPACE, (CFEBase*)0,
		*@2.begin.filename, @2.begin.line, @2.begin.column);

	    driver.setCurrentContext($<lib>$);
	} lib_definition_list RBRACE
	{
	    $$ = $<lib>4;
	    $$->AddComponents($5);
	    $$->m_sourceLoc = @$;
	    driver.leaveCurrentContext();
	}
	;

if_or_lib_attributes :
	  { driver.expect_attr = true; } LBRACKET if_or_lib_attribute_list { driver.expect_attr = false; } RBRACKET
	{
	    $$ = $3;
	}
	/* ignore CORBA specific interface attribute "abstract" */
	;

/* library_attributes is contained in interface_attribute: library has to
 * check for valid attributes later.
 */
if_or_lib_attribute_list :
	  if_or_lib_attribute_list COMMA interface_attribute
	{
	    $$ = $1;
	    if ($3)
		$$->push_back($3);
	}
	| interface_attribute
	{
	    $$ = new vector<CFEAttribute*>();
	    if ($1)
		$$->push_back($1);
	}
	;

lib_definition_list :
	  lib_definition_list lib_definition
	{
	    $$ = $1;
	    if ($2)
		$$->push_back($2);
	}
	| /* empty */
	{
	    $$ = new vector<CFEFileComponent*>();
	}
	;

lib_definition :
	  interface SEMICOLON
	{ $$ = $1; }
	| export
	{
	    // export is already added to context
	    $$ = NULL;
	}
	| library SEMICOLON
	{ $$ = $1; }
	| SEMICOLON
	{
	    $$ = NULL;
	}
	/* ignore CORBA's value declaration */
	;

identifier_list :
	  identifier_list COMMA scoped_name
	{
	    $$ = $1;
	    if ($3)
	    {
		CFEIdentifier *t = new CFEIdentifier(*$3);
		t->m_sourceLoc = @3;
		$$->push_back(t);
	    }
	}
	| scoped_name
	{
	    $$ = new vector<CFEIdentifier*>();
	    if ($1)
	    {
		CFEIdentifier* t = new CFEIdentifier(*$1);
		t->m_sourceLoc = @1;
		$$->push_back(t);
	    }
	}
	;

scoped_name :
	  scoped_name SCOPE ID
	{
	    $$ = $1;
	    $1->append("::");
	    $1->append(*$3);
	}
	| SCOPE ID
	{
	    $$ = new std::string("::");
	    $$->append(*$2);
	}
	| ID
	;

excep_name_list :
	  excep_name_list COMMA ID
	{
	    $$ = $1;
	    CFEIdentifier *tId = new CFEIdentifier(*$3);
	    tId->m_sourceLoc = @3;
	    $$->push_back(tId);
	}
	| ID
	{
	    $$ = new vector<CFEIdentifier*>();
	    CFEIdentifier *tId = new CFEIdentifier(*$1);
	    tId->m_sourceLoc = @1;
	    $$->push_back(tId);
	}
	;

ptr_attr :
	  REF
	{
	    $$ = new CFEAttribute(ATTR_REF);
	    $$->m_sourceLoc = @$;
	}
	| UNIQUE
	{
	    $$ = new CFEAttribute(ATTR_UNIQUE);
	    $$->m_sourceLoc = @$;
	}
	| PTR
	{
	    $$ = new CFEAttribute(ATTR_PTR);
	    $$->m_sourceLoc = @$;
	}
	| IID_IS LPAREN attr_var_list RPAREN
	{
	    $$ = new CFEIsAttribute(ATTR_IID_IS, $3);
	    $$->m_sourceLoc = @$;
	}
	;

pointer :
	  pointer ASTERISK
	{
	    $$ = $1 + 1;
	}
	| ASTERISK
	{
	    $$ = 1;
	}
	;

attr_var_list :
	  attr_var_list COMMA attr_var
	{
	    $$ = $1;
	    if ($3)
		$$->push_back($3);
	}
	| attr_var
	{
	    $$ = new vector<CFEDeclarator*>();
	    if ($1)
		$$->push_back($1);
	}
	;

attr_var:
	  ASTERISK ID
	{
	    $$ = new CFEDeclarator(DECL_ATTR_VAR, *$2, 1);
	    $$->m_sourceLoc = @$;
	}
	| ID
	{
	    $$ = new CFEDeclarator(DECL_ATTR_VAR, *$1);
	    $$->m_sourceLoc = @$;
	}
	| /* empty */
	{
	    $$ = new CFEDeclarator(DECL_VOID);
	}
	;

interface :
	  if_or_lib_attributes INTERFACE scoped_name inheritance_spec LBRACE
	{
	    if (driver.trace_scanning)
		std::cerr << "C-Parser: INTERFACE checking " << *$3 << "\n";
	    // check if identifier is already used as class id (structs are
	    // classes)
	    if (driver.check_token(*$3, dice::parser::CSymbolTable::CLASS))
		driver.error(@3, "Interface ID \"" + *$3 + "\" already defined.");
	    // add aliases to symbol table
	    driver.add_token(*$3, dice::parser::CSymbolTable::CLASS, (CFEBase*)0,
		*@3.begin.filename, @3.begin.line, @3.begin.column);

	    $<iface>$ = new CFEInterface($1, *$3, $4,
		driver.getCurrentContext());
	    driver.setCurrentContext($<iface>$);
	} interface_component_list RBRACE
	{
	    $$ = $<iface>6;
	    $$->AddComponents($7);
	    $$->m_sourceLoc = @$;
	    driver.leaveCurrentContext();
	}
	| INTERFACE scoped_name inheritance_spec LBRACE
	{
	    if (driver.trace_scanning)
		std::cerr << "C-Parser: INTERFACE checking " << *$2 << "\n";
	    // check if identifier is already used as class id (structs are
	    // classes)
	    if (driver.check_token(*$2, dice::parser::CSymbolTable::CLASS))
		driver.error(@2, "Interface ID \"" + *$2 + "\" already defined.");
	    // add aliases to symbol table
	    driver.add_token(*$2, dice::parser::CSymbolTable::CLASS, (CFEBase*)0,
		*@2.begin.filename, @2.begin.line, @2.begin.column);

	    $<iface>$ = new CFEInterface(NULL, *$2, $3,
		driver.getCurrentContext());
	    driver.setCurrentContext($<iface>$);
	} interface_component_list RBRACE
	{
	    $$ = $<iface>5;
	    $$->AddComponents($6);
	    $$->m_sourceLoc = @$;
	    driver.leaveCurrentContext();
	}
	| INTERFACE scoped_name
	{
	    // declaration of an not yet defined or external or following
	    // interface.  we simply define the name of the interface in the
	    // current scope.
	    if (driver.trace_scanning)
		std::cerr << "C-Parser: INTERFACE checking " << *$2 << "\n";
	    // check if identifier is already used as class id
	    if (driver.check_token(*$2, dice::parser::CSymbolTable::CLASS))
		driver.error(@2, "Interface ID \"" + *$2 + "\" already defined.");
	    // add aliases to symbol table
	    driver.add_token(*$2, dice::parser::CSymbolTable::CLASS, (CFEBase*)0,
		*@2.begin.filename, @2.begin.line, @2.begin.column);

	    $$ = NULL;
	}
	;

interface_attribute :
	  UUID LPAREN const_expr RPAREN
	{
	    $$ = new CFEIntAttribute(ATTR_UUID, $3->GetIntValue());
	    $$->m_sourceLoc = @$;
	}
	| VERSION_ATTR LPAREN VERSION_STR RPAREN
	{
		version_t t;
		std::string::size_type pos = $3->find('.');
		if (pos == std::string::npos)
			pos = $3->find(',');
		if (pos != std::string::npos)
		{
			t.nMajor = std::atoi($3->substr(0, pos).c_str());
			t.nMinor = std::atoi($3->substr(pos).c_str());
		}
		else
		{
			t.nMajor = std::atoi($3->c_str());
			t.nMinor = 0;
		}
		$$ = new CFEVersionAttribute(t);
		$$->m_sourceLoc = @$;
	}
	| ENDPOINT LPAREN PORTSPEC RPAREN
	{
	    $$ = new CFEEndPointAttribute($3);
	    $$->m_sourceLoc = @$;
	}
	| EXCEPTIONS LPAREN excep_name_list RPAREN
	{
	    $$ = new CFEExceptionAttribute($3);
	    $$->m_sourceLoc = @$;
	}
	| LOCAL
	{
	    $$ = new CFEAttribute(ATTR_LOCAL);
	    $$->m_sourceLoc = @$;
	}
	| INIT_RCVSTRING
	{
	    $$ = new CFEAttribute(ATTR_INIT_RCVSTRING);
	    $$->m_sourceLoc = @$;
	}
	| INIT_RCVSTRING LPAREN ID RPAREN
	{
	    $$ = new CFEStringAttribute(ATTR_INIT_RCVSTRING, *$3);
	    $$->m_sourceLoc = @$;
	}
	| INIT_RCVSTRING_CLIENT LPAREN ID RPAREN
	{
	    $$ = new CFEStringAttribute(ATTR_INIT_RCVSTRING_CLIENT, *$3);
	    $$->m_sourceLoc = @$;
	}
	| INIT_RCVSTRING_SERVER LPAREN ID RPAREN
	{
	    $$ = new CFEStringAttribute(ATTR_INIT_RCVSTRING_SERVER, *$3);
	    $$->m_sourceLoc = @$;
	}
	| DEFAULT_FUNCTION LPAREN ID RPAREN
	{
	    $$ = new CFEStringAttribute(ATTR_DEFAULT_FUNCTION, *$3);
	    $$->m_sourceLoc = @$;
	}
	| ERROR_FUNCTION LPAREN ID RPAREN
	{
	    $$ = new CFEStringAttribute(ATTR_ERROR_FUNCTION, *$3);
	    $$->m_sourceLoc = @$;
	}
	| ERROR_FUNCTION_CLIENT LPAREN ID RPAREN
	{
	    $$ = new CFEStringAttribute(ATTR_ERROR_FUNCTION_CLIENT, *$3);
	    $$->m_sourceLoc = @$;
	}
	| ERROR_FUNCTION_SERVER LPAREN ID RPAREN
	{
	    $$ = new CFEStringAttribute(ATTR_ERROR_FUNCTION_SERVER, *$3);
	    $$->m_sourceLoc = @$;
	}
	| POINTER_DEFAULT LPAREN ptr_attr RPAREN
	{
	    $$ = new CFEPtrDefaultAttribute($3);
	    $$->m_sourceLoc = @$;
	}
	| OBJECT
	{
	    $$ = new CFEAttribute(ATTR_OBJECT);
	    $$->m_sourceLoc = @$;
	}
	| ABSTRACT
	{
	    $$ = new CFEAttribute(ATTR_ABSTRACT);
	    $$->m_sourceLoc = @$;
	}
	| DEDICATED_PARTNER
	{
	    $$ = new CFEAttribute(ATTR_DEDICATED_PARTNER);
	    $$->m_sourceLoc = @$;
	}
	| DEFAULT_TIMEOUT
	{
	    $$ = new CFEAttribute(ATTR_DEFAULT_TIMEOUT);
	    $$->m_sourceLoc = @$;
	}
	;

inheritance_spec :
	  COLON base_interface_list
	{
	    $$ = $2;
	}
	| /* empty */
	{
	    $$ = NULL;
	}
	;

base_interface_list :
	  base_interface_list COMMA base_interface
	{
	    $$ = $1;
	    if ($3)
	    {
		CFEIdentifier *tId = new CFEIdentifier(*$3);
		tId->m_sourceLoc = @3;
		$$->push_back(tId);
	    }
	}
	| base_interface
	{
	    $$ = new vector<CFEIdentifier*>();
	    if ($1)
	    {
		CFEIdentifier *tId = new CFEIdentifier(*$1);
		tId->m_sourceLoc = @1;
		$$->push_back(tId);
	    }
	}
	;

base_interface :
	  scoped_name
	;

interface_component_list :
	  interface_component_list interface_component
	{
	    $$ = $1;
	    if ($2)
		$$->push_back($2);
	}
	| /* empty */
	{
	    $$ = new vector<CFEInterfaceComponent*>();
	}
	;

interface_component :
	  export
	{
	    // export is already added to context
	    $$ = NULL;
	}
	| op_declarator SEMICOLON
	{ $$ = $1; }
	| attr_declarator SEMICOLON
	{ $$ = $1; }
	/* ignore CORBA's value declaration */
	;

type_declarator :
	  TYPEDEF type_attributes type_spec declarator_list
	{
	    // add type to symbol table
	    if ($4)
	    {
		vector<CFEDeclarator*>::iterator i;
		for (i = $4->begin(); i != $4->end(); i++)
		{
		    std::string sName = (*i)->GetName();
		    if (driver.trace_scanning)
			std::cerr << "C-Parser: TYPEDEF checking " << sName << "\n";
		    // check if any of the aliases has been declared before (as type)
		    if (driver.check_token(sName, dice::parser::CSymbolTable::TYPENAME))
			driver.error(@4, "Type \"" + sName + "\" already defined.");
		    // add aliases to symbol table
		    driver.add_token(sName, dice::parser::CSymbolTable::TYPENAME, (CFEBase*)0,
			*@4.begin.filename, @4.begin.line, @4.begin.column);
		}
	    }
	    $$ = new CFETypedDeclarator(TYPEDECL_TYPEDEF, $3, $4, $2);
	    $$->m_sourceLoc = @$;
	    $3->SetParent($$);
	}
	| TYPEDEF type_spec declarator_list
	{
	    // add type to symbol table
	    if ($3)
	    {
		vector<CFEDeclarator*>::iterator i;
		for (i = $3->begin(); i != $3->end(); i++)
		{
		    std::string sName = (*i)->GetName();
		    if (driver.trace_scanning)
			std::cerr << "C-Parser: TYPEDEF checking " << sName << "\n";
		    // check if any of the aliases has been declared before (as type)
		    if (driver.check_token(sName, dice::parser::CSymbolTable::TYPENAME))
			driver.error(@3, "Type \"" + sName + "\" already defined.");
		    // add aliases to symbol table
		    driver.add_token(sName, dice::parser::CSymbolTable::TYPENAME, (CFEBase*)0,
			*@3.begin.filename, @3.begin.line, @3.begin.column);
		}
	    }
	    $$ = new CFETypedDeclarator(TYPEDECL_TYPEDEF, $2, $3);
	    $$->m_sourceLoc = @$;
	    $2->SetParent($$);
	}
	/* CORBA specifics */
	| constructed_type_spec
	{
	    // extract the name
	    std::string sPre;
	    if (dynamic_cast<CFEStructType*>($1))
		sPre = "_struct_";
	    else if (dynamic_cast<CFEUnionType*>($1))
		sPre = "_union_";
	    else if (dynamic_cast<CFEEnumType*>($1))
		sPre = "_enum_";
	    CFEDeclarator *tDecl =  new CFEDeclarator(DECL_IDENTIFIER, sPre + $1->GetTag());
	    // modify tag, so compiler won't warn
	    vector<CFEDeclarator*> *tVecD = new vector<CFEDeclarator*>();
	    tVecD->push_back(tDecl);
	    // create a new typed decl
	    $$ = new CFETypedDeclarator(TYPEDECL_TYPEDEF, $1, tVecD);
	    $$->m_sourceLoc = @$;
	    $1->SetParent($$);
	    tDecl->SetParent($$);
        }
	| NATIVE ID
	{
	    // "native" make types of the taget language known in the IDL. Thus
	    // the name after "native" specifies a type. Make this type known in
	    // this scope. We cannot add a type, e.g., user-defined type, to the
	    // current scope because we cannot make a typedef without an original
	    // type and a 'typedef ID ID' is not really clever.
	    if (driver.trace_scanning)
		std::cerr << "C-Parser: TYPEDEF checking " << *$2 << "\n";
	    // check if any of the aliases has been declared before (as type)
	    if (driver.check_token(*$2, dice::parser::CSymbolTable::TYPENAME))
		driver.error(@2, "Type \"" + *$2 + "\" already defined.");
	    // add aliases to symbol table
	    driver.add_token(*$2, dice::parser::CSymbolTable::TYPENAME, (CFEBase*)0,
		*@2.begin.filename, @2.begin.line, @2.begin.column);
	}
	;

type_attributes :
	  { driver.expect_attr = true; } LBRACKET type_attribute_list { driver.expect_attr = false; } RBRACKET
	{
	    $$ = $3;
	}
	;

type_attribute_list :
	  type_attribute_list COMMA type_attribute
	{
	    $$ = $1;
	    if ($3)
		$$->push_back($3);
	}
	| type_attribute
	{
	    $$ = new vector<CFEAttribute*>();
	    if ($1)
		$$->push_back($1);
	}
	;

type_attribute :
	  TRANSMIT_AS LPAREN simple_type_spec RPAREN
	{
	    $$ = new CFETypeAttribute(ATTR_TRANSMIT_AS, $3);
	    $$->m_sourceLoc = @$;
	    $3->SetParent($$);
	}
	| HANDLE
	{
	    $$ = new CFEAttribute(ATTR_HANDLE);
	    $$->m_sourceLoc = @$;
	}
	| usage_attribute
	| union_type_switch_attr
	| ptr_attr
	;

const_declarator :
	  CONST const_type_spec ID IS const_expr
	{
	    $$ = new CFEConstDeclarator($2, *$3, $5);
	    $$->m_sourceLoc = @$;
	    $2->SetParent($$);
	    $5->SetParent($$);
	}
	;

const_type_spec :
	  integer_type
	| CHAR
	{
	    $$ = new CFESimpleType(TYPE_CHAR);
	    $$->m_sourceLoc = @$;
	}
	| BOOLEAN
	{
	    $$ = new CFESimpleType(TYPE_BOOLEAN);
	    $$->m_sourceLoc = @$;
	}
	| CHAR_PTR
	{
	    $$ = new CFESimpleType(TYPE_CHAR_ASTERISK);
	    $$->m_sourceLoc = @$;
	}
	| VOID_PTR
	{
	    $$ = new CFESimpleType(TYPE_VOID_ASTERISK);
	    $$->m_sourceLoc = @$;
	}
	/* CORBA specification: */
	| WCHAR
	{
	    $$ = new CFESimpleType(TYPE_WCHAR);
	    $$->m_sourceLoc = @$;
	}
	| OCTET
	{
	    $$ = new CFESimpleType(TYPE_OCTET);
	    $$->m_sourceLoc = @$;
	}
	| string_type
	| floating_pt_type
	| fixed_type
	| scoped_name
	{
	    $$ = new CFEUserDefinedType(*$1);
	    $$->m_sourceLoc = @$;
	}
	;

tagged_declarator :
	  tagged_struct_declarator
	{ $$ = $1; }
	| tagged_union_declarator
	{ $$ = $1; }
	| tagged_enumeration_type
	{ $$ = $1; }
	;

exception_declarator :
	  EXCEPTION ID LBRACE member_list RBRACE
	{
	    CFEStructType *tType = new CFEStructType(*$2, $4);

	    CFEDeclarator *tDecl = new CFEDeclarator(DECL_IDENTIFIER, *$2);
	    vector<CFEDeclarator*> *tVecD = new vector<CFEDeclarator*>();
	    tVecD->push_back(tDecl);

	    $$ = new CFETypedDeclarator(TYPEDECL_EXCEPTION, tType, tVecD);
	    $$->m_sourceLoc = @$;
	    tType->SetParent($$);
	    tDecl->SetParent($$);
	}
	| EXCEPTION ID LBRACE RBRACE
	{
	    CFESimpleType *tType = new CFESimpleType(TYPE_INTEGER, false, true, 4/*value for LONG*/, false);

	    CFEDeclarator *tDecl = new CFEDeclarator(DECL_IDENTIFIER, *$2);
	    vector<CFEDeclarator*> *tVecD = new vector<CFEDeclarator*>();
	    tVecD->push_back(tDecl);

	    $$ = new CFETypedDeclarator(TYPEDECL_EXCEPTION, tType, tVecD);
	    $$->m_sourceLoc = @$;
	    tDecl->SetParent($$);
	    tType->SetParent($$);
	}
	;

op_declarator :
	  operation_attributes simple_type_spec ID LPAREN param_declarators RPAREN raises_declarator context_expr
	{
	    // ignore context
	    $$ = new CFEOperation($2, *$3, $5, $1, $7);
	    $$->m_sourceLoc = @$;
	    $2->SetParent($$);
	}
	| operation_attributes simple_type_spec ID LPAREN VOID RPAREN raises_declarator context_expr
	{
	    // ignore context
	    $$ = new CFEOperation($2, *$3, NULL, $1, $7);
	    $$->m_sourceLoc = @$;
	    $2->SetParent($$);
	}
	| operation_attributes simple_type_spec ID LPAREN param_declarators RPAREN context_expr
	{
	    // ignore context
	    $$ = new CFEOperation($2, *$3, $5, $1);
	    $$->m_sourceLoc = @$;
	    $2->SetParent($$);
	}
	| operation_attributes simple_type_spec ID LPAREN VOID RPAREN context_expr
	{
	    // ignore context
	    $$ = new CFEOperation($2, *$3, NULL, $1);
	    $$->m_sourceLoc = @$;
	    $2->SetParent($$);
	}
	| simple_type_spec ID LPAREN param_declarators RPAREN raises_declarator context_expr
	{
	    // ignore context
	    $$ = new CFEOperation($1, *$2, $4, NULL, $6);
	    $$->m_sourceLoc = @$;
	    $1->SetParent($$);
	}
	| simple_type_spec ID LPAREN VOID RPAREN raises_declarator context_expr
	{
	    // ignore context
	    $$ = new CFEOperation($1, *$2, NULL, NULL, $6);
	    $$->m_sourceLoc = @$;
	    $1->SetParent($$);
	}
	| simple_type_spec ID LPAREN param_declarators RPAREN context_expr
	{
	    // ignore context
	    $$ = new CFEOperation($1, *$2, $4);
	    $$->m_sourceLoc = @$;
	    $1->SetParent($$);
	}
	| simple_type_spec ID LPAREN VOID RPAREN context_expr
	{
	    // ignore context
	    $$ = new CFEOperation($1, *$2, NULL);
	    $$->m_sourceLoc = @$;
	    $1->SetParent($$);
	}
	;

raises_declarator :
	  RAISES LPAREN identifier_list RPAREN
	{
	    $$ = $3;
	}
	;

context_expr :
	  CONTEXT LPAREN lit_str_list RPAREN
	| /* empty */
	;

lit_str_list :
	  lit_str_list COMMA string
	{
	    if ($3)
		delete $3;
	}
	| string
	{
	    if ($1)
		delete $1;
	}
	;

operation_attributes :
	  /* ignore CORBA specific attribute "oneway" without brackets */
	  { driver.expect_attr = true; } LBRACKET operation_attribute_list { driver.expect_attr = false; } RBRACKET
	{
	    $$ = $3;
	}
	;

operation_attribute_list :
	  operation_attribute_list COMMA operation_attribute
	{
	    $$ = $1;
	    if ($3)
		$$->push_back($3);
	}
	| operation_attribute
	{
	    $$ = new vector<CFEAttribute*>();
	    if ($1)
		$$->push_back($1);
	}
	;

operation_attribute :
	  IDEMPOTENT
	{
	    $$ = new CFEAttribute(ATTR_IDEMPOTENT);
	    $$->m_sourceLoc = @$;
	}
	| BROADCAST
	{
	    $$ = new CFEAttribute(ATTR_BROADCAST);
	    $$->m_sourceLoc = @$;
	}
	| MAYBE
	{
	    $$ = new CFEAttribute(ATTR_MAYBE);
	    $$->m_sourceLoc = @$;
	}
	| REFLECT_DELETIONS
	{
	    $$ = new CFEAttribute(ATTR_REFLECT_DELETIONS);
	    $$->m_sourceLoc = @$;
	}
	| UUID LPAREN const_expr uuid_absolute RPAREN
	{
	    $$ = new CFEIntAttribute(ATTR_UUID, $3->GetIntValue(), $4);
	    $$->m_sourceLoc = @$;
	}
	| UUID LPAREN const_expr DOTDOT const_expr uuid_absolute RPAREN
	{
		$$ = new CFERangeAttribute(ATTR_UUID_RANGE, $3->GetIntValue(), $5->GetIntValue(), $6);
		$$->m_sourceLoc = @$;
	}
	| usage_attribute
	| ptr_attr
	| directional_attribute
	| ONEWAY
	{
	    $$ = new CFEAttribute(ATTR_IN);
	    $$->m_sourceLoc = @$;
	}
	| CALLBACK
	{
	    $$ = new CFEAttribute(ATTR_OUT);
	    $$->m_sourceLoc = @$;
	}
	| NOOPCODE
	{
	    $$ = new CFEAttribute(ATTR_NOOPCODE);
	    $$->m_sourceLoc = @$;
	}
	| NOEXCEPTIONS
	{
	    $$ = new CFEAttribute(ATTR_NOEXCEPTIONS);
	    $$->m_sourceLoc = @$;
	}
	| ALLOW_REPLY_ONLY
	{
	    $$ = new CFEAttribute(ATTR_ALLOW_REPLY_ONLY);
	    $$->m_sourceLoc = @$;
	}
	| SCHED_DONATE
	{
	    $$ = new CFEAttribute(ATTR_SCHED_DONATE);
	    $$->m_sourceLoc = @$;
	}
	| DEFAULT_TIMEOUT
	{
	    $$ = new CFEAttribute(ATTR_DEFAULT_TIMEOUT);
	    $$->m_sourceLoc = @$;
	}
	;

uuid_absolute :
	  /* empty */
	{
		$$ = 0;
	}
	| COMMA ABS
	{
		$$ = 1;
	}
	;

usage_attribute :
	  STRING
	{
	    $$ = new CFEAttribute(ATTR_STRING);
	    $$->m_sourceLoc = @$;
	}
	| CONTEXT_HANDLE
	{
	    $$ = new CFEAttribute(ATTR_CONTEXT_HANDLE);
	    $$->m_sourceLoc = @$;
	}
	;

union_type_switch_attr :
	  SWITCH_TYPE LPAREN switch_type_spec RPAREN
	{
	    $$ = new CFETypeAttribute(ATTR_SWITCH_TYPE, $3);
	    $$->m_sourceLoc = @$;
	    $3->SetParent($$);
	}
	;

directional_attribute :
	  IN
	{
	    $$ = new CFEAttribute(ATTR_IN);
	    $$->m_sourceLoc = @$;
	}
	| OUT
	{
	    $$ = new CFEAttribute(ATTR_OUT);
	    $$->m_sourceLoc = @$;
	}
	;

param_declarators :
	  /* empty */
	{
	$$ = NULL;
	}
	| param_declarator_list
	;

param_declarator_list :
	  param_declarator_list COMMA param_declarator
	{
	    $$ = $1;
	    if ($3)
		$$->push_back($3);
	}
	| param_declarator
	{
	    $$ = new vector<CFETypedDeclarator*>();
	    if ($1)
		$$->push_back($1);
	}
	;

param_declarator :
	  param_attributes type_spec declarator
	{
	    if ($2)
	    {
		vector<CFEDeclarator*> *tVecD = new vector<CFEDeclarator*>();
		tVecD->push_back($3);

		$$ = new CFETypedDeclarator(TYPEDECL_PARAM, $2, tVecD, $1);
		$$->m_sourceLoc = @$;
		$2->SetParent($$);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| type_spec declarator
	{
	    if ($1)
	    {
		CFEAttribute *tAttr = new CFEAttribute(ATTR_NONE);
		vector<CFEAttribute*> *tVecA = new vector<CFEAttribute*>();
		tVecA->push_back(tAttr);

		vector<CFEDeclarator*> *tVecD = new vector<CFEDeclarator*>();
		tVecD->push_back($2);

		$$ = new CFETypedDeclarator(TYPEDECL_PARAM, $1, tVecD, tVecA);
		$$->m_sourceLoc = @$;
		tAttr->SetParent($$);
		$2->SetParent($$);
		$1->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	;

param_attributes :
	  { driver.expect_attr = true; } LBRACKET param_attribute_list { driver.expect_attr = false; } RBRACKET
	{
	    $$ = $3;
	}
	/* directly declared CORBA attributes */
	| IN
	{
	    $$ = new vector<CFEAttribute*>();
	    CFEAttribute *tAttr = new CFEAttribute(ATTR_IN);
	    tAttr->m_sourceLoc = @1;
	    $$->push_back(tAttr);
	}
	| OUT
	{
	    $$ = new vector<CFEAttribute*>();
	    CFEAttribute *tAttr = new CFEAttribute(ATTR_OUT);
	    tAttr->m_sourceLoc = @1;
	    $$->push_back(tAttr);
	}
	| INOUT
	{
	    $$ = new vector<CFEAttribute*>();
	    CFEAttribute *tAttr = new CFEAttribute(ATTR_IN);
	    tAttr->m_sourceLoc = @1;
	    $$->push_back(tAttr);
	    tAttr = new CFEAttribute(ATTR_OUT);
	    tAttr->m_sourceLoc = @1;
	    $$->push_back(tAttr);
	}
	;

param_attribute_list :
	  param_attribute_list COMMA param_attribute
	{
	    $$ = $1;
	    if ($3)
		$$->push_back($3);
	}
	| param_attribute_list COMMA INOUT
	{
	    /* special handling of [inout] because we emit two attribute objects */
	    $$ = $1;
	    CFEAttribute *tAttr = new CFEAttribute(ATTR_IN);
	    tAttr->m_sourceLoc = @3;
	    $$->push_back(tAttr);
	    tAttr = new CFEAttribute(ATTR_OUT);
	    tAttr->m_sourceLoc = @3;
	    $$->push_back(tAttr);
	}
	| param_attribute
	{
	    $$ = new vector<CFEAttribute*>();
	    if ($1)
		$$->push_back($1);
	}
	| INOUT
	{
	    /* special handling of [inout] because we emit two attribute objects */
	    $$ = new vector<CFEAttribute*>();
	    CFEAttribute *tAttr = new CFEAttribute(ATTR_IN);
	    tAttr->m_sourceLoc = @1;
	    $$->push_back(tAttr);
	    tAttr = new CFEAttribute(ATTR_OUT);
	    tAttr->m_sourceLoc = @1;
	    $$->push_back(tAttr);
	}
	| /* empty */
	{
	    $$ = new vector<CFEAttribute*>();
	    CFEAttribute *tAttr = new CFEAttribute(ATTR_NONE);
	    $$->push_back(tAttr);
	}
	;

param_attribute :
	  directional_attribute
	| field_attribute
	| PREALLOC_CLIENT
	{
	    $$ = new CFEAttribute(ATTR_PREALLOC_CLIENT);
	    $$->m_sourceLoc = @$;
	}
	| PREALLOC_SERVER
	{
	    $$ = new CFEAttribute(ATTR_PREALLOC_SERVER);
	    $$->m_sourceLoc = @$;
	}
	| TRANSMIT_AS LPAREN simple_type_spec RPAREN
	{
	    $$ = new CFETypeAttribute(ATTR_TRANSMIT_AS, $3);
	    $$->m_sourceLoc = @$;
	    $3->SetParent($$);
	}
	;

type_spec :
	  simple_type_spec
	| constructed_type_spec
	{ $$ = $1; }
	;

simple_type_spec :
	  base_type_spec
	| predefined_type_spec
	| TYPENAME
	{
	    $$ = new CFEUserDefinedType(*$1);
	    $$->m_sourceLoc = @$;
	}
	/* CORBA specification: */
	| template_type_spec
	;

declarator_list :
	  declarator_list COMMA declarator
	{
	    $$ = $1;
	    if ($3)
		$$->push_back($3);
	}
	| declarator
	{
	    $$ = new vector<CFEDeclarator*>();
	    if ($1)
		$$->push_back($1);
	}
	;

declarator :
	  pointer attribute_list direct_declarator
	{
	    $$ = $3;
	    $$->SetStars($1);
	    $$->m_sourceLoc = @$;
	}
	| pointer attribute_list direct_declarator COLON const_expr
	{
		$$ = $3;
		if ($5)
			$$->SetBitfields($5->GetIntValue());
		$$->SetStars($1);
		$$->m_sourceLoc = @$;
	}
	| attribute_list direct_declarator
	{
	    $$ = $2;
	}
	| attribute_list direct_declarator COLON const_expr
	{
		$$ = $2;
		if ($4)
			$$->SetBitfields($4->GetIntValue());
		$$->m_sourceLoc = @$;
	}
	;

direct_declarator :
	  ID
	{
	    $$ = new CFEDeclarator(DECL_IDENTIFIER, *$1);
	    $$->m_sourceLoc = @$;
	}
	| LPAREN declarator RPAREN
	{
	    $$ = $2;
	    $$->m_sourceLoc = @$;
	}
	| array_declarator
	{ $$ = $1; }
	| function_declarator
	;

array_declarator :
	  direct_declarator LBRACKET RBRACKET
	{
	    $$ = dynamic_cast<CFEArrayDeclarator*>($1);
	    if (!$$)
	    {
		$$ = new CFEArrayDeclarator($1);
		delete $1;
	    }
	    $$->AddBounds(0, 0);
	    $$->m_sourceLoc = @$;
	}
	| direct_declarator LBRACKET array_bound RBRACKET
	{
	    $$ = dynamic_cast<CFEArrayDeclarator*>($1);
	    if (!$$)
	    {
		$$ = new CFEArrayDeclarator($1);
		delete $1;
	    }
	    $$->AddBounds(0, $3);
	    $$->m_sourceLoc = @$;
	    $3->SetParent($$);
	}
	| direct_declarator LBRACKET array_bound DOTDOT RBRACKET
	{
	    $$ = dynamic_cast<CFEArrayDeclarator*>($1);
	    if (!$$)
	    {
		$$ = new CFEArrayDeclarator($1);
		delete $1;
	    }
	    CFEExpression *tExpr = new CFEExpression(static_cast<signed char>('*'));
	    tExpr->m_sourceLoc = @4;
	    $$->AddBounds($3, tExpr);
	    $$->m_sourceLoc = @$;
	    $3->SetParent($$);
	    tExpr->SetParent($$);
	}
	| direct_declarator LBRACKET array_bound DOTDOT array_bound RBRACKET
	{
	    $$ = dynamic_cast<CFEArrayDeclarator*>($1);
	    if (!$$)
	    {
		$$ = new CFEArrayDeclarator($1);
		delete $1;
	    }
	    $$->AddBounds($3, $5);
	    $$->m_sourceLoc = @$;
	    $3->SetParent($$);
	    $5->SetParent($$);
	}
	;

array_bound :
	  const_expr
	| ASTERISK
	{
	    $$ = new CFEExpression(static_cast<signed char>('*'));
	    $$->m_sourceLoc = @$;
	}
	;

function_declarator :
	  direct_declarator LPAREN param_declarators RPAREN
	{
	    $$ = new CFEFunctionDeclarator($1, $3);
	    $$->m_sourceLoc = @$;
	}
	| direct_declarator LPAREN VOID RPAREN
	{
	    $$ = new CFEFunctionDeclarator($1, NULL);
	    $$->m_sourceLoc = @$;
	}
	;

predefined_type_spec :
	  ERROR_STATUS_T
	{
	    $$ = new CFESimpleType(TYPE_ERROR_STATUS_T);
	    $$->m_sourceLoc = @$;
	}
	| FLEXPAGE
	{
	    $$ = new CFESimpleType(TYPE_FLEXPAGE);
	    $$->m_sourceLoc = @$;
	}
	| ISO_LATIN_1
	{
	    $$ = new CFESimpleType(TYPE_ISO_LATIN_1);
	    $$->m_sourceLoc = @$;
	}
	| ISO_MULTI_LINGUAL
	{
	    $$ = new CFESimpleType(TYPE_ISO_MULTILINGUAL);
	    $$->m_sourceLoc = @$;
	}
	| ISO_UCS
	{
	    $$ = new CFESimpleType(TYPE_ISO_UCS);
	    $$->m_sourceLoc = @$;
	}
	;

base_type_spec :
	  floating_pt_type
	| integer_type
	| char_type
	{ $$ = $1; }
	| boolean_type
	| BYTE
	{
	    $$ = new CFESimpleType(TYPE_BYTE);
	    $$->m_sourceLoc = @$;
	}
	| VOID
	{
	    $$ = new CFESimpleType(TYPE_VOID);
	    $$->m_sourceLoc = @$;
	}
	| HANDLE_T
	{
	    $$ = new CFESimpleType(TYPE_HANDLE_T);
	    $$->m_sourceLoc = @$;
	}
	| TYPEOF LPAREN const_expr RPAREN
	{
	    $$ = new CFETypeOfType($3);
	    $$->m_sourceLoc = @$;
	}
	| VOID_PTR
	{
	    $$ = new CFESimpleType(TYPE_VOID_ASTERISK);
	    $$->m_sourceLoc = @$;
	}
	| CHAR_PTR
	{
	    $$ = new CFESimpleType(TYPE_CHAR_ASTERISK);
	    $$->m_sourceLoc = @$;
	}
	/* CORBA specification: */
	| OCTET
	{
	    $$ = new CFESimpleType(TYPE_OCTET);
	    $$->m_sourceLoc = @$;
	}
	/* ignored: any, object, value_base_type */
	;

floating_pt_type :
	  FLOAT
	{
	    $$ = new CFESimpleType(TYPE_FLOAT);
	    $$->m_sourceLoc = @$;
	}
	| DOUBLE
	{
	    $$ = new CFESimpleType(TYPE_DOUBLE);
	    $$->m_sourceLoc = @$;
	}
	| LONG DOUBLE
	{
	    $$ = new CFESimpleType(TYPE_LONG_DOUBLE);
	    $$->m_sourceLoc = @$;
	}
	;

fixed_type :
	  FIXED LT const_expr COMMA const_expr GT
	{
		int nFirst = $3 ? $3->GetIntValue() : 0;
		int nSecond = $5 ? $5->GetIntValue() : 0;
		$$ = new CFESimpleType(TYPE_FIXED, nFirst, nSecond);
		$$->m_sourceLoc = @$;
		if ($3)
			delete $3;
		if ($5)
			delete $5;
	}
	| FIXED
	{
	    $$ = new CFESimpleType(TYPE_FIXED, 0, 0);
	    $$->m_sourceLoc = @$;
	}
	;

integer_type :
	  SIGNED INT
	{
	    $$ = new CFESimpleType(TYPE_INTEGER, false, false);
	    $$->m_sourceLoc = @$;
	}
	| SIGNED integer_size
	{
		if ($2 == 4) // it was LONG
			$$ = new CFESimpleType(TYPE_LONG, false, false, $2, false);
		else
			$$ = new CFESimpleType(TYPE_INTEGER, false, false, $2, false);
		$$->m_sourceLoc = @$;
	}
	| SIGNED integer_size INT
	{
	    if ($2 == 4) // it was LONG
		$$ = new CFESimpleType(TYPE_LONG, false, false, $2);
	    else
		$$ = new CFESimpleType(TYPE_INTEGER, false, false, $2);
	    $$->m_sourceLoc = @$;
	}
	| UNSIGNED INT
	{
	    $$ = new CFESimpleType(TYPE_INTEGER, true, false);
	    $$->m_sourceLoc = @$;
	}
	| UNSIGNED integer_size
	{
	    if ($2 == 4) // it was LONG
		$$ = new CFESimpleType(TYPE_LONG, true, false, $2, false);
	    else
		$$ = new CFESimpleType(TYPE_INTEGER, true, false, $2, false);
	    $$->m_sourceLoc = @$;
	}
	| UNSIGNED integer_size INT
	{
	    if ($2 == 4) // it was LONG
		$$ = new CFESimpleType(TYPE_LONG, true, false, $2);
	    else
		$$ = new CFESimpleType(TYPE_INTEGER, true, false, $2);
	    $$->m_sourceLoc = @$;
	}
	| INT
	{
	    $$ = new CFESimpleType(TYPE_INTEGER, false, false);
	    $$->m_sourceLoc = @$;
	}
	| integer_size INT
	{
	    if ($1 == 4) // it was LONG
		$$ = new CFESimpleType(TYPE_LONG, false, false, $1);
	    else
		$$ = new CFESimpleType(TYPE_INTEGER, false, false, $1);
	    $$->m_sourceLoc = @$;
	}
	| integer_size
	{
	    if ($1 == 4) // it was LONG
		$$ = new CFESimpleType(TYPE_LONG, false, false, $1, false);
	    else
		$$ = new CFESimpleType(TYPE_INTEGER, false, false, $1, false);
	    $$->m_sourceLoc = @$;
	}
	| SIGNED
	{
	    $$ = new CFESimpleType(TYPE_INTEGER, false);
	    $$->m_sourceLoc = @$;
	}
	| UNSIGNED
	{
	    $$ = new CFESimpleType(TYPE_INTEGER, true);
	    $$->m_sourceLoc = @$;
	}
	;

integer_size :
	  SHORT
	{
	    $$ = 2;
	}
	| LONG
	{
	    $$ = 4;
	}
	| LONGLONG
	{
	    $$ = 8;
	}
	| SMALL
	{
	    $$ = 1;
	}
	| HYPER
	{
	    $$ = 8;
	}
	;

char_type :
	  UNSIGNED char_base
	{
	    $$ = $2;
	    $$->SetUnsigned(true);
	    $$->m_sourceLoc = @$;
	}
	| UNSIGNED CHAR_PTR
	{
	    $$ = new CFESimpleType(TYPE_CHAR_ASTERISK, true);
	    $$->m_sourceLoc = @$;
	}
	| char_base
	| SIGNED char_base
	{
	    $$ = $2;
	    $$->SetUnsigned(false);
	    $$->m_sourceLoc = @$;
	}
	| SIGNED CHAR_PTR
	{
	    $$ = new CFESimpleType(TYPE_CHAR_ASTERISK);
	    $$->m_sourceLoc = @$;
	}
	;

char_base :
	  CHAR
	{
	    $$ = new CFESimpleType(TYPE_CHAR);
	    $$->m_sourceLoc = @$;
	}
	| WCHAR
	{
	    $$ = new CFESimpleType(TYPE_WCHAR);
	    $$->m_sourceLoc = @$;
	}
	;

string_type :
	  STRING LT const_expr GT
	{
	    // according to
	    // - CORBA C Language Mapping 1.12 and 1.13, and
	    // - CORBA C++ Language Mapping 1.7 and 1.8
	    // strings, no matter if bound or unbound
	    // are mapped to char* and wchar* respectively
	    $$ = new CFESimpleType(TYPE_STRING);
	    $$->m_sourceLoc = @$;
	}
	| STRING
	{
	    $$ = new CFESimpleType(TYPE_STRING);
	    $$->m_sourceLoc = @$;
	}
	| WSTRING LT const_expr GT
	{
	    // according to
	    // - CORBA C Language Mapping 1.12 and 1.13, and
	    // - CORBA C++ Language Mapping 1.7 and 1.8
	    // strings, no matter if bound or unbound
	    // are mapped to char* and wchar* respectively
	    $$ = new CFESimpleType(TYPE_WSTRING);
	    $$->m_sourceLoc = @$;
	}
	| WSTRING
	{
	    $$ = new CFESimpleType(TYPE_WSTRING);
	    $$->m_sourceLoc = @$;
	}
	;

boolean_type :
	  BOOLEAN
	{
	    $$ = new CFESimpleType(TYPE_BOOLEAN);
	    $$->m_sourceLoc = @$;
	}
	;

template_type_spec :
	  sequence_type
	| string_type
	| fixed_type
	;

sequence_type :
	  SEQUENCE LT simple_type_spec COMMA const_expr GT
	{
	    $$ = new CFEArrayType($3, $5);
	    $3->SetParent($$);
	    $5->SetParent($$);
	    $$->m_sourceLoc = @$;
	}
	| SEQUENCE LT simple_type_spec GT
	{
	    $$ = new CFEArrayType($3);
	    $3->SetParent($$);
	    $$->m_sourceLoc = @$;
	}
	;

constructed_type_spec :
	  attribute_list STRUCT LBRACE member_list RBRACE
	{
	    $$ = new CFEStructType(std::string(), $4);
	    $$->AddAttributes($1);
	    $$->m_sourceLoc = @$;
	}
	| union_type
	{ $$ = $1; }
	| enumeration_type
	{ $$ = $1; }
	| tagged_declarator
	{ $$ = $1; }
	| PIPE type_spec
	{
	    $$ = new CFEPipeType($2);
	    $$->m_sourceLoc = @$;
	    $2->SetParent($$);
	}
	;

tagged_struct_declarator :
	  attribute_list STRUCT tag LBRACE member_list RBRACE
	{
	    $$ = new CFEStructType(*$3, $5);
	    $$->m_sourceLoc = @$;
	    $$->AddAttributes($1);
	}
	| attribute_list STRUCT tag
	{
	    $$ = new CFEStructType(*$3, NULL);
	    $$->m_sourceLoc = @$;
	    $$->AddAttributes($1);
	}
	;

tag :
	  ID
	;

member_list :
	  /* empty */
	{
	    $$ = new vector<CFETypedDeclarator*>();
	}
	| member_list_1
	/* no trailing semicolon */
	| member_list_1 member
	{
	    $$ = $1;
	    if ($2)
		$$->push_back($2);
	}
	;

member_list_1 :
	  member_list_1 member SEMICOLON
	{
	    $$ = $1;
	    if ($2)
		$$->push_back($2);
	}
	| member SEMICOLON
	{
	    $$ = new vector<CFETypedDeclarator*>();
	    if ($1)
		$$->push_back($1);
	}
	;

member :
	  field_declarator
	;

tagged_union_declarator :
	  UNION tag
	{
	    $$ = new CFEUnionType(*$2, NULL);
	    $$->m_sourceLoc = @$;
	}
	| UNION tag union_type_header
	{
	    $$ = $3;
	    $$->SetTag(*$2);
	    $$->m_sourceLoc = @$;
	}
	;

union_type :
	  UNION union_type_header
	{
	    $$ = $2;
	    $$->m_sourceLoc = @$;
	}
	;

union_type_header :
	  SWITCH LPAREN switch_type_spec ID RPAREN ID LBRACE union_body RBRACE
	{
	    $$ = new CFEIDLUnionType(std::string(), $8, $3, *$4, *$6);
	    $$->m_sourceLoc = @$;
	    $3->SetParent($$);
	}
	| SWITCH LPAREN switch_type_spec ID RPAREN LBRACE union_body RBRACE
	{
	    $$ = new CFEIDLUnionType(std::string(), $7, $3, *$4, std::string());
	    $$->m_sourceLoc = @$;
	    $3->SetParent($$);
	}
	| LBRACE union_body_n_e RBRACE
	{
	    $$ = new CFEUnionType(std::string(), $2);
	    $$->m_sourceLoc = @$;
	}
	;

switch_type_spec :
	  integer_type
	| char_type
	{ $$ = $1; }
	| boolean_type
	| TYPENAME
	{
	    $$ = new CFEUserDefinedType(*$1);
	    $$->m_sourceLoc = @$;
	}
	| enumeration_type
	{ $$ = $1; }
	| tagged_enumeration_type
	{ $$ = $1; }
	;

union_body :
	  union_body union_case
	{
	    $$ = $1;
	    if ($2)
		$$->push_back($2);
	}
	| union_case
	{
	    $$ = new vector<CFEUnionCase*>();
	    if ($1)
		$$->push_back($1);
	}
	;

union_body_n_e :
	  union_body_n_e union_case_n_e
	{
	    $$ = $1;
	    if ($2)
		$$->push_back($2);
	}
	| union_case_n_e
	{
	    $$ = new vector<CFEUnionCase*>();
	    if ($1)
		$$->push_back($1);
	}
	;

union_case :
	  union_case_label_list union_arm
	{
	    $$ = new CFEUnionCase($2, $1);
	    $$->m_sourceLoc = @$;
	    $2->SetParent($$);
	}
	| DEFAULT COLON union_arm
	{
	    $$ = new CFEUnionCase($3);
	    $$->m_sourceLoc = @$;
	    $3->SetParent($$);
	}
	;

union_case_n_e :
	  LBRACKET CASE LPAREN const_expr_list RPAREN RBRACKET union_arm
	{
	    $$ = new CFEUnionCase($7, $4);
	    $$->m_sourceLoc = @$;
	    $7->SetParent($$);
	}
	| LBRACKET DEFAULT RBRACKET union_arm
	{
	    $$ = new CFEUnionCase($4);
	    $$->m_sourceLoc = @$;
	    $4->SetParent($$);
	}
	| union_arm
	{
	    // support C/C++ unions
	    $$ = new CFEUnionCase($1);
	    $$->m_sourceLoc = @$;
	    $1->SetParent($$);
	}
	;

union_case_label_list :
	  union_case_label_list union_case_label
	{
	    $$ = $1;
	    if ($2)
		$$->push_back($2);
	}
	| union_case_label
	{
	    $$ = new vector<CFEExpression*>();
	    if ($1)
		$$->push_back($1);
	}
	;

union_case_label :
	  CASE const_expr COLON
	{
	    $$ = $2;
	    $$->m_sourceLoc = @$;
	}
	;

union_arm :
	  field_declarator SEMICOLON
	| SEMICOLON
	{
	    $$ = new CFETypedDeclarator(TYPEDECL_VOID, 0, 0);
	    $$->m_sourceLoc = @$;
	}
	;

union_instance_switch_attr :
	  SWITCH_IS LPAREN attr_var RPAREN
	{
	    vector<CFEDeclarator*> *tVecD = new vector<CFEDeclarator*>();
	    tVecD->push_back($3);
	    $$ = new CFEIsAttribute(ATTR_SWITCH_IS, tVecD);
	    $$->m_sourceLoc = @$;
	    $3->SetParent($$);
	}
	;

enumeration_type :
	  ENUM LBRACE enumeration_declarator_list RBRACE
	{
	    // a vector<CFEDeclarator*> is not the same as a
	    // vector<CFEIdentifier*> which is expected. Therefore, create a
	    // new vector and copy the elements.
	    vector<CFEIdentifier*>* tVecI = new vector<CFEIdentifier*>();
	    tVecI->insert(tVecI->begin(), $3->begin(), $3->end());
	    delete $3;
	    $$ = new CFEEnumType(std::string(), tVecI);
	    $$->m_sourceLoc = @$;
	}
	;

tagged_enumeration_type :
	  ENUM ID LBRACE enumeration_declarator_list RBRACE
	{
	    // a vector<CFEDeclarator*> is not the same as a
	    // vector<CFEIdentifier*> which is expected. Therefore, create a
	    // new vector and copy the elements.
	    vector<CFEIdentifier*>* tVecI = new vector<CFEIdentifier*>();
	    tVecI->insert(tVecI->begin(), $4->begin(), $4->end());
	    delete $4;
	    $$ = new CFEEnumType(*$2, tVecI);
	    $$->m_sourceLoc = @$;
	}
	| ENUM ID
	{
	    $$ = new CFEEnumType(*$2, NULL);
	    $$->m_sourceLoc = @$;
	}
	;

enumeration_declarator_list :
	  enumeration_declarator_list COMMA enumeration_declarator
	{
	    $$ = $1;
	    if ($3)
		$$->push_back($3);
	}
	| enumeration_declarator
	{
	    $$ = new vector<CFEDeclarator*>();
	    if ($1)
		$$->push_back($1);
	}
	;

enumeration_declarator :
	  ID
	{
	    $$ = new CFEEnumDeclarator(*$1);
	    $$->m_sourceLoc = @$;
	}
	| ID IS const_expr
	{
	    $$ = new CFEEnumDeclarator(*$1, $3);
	    $$->m_sourceLoc = @$;
	    $3->SetParent($$);
	}
	;

field_declarator :
	  field_attributes type_spec declarator_list
	{
	    $$ = new CFETypedDeclarator(TYPEDECL_FIELD, $2, $3, $1);
	    $$->m_sourceLoc = @$;
	    $2->SetParent($$);
	}
	| type_spec declarator_list
	{
	    $$ = new CFETypedDeclarator(TYPEDECL_FIELD, $1, $2);
	    $$->m_sourceLoc = @$;
	    $1->SetParent($$);
	}
	;

field_attributes :
	  { driver.expect_attr = true; } LBRACKET field_attribute_list { driver.expect_attr = false; } RBRACKET
	{
	    $$ = $3;
	}
	;

field_attribute_list :
	  field_attribute_list COMMA field_attribute
	{
	    $$ = $1;
	    if ($3)
		$$->push_back($3);
	}
	| field_attribute
	{
	    $$ = new vector<CFEAttribute*>();
	    if ($1)
		$$->push_back($1);
	}
	;

field_attribute :
	  FIRST_IS LPAREN attr_var_list RPAREN
	{
	    $$ = new CFEIsAttribute(ATTR_FIRST_IS, $3);
	    $$->m_sourceLoc = @$;
	}
	| FIRST_IS LPAREN LIT_INT RPAREN
	{
	    $$ = new CFEIntAttribute(ATTR_FIRST_IS, $3);
	    $$->m_sourceLoc = @$;
	}
	| LAST_IS LPAREN attr_var_list RPAREN
	{
	    $$ = new CFEIsAttribute(ATTR_LAST_IS, $3);
	    $$->m_sourceLoc = @$;
	}
	| LAST_IS LPAREN LIT_INT RPAREN
	{
	    $$ = new CFEIntAttribute(ATTR_LAST_IS, $3);
	    $$->m_sourceLoc = @$;
	}
	| LENGTH_IS LPAREN attr_var_list RPAREN
	{
	    $$ = new CFEIsAttribute(ATTR_LENGTH_IS, $3);
	    $$->m_sourceLoc = @$;
	}
	| LENGTH_IS LPAREN LIT_INT RPAREN
	{
	    $$ = new CFEIntAttribute(ATTR_LENGTH_IS, $3);
	    $$->m_sourceLoc = @$;
	}
	| MIN_IS LPAREN attr_var_list RPAREN
	{
	    $$ = new CFEIsAttribute(ATTR_MIN_IS, $3);
	    $$->m_sourceLoc = @$;
	}
	| MIN_IS LPAREN LIT_INT RPAREN
	{
	    $$ = new CFEIntAttribute(ATTR_MIN_IS, $3);
	    $$->m_sourceLoc = @$;
	}
	| MAX_IS LPAREN attr_var_list RPAREN
	{
	    $$ = new CFEIsAttribute(ATTR_MAX_IS, $3);
	    $$->m_sourceLoc = @$;
	}
	| MAX_IS LPAREN LIT_INT RPAREN
	{
	    $$ = new CFEIntAttribute(ATTR_MAX_IS, $3);
	    $$->m_sourceLoc = @$;
	}
	| SIZE_IS LPAREN attr_var_list RPAREN
	{
	    $$ = new CFEIsAttribute(ATTR_SIZE_IS, $3);
	    $$->m_sourceLoc = @$;
	}
	| SIZE_IS LPAREN LIT_INT RPAREN
	{
	    $$ = new CFEIntAttribute(ATTR_SIZE_IS, $3);
	    $$->m_sourceLoc = @$;
	}
	| usage_attribute
	| union_instance_switch_attr
	| IGNORE
	{
	    $$ = new CFEAttribute(ATTR_IGNORE);
	    $$->m_sourceLoc = @$;
	}
	| ptr_attr
	;

attr_declarator :
	  READONLY ATTRIBUTE type_spec declarator
	{
	    CFEAttribute *tAttr = new CFEAttribute(ATTR_READONLY);
	    tAttr->m_sourceLoc = @1;
	    vector<CFEAttribute*> *tVecA = new vector<CFEAttribute*>();
	    tVecA->push_back(tAttr);

	    vector<CFEDeclarator*> *tVecD = new vector<CFEDeclarator*>();
	    tVecD->push_back($4);

	    $$ = new CFEAttributeDeclarator($3, tVecD, tVecA);
	    $$->m_sourceLoc = @$;
	    $3->SetParent($$);
	    $4->SetParent($$);
	    tAttr->SetParent($$);
	}
	| ATTRIBUTE type_spec declarator
	{
	    vector<CFEAttribute*> *tVecA = new vector<CFEAttribute*>();

	    vector<CFEDeclarator*> *tVecD = new vector<CFEDeclarator*>();
	    tVecD->push_back($3);

	    $$ = new CFEAttributeDeclarator($2, tVecD, tVecA);
	    $$->m_sourceLoc = @$;
	    $2->SetParent($$);
	    $3->SetParent($$);
	}
	;

const_expr_list :
	  const_expr_list COMMA const_expr
	{
	    $$ = $1;
	    if ($3)
		$$->push_back($3);
	}
	| const_expr
	{
	    $$ = new vector<CFEExpression*>();
	    if ($1)
		$$->push_back($1);
	}
	;

const_expr :
	  conditional_expr
	| EXPNULL
	{
	    $$ = new CFEExpression(EXPR_NULL);
	    $$->m_sourceLoc = @$;
	}
	| TRUE
	{
	    $$ = new CFEExpression(EXPR_TRUE);
	    $$->m_sourceLoc = @$;
	}
	| FALSE
	{
	    $$ = new CFEExpression(EXPR_FALSE);
	    $$->m_sourceLoc = @$;
	}
	/* gcc extension */
	| unary_expr assignment_operator const_expr
	{
	    $$ = new CFEBinaryExpression(EXPR_BINARY, $1, $2, $3);
	    $$->m_sourceLoc = @$;
	    if ($1)
		$1->SetParent($$);
	    if ($3)
		$3->SetParent($$);
	}
	;

assignment_operator :
	  IS
	{
	    $$ = EXPR_ASSIGN;
	}
	| RS_ASSIGN
	{
	    $$ = EXPR_RSHIFT_ASSIGN;
	}
	| LS_ASSIGN
	{
	    $$ = EXPR_LSHIFT_ASSIGN;
	}
	| ADD_ASSIGN
	{
	    $$ = EXPR_PLUS_ASSIGN;
	}
	| SUB_ASSIGN
	{
	    $$ = EXPR_MINUS_ASSIGN;
	}
	| MUL_ASSIGN
	{
	    $$ = EXPR_MUL_ASSIGN;
	}
	| DIV_ASSIGN
	{
	    $$ = EXPR_DIV_ASSIGN;
	}
	| MOD_ASSIGN
	{
	    $$ = EXPR_MOD_ASSIGN;
	}
	| AND_ASSIGN
	{
	    $$ = EXPR_AND_ASSIGN;
	}
	| XOR_ASSIGN
	{
	    $$ = EXPR_XOR_ASSIGN;
	}
	| OR_ASSIGN
	{
	    $$ = EXPR_OR_ASSIGN;
	}
	;

conditional_expr :
	  logical_or_expr
	| logical_or_expr QUESTION const_expr COLON conditional_expr
	{
	    $$ = new CFEConditionalExpression($1, $3, $5);
	    $$->m_sourceLoc = @$;
	    if ($1)
		$1->SetParent($$);
	    if ($3)
		$3->SetParent($$);
	    if ($5)
		$5->SetParent($$);
	}
	| logical_or_expr QUESTION COLON conditional_expr
	{
	    $$ = new CFEConditionalExpression($1, NULL, $4);
	    $$->m_sourceLoc = @$;
	    if ($1)
		$1->SetParent($$);
	    if ($4)
		$4->SetParent($$);
	}
	;

logical_or_expr :
	  logical_and_expr
	| logical_or_expr LOGICALOR logical_and_expr
	{
	    $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LOGOR, $3);
	    $$->m_sourceLoc = @$;
	    if ($1)
		$1->SetParent($$);
	    if ($3)
		$3->SetParent($$);
	}
	;

logical_and_expr :
	  inclusive_or_expr
	| logical_and_expr LOGICALAND inclusive_or_expr
	{
	    $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LOGAND, $3);
	    $$->m_sourceLoc = @$;
	    if ($1)
		$1->SetParent($$);
	    if ($3)
		$3->SetParent($$);
	}
	;

inclusive_or_expr :
	  exclusive_or_expr
	| inclusive_or_expr BITOR exclusive_or_expr
	{
	    $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITOR, $3);
	    $$->m_sourceLoc = @$;
	    if ($1)
		$1->SetParent($$);
	    if ($3)
		$3->SetParent($$);
	}
	;

exclusive_or_expr :
	  and_expr
	| exclusive_or_expr BITXOR and_expr
	{
	    $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITXOR, $3);
	    $$->m_sourceLoc = @$;
	    if ($1)
		$1->SetParent($$);
	    if ($3)
		$3->SetParent($$);
	}
	;

and_expr :
	  equality_expr
	| and_expr BITAND equality_expr
	{
	    $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITAND, $3);
	    $$->m_sourceLoc = @$;
	    if ($1)
		$1->SetParent($$);
	    if ($3)
		$3->SetParent($$);
	}
	;

equality_expr :
	  relational_expr
	| equality_expr EQUAL relational_expr
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_EQUALS, $3);
		$$->m_sourceLoc = @$;
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
	    {
		if ($1)
		    delete $1;
		if ($3)
		    delete $3;
		$$ = NULL;
	    }
	}
	| equality_expr NOTEQUAL relational_expr
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_NOTEQUAL, $3);
		$$->m_sourceLoc = @$;
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
	    {
		if ($1)
		    delete $1;
		if ($3)
		    delete $3;
		$$ = NULL;
	    }
	}
	;

relational_expr :
	  shift_expr
	| relational_expr LTEQUAL shift_expr
	{
	    $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LTEQU, $3);
	    $$->m_sourceLoc = @$;
	    if ($1)
		$1->SetParent($$);
	    if ($3)
		$3->SetParent($$);
	}
	| relational_expr GTEQUAL shift_expr
	{
	    $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_GTEQU, $3);
	    $$->m_sourceLoc = @$;
	    if ($1)
		$1->SetParent($$);
	    if ($3)
		$3->SetParent($$);
	}
	| relational_expr LT shift_expr
	{
	    $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LT, $3);
	    $$->m_sourceLoc = @$;
	    if ($1)
		$1->SetParent($$);
	    if ($3)
		$3->SetParent($$);
	}
	| relational_expr GT shift_expr
	{
	    $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_GT, $3);
	    $$->m_sourceLoc = @$;
	    if ($1)
		$1->SetParent($$);
	    if ($3)
		$3->SetParent($$);
	}
	;

shift_expr :
	  additive_expr
	| shift_expr LSHIFT additive_expr
	{
	    $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LSHIFT, $3);
	    $$->m_sourceLoc = @$;
	    if ($1)
		$1->SetParent($$);
	    if ($3)
		$3->SetParent($$);
	}
	| shift_expr RSHIFT additive_expr
	{
	    $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_RSHIFT, $3);
	    $$->m_sourceLoc = @$;
	    if ($1)
		$1->SetParent($$);
	    if ($3)
		$3->SetParent($$);
	}
	;

additive_expr :
	  multiplicative_expr
	| additive_expr PLUS multiplicative_expr
	{
	    $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_PLUS, $3);
	    $$->m_sourceLoc = @$;
	    if ($1)
		$1->SetParent($$);
	    if ($3)
		$3->SetParent($$);
	}
	| additive_expr MINUS multiplicative_expr
	{
	    $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MINUS, $3);
	    $$->m_sourceLoc = @$;
	    if ($1)
		$1->SetParent($$);
	    if ($3)
		$3->SetParent($$);
	}
	;

multiplicative_expr :
	  /* unary_expr */
	  cast_expr
	| multiplicative_expr ASTERISK unary_expr
	{
	    $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MUL, $3);
	    $$->m_sourceLoc = @$;
	    if ($1)
		$1->SetParent($$);
	    if ($3)
		$3->SetParent($$);
	}
	| multiplicative_expr DIV unary_expr
	{
	    $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_DIV, $3);
	    $$->m_sourceLoc = @$;
	    if ($1)
		$1->SetParent($$);
	    if ($3)
		$3->SetParent($$);
	}
	| multiplicative_expr MOD unary_expr
	{
	    $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MOD, $3);
	    $$->m_sourceLoc = @$;
	    if ($1)
		$1->SetParent($$);
	    if ($3)
		$3->SetParent($$);
	}
	;

unary_expr :
	  postfix_expr
	| unary_operator cast_expr
	{
	    if ($2)
	    {
		$$ = new CFEUnaryExpression(EXPR_UNARY, $1, $2);
		$$->m_sourceLoc = @$;
		$2->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| BITAND ID
	{
	    if ($2)
	    {
		CFEExpression *tmp = new CFEUserDefinedExpression(*$2);
		tmp->m_sourceLoc = @2;
		$$ = new CFEUnaryExpression(EXPR_UNARY, EXPR_BITAND, tmp);
		$$->m_sourceLoc = @$;
		tmp->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| SIZEOF unary_expr
	{
	    if ($2)
	    {
		$$ = new CFESizeOfExpression($2);
		$$->m_sourceLoc = @$;
		$2->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| SIZEOF LPAREN TYPENAME RPAREN
	{
	    if ($3)
	    {
		CFETypeSpec *tmp = new CFEUserDefinedType(*$3);
		tmp->m_sourceLoc = @3;
		$$ = new CFESizeOfExpression(tmp);
		$$->m_sourceLoc = @$;
		tmp->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| SIZEOF LPAREN base_type_spec RPAREN
	{
	    if ($3)
	    {
		$$ = new CFESizeOfExpression($3);
		$$->m_sourceLoc = @$;
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	;

unary_operator :
	  PLUS
	{
	    $$ = EXPR_SPLUS;
	}
	| MINUS
	{
	    $$ = EXPR_SMINUS;
	}
	| TILDE
	{
	    $$ = EXPR_TILDE;
	}
	| EXCLAM
	{
	    $$ = EXPR_EXCLAM;
	}
	;

/* gcc specifics */
postfix_expr :
	  primary_expr
	| postfix_expr LBRACKET const_expr_list RBRACKET
	{
	    $$ = NULL;
	}
	| postfix_expr LPAREN const_expr_list RPAREN
	{
	    $$ = NULL;
	}
	| postfix_expr LPAREN RPAREN
	{
	    $$ = NULL;
	}
	| postfix_expr DOT ID
	{
	    $$ = NULL;
	}
	| postfix_expr PTR_OP ID
	{
	    $$ = NULL;
	}
	| postfix_expr INC_OP
	{
	    $$ = new CFEUnaryExpression(EXPR_UNARY, EXPR_INCR, $1);
	    $$->m_sourceLoc = @$;
	}
	| postfix_expr DEC_OP
	{
	    $$ = new CFEUnaryExpression(EXPR_UNARY, EXPR_DECR, $1);
	    $$->m_sourceLoc = @$;
	}
	;

primary_expr :
	  LIT_INT
	{
	    $$ = new CFEPrimaryExpression(EXPR_INT, static_cast<long int>($1));
	    $$->m_sourceLoc = @$;
	}
	| LIT_LONG
	{
	    $$ = new CFEPrimaryExpression(EXPR_INT, static_cast<long int>($1));
	    $$->m_sourceLoc = @$;
	}
	| LIT_ULONG
	{
	    $$ = new CFEPrimaryExpression(EXPR_UINT, static_cast<unsigned long int>($1));
	    $$->m_sourceLoc = @$;
	}
	| LIT_LLONG
	{
#if HAVE_ATOLL
		$$ = new CFEPrimaryExpression(EXPR_LLONG, static_cast<long long int>($1));
#else
		$$ = new CFEPrimaryExpression(EXPR_INT, static_cast<long int>($1));
#endif
		$$->m_sourceLoc = @$;
	}
	| LIT_ULLONG
	{
#if HAVE_ATOLL
		$$ = new CFEPrimaryExpression(EXPR_ULLONG, static_cast<unsigned long long int>($1));
#else
		$$ = new CFEPrimaryExpression(EXPR_UINT, static_cast<unsigned long int>($1));
#endif
		$$->m_sourceLoc = @$;
	}
	| LIT_FLOAT
	{
	    $$ = new CFEPrimaryExpression(EXPR_FLOAT, static_cast<long double>($1));
	    $$->m_sourceLoc = @$;
	}
	| VERSION_STR
	{
	    $$ = new CFEPrimaryExpression(EXPR_FLOAT, std::atol($1->c_str()));
	    $$->m_sourceLoc = @$;
	}
	| ID
	{
	    $$ = new CFEUserDefinedExpression(*$1);
	    $$->m_sourceLoc = @$;
	}
	| LPAREN const_expr_list RPAREN
	{
	    // extract first const expression if available
	    CFEExpression *pExpr = NULL;
	    if ($2)
	    {
		vector<CFEExpression*>::iterator iter = $2->begin();
		if (iter != $2->end())
		    pExpr = *iter;
	    }
	    if (pExpr)
	    {
		$$ = new CFEPrimaryExpression(EXPR_PAREN, pExpr);
		$$->m_sourceLoc = @$;
		pExpr->SetParent($$);
	    }
	    else
		$$ = NULL;
        }
	/* CORBA specification: */
	| string
	{
	    $$ = new CFEExpression(*$1);
	    $$->m_sourceLoc = @$;
	}
	| LIT_CHAR
	{
	    $$ = new CFEExpression($1);
	    $$->m_sourceLoc = @$;
	}
	| LIT_WCHAR
	{
	    $$ = new CFEExpression($1);
	    $$->m_sourceLoc = @$;
	}
	;

/* gcc specifics */
cast_expr :
	  unary_expr
	| LPAREN TYPENAME RPAREN cast_expr
	{
	    CFETypeSpec *tmp = new CFEUserDefinedType(*$2);
	    tmp->m_sourceLoc = @2;
	    $$ = new CFEUnaryExpression(tmp, $4);
	    $$->m_sourceLoc = @$;
	}
	| LPAREN TYPENAME RPAREN LBRACE RBRACE
	{
	    $$ = NULL;
	}
	| LPAREN TYPENAME RPAREN LBRACE initializer_list RBRACE
	{
	    $$ = NULL;
	}
	| LPAREN TYPENAME RPAREN LBRACE initializer_list COMMA RBRACE
	{
	    $$ = NULL;
	}
	;

initializer_list :
	  initializer_list COMMA initializer
	| initializer
	;

initializer :
	  const_expr
	{
	    if ($1)
		delete $1;
	}
	| LBRACE initializer_list RBRACE
	| LBRACE initializer_list COMMA RBRACE
	;

attribute_list :
	  /* empty */
	{
	    $$ = new vector<CFEAttribute*>();
	}
	| attribute_list attribute
	{
	    $$ = $1;
	    $$->insert($$->end(), $2->begin(), $2->end());
	    delete $2;
	}
	;

attribute :
	  ATTRIBUTE LPAREN LPAREN attributes_list RPAREN RPAREN
	{
	    $$ = $4;
	}
	;

attributes_list :
	  attributes_list COMMA attributes
	{
	    $$ = $1;
	    if ($3)
		$$->push_back($3);
	}
	| attributes
	{
	    $$ = new vector<CFEAttribute*>();
	    if ($1)
		$$->push_back($1);
	}
	;

attributes :
	  /* empty */
	{
	    $$ = NULL;
	}
	| attribute_keyword
	{
	    $$ = new CFEStringAttribute(ATTR_C, *$1);
	    $$->m_sourceLoc = @$;
	}
	| attribute_keyword LPAREN attribute_parameter_list RPAREN
	{
	    $$ = new CFEStringAttribute(ATTR_C, *$1);
	    $$->m_sourceLoc = @$;
	}
	;

/* the following keywords are supported, but we ignore them and only check for
 * tokenzied ones:
 *
 * variable:
 * aligned, mode, nocommon, packed, section, transparent_union, unused,
 * deprecated, vector_size, and weak
 *
 * function:
 * noreturn noinline always_inline pure const nothrow format format_arg
 * no_instrument_function section connstructor destructor used unused
 * deprecated weak malloc alias nonnull regparm
 *
 * type:
 * aligned packed transparent_union unused deprecated may_alias
 */
attribute_keyword :
	  ID
	;

attribute_parameter_list :
	  attribute_parameter_list COMMA attribute_parameter
	| attribute_parameter
	;

attribute_parameter :
	  /* ID covered by expression */
	  const_expr
	{
	    if ($1)
		delete $1;
	}
	;

string :
	  LIT_STR
	| LIT_WSTR
	| QUOT QUOT
	{
	    $$ = new std::string();
	}
	| string LIT_STR
	{
	    $$ = $1;
	    $$->append(*$2);
	}
	| string LIT_WSTR
	{
	    $$ = $1;
	    $$->append(*$2);
	}
	;

%%

void
yy::idl_parser::error (const yy::idl_parser::location_type& l,
  const std::string& m)
{
  driver.error (l, m);
}

/*************************************************************
 * Appendix A: CORBA's value statement, which we ignore
 *************************************************************/
/*
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
    | value_name
    ;

value_name :
      scoped_name
    ;

value_element    :
      export
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
    | init_param_decl
    ;

init_param_decl :
      IN param_type_spec simple_declarator
    ;

*/
/**************************************************************
 * End of Appendix A
 **************************************************************/
