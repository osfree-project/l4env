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

#include "fe/FEFileComponent.h"

void gcc_cxxerror(char *);
#include "parser.h"
int gcc_cxxlex(YYSTYPE*);

#define YYDEBUG	1

// collection for elements
extern CFEFileComponent *pCurFileComponent;

// error stuff
extern int errcount;
extern int warningcount;
extern int erroccured;

// #include/import special treatment
extern String sInFileName;

// indicate which TOKENS should be recognized as tokens
extern int c_inc;
extern int c_inc_old;

// import helper
int nParseErrorGCC_CXX = 0;


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
}

%token EXTENSION	ASM_KEYWORD	NAMESPACE	USING	NSNAME		SCOPE
%token EXTERN_LANG_STRING		TEMPLATE	LTEQUAL GTEQUAL
%token EMPTY		END_OF_SAVED_INPUT		SELFNAME			LEFT_RIGHT
%token RETURN_KEYWORD			IDENTIFIER	TYPENAME	PTYPENAME
%token IDENTIFIER_DEFN	TYPENAME_DEFN	PTYPENAME_DEFN	RSHIFT
%token PLUSPLUS		MINUSMINUS	UNARY		ANDAND		SIZEOF	HYPERUNARY
%token ALIGNOF		REALPART	IMAGPART	POINTSAT_STAR		DOT_STAR
%token LSHIFT		EQUAL	CPP_MIN CPP_MAX		OROR		THROW
%token PFUNCNAME	CONSTANT	THIS		DYNAMIC_CAST
%token STATIC_CAST	REINTERPRET_CAST	CONST_CAST	TYPEID	NEW
%token DELETE	CXX_TRUE	CXX_FALSE	STRING	POINTSAT	TYPEOF	SIGOF
%token ATTRIBUTE	PRE_PARSED_FUNCTION_DECL	DEFARG_MARKER	ENUM
%token CLASS STRUCT UNION ENUM	COMMA	ID	TRY	PAREN_STAR_PAREN	LABEL	IF	ELSE	WHILE
%token DO	FOR	SWITCH	CASE	ELLIPSIS	DEFAULT	BREAK	CONTINUE	GOTO
%token CATCH	DEFARG	OPERATOR

%token LPAREN RPAREN LBRACE RBRACE LBRACKET RBRACKET
%token NAMESPACE USING THROW THIS NEW DELETE
%token OPERATOR CXX_TRUE CXX_FALSE
%token AUTO EXTERN REGISTER STATIC TYPEDEF INLINE
%token CONST VOLATILE RESTRICT BYCOPY BYREF IN OUT INOUT ONEWAY
%token CPP_PLUS_EQ CPP_MINUS_EQ CPP_MULT_EQ CPP_DIV_EQ CPP_MOD_EQ
%token CPP_AND_EQ CPP_OR_EQ CPP_XOR_EQ CPP_RSHIFT_EQ CPP_LSHIFT_EQ
%token CPP_MIN_EQ CPP_MAX_EQ

%token EOF_TOKEN    /* used to end parsing */

%left EMPTY			/* used to resolve s/r with epsilon */

%left error

/* Add precedence rules to solve dangling else s/r conflict */
%nonassoc IF
%nonassoc ELSE

%left IDENTIFIER PFUNCNAME TYPENAME SELFNAME PTYPENAME ENUM CLASS STRUCT UNION ELLIPSIS TYPEOF SIGOF OPERATOR NSNAME

%left LBRACE COMMA SEMICOLON

%nonassoc THROW
%right COLON
%right CPP_PLUS_EQ CPP_MINUS_EQ CPP_MULT_EQ CPP_DIV_EQ CPP_MOD_EQ
%right CPP_AND_EQ CPP_OR_EQ CPP_XOR_EQ CPP_RSHIFT_EQ CPP_LSHIFT_EQ
%right CPP_MIN_EQ CPP_MAX_EQ IS
%right QUESTION
%left OROR
%left ANDAND
%left BITOR
%left BITXOR
%left BITAND
%left CPP_MIN CPP_MAX
%left EQUAL
%left LTEQUAL GTEQUAL LT GT
%left LSHIFT RSHIFT
%left PLUS MINUS
%left MUL DIV MOD
%left POINTSAT_STAR DOT_STAR
%right UNARY PLUSPLUS MINUSMINUS BITNOT
%left HYPERUNARY
%left PAREN_STAR_PAREN LEFT_RIGHT
%left POINTSAT DOT LPAREN LBRACKET

%right SCOPE			/* C++ extension */
%nonassoc NEW DELETE TRY CATCH

/* Used in lex.c for parsing pragmas.  */
%token END_OF_LINE

/* lex.c and pt.c depend on this being the last token.  Define
   any new tokens before this one!  */
%token END_OF_SAVED_INPUT

%%
program:
	  /* empty */
	| extdefs
	;

extdefs:
	  lang_extdef
	| extdefs lang_extdef
	;

extdefs_opt:
	  extdefs
	| /* empty */
	;

.hush_warning:
	;
.warning_ok:
	;

extension:
	EXTENSION
	;

asm_keyword:
	  ASM_KEYWORD
	;

lang_extdef:
	  extdef
	;

extdef:
	  fndef eat_saved_input
	| datadef
	| template_def
	| asm_keyword LPAREN string RPAREN SEMICOLON
	| extern_lang_string LBRACE extdefs_opt RBRACE
	| extern_lang_string .hush_warning fndef .warning_ok eat_saved_input
	| extern_lang_string .hush_warning datadef .warning_ok
	| NAMESPACE identifier LBRACE extdefs_opt RBRACE
	| NAMESPACE LBRACE extdefs_opt RBRACE
	| namespace_alias
	| using_decl SEMICOLON
	| using_directive
	| extension extdef
	| EOF_TOKEN
	{
	    YYACCEPT;
	}
	;

namespace_alias:
    NAMESPACE identifier IS any_id SEMICOLON
	;

using_decl:
	  USING qualified_id
	| USING global_scope qualified_id
	| USING global_scope unqualified_id
	;

