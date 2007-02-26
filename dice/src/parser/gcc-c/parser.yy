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

void gcc_cerror(char *);
void gcc_cerror2(const char *fmt, ...);

#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"

#include "fe/FEEnumDeclarator.h"
#include "fe/FEArrayDeclarator.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEFunctionDeclarator.h"
#include "fe/FEUnionCase.h"

#include "fe/FETaggedStructType.h"
#include "fe/FETaggedUnionType.h"
#include "fe/FETaggedEnumType.h"
#include "fe/FESimpleType.h"
#include "fe/FEUserDefinedType.h"

#include "fe/FEUserDefinedExpression.h"
#include "fe/FEConditionalExpression.h"
#include "fe/FESizeOfExpression.h"

#include "parser.h"
int gcc_clex(YYSTYPE*);

#define YYDEBUG	1

// collection for elements
extern CFEFileComponent *pCurFileComponent;

// error stuff
extern int errcount;
extern int warningcount;
extern int erroccured;


// #include/import special treatment
extern String sInFileName;
extern int gLineNumber;


// indicate which TOKENS should be recognized as tokens
extern int c_inc;
extern int c_inc_old;

// import helper
int nParseErrorGCC_C = 0;

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
  unsigned char	_bool;

  CFEDeclarator*	    _decl;
  CFEExpression*        _expr;
  CFETypedDeclarator*   _typeddecl;
  CFETypeSpec*          _typespec;
  CFEUnionCase*         _unioncase;

  Vector*               _collection;
}

%token <_str> IDENTIFIER
%token <_str> TYPENAME
%token <_str> STRING
%token <_int> INTEGER_CONST
%token <_float> FLOAT_CONST
%token <_char> CHAR_CONST

%token COMMA
%token SCOPE
%token RPAREN
%token RBRACE
%token RBRACKET
%token LT
%token GT
%token BITNOT
%token TILDE
%token EXCLAM
%token LTEQUAL
%token GTEQUAL
%token ELLIPSIS
%token DOT

%token MUL_ASSIGN
%token DIV_ASSIGN
%token MOD_ASSIGN
%token PLUS_ASSIGN
%token MINUS_ASSIGN
%token LSHIFT_ASSIGN
%token RSHIFT_ASSIGN
%token AND_ASSIGN
%token OR_ASSIGN
%token XOR_ASSIGN

%token SIZEOF ENUM STRUCT UNION IF ELSE WHILE DO FOR SWITCH CASE DEFAULT
%token BREAK CONTINUE RETURN GOTO ASM_KEYWORD TYPEOF ALIGNOF
%token ATTRIBUTE LABEL
%token REALPART IMAGPART
%token CONST VOLATILE RESTRICT BYCOPY BYREF
%token IN OUT INOUT ONEWAY AUTO
%token EXTERN REGISTER STATIC INLINE TYPEDEF

%token IDENTIFIER SIZEOF

%token CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE CONST VOID

/* gcc tokens */
%token LONGLONG
%token UBOOL
%token UCOMPLEX
%token UIMAGINARY
%token LOCAL_LABEL

/* dice internal tokens */
%token EOF_TOKEN

/* Return types */
%type <_decl>       abstract_declarator
%type <_expr>       additive_expression
%type <_expr>       and_expression
%type <_collection> argument_expression_list
%type <_decl>       array_declarator
%type <_expr>       assignment_expression
%type <_expr>       cast_expression
%type <_typespec>   char_type
%type <_expr>       conditional_expression
%type <_expr>       constant
%type <_expr>       constant_expression
%type <_decl>       declarator
%type <_typespec>   declaration_specifier
%type <_collection> declaration_specifiers
%type <_decl>       direct_abstract_declarator
%type <_decl>       direct_declarator
%type <_typespec>   enum_specifier
%type <_decl>       enumerator
%type <_collection> enumerator_list
%type <_expr>       equality_expression
%type <_expr>       exclusive_or_expression
%type <_expr>       expression
%type <_expr>       expression_opt
%type <_typespec>   floating_pt_type
%type <_decl>       function_declarator
%type <_expr>       inclusive_or_expression
%type <_decl>       init_declarator
%type <_collection> init_declarator_list
%type <_int>        integer_size
%type <_typespec>   integer_type
%type <_expr>       logical_and_expression
%type <_expr>       logical_or_expression
%type <_expr>       multiplicative_expression
%type <_typeddecl>  parameter_declaration
%type <_collection> parameter_list
%type <_collection> parameter_type_list
%type <_int>        pointer
%type <_expr>       postfix_expression
%type <_expr>       primary_expression
%type <_expr>       relational_expression
%type <_expr>       shift_expression
%type <_typespec>   specifier_qualifier
%type <_collection> specifier_qualifier_list
%type <_str>        string
%type <_typeddecl>  struct_declaration
%type <_collection> struct_declaration_list
%type <_decl>       struct_declarator
%type <_collection> struct_declarator_list
%type <_typespec>   struct_or_union_specifier
%type <_typespec>   type_name
%type <_typespec>   type_specifier
%type <_str>        typedef_name
%type <_expr>       unary_expression
%type <_int>        unary_operator
%type <_unioncase>  union_arm
%type <_collection> union_body

%nonassoc IF
%nonassoc ELSE

%right IS
%right QUESTION COLON
%left OROR
%left ANDAND
%left BITOR
%left BITXOR
%left BITAND
%left EQUAL
%left NOTEQUAL
%left LSHIFT RSHIFT
%left PLUS MINUS
%left MUL DIV MOD
%right INCR DECR
%left POINTSAT
%left LPAREN LBRACKET
%right LBRACE
%left SEMICOLON

/* 4 shift/reduce conflicts:
   2 - due to variable_attribute conflicting with function_attribute in declarator
   1 - due to IF ELSE construct (doesn't know whether to reduce ELSE or shift it
   1 - due to COMMA in expression and attribute_parameter -> reduce to attribute_parameter_list shift to expression
 */
/*%expect 4*/

%start file
%%

/* A.2.1 Expressions */

primary_expression:
	  /*IDENTIFIER*/
	  constant
    {
	    $$ = $1;
    }
	| string
	{
		$$ = new CFEUserDefinedExpression(*$1);
		delete $1;
		$$->SetSourceLine(gLineNumber);
	}
	| LPAREN expression RPAREN
	{
		if ($2)
		{
			$$ = new CFEPrimaryExpression(EXPR_PAREN, $2);
			$2->SetParent($$);
			$$->SetSourceLine(gLineNumber);
		}
		else
			$$ = NULL;
	}
	;