namespace_using_decl:
	  USING namespace_qualifier identifier
	| USING global_scope identifier
	| USING global_scope namespace_qualifier identifier
	;

using_directive:
	  USING NAMESPACE any_id SEMICOLON
	;

namespace_qualifier:
	  NSNAME SCOPE
	| namespace_qualifier NSNAME SCOPE
	;

any_id:
	  unqualified_id
	| qualified_id
	| global_scope qualified_id
	| global_scope unqualified_id
	;

extern_lang_string:
	EXTERN_LANG_STRING
	| extern_lang_string EXTERN_LANG_STRING
	;

template_header:
	  TEMPLATE LT template_parm_list GT
	| TEMPLATE LT GT
	;

template_parm_list:
	  template_parm
	| template_parm_list COMMA template_parm
	;

maybe_identifier:
	  identifier
	|	/* empty */
	;		

template_type_parm:
	  aggr maybe_identifier
	| CLASS maybe_identifier
	;

template_template_parm:
	  template_header aggr maybe_identifier
	;

template_parm:
	  template_type_parm
	| template_type_parm IS type_id
	| parm
	| parm IS expr_no_commas  %prec LTEQUAL GTEQUAL
	| template_template_parm
	| template_template_parm IS template_arg
	;

template_def:
	  template_header template_extdef
	| template_header error  %prec EMPTY
	;

template_extdef:
	  fndef eat_saved_input
	| template_datadef
	| template_def
	| extern_lang_string .hush_warning fndef .warning_ok eat_saved_input
	| extern_lang_string .hush_warning template_datadef .warning_ok
	| extension template_extdef
	;

template_datadef:
	  nomods_initdecls SEMICOLON
	| declmods notype_initdecls SEMICOLON
	| typed_declspecs initdecls SEMICOLON
	| structsp SEMICOLON
	;

datadef:
	  nomods_initdecls SEMICOLON
	| declmods notype_initdecls SEMICOLON
	| typed_declspecs initdecls SEMICOLON
    | declmods SEMICOLON
	| explicit_instantiation SEMICOLON
	| typed_declspecs SEMICOLON
	| error SEMICOLON
	| error RBRACE
	| SEMICOLON
	;

ctor_initializer_opt:
	  nodecls
	| base_init
	;

maybe_return_init:
	  /* empty */
	| return_init
	| return_init SEMICOLON
	;

eat_saved_input:
	  /* empty */
	| END_OF_SAVED_INPUT
	;

fndef:
	  fn.def1 maybe_return_init ctor_initializer_opt compstmt_or_error
	| fn.def1 maybe_return_init function_try_block
	| fn.def1 maybe_return_init error
	;

constructor_declarator:
	  nested_name_specifier SELFNAME LPAREN parmlist RPAREN cv_qualifiers exception_specification_opt
	| nested_name_specifier SELFNAME LEFT_RIGHT cv_qualifiers exception_specification_opt
	| global_scope nested_name_specifier SELFNAME LPAREN parmlist RPAREN cv_qualifiers exception_specification_opt
	| global_scope nested_name_specifier SELFNAME LEFT_RIGHT cv_qualifiers exception_specification_opt
	| nested_name_specifier self_template_type LPAREN parmlist RPAREN cv_qualifiers exception_specification_opt
	| nested_name_specifier self_template_type LEFT_RIGHT cv_qualifiers exception_specification_opt
	| global_scope nested_name_specifier self_template_type LPAREN parmlist RPAREN cv_qualifiers exception_specification_opt
	| global_scope nested_name_specifier self_template_type LEFT_RIGHT cv_qualifiers exception_specification_opt
	;

fn.def1:
	  typed_declspecs declarator
	| declmods notype_declarator
	| notype_declarator
	| declmods constructor_declarator
	| constructor_declarator
	;

/* useless
component_constructor_declarator:
	  SELFNAME LPAREN parmlist RPAREN cv_qualifiers exception_specification_opt
	| SELFNAME LEFT_RIGHT cv_qualifiers exception_specification_opt
	| self_template_type LPAREN parmlist RPAREN cv_qualifiers exception_specification_opt
	| self_template_type LEFT_RIGHT cv_qualifiers exception_specification_opt
	;
*/

/* useless
fn.def2:
	  declmods component_constructor_declarator
	| component_constructor_declarator
	| typed_declspecs declarator
	| declmods notype_declarator
	| notype_declarator
	| declmods constructor_declarator
	| constructor_declarator
	;
*/

return_id:
	  RETURN_KEYWORD IDENTIFIER
	;

return_init:
	  return_id maybe_init
	| return_id LPAREN nonnull_exprlist RPAREN
	| return_id LEFT_RIGHT
	;

base_init:
	  COLON .set_base_init member_init_list {}
	;

.set_base_init:
	  /* empty */
	;

member_init_list:
	  /* empty */
	| member_init
	| member_init_list COMMA member_init
	| member_init_list error
	;

member_init:
	  LPAREN nonnull_exprlist RPAREN
	| LEFT_RIGHT
	| notype_identifier LPAREN nonnull_exprlist RPAREN
	| notype_identifier LEFT_RIGHT
	| nonnested_type LPAREN nonnull_exprlist RPAREN
	| nonnested_type LEFT_RIGHT
	| typename_sub LPAREN nonnull_exprlist RPAREN
	| typename_sub LEFT_RIGHT
	;

identifier:
	  IDENTIFIER
	| TYPENAME
	| SELFNAME
/*	| PTYPENAME */
/*	| NSNAME */
	;

notype_identifier:
	  IDENTIFIER
/*	| PTYPENAME */
/*	| NSNAME  %prec EMPTY */
	;
/* useless
identifier_defn:
	  IDENTIFIER_DEFN
	| TYPENAME_DEFN
	| PTYPENAME_DEFN
	;
*/

explicit_instantiation:
	  TEMPLATE begin_explicit_instantiation typespec SEMICOLON end_explicit_instantiation
	| TEMPLATE begin_explicit_instantiation typed_declspecs declarator end_explicit_instantiation
	| TEMPLATE begin_explicit_instantiation notype_declarator end_explicit_instantiation
	| TEMPLATE begin_explicit_instantiation constructor_declarator end_explicit_instantiation
	| storage_class_specifier TEMPLATE begin_explicit_instantiation typespec SEMICOLON end_explicit_instantiation
	| storage_class_specifier TEMPLATE begin_explicit_instantiation typed_declspecs declarator end_explicit_instantiation
	| storage_class_specifier TEMPLATE begin_explicit_instantiation notype_declarator end_explicit_instantiation
	| storage_class_specifier TEMPLATE begin_explicit_instantiation constructor_declarator end_explicit_instantiation
	;

begin_explicit_instantiation:
    ;

end_explicit_instantiation:
    ;

template_type:
/*	  PTYPENAME LT template_arg_list_opt template_close_bracket .finish_template_type
	|*/ TYPENAME  LT template_arg_list_opt template_close_bracket .finish_template_type
	| self_template_type
	;

apparent_template_type:
	  template_type
	| identifier LT template_arg_list_opt GT .finish_template_type
	;

self_template_type:
	  SELFNAME  LT template_arg_list_opt template_close_bracket .finish_template_type
	;

.finish_template_type:
    ;

template_close_bracket:
	  GT
	| RSHIFT
	;

template_arg_list_opt:
	 /* empty */
	| template_arg_list
	;

template_arg_list:
        template_arg
	| template_arg_list COMMA template_arg
	;

template_arg:
	  type_id
/*	| PTYPENAME*/
	| expr_no_commas  %prec LTEQUAL GTEQUAL
	;

unop:
	  MINUS
	| PLUS
	| PLUSPLUS
	| MINUSMINUS
	| '!'
	;

expr:
	  nontrivial_exprlist
	| expr_no_commas
	;

paren_expr_or_null:
	LEFT_RIGHT
	| LPAREN expr RPAREN
	;

paren_cond_or_null:
	LEFT_RIGHT
	| LPAREN condition RPAREN
	;

xcond:
	  /* empty */
	| condition
	| error
	;

condition:
	  type_specifier_seq declarator maybeasm maybe_attribute IS init
	| expr
	;

compstmtend:
	  RBRACE
	| maybe_label_decls stmts RBRACE
	| maybe_label_decls stmts error RBRACE
	| maybe_label_decls error RBRACE
	;

already_scoped_stmt:
	  LBRACE
	  compstmtend
	| simple_stmt
	;

nontrivial_exprlist:
	  expr_no_commas COMMA expr_no_commas
	| expr_no_commas COMMA error
	| nontrivial_exprlist COMMA expr_no_commas
	| nontrivial_exprlist COMMA error
	;

nonnull_exprlist:
	  expr_no_commas
	| nontrivial_exprlist
	;

unary_expr:
	  primary  %prec UNARY
	| extension cast_expr  	  %prec UNARY
	| MUL cast_expr   %prec UNARY
	| BITAND cast_expr   %prec UNARY
	| BITNOT cast_expr
	| unop cast_expr  %prec UNARY
	| ANDAND identifier
	| SIZEOF unary_expr  %prec UNARY
	| SIZEOF LPAREN type_id RPAREN  %prec HYPERUNARY
	| ALIGNOF unary_expr  %prec UNARY
	| ALIGNOF LPAREN type_id RPAREN  %prec HYPERUNARY
	| new new_type_id  %prec EMPTY
	| new new_type_id new_initializer
	| new new_placement new_type_id  %prec EMPTY
	| new new_placement new_type_id new_initializer
	| new LPAREN .begin_new_placement type_id .finish_new_placement %prec EMPTY
	| new LPAREN .begin_new_placement type_id .finish_new_placement new_initializer
	| new new_placement LPAREN .begin_new_placement type_id .finish_new_placement   %prec EMPTY
	| new new_placement LPAREN .begin_new_placement type_id .finish_new_placement  new_initializer
	| delete cast_expr  %prec UNARY
	| delete LBRACKET RBRACKET cast_expr  %prec UNARY
	| delete LBRACKET expr RBRACKET cast_expr  %prec UNARY
	| REALPART cast_expr %prec UNARY
	| IMAGPART cast_expr %prec UNARY
	;

.finish_new_placement:
	  RPAREN
    ;

.begin_new_placement:
    ;

new_placement:
	  LPAREN .begin_new_placement nonnull_exprlist RPAREN
	| LBRACE .begin_new_placement nonnull_exprlist RBRACE
	;

new_initializer:
	  LPAREN nonnull_exprlist RPAREN
	| LEFT_RIGHT
	| LPAREN typespec RPAREN
	| IS init
	;

regcast_or_absdcl:
	  LPAREN type_id RPAREN  %prec EMPTY
	| regcast_or_absdcl LPAREN type_id RPAREN  %prec EMPTY
	;

cast_expr:
	  unary_expr
	| regcast_or_absdcl unary_expr  %prec UNARY
	| regcast_or_absdcl LBRACE initlist maybecomma RBRACE  %prec UNARY
	;