constant:
      INTEGER_CONST
	{
		$$ = new CFEPrimaryExpression(EXPR_INT, $1);
		$$->SetSourceLine(gLineNumber);
	}
    | FLOAT_CONST
	{
		$$ = new CFEPrimaryExpression(EXPR_FLOAT, (long double)$1);
		$$->SetSourceLine(gLineNumber);
	}
	| IDENTIFIER /* enumeration_constant */
	{
		$$ = new CFEUserDefinedExpression(*$1);
		delete $1;
		$$->SetSourceLine(gLineNumber);
	}
	| CHAR_CONST
	{
	    String s($1);
		$$ = new CFEUserDefinedExpression(s);
		$$->SetSourceLine(gLineNumber);
	}
	;

postfix_expression:
	  primary_expression
    {
	    $$ = $1;
    }
	| postfix_expression LBRACKET expression RBRACKET
	{
	    $$ = NULL;
		if ($1)
		    delete $1;
		if ($3)
		    delete $3;
	}
	| postfix_expression LPAREN RPAREN
	{
	    $$ = NULL;
		if ($1)
		    delete $1;
	}
	| postfix_expression LPAREN argument_expression_list RPAREN
	{
	    $$ = NULL;
		if ($1)
		    delete $1;
		if ($3)
		    delete $3;
	}
	| postfix_expression DOT IDENTIFIER
	{
	    $$ = NULL;
		if ($1)
		    delete $1;
	}
	| postfix_expression POINTSAT IDENTIFIER
	{
	    $$ = NULL;
		if ($1)
		    delete $1;
	}
	| postfix_expression INCR
	{
	    $$ = NULL;
		if ($1)
		    delete $1;
	}
	| postfix_expression DECR
	{
	    $$ = NULL;
		if ($1)
		    delete $1;
	}
	| LPAREN type_name RPAREN LBRACE initializer_list RBRACE
	{
	    if ($2)
		    delete $2;
	    $$ = NULL;
	}
	| LPAREN type_name RPAREN LBRACE initializer_list COMMA RBRACE
	{
	    if ($2)
		    delete $2;
	    $$ = NULL;
	}
	;

argument_expression_list:
	  assignment_expression
    {
	    if ($1)
		    $$ = new Vector(RUNTIME_CLASS(CFEExpression), 1, $1);
        else
		    $$ = new Vector(RUNTIME_CLASS(CFEExpression));
    }
	| argument_expression_list COMMA assignment_expression
	{
	    if ($3)
		    $1->Add($3);
        $$ = $1;
	}
	;

unary_expression:
	  postfix_expression
    {
	    $$ = $1;
    }
	| INCR unary_expression
	{
		$$ = new CFEUnaryExpression(EXPR_UNARY, EXPR_INCR, $2);
		// set parent relationship
		$2->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| DECR unary_expression
	{
		$$ = new CFEUnaryExpression(EXPR_UNARY, EXPR_DECR, $2);
		// set parent relationship
		$2->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| unary_operator cast_expression
	{
	    if ($2)
		{
			$$ = new CFEUnaryExpression(EXPR_UNARY, (EXPT_OPERATOR)$1, $2);
			// set parent relationship
			$2->SetParent($$);
			$$->SetSourceLine(gLineNumber);
		}
		else
		    $$ = 0;
	}
	| SIZEOF unary_expression
	{
	    $$ = new CFESizeOfExpression($2);
		$2->SetParent($$);
	    $$->SetSourceLine(gLineNumber);
	}
	| SIZEOF LPAREN type_name RPAREN
	{
		if ($3)
        {
    	    $$ = new CFESizeOfExpression($3);
			$3->SetParent($$);
			$$->SetSourceLine(gLineNumber);
		}
        else
		    $$ = NULL;
	}
	;

unary_operator:
	  BITAND
    {
	    $$ = EXPR_BITAND;
    }
	| MUL
	{
	    $$ = EXPR_MUL;
	}
	| PLUS
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
	/* gcc extension */
	/* this allows to create pointers to labels,
	   e.g., void *lbl_ptr = &&label1; */
	| ANDAND
	{
	    $$ = EXPR_LOGAND;
	}
	;

cast_expression:
	  unary_expression
    {
	    $$ = $1;
    }
	| LPAREN type_name RPAREN cast_expression
	{
	    $$ = $4;
		if ($2)
			delete $2;
	}
	;

multiplicative_expression:
	  cast_expression
    {
	    $$ = $1;
    }
	| multiplicative_expression MUL cast_expression
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MUL, $3);
		// set parent relationship
		if ($1)
			$1->SetParent($$);
		if ($3)
			$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| multiplicative_expression DIV cast_expression
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_DIV, $3);
		// set parent relationship
		if ($1)
			$1->SetParent($$);
		if ($3)
			$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| multiplicative_expression MOD cast_expression
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MOD, $3);
		// set parent relationship
		if ($1)
			$1->SetParent($$);
		if ($3)
			$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

additive_expression:
	  multiplicative_expression
    {
	    $$ = $1;
    }
	| additive_expression PLUS multiplicative_expression
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_PLUS, $3);
		// set parent relationship
		if ($1)
			$1->SetParent($$);
		if ($3)
			$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| additive_expression MINUS multiplicative_expression
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MINUS, $3);
		// set parent relationship
		if ($1)
			$1->SetParent($$);
		if ($3)
			$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| additive_expression INTEGER_CONST
	{
	    if ($2 >= 0)
		{
		    gcc_cerror2("Invalid expression\n");
			YYABORT;
		}
		// $2 is negative -> '$1 - (-$2)'
		CFEExpression *tmp = new CFEPrimaryExpression(EXPR_INT, -$2);
		tmp->SetSourceLine(gLineNumber);
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MINUS, tmp);
		if ($1)
		    $1->SetParent($$);
		tmp->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

shift_expression:
	  additive_expression
    {
	    $$ = $1;
    }
	| shift_expression LSHIFT additive_expression
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LSHIFT, $3);
		// set parent relationship
		if ($1)
			$1->SetParent($$);
		if ($3)
			$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| shift_expression RSHIFT additive_expression
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_RSHIFT, $3);
		// set parent relationship
		if ($1)
			$1->SetParent($$);
		if ($3)
			$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