expr_no_commas:
	  cast_expr
	| expr_no_commas POINTSAT_STAR expr_no_commas
	| expr_no_commas DOT_STAR expr_no_commas
	| expr_no_commas PLUS expr_no_commas
	| expr_no_commas MINUS expr_no_commas
	| expr_no_commas MUL expr_no_commas
	| expr_no_commas DIV expr_no_commas
	| expr_no_commas MOD expr_no_commas
	| expr_no_commas LSHIFT expr_no_commas
	| expr_no_commas RSHIFT expr_no_commas
	| expr_no_commas LTEQUAL expr_no_commas
	| expr_no_commas GTEQUAL expr_no_commas
	| expr_no_commas LT expr_no_commas
	| expr_no_commas GT expr_no_commas
	| expr_no_commas EQUAL expr_no_commas
	| expr_no_commas CPP_MIN expr_no_commas
	| expr_no_commas CPP_MAX expr_no_commas
	| expr_no_commas BITAND expr_no_commas
	| expr_no_commas BITOR expr_no_commas
	| expr_no_commas BITXOR expr_no_commas
	| expr_no_commas ANDAND expr_no_commas
	| expr_no_commas OROR expr_no_commas
	| expr_no_commas QUESTION xexpr COLON expr_no_commas
	| expr_no_commas IS expr_no_commas
	| expr_no_commas CPP_PLUS_EQ expr_no_commas
	| expr_no_commas CPP_MINUS_EQ expr_no_commas
	| expr_no_commas CPP_MULT_EQ expr_no_commas
	| expr_no_commas CPP_DIV_EQ expr_no_commas
	| expr_no_commas CPP_MOD_EQ expr_no_commas
	| expr_no_commas CPP_AND_EQ expr_no_commas
	| expr_no_commas CPP_OR_EQ expr_no_commas
	| expr_no_commas CPP_XOR_EQ expr_no_commas
	| expr_no_commas CPP_RSHIFT_EQ expr_no_commas
	| expr_no_commas CPP_LSHIFT_EQ expr_no_commas
	| expr_no_commas CPP_MIN_EQ expr_no_commas
	| expr_no_commas CPP_MAX_EQ expr_no_commas
	| THROW
	| THROW expr_no_commas
	;

notype_unqualified_id:
	  BITNOT see_typename identifier
	| BITNOT see_typename template_type
    | template_id
	| operator_name
	| IDENTIFIER
/*	| PTYPENAME*/
/*	| NSNAME  %prec EMPTY */
	;

do_id:
    ;

template_id:
/*      PFUNCNAME LT do_id template_arg_list_opt template_close_bracket
    |*/ operator_name LT do_id template_arg_list_opt template_close_bracket
	;

object_template_id:
      TEMPLATE identifier LT template_arg_list_opt template_close_bracket
/*    | TEMPLATE PFUNCNAME LT template_arg_list_opt template_close_bracket*/
    | TEMPLATE operator_name LT template_arg_list_opt template_close_bracket
    ;

unqualified_id:
	  notype_unqualified_id
	| TYPENAME
	| SELFNAME
	;

expr_or_declarator_intern:
	  expr_or_declarator
	| attributes expr_or_declarator
	;

expr_or_declarator:
	  notype_unqualified_id
	| MUL expr_or_declarator_intern  %prec UNARY
	| BITAND expr_or_declarator_intern  %prec UNARY
	| LPAREN expr_or_declarator_intern RPAREN
	;

notype_template_declarator:
	  IDENTIFIER LT template_arg_list_opt template_close_bracket
/*	| NSNAME LT template_arg_list template_close_bracket */
	;
direct_notype_declarator:
	  complex_direct_notype_declarator
	| notype_unqualified_id %prec LPAREN
	| notype_template_declarator
	| LPAREN expr_or_declarator_intern RPAREN
	;

primary:
	  notype_unqualified_id
	| CONSTANT
	| boolean.literal
	| string
	| LPAREN expr RPAREN
	| LPAREN expr_or_declarator_intern RPAREN
	| LPAREN error RPAREN
	| LPAREN compstmt RPAREN
    | notype_unqualified_id LPAREN nonnull_exprlist RPAREN
    | notype_unqualified_id LEFT_RIGHT
	| primary LPAREN nonnull_exprlist RPAREN
	| primary LEFT_RIGHT
	| primary LBRACKET expr RBRACKET
	| primary PLUSPLUS
	| primary MINUSMINUS
	| THIS
	| cv_qualifier LPAREN nonnull_exprlist RPAREN
	| functional_cast
	| DYNAMIC_CAST LT type_id GT LPAREN expr RPAREN
	| STATIC_CAST LT type_id GT LPAREN expr RPAREN
	| REINTERPRET_CAST LT type_id GT LPAREN expr RPAREN
	| CONST_CAST LT type_id GT LPAREN expr RPAREN
	| TYPEID LPAREN expr RPAREN
	| TYPEID LPAREN type_id RPAREN
	| global_scope IDENTIFIER
	| global_scope template_id
	| global_scope operator_name
	| overqualified_id  %prec HYPERUNARY
	| overqualified_id LPAREN nonnull_exprlist RPAREN
	| overqualified_id LEFT_RIGHT
    | object object_template_id %prec UNARY
    | object object_template_id LPAREN nonnull_exprlist RPAREN
	| object object_template_id LEFT_RIGHT
	| object unqualified_id  %prec UNARY
	| object overqualified_id  %prec UNARY
	| object unqualified_id LPAREN nonnull_exprlist RPAREN
	| object unqualified_id LEFT_RIGHT
	| object overqualified_id LPAREN nonnull_exprlist RPAREN
	| object overqualified_id LEFT_RIGHT
	| object BITNOT type_qualifier LEFT_RIGHT
	| object type_qualifier SCOPE BITNOT type_qualifier LEFT_RIGHT
	| object error
	;

new:
	  NEW
	| global_scope NEW
	;

delete:
	  DELETE
	| global_scope delete
	;

boolean.literal:
	  CXX_TRUE
	| CXX_FALSE
	;

string:
	  STRING
	| string STRING
	;

nodecls:
	  /* empty */
	;

object:
	  primary DOT
	| primary POINTSAT
	;

decl:
	  typespec initdecls SEMICOLON
	| typed_declspecs initdecls SEMICOLON
	| declmods notype_initdecls SEMICOLON
	| typed_declspecs SEMICOLON
	| declmods SEMICOLON
	| extension decl
	;

declarator:
	  after_type_declarator  %prec EMPTY
	| notype_declarator  %prec EMPTY
	;

fcast_or_absdcl:
	  LEFT_RIGHT  %prec EMPTY
	| fcast_or_absdcl LEFT_RIGHT  %prec EMPTY
	;

type_id:
	  typed_typespecs absdcl
	| nonempty_cv_qualifiers absdcl
	| typespec absdcl
	| typed_typespecs  %prec EMPTY
	| nonempty_cv_qualifiers  %prec EMPTY
	;

typed_declspecs:
	  typed_typespecs  %prec EMPTY
	| typed_declspecs1
	;

typed_declspecs1:
	  declmods typespec
	| typespec reserved_declspecs  %prec HYPERUNARY
	| typespec reserved_typespecquals reserved_declspecs
	| declmods typespec reserved_declspecs
	| declmods typespec reserved_typespecquals
	| declmods typespec reserved_typespecquals reserved_declspecs
	;

reserved_declspecs:
	  storage_class_specifier
	| reserved_declspecs typespecqual_reserved
	| reserved_declspecs storage_class_specifier
	| reserved_declspecs attributes
	| attributes
	;

declmods:
	  nonempty_cv_qualifiers  %prec EMPTY
	| storage_class_specifier
	| declmods cv_qualifier
	| declmods storage_class_specifier
	| declmods attributes
	| attributes  %prec EMPTY
	;

typed_typespecs:
	  typespec  %prec EMPTY
	| nonempty_cv_qualifiers typespec
	| typespec reserved_typespecquals
	| nonempty_cv_qualifiers typespec reserved_typespecquals
	;

reserved_typespecquals:
	  typespecqual_reserved
	| reserved_typespecquals typespecqual_reserved
	;

typespec:
	  structsp
	| type_qualifier  %prec EMPTY
	| complete_type_name
	| TYPEOF LPAREN expr RPAREN
	| TYPEOF LPAREN type_id RPAREN
	| SIGOF LPAREN expr RPAREN
	| SIGOF LPAREN type_id RPAREN
	;

typespecqual_reserved:
	  type_qualifier
	| cv_qualifier
	| structsp
	;

initdecls:
	  initdcl0
	| initdecls COMMA initdcl
	;

notype_initdecls:
	  notype_initdcl0
	| notype_initdecls COMMA initdcl
	;

nomods_initdecls:
	  nomods_initdcl0
	| nomods_initdecls COMMA initdcl
	;

maybeasm:
	  /* empty */
	| asm_keyword LPAREN string RPAREN
	;

initdcl:
	  declarator maybeasm maybe_attribute IS init
	| declarator maybeasm maybe_attribute
	;

initdcl0_innards:
	  maybe_attribute IS init
	| maybe_attribute
  	;

initdcl0:
	  declarator maybeasm initdcl0_innards
	;

notype_initdcl0:
      notype_declarator maybeasm initdcl0_innards
    ;

nomods_initdcl0:
      notype_declarator maybeasm initdcl0_innards 
	| constructor_declarator maybeasm maybe_attribute
	;

maybe_attribute:
	  /* empty */
	| attributes
	;
attributes:
      attribute
	| attributes attribute
	;

attribute:
      ATTRIBUTE LPAREN LPAREN attribute_list RPAREN RPAREN
	;

attribute_list:
      attrib
	| attribute_list COMMA attrib
	;
attrib:
	  /* empty */
	| any_word
	| any_word LPAREN IDENTIFIER RPAREN
	| any_word LPAREN IDENTIFIER COMMA nonnull_exprlist RPAREN
	| any_word LPAREN nonnull_exprlist RPAREN
	;

any_word:
	  identifier
	| storage_class_specifier
	| type_qualifier
	| cv_qualifier
	;

identifiers_or_typenames:
	  identifier
	| identifiers_or_typenames COMMA identifier
	;

maybe_init:
	  /* empty */  %prec EMPTY
	| IS init
	;

init:
	  expr_no_commas  %prec IS
	| LBRACE RBRACE
	| LBRACE initlist RBRACE
	| LBRACE initlist COMMA RBRACE
	| error
	;

initlist:
	  init
	| initlist COMMA init
	| LBRACKET expr_no_commas RBRACKET init
	| identifier COLON init
	| initlist COMMA identifier COLON init
	;

fn.defpen:
	PRE_PARSED_FUNCTION_DECL
	;

pending_inline:
	  fn.defpen maybe_return_init ctor_initializer_opt compstmt_or_error
	| fn.defpen maybe_return_init function_try_block
	| fn.defpen maybe_return_init error
	;

pending_inlines:
	/* empty */
	| pending_inlines pending_inline eat_saved_input
	;

defarg_again:
	DEFARG_MARKER expr_no_commas END_OF_SAVED_INPUT
	| DEFARG_MARKER error END_OF_SAVED_INPUT
    ;

pending_defargs:
	  /* empty */ %prec EMPTY
	| pending_defargs defarg_again
	| pending_defargs error
	;

structsp:
	  ENUM identifier LBRACE
	  enumlist maybecomma_warn RBRACE
	| ENUM identifier LBRACE RBRACE
	| ENUM LBRACE
	  enumlist maybecomma_warn RBRACE
	| ENUM LBRACE RBRACE
	| ENUM identifier
	| ENUM complex_type_name
	| CLASS typename_sub
	| named_class_head LBRACE components RBRACE maybe_attribute pending_defargs pending_inlines
	| named_class_head  %prec EMPTY
	;

maybecomma:
	  /* empty */
	| COMMA
	;

maybecomma_warn:
	  /* empty */
	| COMMA
	;

aggr:
	  CLASS
    | STRUCT
	| UNION
	| ENUM
	| aggr storage_class_specifier
	| aggr type_qualifier
	| aggr cv_qualifier
	| aggr CLASS
	| aggr STRUCT
	| aggr UNION
	| aggr ENUM
	| aggr attributes
	;

named_class_head_sans_basetype:
	  aggr identifier
	;

named_class_head_sans_basetype_defn:
/*	  aggr identifier_defn %prec EMPTY
	|*/ named_class_head_sans_basetype LBRACE
	| named_class_head_sans_basetype COLON
	;

named_complex_class_head_sans_basetype:
	  aggr nested_name_specifier identifier
	| aggr global_scope nested_name_specifier identifier
	| aggr global_scope identifier
	| aggr apparent_template_type
	| aggr nested_name_specifier apparent_template_type
	;

named_class_head:
	  named_class_head_sans_basetype  %prec EMPTY
	| named_class_head_sans_basetype_defn maybe_base_class_list  %prec EMPTY
	| named_complex_class_head_sans_basetype maybe_base_class_list
	;