relational_expression:
	  shift_expression
    {
	    $$ = $1;
    }
	| relational_expression LT shift_expression
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LT, $3);
		// set parent relationship
		if ($1)
			$1->SetParent($$);
		if ($3)
			$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| relational_expression GT shift_expression
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_GT, $3);
		// set parent relationship
		if ($1)
			$1->SetParent($$);
		if ($3)
			$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| relational_expression LTEQUAL shift_expression
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LTEQU, $3);
		// set parent relationship
		if ($1)
			$1->SetParent($$);
		if ($3)
			$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| relational_expression GTEQUAL shift_expression
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_GTEQU, $3);
		// set parent relationship
		if ($1)
			$1->SetParent($$);
		if ($3)
			$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

equality_expression:
	  relational_expression
    {
	    $$ = $1;
    }
	| equality_expression EQUAL relational_expression
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
	| equality_expression NOTEQUAL relational_expression
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

and_expression:
	  equality_expression
    {
	    $$ = $1;
    }
	| and_expression BITAND equality_expression
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITAND, $3);
		// set parent relationship
		if ($1)
			$1->SetParent($$);
		if ($3)
			$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

exclusive_or_expression:
	  and_expression
    {
	    $$ = $1;
    }
	| exclusive_or_expression BITXOR and_expression
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITXOR, $3);
		// set parent relationship
		if ($1)
			$1->SetParent($$);
		if ($3)
			$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

inclusive_or_expression:
	  exclusive_or_expression
    {
	    $$ = $1;
    }
	| inclusive_or_expression BITOR exclusive_or_expression
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITOR, $3);
		// set parent relationship
		if ($1)
			$1->SetParent($$);
		if ($3)
			$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

logical_and_expression:
	  inclusive_or_expression
    {
	    $$ = $1;
    }
	| logical_and_expression ANDAND inclusive_or_expression
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LOGAND, $3);
		// set parent relationship
		if ($1)
			$1->SetParent($$);
		if ($3)
			$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

logical_or_expression:
	  logical_and_expression
    {
	    $$ = $1;
    }
	| logical_or_expression OROR logical_and_expression
	{
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LOGOR, $3);
		// set parent relationship
		if ($1)
			$1->SetParent($$);
		if ($3)
			$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

conditional_expression:
	  logical_or_expression
    {
	    $$ = $1;
    }
	| logical_or_expression QUESTION expression COLON conditional_expression
	{
	    $$ = new CFEConditionalExpression($1, $3, $5);
		// set paren relationship
		if ($1)
			$1->SetParent($$);
		if ($3)
			$3->SetParent($$);
		if ($5)
			$5->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	/* gcc extension */
	/* Conditionals with Omitted Operands */
	| logical_or_expression QUESTION COLON conditional_expression
	{
	    $$ = new CFEConditionalExpression($1, NULL, $4);
		// set parent relationship
		if ($1)
			$1->SetParent($$);
		if ($4)
			$4->SetParent($$);
        $$->SetSourceLine(gLineNumber);
	}
	;

assignment_expression:
	  conditional_expression
    {
	    $$ = $1;
    }
	| unary_expression assignment_operator assignment_expression
	{
	    if ($1)
		    delete $1;
	    if ($3)
		    delete $3;
	    $$ = NULL;
	}
	;

assignment_operator:
	  IS
	| MUL_ASSIGN
	| DIV_ASSIGN
	| MOD_ASSIGN
	| PLUS_ASSIGN
	| MINUS_ASSIGN
	| LSHIFT_ASSIGN
	| RSHIFT_ASSIGN
	| AND_ASSIGN
	| XOR_ASSIGN
	| OR_ASSIGN
	;

expression:
	  assignment_expression
    {
	    $$ = $1;
    }
	| expression COMMA assignment_expression
	{
		$$ = $3;
	}
	/* gcc extension */
	| LPAREN compound_statement RPAREN
	{
	    $$ = 0;
	}
	;

constant_expression:
	  conditional_expression
    {
	    $$ = $1;
    }
	;

/* A.2.2 Declarations */

declaration:
      declaration_specifiers SEMICOLON
    {
		CFETypeSpec *pType = (CFETypeSpec*)$1->RemoveFirst();
		if (pType->IsKindOf(RUNTIME_CLASS(CFETaggedStructType)) ||
		    pType->IsKindOf(RUNTIME_CLASS(CFETaggedUnionType)) ||
			pType->IsKindOf(RUNTIME_CLASS(CFETaggedEnumType)))
		{
			// now add tagged decl to scope
			if (!CParser::GetCurrentFile())
			{
				CCompiler::GccError(NULL, 0, "Fatal Error: current file vanished (typedef)");
				YYABORT;
			}
			if (pCurFileComponent)
			{
				if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFELibrary)))
					((CFELibrary*)pCurFileComponent)->AddTaggedDecl((CFEConstructedType*)pType);
				else if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFEInterface)))
					((CFEInterface*)pCurFileComponent)->AddTaggedDecl((CFEConstructedType*)pType);
				else
				{
					TRACE("current file component is unknown type: %s\n", (const char*)pCurFileComponent->GetClassName());
					assert(false);
				}
			}
			else
				CParser::GetCurrentFile()->AddTaggedDecl((CFEConstructedType*)pType);
		}
	    // discard the rest of the specifiers
		CObject *obj;
		while ((obj = $1->RemoveFirst()) != 0)
		    delete obj;
		delete $1;
    }
	| declaration_specifiers init_declarator_list SEMICOLON
    {
	    // discard the specifiers
		CObject *obj;
		while ((obj = $1->RemoveFirst()) != 0)
		    delete obj;
		delete $1;
		// discard the init decls
		/*while ((obj = $2->RemoveFirst()) != 0)
		    delete obj;
		delete $2;*/
    }
	| storage_class_specifier_opt TYPEDEF declaration_specifiers type_attribute_opt init_declarator_list SEMICOLON
	{
		// check if type_names already exist
		CParser *pCurParser = CParser::GetCurrentParser();
		// we can only test the import scope
		CFEFile *pRoot = pCurParser->GetTopFileInScope();
		assert(pRoot);
		VectorElement *pIter;
		CFEDeclarator *pDecl = 0;
		for (pIter = $5->GetFirst(); pIter != NULL; pIter = pIter->GetNext()) {
			pDecl = (CFEDeclarator*)(pIter->GetElement());
			if (pDecl != NULL) {
				if (pDecl->GetName() != NULL) {
					if (pRoot->FindUserDefinedType(pDecl->GetName()) != NULL)
					{
						gcc_cerror2("\"%s\" has already been defined as type.",(const char*)pDecl->GetName());
						YYABORT;
					}
				}
			}
		}
		// get type from declaration_specifiers
		CFETypeSpec *pType = (CFETypeSpec*)$3->RemoveFirst();
		// create new typedef
		CFETypedDeclarator *pTypedef = new CFETypedDeclarator(TYPEDECL_TYPEDEF, pType, $5);
		pType->SetParent(pTypedef);
		$5->SetParentOfElements(pTypedef);
		pTypedef->SetSourceLine(gLineNumber);
		// remove rest of declaration_specifiers
		while ((pType = (CFETypeSpec*)$3->RemoveFirst()) != 0)
		    delete pType;
		delete $3;

		// now add typedef to scope
		if (!CParser::GetCurrentFile())
		{
			CCompiler::GccError(NULL, 0, "Fatal Error: current file vanished (typedef)");
			YYABORT;
		}
		if (pCurFileComponent)
		{
			if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFELibrary)))
				((CFELibrary*)pCurFileComponent)->AddTypedef(pTypedef);
			else if (pCurFileComponent->IsKindOf(RUNTIME_CLASS(CFEInterface)))
				((CFEInterface*)pCurFileComponent)->AddTypedef(pTypedef);
			else
			{
				TRACE("current file component is unknown type: %s\n", (const char*)pCurFileComponent->GetClassName());
				assert(false);
			}
		}
		else
			CParser::GetCurrentFile()->AddTypedef(pTypedef);
	}
	;