maybe_base_class_list:
	/* empty */
	| base_class_list
	;

base_class_list:
	base_class
	| base_class_list COMMA base_class
	;

base_class:
	ID
	;

/* useless
unnamed_class_head:
	  aggr LBRACE
	| fn.def2 COLON /* base_init compstmt *//*
	| fn.def2 TRY /* base_init compstmt *//*
	| fn.def2 RETURN_KEYWORD /* base_init compstmt *//*
	| fn.def2 LBRACE /* nodecls compstmt *//*
	| SEMICOLON
	| extension component_declarator
    | template_header component_declarator
	| template_header typed_declspecs SEMICOLON
	;
*/

/* useless
component_decl_1:
	  typed_declspecs components
	| declmods notype_components
	| notype_declarator maybeasm maybe_attribute maybe_init
	| constructor_declarator maybeasm maybe_attribute maybe_init
	| COLON expr_no_commas
	| error
	| declmods component_constructor_declarator maybeasm maybe_attribute maybe_init
	| component_constructor_declarator maybeasm maybe_attribute maybe_init
	| using_decl
	;
*/

components:
	  /* empty: possibly anonymous */
	| component_declarator0
	| components COMMA component_declarator
	;

/* useless
notype_components:
	  /* empty: possibly anonymous *//*
	| notype_component_declarator0
	| notype_components COMMA notype_component_declarator
	;
*/

component_declarator0:
	  after_type_component_declarator0
	| notype_component_declarator0
	;

component_declarator:
	  after_type_component_declarator
	| notype_component_declarator
	;

after_type_component_declarator0:
	  after_type_declarator maybeasm maybe_attribute maybe_init
	| TYPENAME COLON expr_no_commas maybe_attribute
	;

notype_component_declarator0:
	  notype_declarator maybeasm maybe_attribute maybe_init
	| constructor_declarator maybeasm maybe_attribute maybe_init
	| IDENTIFIER COLON expr_no_commas maybe_attribute
	| COLON expr_no_commas maybe_attribute
	;

after_type_component_declarator:
	  after_type_declarator maybeasm maybe_attribute maybe_init
	| TYPENAME COLON expr_no_commas maybe_attribute
	;

notype_component_declarator:
	  notype_declarator maybeasm maybe_attribute maybe_init
	| IDENTIFIER COLON expr_no_commas maybe_attribute
	| COLON expr_no_commas maybe_attribute
	;

enumlist:
	  enumerator
	| enumlist COMMA enumerator
	;

enumerator:
	  identifier
	| identifier IS expr_no_commas
	;

new_type_id:
	  type_specifier_seq new_declarator
	| type_specifier_seq  %prec EMPTY
	| LPAREN .begin_new_placement type_id .finish_new_placement LBRACKET expr RBRACKET
	;

cv_qualifiers:
	  /* empty */  %prec EMPTY
	| cv_qualifiers cv_qualifier
	;

nonempty_cv_qualifiers:
	  cv_qualifier
	| nonempty_cv_qualifiers cv_qualifier
	;

suspend_mom:
	  /* empty */
	;

nonmomentary_expr:
	  suspend_mom expr
	;

maybe_parmlist:
	  suspend_mom LPAREN nonnull_exprlist RPAREN
	| suspend_mom LPAREN parmlist RPAREN
	| suspend_mom LEFT_RIGHT
	| suspend_mom LPAREN error RPAREN
	;

after_type_declarator_intern:
	  after_type_declarator
	| attributes after_type_declarator
	;

after_type_declarator:
	  MUL nonempty_cv_qualifiers after_type_declarator_intern  %prec UNARY
	| BITAND nonempty_cv_qualifiers after_type_declarator_intern  %prec UNARY
	| MUL after_type_declarator_intern  %prec UNARY
	| BITAND after_type_declarator_intern  %prec UNARY
	| ptr_to_mem cv_qualifiers after_type_declarator_intern
	| direct_after_type_declarator
	;

direct_after_type_declarator:
	  direct_after_type_declarator maybe_parmlist cv_qualifiers exception_specification_opt  %prec DOT
	| direct_after_type_declarator LBRACKET nonmomentary_expr RBRACKET
	| direct_after_type_declarator LBRACKET RBRACKET
	| LPAREN after_type_declarator_intern RPAREN
	| nested_name_specifier type_name  %prec EMPTY
	| type_name  %prec EMPTY
	;

nonnested_type:
	  type_name  %prec EMPTY
	| global_scope type_name
	;

complete_type_name:
	  nonnested_type
	| nested_type
	| global_scope nested_type
	;

nested_type:
	  nested_name_specifier type_name  %prec EMPTY
	;

notype_declarator_intern:
	  notype_declarator
	| attributes notype_declarator
	;
notype_declarator:
	  MUL nonempty_cv_qualifiers notype_declarator_intern  %prec UNARY
	| BITAND nonempty_cv_qualifiers notype_declarator_intern  %prec UNARY
	| MUL notype_declarator_intern  %prec UNARY
	| BITAND notype_declarator_intern  %prec UNARY
	| ptr_to_mem cv_qualifiers notype_declarator_intern
	| direct_notype_declarator
	;

complex_notype_declarator:
	  MUL nonempty_cv_qualifiers notype_declarator_intern  %prec UNARY
	| BITAND nonempty_cv_qualifiers notype_declarator_intern  %prec UNARY
	| MUL complex_notype_declarator  %prec UNARY
	| BITAND complex_notype_declarator  %prec UNARY
	| ptr_to_mem cv_qualifiers notype_declarator_intern
	| complex_direct_notype_declarator
	;

complex_direct_notype_declarator:
	  direct_notype_declarator maybe_parmlist cv_qualifiers exception_specification_opt  %prec DOT
	| LPAREN complex_notype_declarator RPAREN
	| direct_notype_declarator LBRACKET nonmomentary_expr RBRACKET
	| direct_notype_declarator LBRACKET RBRACKET
	| notype_qualified_id
    | nested_name_specifier notype_template_declarator
	;

qualified_id:
	  nested_name_specifier unqualified_id
    | nested_name_specifier object_template_id
	;

notype_qualified_id:
	  nested_name_specifier notype_unqualified_id
    | nested_name_specifier object_template_id
	;

overqualified_id:
	  notype_qualified_id
	| global_scope notype_qualified_id
	;

functional_cast:
	  typespec LPAREN nonnull_exprlist RPAREN
	| typespec LPAREN expr_or_declarator_intern RPAREN
	| typespec fcast_or_absdcl  %prec EMPTY
	;
type_name:
	  TYPENAME
	| SELFNAME
	| template_type  %prec EMPTY
	;

nested_name_specifier:
	  nested_name_specifier_1
	| nested_name_specifier nested_name_specifier_1
	| nested_name_specifier TEMPLATE explicit_template_type SCOPE
	;

nested_name_specifier_1:
	  TYPENAME SCOPE
	| SELFNAME SCOPE
/*	| NSNAME SCOPE */
	| template_type SCOPE
	;

typename_sub:
	  typename_sub0
	| global_scope typename_sub0
	;

typename_sub0:
	  typename_sub1 identifier %prec EMPTY
	| typename_sub1 template_type %prec EMPTY
	| typename_sub1 explicit_template_type %prec EMPTY
	| typename_sub1 TEMPLATE explicit_template_type %prec EMPTY
	;

typename_sub1:
	  typename_sub2
	| typename_sub1 typename_sub2
	| typename_sub1 explicit_template_type SCOPE
	| typename_sub1 TEMPLATE explicit_template_type SCOPE
	;

typename_sub2:
	  TYPENAME SCOPE
	| SELFNAME SCOPE
	| template_type SCOPE
/*	| PTYPENAME SCOPE*/
	| IDENTIFIER SCOPE
/*	| NSNAME SCOPE */
	;

explicit_template_type:
	  identifier LT template_arg_list_opt template_close_bracket
	;

complex_type_name:
	  global_scope type_name
	| nested_type
	| global_scope nested_type
	;

ptr_to_mem:
	  nested_name_specifier MUL
	| global_scope nested_name_specifier MUL
	;

global_scope:
	  SCOPE
	;

new_declarator:
	  MUL cv_qualifiers new_declarator
	| MUL cv_qualifiers  %prec EMPTY
	| BITAND cv_qualifiers new_declarator  %prec EMPTY
	| BITAND cv_qualifiers  %prec EMPTY
	| ptr_to_mem cv_qualifiers  %prec EMPTY
	| ptr_to_mem cv_qualifiers new_declarator
	| direct_new_declarator  %prec EMPTY
	;

direct_new_declarator:
	  LBRACKET expr RBRACKET
	| direct_new_declarator LBRACKET nonmomentary_expr RBRACKET
	;

absdcl_intern:
	  absdcl
	| attributes absdcl
	;

absdcl:
	  MUL nonempty_cv_qualifiers absdcl_intern
	| MUL absdcl_intern
	| MUL nonempty_cv_qualifiers  %prec EMPTY
	| MUL  %prec EMPTY
	| BITAND nonempty_cv_qualifiers absdcl_intern
	| BITAND absdcl_intern
	| BITAND nonempty_cv_qualifiers  %prec EMPTY
	| BITAND  %prec EMPTY
	| ptr_to_mem cv_qualifiers  %prec EMPTY
	| ptr_to_mem cv_qualifiers absdcl_intern
	| direct_abstract_declarator  %prec EMPTY
	;

direct_abstract_declarator:
	  LPAREN absdcl_intern RPAREN
	| PAREN_STAR_PAREN
	| direct_abstract_declarator LPAREN parmlist RPAREN cv_qualifiers exception_specification_opt  %prec DOT
	| direct_abstract_declarator LEFT_RIGHT cv_qualifiers exception_specification_opt  %prec DOT
	| direct_abstract_declarator LBRACKET nonmomentary_expr RBRACKET  %prec DOT
	| direct_abstract_declarator LBRACKET RBRACKET  %prec DOT
	| LPAREN complex_parmlist RPAREN cv_qualifiers exception_specification_opt  %prec DOT
	| regcast_or_absdcl cv_qualifiers exception_specification_opt  %prec DOT
	| fcast_or_absdcl cv_qualifiers exception_specification_opt  %prec DOT
	| LBRACKET nonmomentary_expr RBRACKET  %prec DOT
	| LBRACKET RBRACKET  %prec DOT
	;

stmts:
	  stmt
	| errstmt
	| stmts stmt
	| stmts errstmt
	;

errstmt:
	  error SEMICOLON
	;

maybe_label_decls:
	  /* empty */
	| label_decls
	;

label_decls:
	  label_decl
	| label_decls label_decl
	;

label_decl:
	  LABEL identifiers_or_typenames SEMICOLON
	;

compstmt_or_error:
	  compstmt
	| error compstmt
	;

compstmt:
	  LBRACE compstmtend 
	;

simple_if:
	  IF paren_cond_or_null implicitly_scoped_stmt
	;

implicitly_scoped_stmt:
	  compstmt
	| simple_stmt 
	;

stmt:
	  compstmt
	| simple_stmt
	;

simple_stmt:
	  decl
	| expr SEMICOLON
	| simple_if ELSE implicitly_scoped_stmt
	| simple_if  %prec IF
	| WHILE paren_cond_or_null already_scoped_stmt
	| DO implicitly_scoped_stmt WHILE paren_expr_or_null SEMICOLON 
	| FOR LPAREN for.init.statement xcond SEMICOLON xexpr RPAREN already_scoped_stmt
	| SWITCH LPAREN condition RPAREN implicitly_scoped_stmt
	| CASE expr_no_commas COLON stmt
	| CASE expr_no_commas ELLIPSIS expr_no_commas COLON stmt
	| DEFAULT COLON stmt
	| BREAK SEMICOLON
	| CONTINUE SEMICOLON
	| RETURN_KEYWORD SEMICOLON
	| RETURN_KEYWORD expr SEMICOLON
	| asm_keyword maybe_cv_qualifier LPAREN string RPAREN SEMICOLON
	| asm_keyword maybe_cv_qualifier LPAREN string COLON asm_operands RPAREN SEMICOLON
	| asm_keyword maybe_cv_qualifier LPAREN string COLON asm_operands COLON asm_operands RPAREN SEMICOLON
	| asm_keyword maybe_cv_qualifier LPAREN string COLON asm_operands COLON asm_operands COLON asm_clobbers RPAREN SEMICOLON
	| GOTO MUL expr SEMICOLON
	| GOTO identifier SEMICOLON
	| label_colon stmt
	| label_colon RBRACE
	| SEMICOLON
	| try_block
	| using_directive
	| namespace_using_decl
	| namespace_alias
	;