declaration_specifiers:
	  declaration_specifier
    {
	    if ($1)
		    $$ = new Vector(RUNTIME_CLASS(CFETypeSpec), 1, $1);
        else
		    $$ = new Vector(RUNTIME_CLASS(CFETypeSpec));
    }
	| declaration_specifiers declaration_specifier
	{
	    if ($2)
		    $1->Add($2);
	    $$ = $1;
	}
	;

declaration_specifier:
      storage_class_specifier
    {
	    $$ = 0;
    }
	| type_specifier
	{
	    $$ = $1;
	}
	| type_qualifier
	{
	    $$ = 0;
	}
	| INLINE /* function_specifier */
	{
	    $$ = 0;
	}
	;

init_declarator_list:
	  init_declarator
    {
	    if ($1)
		    $$ = new Vector(RUNTIME_CLASS(CFEDeclarator), 1, $1);
        else
		    $$ = new Vector(RUNTIME_CLASS(CFEDeclarator));
    }
	| init_declarator_list COMMA init_declarator
	{
	    if ($3)
			$1->Add($3);
	    $$ = $1;
	}
	;

init_declarator:
	  declarator
    {
	    $$ = $1;
    }
	| declarator IS initializer
	{
	    // discard intializer
		$$ = $1;
	}
	;

storage_class_specifier:
	  EXTERN
	| STATIC
	| AUTO
	| REGISTER
	;

storage_class_specifier_opt:
      /* empty */
    | storage_class_specifier
	;

type_specifier:
	  VOID
	{
		$$ = new CFESimpleType(TYPE_VOID);
		$$->SetSourceLine(gLineNumber);
	}
	| char_type
	{
	    $$ = $1;
	}
	| integer_type
	{
	    $$ = $1;
	}
	| floating_pt_type
	{
	    $$ = $1;
	}
	| UBOOL
	{
		$$ = new CFESimpleType(TYPE_GCC);
		$$->SetSourceLine(gLineNumber);
	}
	| UCOMPLEX
	{
		$$ = new CFESimpleType(TYPE_GCC);
		$$->SetSourceLine(gLineNumber);
	}
	| UIMAGINARY
	{
		$$ = new CFESimpleType(TYPE_GCC);
		$$->SetSourceLine(gLineNumber);
	}
	| struct_or_union_specifier
	{
	    $$ = $1;
	}
	| enum_specifier
	{
	    $$ = $1;
	}
	| typedef_name
	{
		$$ = new CFEUserDefinedType(*$1);
		delete $1;
		$$->SetSourceLine(gLineNumber);
	}
	/* gcc extension */
	/* use typeof(...) to obtain the type of something */
	| TYPEOF LPAREN expression RPAREN
	{
	    if ($3)
			delete $3;
		$$ = new CFESimpleType(TYPE_GCC);
		$$->SetSourceLine(gLineNumber);
	}
	| TYPEOF LPAREN type_name RPAREN
	{
		$$ = $3;
	}
	;

/* Dice extension */
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
	    /* this causes 5 shift/reduce conflicts with "SIGNED CHAR" */
		$$ = new CFESimpleType(TYPE_INTEGER, false);
		$$->SetSourceLine(gLineNumber);
	}
	| UNSIGNED
	{
	    /* this causes 5 shift/reduce conflicts with "UNSIGNED CHAR" */
		$$ = new CFESimpleType(TYPE_INTEGER, true);
		$$->SetSourceLine(gLineNumber);
	}
	;

integer_size
	: LONG { $$ = 4; }
	| LONGLONG { $$ = 8; }
	| SHORT { $$ = 2; }
	;