function_try_block:
	  TRY ctor_initializer_opt compstmt handler_seq
	;

try_block:
	  TRY compstmt handler_seq
	;

handler_seq:
	  handler
	| handler_seq handler
	;

handler:
	  CATCH handler_args compstmt
	;

type_specifier_seq:
	  typed_typespecs  %prec EMPTY
	| nonempty_cv_qualifiers  %prec EMPTY
	;

handler_args:
	  LPAREN ELLIPSIS RPAREN
	| LPAREN parm RPAREN
	;

label_colon:
	  IDENTIFIER COLON
/*	| PTYPENAME COLON*/
	| TYPENAME COLON
	| SELFNAME COLON
	;

for.init.statement:
	  xexpr SEMICOLON
	| decl
	| LBRACE compstmtend
	;

maybe_cv_qualifier:
	  /* empty */
	| cv_qualifier
	;

xexpr:
	  /* empty */
	| expr
	| error
	;

asm_operands:
	  /* empty */
	| nonnull_asm_operands
	;

nonnull_asm_operands:
	  asm_operand
	| nonnull_asm_operands COMMA asm_operand
	;

asm_operand:
	  STRING LPAREN expr RPAREN
	;

asm_clobbers:
	  STRING
	| asm_clobbers COMMA STRING
	;

parmlist:
	  /* empty */
	| complex_parmlist
	| type_id
	;

complex_parmlist:
	  parms
	| parms_comma ELLIPSIS
	| parms ELLIPSIS
	| type_id ELLIPSIS
	| ELLIPSIS
	| parms COLON
	| type_id COLON
	;

defarg:
	  IS defarg1
	;

defarg1:
	  DEFARG
	| init
	;

parms:
	  named_parm
	| parm defarg
	| parms_comma full_parm
	| parms_comma bad_parm
	| parms_comma bad_parm IS init
	;

parms_comma:
	  parms COMMA
	| type_id COMMA
	;

named_parm:
	  typed_declspecs1 declarator
	| typed_typespecs declarator
	| typespec declarator
	| typed_declspecs1 absdcl
	| typed_declspecs1  %prec EMPTY
	| declmods notype_declarator
	;

full_parm:
	  parm
	| parm defarg
	;

parm:
	  named_parm
	| type_id
	;

see_typename:
	  /* empty */  %prec EMPTY
	;

bad_parm:
	  /* empty */ %prec EMPTY
	| notype_declarator 
	;

exception_specification_opt:
	  /* empty */  %prec EMPTY
	| THROW LPAREN ansi_raise_identifiers  RPAREN  %prec EMPTY
	| THROW LEFT_RIGHT  %prec EMPTY
	;

ansi_raise_identifier:
	  type_id
	;

ansi_raise_identifiers:
	  ansi_raise_identifier
	| ansi_raise_identifiers COMMA ansi_raise_identifier
	;

conversion_declarator:
	  /* empty */  %prec EMPTY
	| MUL cv_qualifiers conversion_declarator 
	| BITAND cv_qualifiers conversion_declarator
	| ptr_to_mem cv_qualifiers conversion_declarator
	;

operator:
	  OPERATOR
	;

operator_name:
	  operator MUL
	| operator DIV
	| operator MOD
	| operator PLUS
	| operator MINUS
	| operator BITAND
	| operator BITOR
	| operator BITXOR
	| operator BITNOT
	| operator COMMA
	| operator LTEQUAL
	| operator GTEQUAL
	| operator LT
	| operator GT
	| operator EQUAL
	| operator CPP_PLUS_EQ
	| operator CPP_MINUS_EQ
	| operator CPP_MULT_EQ
	| operator CPP_DIV_EQ
	| operator CPP_MOD_EQ
	| operator CPP_AND_EQ
	| operator CPP_OR_EQ
	| operator CPP_XOR_EQ
	| operator CPP_RSHIFT_EQ
	| operator CPP_LSHIFT_EQ
	| operator CPP_MIN_EQ
	| operator CPP_MAX_EQ
	| operator IS
	| operator LSHIFT
	| operator RSHIFT
	| operator PLUSPLUS
	| operator MINUSMINUS
	| operator ANDAND
	| operator OROR
	| operator '!'
	| operator QUESTION COLON
	| operator CPP_MIN
	| operator CPP_MAX
	| operator POINTSAT  %prec EMPTY
	| operator POINTSAT_STAR  %prec EMPTY
	| operator LEFT_RIGHT
	| operator LBRACKET RBRACKET
	| operator NEW  %prec EMPTY
	| operator DELETE  %prec EMPTY
	| operator NEW LBRACKET RBRACKET
	| operator DELETE LBRACKET RBRACKET
	/* Names here should be looked up in class scope ALSO.  */
	| operator type_specifier_seq conversion_declarator
	| operator error
	;

storage_class_specifier:
	  AUTO
	| EXTERN
	| REGISTER
	| STATIC
	| TYPEDEF
	| INLINE
	;

cv_qualifier:
      CONST
    | VOLATILE
	;

type_qualifier:
	  RESTRICT
	| BYCOPY
	| BYREF
	| IN
	| OUT
	| INOUT
	| ONEWAY
	;


%%

void gcc_cxxerror(char* s)
{
	printf("Error: %s\n", s);
}