char_type
	: UNSIGNED CHAR
	{
		$$ = new CFESimpleType(TYPE_CHAR, true);
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
	;

struct_or_union_specifier:
	  STRUCT LBRACE struct_declaration_list RBRACE type_attribute_opt
	{
		$$ = new CFEStructType($3);
		$3->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	| STRUCT IDENTIFIER LBRACE struct_declaration_list RBRACE type_attribute_opt
	{
		$$ = new CFETaggedStructType(*$2, $4);
		$4->SetParentOfElements($$);
		delete $2;
		$$->SetSourceLine(gLineNumber);
	}
	| STRUCT IDENTIFIER
	{
		$$ = new CFETaggedStructType(*$2);
		delete $2;
		$$->SetSourceLine(gLineNumber);
	}
	| UNION LBRACE union_body RBRACE type_attribute_opt
	{
		$$ = new CFEUnionType($3);
		$3->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	| UNION IDENTIFIER LBRACE union_body RBRACE type_attribute_opt
	{
		CFEUnionType *body = new CFEUnionType($4);
		$4->SetParentOfElements(body);
		body->SetSourceLine(gLineNumber);
		$$ = new CFETaggedUnionType(*$2, body);
		// set parent relationship
		body->SetParent($$);
		delete $2;
		$$->SetSourceLine(gLineNumber);
	}
	| UNION IDENTIFIER
	{
		$$ = new CFETaggedUnionType(*$2);
		delete $2;
		$$->SetSourceLine(gLineNumber);
	}
	/* gcc extension */
	/* empty structs */
	| STRUCT LBRACE RBRACE
	{
		$$ = new CFEStructType(NULL);
		$$->SetSourceLine(gLineNumber);
	}
	| STRUCT IDENTIFIER LBRACE RBRACE
	{
		$$ = new CFETaggedStructType(*$2);
		delete $2;
		$$->SetSourceLine(gLineNumber);
	}
	;

union_body:
	  union_body union_arm
	{
		if ($2)
			$1->Add($2);
		$$ = $1;
	}
	| union_arm
	{
		$$ = new Vector(RUNTIME_CLASS(CFEUnionCase), 1, $1);
	}
	;

union_arm
	: struct_declaration
	{
		$$ = new CFEUnionCase($1);
		$1->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| SEMICOLON
	{
		CFETypedDeclarator *tmp = new CFETypedDeclarator(TYPEDECL_VOID, 0, 0);
		tmp->SetSourceLine(gLineNumber);
		$$ = new CFEUnionCase(tmp);
		tmp->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

struct_declaration_list:
	  struct_declaration
    {
	    $$ = new Vector(RUNTIME_CLASS(CFETypedDeclarator), 1, $1);
    }
	| struct_declaration_list struct_declaration
	{
	    if ($2)
		    $1->Add($2);
	    $$ = $1;
	}
	;

struct_declaration:
	  specifier_qualifier_list struct_declarator_list SEMICOLON
    {
	    // extract type (should be 1.)
		CFETypeSpec *pType = (CFETypeSpec*)$1->RemoveFirst();
	    $$ = new CFETypedDeclarator(TYPEDECL_FIELD, pType, $2);
		pType->SetParent($$);
		$2->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
		// delete rest of type vector
		while ((pType = (CFETypeSpec*)$1->RemoveFirst()) != 0)
		    delete pType;
		delete $1;
    }
	;

specifier_qualifier_list:
	  specifier_qualifier
    {
	    if ($1)
		    $$ = new Vector(RUNTIME_CLASS(CFETypeSpec), 1, $1);
        else
		    $$ = new Vector(RUNTIME_CLASS(CFETypeSpec));
    }
	| specifier_qualifier_list specifier_qualifier
	{
	    if ($2)
		    $1->Add($2);
	    $$ = $1;
	}
	;

specifier_qualifier:
	  type_specifier
    {
	    $$ = $1;
    }
	| type_qualifier
	{
	    $$ = 0;
	}
	;

struct_declarator_list:
	  struct_declarator
    {
	    if ($1)
		    $$ = new Vector(RUNTIME_CLASS(CFEDeclarator), 1, $1);
        else
		    $$ = new Vector(RUNTIME_CLASS(CFEDeclarator));
    }
	| struct_declarator_list COMMA struct_declarator
	{
	    if ($3)
		    $1->Add($3);
	    $$ = $1;
	}
	;

struct_declarator:
	  declarator
    {
	    $$ = $1;
    }
	| declarator COLON constant_expression
	{
	    $$ = $1;
		if ($3)
		{
			$$->SetBitfields($3->GetIntValue());
			delete $3;
		}
	}
	| COLON constant_expression
	{
	    // dicard bitfield without declarator
		if ($2)
			delete $2;
	}
	;

enum_specifier:
	  ENUM LBRACE enumerator_list RBRACE type_attribute_opt
	{
		$$ = new CFEEnumType($3);
		$3->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	| ENUM IDENTIFIER LBRACE enumerator_list RBRACE type_attribute_opt
	{
	    $$ = new CFETaggedEnumType(*$2, $4);
	    delete $2;
	    $4->SetParentOfElements($$);
	    $$->SetSourceLine(gLineNumber);
	}
	| ENUM LBRACE enumerator_list COMMA RBRACE type_attribute_opt
	{
		$$ = new CFEEnumType($3);
		$3->SetParentOfElements($$);
		$$->SetSourceLine(gLineNumber);
	}
	| ENUM IDENTIFIER LBRACE enumerator_list COMMA RBRACE type_attribute_opt
	{
	    $$ = new CFETaggedEnumType(*$2, $4);
	    delete $2;
	    $4->SetParentOfElements($$);
	    $$->SetSourceLine(gLineNumber);
	}
	| ENUM IDENTIFIER
    {
        $$ = new CFETaggedEnumType(*$2, NULL);
        delete $2;
        $$->SetSourceLine(gLineNumber);
    }
	;

enumerator_list:
	  enumerator
    {
	    $$ = new Vector(RUNTIME_CLASS(CFEEnumDeclarator), 1, $1);
    }
	| enumerator_list COMMA enumerator
	{
	    if ($3)
		    $1->Add($3);
	    $$ = $1;
	}
	;

enumerator:
	  IDENTIFIER /* enumeration_constant */
    {
	    $$ = new CFEEnumDeclarator(*$1, NULL);
		delete $1;
		$$->SetSourceLine(gLineNumber);
    }
	| IDENTIFIER /* enumeration_constant */ IS constant_expression
	{
	    $$ = new CFEEnumDeclarator(*$1, $3);
		$3->SetParent($$);
		delete $1;
		$$->SetSourceLine(gLineNumber);
	}
	;

type_qualifier:
	  CONST
	| RESTRICT
	| VOLATILE
	;

declarator:
	  direct_declarator
    {
	    $$ = $1;
    }
	| pointer direct_declarator
	{
	    if ($2)
		    $2->SetStars($1);
	    $$ = $2;
	}
	/* gcc extension */
	/* variable attributes */
	| direct_declarator variable_attribute
	{
	    $$ = $1;
	}
	| pointer direct_declarator variable_attribute
	{
	    if ($2)
		    $2->SetStars($1);
	    $$ = $2;
	}
	;

/* gcc extension */
variable_attribute:
	  ATTRIBUTE LPAREN LPAREN variable_attributes_list RPAREN RPAREN
	;

/* gcc_extension */
variable_attributes_list:
	  variable_attributes
	| variable_attributes_list COMMA variable_attributes
	;

/* gcc extension */
variable_attributes:
	  /* empty */
	| variable_attribute_keyword
	| variable_attribute_keyword LPAREN attribute_parameter_list RPAREN
	;

/* gcc extension */
/* the following keywords are supported, but we ignore them
   and only check for tokenzied ones:
     aligned, mode, nocommon, packed, section,
     transparent_union, unused, deprecated,
     vector_size, and weak
 */
variable_attribute_keyword:
	  IDENTIFIER
    {
	    delete $1;
    }
	;

direct_declarator:
	  IDENTIFIER
	{
		$$ = new CFEDeclarator(DECL_IDENTIFIER, *$1);
		delete $1;
		$$->SetSourceLine(gLineNumber);
	}
	| LPAREN declarator RPAREN
	{
	    $$ = $2;
	}
	| array_declarator
	{
	    $$ = $1;
	}
	| function_declarator
	{
	    $$ = $1;
	}
	| function_declarator function_attribute
	{
	    $$ = $1;
	}
	;

/* Dice extension */
array_declarator:
	  direct_declarator LBRACKET RBRACKET
	{
		if ($1->GetType() == DECL_ARRAY)
			$$ = (CFEArrayDeclarator*)$1;
		else {
			$$ = new CFEArrayDeclarator($1);
			delete $1;
		}
		((CFEArrayDeclarator*)$$)->AddBounds((CFEExpression*)0, (CFEExpression*)0);
		$$->SetSourceLine(gLineNumber);
	}
	| direct_declarator LBRACKET assignment_expression RBRACKET
	{
		if ($1->GetType() == DECL_ARRAY)
			$$ = (CFEArrayDeclarator*)$1;
		else {
			$$ = new CFEArrayDeclarator($1);
			delete $1;
		}
		((CFEArrayDeclarator*)$$)->AddBounds((CFEExpression*)0, $3);
		$3->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| direct_declarator LBRACKET MUL RBRACKET
	{
		CFEExpression *bound = new CFEExpression(EXPR_CHAR, '*');
		bound->SetSourceLine(gLineNumber);
		if ($1->GetType() == DECL_ARRAY)
			$$ = (CFEArrayDeclarator*)$1;
		else {
			$$ = new CFEArrayDeclarator($1);
			delete $1;
		}
		((CFEArrayDeclarator*)$$)->AddBounds((CFEExpression*)0, bound);
		bound->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
    ;

/* Dice extension */
function_declarator:
	   direct_declarator LPAREN parameter_type_list RPAREN
	{
		$$ = new CFEFunctionDeclarator($1, $3);
		// set parent relationship
		$1->SetParent($$);
		if ($3 != 0) {
			$3->SetParentOfElements($$);
		}
		$$->SetSourceLine(gLineNumber);
	}
	| direct_declarator LPAREN RPAREN
	{
		$$ = new CFEFunctionDeclarator($1, NULL);
		// set parent relationship
		$1->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| direct_declarator LPAREN identifier_list RPAREN
	{
		// identifier_list is empty
        // return empty function declarator
		$$ = new CFEFunctionDeclarator($1, NULL);
		// set parent relationship
		$1->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	;

pointer:
	  MUL
    {
	    $$ = 1;
    }
	| MUL type_qualifier_list
	{
	    $$ = 1;
	}
	| MUL pointer
	{
	    $$ = $2 + 1;
	}
	| MUL type_qualifier_list pointer
	{
	    $$ = $3 + 1;
	}
	;

type_qualifier_list:
	  type_qualifier
	| type_qualifier_list type_qualifier
	;

parameter_type_list:
	  parameter_list
    {
	    $$ = $1;
    }
	| parameter_list COMMA ELLIPSIS
	{
	    $$ = $1;
	}
	;

parameter_list:
	  parameter_declaration
    {
	    if ($1)
		    $$ = new Vector(RUNTIME_CLASS(CFETypedDeclarator), 1, $1);
        else
		    $$ = new Vector(RUNTIME_CLASS(CFETypedDeclarator));
    }
	| parameter_list COMMA parameter_declaration
	{
	    if ($3)
		    $1->Add($3);
	    $$ = $1;
	}
	;

parameter_declaration:
	  declaration_specifiers declarator
    {
	    CFETypeSpec *pType = (CFETypeSpec*)$1->RemoveFirst();
	    $$ = new CFETypedDeclarator(TYPEDECL_PARAM, pType, new Vector(RUNTIME_CLASS(CFEDeclarator), 1, $2));
		pType->SetParent($$);
		$$->SetSourceLine(gLineNumber);
		// delete rest of types
		while ((pType = (CFETypeSpec*)$1->RemoveFirst()) != 0)
		    delete pType;
		delete $1;
    }
	| declaration_specifiers
	{
		// delete rest of types
		CObject *obj;
		while ((obj = $1->RemoveFirst()) != 0)
		    delete obj;
		delete $1;
	    $$ = 0;
	}
	| declaration_specifiers abstract_declarator
	{
	    CFETypeSpec *pType = (CFETypeSpec*)$1->RemoveFirst();
	    $$ = new CFETypedDeclarator(TYPEDECL_PARAM, pType, new Vector(RUNTIME_CLASS(CFEDeclarator), 1, $2));
		pType->SetParent($$);
		$$->SetSourceLine(gLineNumber);
		// delete rest of types
		while ((pType = (CFETypeSpec*)$1->RemoveFirst()) != 0)
		    delete pType;
		delete $1;
	}
	/* this does not seem realistic (TYPEDEF within a parameter declaration?)
	| storage_class_specifier_opt TYPEDEF declaration_specifiers declarator
	| storage_class_specifier_opt TYPEDEF declaration_specifiers
	| storage_class_specifier_opt TYPEDEF declaration_specifiers abstract_declarator
	 */
	;

identifier_list:
	  IDENTIFIER
    {
	    if ($1)
		    delete $1;
    }
	| identifier_list COMMA IDENTIFIER
	{
	    if ($3)
		    delete $3;
	}
	;

type_name:
	  specifier_qualifier_list
    {
	    // list of types
		CFETypeSpec *pType = (CFETypeSpec*)$1->RemoveFirst();
		$$ = pType;
        // delete rest of type vector
		while ((pType = (CFETypeSpec*)$1->RemoveFirst()) != 0)
		    delete pType;
		delete $1;
    }
	| specifier_qualifier_list abstract_declarator
	{
        // delete rest of type vector
		CObject *obj;
		while ((obj = $1->RemoveFirst()) != 0)
		    delete obj;
		delete $1;
		// create user defined type from abstract_declarator
	    $$ = new CFEUserDefinedType($2->GetName());
		$$->SetSourceLine(gLineNumber);
		delete $2;
	}
	;

abstract_declarator:
	  pointer
    {
	    $$ = new CFEDeclarator(DECL_VOID);
		$$->SetStars($1);
		$$->SetSourceLine(gLineNumber);
    }
	| direct_abstract_declarator
	{
	    $$ = $1;
	}
	| pointer direct_abstract_declarator
	{
	    $2->SetStars($1);
		$$ = $2;
	}
	;

direct_abstract_declarator:
	  LPAREN abstract_declarator RPAREN
    {
	    $$ = $2;
    }
	| LBRACKET assignment_expression RBRACKET
	{
	    $$ = NULL;
	}
	| LBRACKET RBRACKET
	{
	    $$ = NULL;
	}
	| direct_abstract_declarator LBRACKET assignment_expression RBRACKET
	{
	    $$ = $1;
	}
	| direct_abstract_declarator LBRACKET RBRACKET
	{
	    $$ = $1;
	}
	| direct_abstract_declarator LBRACKET MUL RBRACKET
	{
	    $$ = $1;
	}
	| LPAREN RPAREN
	{
	    $$ = NULL;
	}
	| LPAREN parameter_type_list RPAREN
	{
	    $$ = NULL;
	}
	| direct_abstract_declarator LPAREN RPAREN
	{
		$$ = new CFEFunctionDeclarator($1, NULL);
		// set parent relationship
		$1->SetParent($$);
		$$->SetSourceLine(gLineNumber);
	}
	| direct_abstract_declarator LPAREN parameter_type_list RPAREN
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

typedef_name:
	  TYPENAME
    {
	    $$ = $1;
    }
	;

initializer:
	  assignment_expression
    {
	    if ($1)
		    delete $1;
    }
	| LBRACE initializer_list RBRACE
	| LBRACE initializer_list COMMA RBRACE
	;

initializer_list:
	  initializer
	| designation initializer
	| initializer_list COMMA initializer
	| initializer_list COMMA designation initializer
	;

designation:
	  designator_list IS
    /* gcc 2.95 specific */
    | IDENTIFIER COLON
	{
	    delete $1;
	}
	;

designator_list:
	  designator
	| designator_list designator
	;

designator:
	  LBRACKET constant_expression RBRACKET
	| DOT IDENTIFIER
	;

/* A.2.3 Statements */

statement:
	  labeled_statement
	| compound_statement
	| expression_statement
	| selection_statement
	| iteration_statement
	| jump_statement
	/* gcc extension */
	| asm_statement
	;

labeled_statement:
	  IDENTIFIER COLON statement
    {
	    if ($1)
		    delete $1;
    }
	| CASE constant_expression COLON statement
	{
	    if ($2)
		    delete $2;
	}
	| DEFAULT COLON statement
	/* gcc extension */
	/* case value ranges */
	| CASE constant_expression ELLIPSIS constant_expression COLON statement
	{
	    if ($2)
		    delete $2;
	    if ($4)
		    delete $4;
	}
	;

compound_statement:
	  LBRACE RBRACE
	| LBRACE block_item_list RBRACE
	/* gcc extension */
	| LBRACE local_label_declarations block_item_list RBRACE
	;

/* gcc extension */
local_label_declarations:
	  local_label_declaration
	| local_label_declarations local_label_declaration
	;

/* gcc extension */
local_label_declaration:
	  LOCAL_LABEL identifier_list SEMICOLON
	;

block_item_list:
	  block_item
	| block_item_list block_item
	;

block_item:
	  declaration
	| statement
	/* gcc extension */
	/* nested functions */
	| function_definition
	;

expression_opt:
	  /* empty */
    {
	    $$ = 0;
    }
	| expression
	{
	    $$ = $1;
    }
	;

expression_statement:
	  expression_opt SEMICOLON
    {
	    if ($1)
		    delete $1;
    }
	;

selection_statement:
	  IF LPAREN expression RPAREN statement
    {
	    if ($3)
		    delete $3;
    }
	| IF LPAREN expression RPAREN statement ELSE statement
    {
	    if ($3)
		    delete $3;
    }
	| SWITCH LPAREN expression RPAREN statement
    {
	    if ($3)
		    delete $3;
    }
	;

iteration_statement:
	  WHILE LPAREN expression RPAREN statement
    {
	    if ($3)
		    delete $3;
    }
	| DO statement WHILE LPAREN expression RPAREN SEMICOLON
    {
	    if ($5)
		    delete $5;
    }
	| FOR LPAREN expression_opt SEMICOLON expression_opt SEMICOLON expression_opt RPAREN statement
    {
	    if ($3)
		    delete $3;
	    if ($5)
		    delete $5;
	    if ($7)
		    delete $7;
    }
	| FOR LPAREN declaration SEMICOLON expression_opt SEMICOLON expression_opt RPAREN statement
	{
	    if ($5)
		    delete $5;
	    if ($7)
		    delete $7;
	}
	;

jump_statement:
	  GOTO IDENTIFIER SEMICOLON
	| CONTINUE SEMICOLON
	| BREAK SEMICOLON
	| RETURN expression_opt SEMICOLON
	{
	    if ($2)
		    delete $2;
	}
	/* gcc extension */
	/* allow to jump to label variables,
	   e.g., goto *lbl_ptr; */
	| GOTO MUL expression SEMICOLON
	{
	    if ($3)
		    delete $3;
	}
	;

/* gcc extension */
asm_statement:
      ASM_KEYWORD VOLATILE LPAREN asm_statement_interior RPAREN
    | ASM_KEYWORD LPAREN asm_statement_interior RPAREN
	;

asm_statement_interior:
      asm_code COLON asm_constraints COLON asm_constraints COLON asm_clobber_list
    | asm_code COLON asm_constraints COLON asm_constraints
	| asm_code COLON asm_constraints
	| asm_code
	;

asm_code:
      string
    {
	    delete $1;
    }
    ;

asm_constraints:
      /* empty */
    | asm_constraint_list
    ;

asm_constraint_list:
      asm_constraint
    | asm_constraint_list COMMA asm_constraint
	;

asm_constraint:
      LBRACKET IDENTIFIER RBRACKET string LPAREN expression RPAREN
    {
	    if ($2)
		    delete $2;
        if ($4)
			delete $4;
		if ($6)
			delete $6;
    }
    | string LPAREN expression RPAREN
	{
	    if ($1)
		    delete $1;
        if ($3)
		    delete $3;
	}
	;

asm_clobber_list:
      string
    {
	    delete $1;
    }
    | asm_clobber_list COMMA string
	{
	    delete $3;
	}
	;

/* A.2.4 External definitions */

file:
      /* empty */
    | translation_unit
	| error
	{
	    fprintf(stderr, "Error while parsing C input file \"%s\":%d\n", (const char*)sInFileName, gLineNumber);
		errcount++;
		YYABORT;
	}
    ;

translation_unit:
	  external_declaration
	| translation_unit external_declaration
	;

external_declaration:
	  function_definition
    {
	    // discard function definition
    }
	| declaration
	{
	    // if declaration was important it has been added
		// otherwise it has been deleted
	}
	| EOF_TOKEN
	{
	    YYACCEPT;
	}
	| SEMICOLON
	;

function_definition:
	  function_specifier compound_statement opt_semicolon
	/* gcc extension */
	/* function attributes */
	| function_specifier function_attribute compound_statement opt_semicolon
	;

/* Dice specific */
function_specifier:
	  declaration_specifiers function_declarator
    {
	    // discard the specifiers
		CObject *obj;
		while ((obj = $1->RemoveFirst()) != 0)
		    delete obj;
		delete $1;
		// delete the function declarator
		if ($2)
		    delete $2;
	}
	| declaration_specifiers function_declarator declaration_list
    {
	    // discard the specifiers
		CObject *obj;
		while ((obj = $1->RemoveFirst()) != 0)
		    delete obj;
		delete $1;
		// delete the function declarator
		if ($2)
		    delete $2;
	}
	| declaration_specifiers pointer function_declarator
    {
	    // discard the specifiers
		CObject *obj;
		while ((obj = $1->RemoveFirst()) != 0)
		    delete obj;
		delete $1;
		// delete the function declarator
		if ($3)
		    delete $3;
	}
	| declaration_specifiers pointer function_declarator declaration_list
    {
	    // discard the specifiers
		CObject *obj;
		while ((obj = $1->RemoveFirst()) != 0)
		    delete obj;
		delete $1;
		// delete the function declarator
		if ($3)
		    delete $3;
	}
	;

declaration_list:
	  declaration
	| declaration_list declaration
	;

/* gcc extension */
function_attribute:
	  ATTRIBUTE LPAREN LPAREN function_attributes_list RPAREN RPAREN
	;

/* gcc extension */
function_attributes_list:
	  function_attributes
	| function_attributes_list COMMA function_attributes
	;

/* gcc extension */
function_attributes:
	  /* empty */
	| function_attribute_keyword
	| function_attribute_keyword LPAREN attribute_parameter_list RPAREN
	;

/* gcc extension */
attribute_parameter_list:
	  attribute_parameter
	| attribute_parameter_list COMMA attribute_parameter
	;

/* gcc extension */
attribute_parameter:
	  /* IDENTIFIER /* covered by expression */
	  expression
    {
	    if ($1)
		    delete $1;
    }
	;

/* gcc extension */
/* the following keywords are supported, but we ignore them
   and check for tokenized ones only:
	  noreturn
	| noinline
	| always_inline
	| pure
	| const
	| nothrow
	| format
	| format_arg
	| no_instrument_function
	| section
	| constructor
	| destructor
	| used
	| unused
	| deprecated
	| weak
	| malloc
	| alias
	| nonnull
  */
function_attribute_keyword:
	  CONST
	| IDENTIFIER
	{
	    if ($1)
		    delete $1;
	}
	;

/* gcc extension */
type_attribute_opt:
      /* empty */
    | type_attribute
	;

/* gcc extension */
type_attribute:
      ATTRIBUTE LPAREN LPAREN type_attributes_list RPAREN RPAREN
    ;

/* gcc extension */
type_attributes_list:
      type_attributes
    | type_attributes_list COMMA type_attributes
	;

/* gcc extension */
type_attributes:
      /* empty */
    | type_attribute_keyword
	| type_attribute_keyword LPAREN attribute_parameter_list RPAREN
	;

/* gcc extension */
/* the following keywords are supported, but we ignore them
   and check for tokenized ones only:
      aligned
    | packed
	| transparent_union
	| unused
	| deprecated
	| may_alias
 */
type_attribute_keyword:
      IDENTIFIER
    {
	    if ($1)
		    delete $1;
    }
    ;
/* A.3 Preprocessing directives */
/* we ignore them - processed by CPP */

/* some convenience rules */

opt_semicolon:
      /* empty */
    | SEMICOLON
	;


/* end of grammar */

/* GCC extensions, which are not supported:
   - Constructing Function Calls
   - Generalized Lvalues
   - Complex Numbers (partially supported - as defined by C99)
   - Hex floats
   - Zero length arrays (this of no interest for the parser)
   - Variable length arrays (same as zero length arrays)
   - Macros with a Variable Number of Arguments. (handled by preprocessor)
   - Slightly Looser Rules for Escaped Newlines (handled by preprocessor)
   - String Literals with Embedded Newlines
   - Non-Lvalue Arrays May Have Subscripts (semantic not relevant for IDL parser)
   - Arithmetic on void- and Function-Pointers (...)
   - Compound Literal (ISO C99)
   - Designated Initializers (ISO C99)
   - Cast to a Union Type (irrelevant semantic)
   - Mixed Declarations and Code (ISO C99)
   - Prototypes and Old-Style Function Definitions
   - Dollar Signs in Identifier Names

Todo:
   - Specifying Attributes of Types
   - Variables in Specified Registers
   - Alternate Keywords
   - Incomplete enum Types
   - Function Names as Strings
   - builtins
   - pragmas
   - Unnamed struct/union fields within structs/unions.
   - Thread-Local Storage
 */

string:
      STRING
	| string STRING
	;

%%


void
gcc_cerror(char* s)
{
	// ignore this functions -> it's called by bison code, which is not controlled by us
	nParseErrorGCC_C = 1;
}

void
gcc_cerror2(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	CCompiler::GccErrorVL(CParser::GetCurrentFile(), gLineNumber, fmt, args);
	va_end(args);
	nParseErrorGCC_C = 0;
	erroccured = 1;
	errcount++;
}

