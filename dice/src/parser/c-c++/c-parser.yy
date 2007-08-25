%skeleton "lalr1.cc"
%require "2.1a"
%defines
%define "parser_class_name" "c_parser"

%{
#include "fe/FEFunctionDeclarator.h"
#include "fe/FEArrayDeclarator.h"
#include "fe/FEEnumDeclarator.h"
#include "fe/FEConditionalExpression.h"
#include "fe/FESizeOfExpression.h"
#include "fe/FEAlignOfExpression.h"
#include "fe/FEUserDefinedExpression.h"
#include "fe/FEStringAttribute.h"
#include "fe/FEUserDefinedType.h"
#include "fe/FEEnumType.h"
#include "fe/FEStructType.h"
#include "fe/FEUnionType.h"
#include "fe/FESimpleType.h"
#include "fe/FETypeOfType.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEFile.h"
#include <vector>

class c_parser_driver;

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif
%}

// The parsing context.
%parse-param { c_parser_driver& driver }
%lex-param   { c_parser_driver& driver }

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
  int			ival;
  long			lval;
  unsigned long		ulval;
#if SIZEOF_LONG_LONG > 0
  long long		llval;
  unsigned long long	ullval;
#endif
  double		dval;
  signed char		cval;
  std::string		*sval;
  enum EXPT_OPERATOR	uop;

  CFEDeclarator*		decl;
  CFEFunctionDeclarator*	fDecl;
  CFEArrayDeclarator*		aDecl;
  CFETypedDeclarator*		tDecl;
  CFEExpression*		expr;
  CFEAttribute*			attr;
  CFETypeSpec*			type;
  CFEStructType*		struct_t;
  CFEUnionType*			union_t;
  CFEIdentifier*		ident;
  CFEUnionCase*			union_c;
  CFEInterface*			iface;

  std::vector<CFEIdentifier*>*	vecIdent;
  std::vector<CFEDeclarator*>*	vecDecl;
  std::vector<CFETypedDeclarator*>*	vecTD;
  std::vector<CFEAttribute*>*	vecAttr;
  std::vector<CFEExpression*>*	vecExpr;
  std::vector<CFEUnionCase*>*	vecUC;
  std::vector<CFETypeSpec*>*	vecType;
};

%{
#include "c-parser-driver.hh"
%}

%token		EOF_TOKEN	0 "end of file"
%token		INVALID		1 "invalid token"
%token		BOGUS
%token <sval>	ID		"identifier"
%token <sval>   TYPEDEF_ID	"type name"
%token <sval>	CLASS_ID	"class name"
%token <sval>	NAMESPACE_ID	"namespace name"
%token <sval>	ENUM_ID		"enum name"
%token <sval>	TEMPLATE_ID	"template name"

%token		ADD_ASSIGN	"+="
%token		ALIGNOF		"alignof"
%token		AND_ASSIGN	"&="
%token		ASM_KEYWORD	"asm"
%token		ASTERISK	"*"
%token		ATTRIBUTE	"attribute"
%token		AUTO		"auto"
%token		BITAND		"&"
%token		BITOR		"|"
%token		BITXOR		"^"
%token		BOOLEAN		"boolean"
%token		BREAK		"break"
%token		BYCOPY		"bycopy"
%token		BYREF		"byref"
%token		CASE		"case"
%token		CATCH		"catch"
%token		CHAR		"char"
%token		CLASS		"class"
%token		COLON		":"
%token		COMMA		","
%token		CONST		"const"
%token		CONST_CAST	"const_cast<>"
%token		CONTINUE	"continue"
%token		DEC_OP		"--"
%token		DEFAULT		"default"
%token		DELETE		"delete"
%token		DIV		"/"
%token		DIV_ASSIGN	"/="
%token		DO		"do"
%token		DOT		"."
%token		DOTSTAR_OP	".*"
%token		DOUBLE		"double"
%token		DYNAMIC_CAST	"dynamic_cast<>"
%token		ELSE		"else"
%token		ELLIPSIS	"..."
%token		ENUM		"enum"
%token		EQUAL		"=="
%token		EXCLAM		"!"
%token		EXPLICIT	"explicit"
%token		EXPNULL		"null"
%token		EXPORT		"export"
%token		EXTERN		"extern"
%token		EXTERN_LANG_STRING	"extern \"C\""
%token		FALSE		"false"
%token		FLOAT		"float"
%token		FOR		"for"
%token		FRIEND		"friend"
%token		GOTO		"goto"
%token		GT		">"
%token		GTEQUAL		">="
%token		IF		"if"
%token		IMAGPART	"imaginary part"
%token		IN		"in"
%token		INC_OP		"++"
%token		INLINE		"inline"
%token		INOUT		"inout"
%token		INT		"int"
%token		IS		"="
%token		LBRACE		"{"
%token		LBRACKET	"["
%token		LEFT_RIGHT	"( )"
%token	<cval>	LIT_CHAR	"character"
%token	<dval>	LIT_FLOAT	"float value"
%token	<ival>	LIT_INT		"integer value"
%token	<lval>  LIT_LONG	"long value"
%token	<ulval>	LIT_ULONG	"unsigned long value"
%token	<llval>	LIT_LLONG	"long long value"
%token	<ullval> LIT_ULLONG	"unsigned long long value"
%token	<sval>	LIT_STR		"literal string"
%token		LOGICALAND	"&&"
%token		LOGICALOR	"||"
%token		LONG		"long"
%token		LONGLONG	"long long"
%token		LPAREN		"("
%token		LS_ASSIGN	"<<="
%token		LSHIFT		"<<"
%token		LT		"<"
%token		LTEQUAL		"<="
%token		MAX		">?"
%token		MAX_ASSIGN	">?="
%token		MIN		"<?"
%token		MIN_ASSIGN	"<?="
%token		MINUS		"-"
%token		MOD		"%"
%token		MOD_ASSIGN	"%="
%token		MUL_ASSIGN	"*="
%token		MUTABLE		"mutable"
%token		NAMESPACE	"namespace"
%token		NEW		"new"
%token		NOTEQUAL	"!="
%token		ONEWAY		"oneway"
%token		OPERATOR	"operator"
%token		OR_ASSIGN	"|="
%token		OUT		"out"
%token		PAREN_STAR_PAREN	"( * )"
%token		PLUS		"+"
%token		PRIVATE		"private"
%token		PROTECTED	"protected"
%token		PTR_OP		"->"
%token		PTRSTAR_OP	"->*"
%token		PUBLIC		"public"
%token		QUESTION	"?"
%token		RBRACE		"}"
%token		RBRACKET	"]"
%token		REALPART	"real part"
%token		REGISTER	"register"
%token		REINTERPRET_CAST	"reinterpret_cast<>"
%token		RESTRICT	"restrict"
%token		RETURN		"return"
%token		RPAREN		")"
%token		RS_ASSIGN	">>="
%token		RSHIFT		">>"
%token		SCOPE		"::"
%token		SEMICOLON	";"
%token		SHORT		"short"
%token		SIGNED		"signed"
%token		SIZEOF		"sizeof"
%token		STATIC		"static"
%token		STATIC_CAST	"static_cast<>"
%token		STRUCT		"struct"
%token		SUB_ASSIGN	"-="
%token		SWITCH		"switch"
%token		TEMPLATE	"template"
%token		THIS		"this"
%token		THROW		"throw"
%token		TILDE		"~"
%token		TRUE		"true"
%token		TRY		"try"
%token		TYPEDEF		"typedef"
%token		TYPEID		"typeid"
%token		TYPENAME	"typename"
%token		TYPEOF		"typeof"
%token		UCOMPLEX	"_Complex"
%token		UNION		"union"
%token		UNSIGNED	"unsigned"
%token		USING		"using"
%token		VIRTUAL		"virtual"
%token		VOID		"void"
%token		VOLATILE	"volatile"
%token		WCHAR		"wchar_t"
%token		WHILE		"while"
%token		XOR_ASSIGN	"^="

%type	<decl>		abstract_declarator
%type	<expr>		additive_expression
%type	<expr>		and_expression
%type	<sval>		any_word
%type	<aDecl>		array_declarator
%type	<vecAttr>	attribute
%type	<vecAttr>	attribute_list
%type	<attr>		attribute_parameter
%type	<vecAttr>	attribute_seq
%type	<expr>		assignment_expression
%type   <uop>		assignment_operator
%type	<vecIdent>	base_clause
%type	<ident>		base_specifier
%type	<vecIdent>	base_specifier_list
%type	<expr>		cast_expression
%type	<type>		char_type
%type	<expr>		conditional_expression
%type	<sval>		cv_qualifier
%type	<decl>		declarator
%type	<decl>		declarator_id
%type	<type>		decl_specifier
%type	<type>		decl_specifier0
%type	<vecType>	decl_specifier_seq
%type	<expr>		delete_expression
%type	<decl>		direct_abstract_declarator
%type	<decl>		direct_declarator
%type	<type>		enum_specifier
%type	<decl>		enumerator_definition
%type	<sval>		enumerator
%type	<vecIdent>	enumerator_list
%type	<vecIdent>	enumerator_list_opt
%type	<expr>		equality_expression
%type	<expr>		exclusive_or_expression
%type	<expr>		expression
%type	<type>		floating_pt_type
%type	<fDecl>		function_declarator
%type	<sval>		function_specifier
%type	<decl>		id_expression
%type	<expr>		inclusive_or_expression
%type	<decl>		init_declarator
%type	<vecDecl>	init_declarator_list
%type	<type>		integer_type
%type	<ival>		integer_size
%type	<expr>		literal
%type	<expr>		logical_and_expression
%type	<expr>		logical_or_expression
%type	<vecAttr>	maybeattribute
%type	<tDecl>		member_declaration
%type	<decl>		member_declarator
%type	<vecDecl>	member_declarator_list
%type	<vecTD>		member_specification_opt
%type	<vecTD>		member_specification
%type	<expr>		multiplicative_expression
%type	<sval>		nested_name_specifier
%type	<expr>		new_initializer
%type	<expr>		new_placement
%type	<sval>		operator_function_id
%type	<tDecl>		parameter_declaration
%type	<vecTD>		parameter_declaration_clause
%type	<vecTD>		parameter_declaration_list
%type	<expr>		pm_expression
%type	<expr>		postfix_expression
%type	<expr>		primary_expression
%type	<ival>		ptr_operator
%type	<ival>		ptr_operator_seq
%type	<decl>		qualified_id
%type	<sval>		qualified_namespace_specifier
%type	<expr>		relational_expression
%type	<type>		simple_type_specifier
%type	<expr>		shift_expression
%type	<sval>		storage_class_specifier
%type	<struct_t>	struct_head
%type	<struct_t>	struct_specifier
%type	<sval>		template_id
%type	<type>		type_id
%type	<sval>		type_name
%type	<type>		type_specifier
%type	<vecType>	type_specifier_seq
%type	<expr>		throw_expression
%type	<expr>		unary_expression
%type	<uop>		unary_operator
%type   <union_c>	union_arm
%type	<vecUC>		union_body_opt
%type	<vecUC>		union_body
%type	<union_t>	union_head
%type	<union_t>	union_specifier
%type	<decl>		unqualified_id

%printer    { debug_stream () << *$$; } "identifier" "literal string"
%destructor { delete $$; } "identifier" "literal string"

%printer    { debug_stream () << $$; } "character" "float value" "integer value"

%left error

/* Add precedence rules to solve dangling else s/r conflict */
%nonassoc IF
%nonassoc ELSE

%left TYPENAME ENUM CLASS STRUCT UNION ELLIPSIS TYPEOF OPERATOR

%right SIGNED UNSIGNED
%left LBRACE COMMA SEMICOLON

%nonassoc THROW
%right COLON
%right ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN
%right AND_ASSIGN OR_ASSIGN  XOR_ASSIGN RS_ASSIGN  LS_ASSIGN
%right MIN_ASSIGN MAX_ASSIGN
%right QUESTION
%left LOGICALXOR
%left LOGICALOR
%left LOGICALAND
%left BITOR
%left BITXOR
%left BITAND
%left MIN MAX
%left EQUAL
%left LTEQUAL GTEQUAL LT GT
%left LSHIFT RSHIFT
%left PLUS MINUS
%left ASTERISK DIV MOD
%left PTRSTAR_OP DOTSTAR_OP
%right UNARY INC_OP DEC_OP TILDE
%left LEFT_RIGHT
%left PTR_OP DOT LPAREN LBRACKET

%right SCOPE            /* C++ extension */
%nonassoc NEW DELETE TRY CATCH

/* intermediate token */

%start file

%%

/* A.1 Keywords */
/* typedef_name : TYPEDEF_ID ; inlined */

/* original_namespace_name and namespace_alias are covered by namespace_name
 */
/* namespace_name : NAMESPACE_ID ; inlined */

/* class_name : CLASS_ID | template_id	; */

/* enum_name : ENUM_ID ; inlined */

/* template_name : TEMPLATE_ID ; inlined */

/* A.2 Lexical conventions: (mostly) covered by scanner */
literal :
	  LIT_INT
	{
	    $$ = new CFEPrimaryExpression(EXPR_INT, (long)$1);
	}
	| LIT_CHAR
	{
	    $$ = new CFEUserDefinedExpression(std::string(1, $1));
	}
	| LIT_FLOAT
	{
	    $$ = new CFEPrimaryExpression(EXPR_FLOAT, (long double)$1);
	}
	| LIT_LONG
	{
	    $$ = new CFEPrimaryExpression(EXPR_INT, $1);
	}
	| LIT_ULONG
	{
	    $$ = new CFEPrimaryExpression(EXPR_UINT, $1);
	}
	| LIT_LLONG
	{
	    $$ = new CFEPrimaryExpression(EXPR_LLONG, $1);
	}
	| LIT_ULLONG
	{
	    $$ = new CFEPrimaryExpression(EXPR_ULLONG, $1);
	}
	| LIT_STR
	{
	    $$ = new CFEUserDefinedExpression(*$1);
	}
	| TRUE
	{
	    $$ = new CFEUserDefinedExpression(std::string("true"));
	}
	| FALSE
	{
	    $$ = new CFEUserDefinedExpression(std::string("false"));
	}
	| EXPNULL
	{
	    $$ = new CFEUserDefinedExpression(std::string("null"));
	}
	;

/* A.3 Basic concepts */
file :
	  declaration_seq
	| /* empty */
	;

/* A.4 Expressions */
primary_expression :
	  literal
	| THIS
	{
	    $$ = new CFEUserDefinedExpression(std::string("this"));
	    $$->m_sourceLoc = @$;
	}
	| SCOPE ID
	{
	    $$ = new CFEUserDefinedExpression(std::string("::") + *$2);
	    $$->m_sourceLoc = @$;
	}
	| SCOPE operator_function_id
	{
	    $$ = new CFEUserDefinedExpression(std::string("::") + *$2);
	    $$->m_sourceLoc = @$;
	}
	| SCOPE qualified_id
	{
	    std::string s("::");
	    if ($2)
		s += $2->GetName();
	    $$ = new CFEUserDefinedExpression(s);
	    $$->m_sourceLoc = @$;
	    if ($2)
		delete $2;
	}
	| id_expression
	{
	    std::string s = $1 ? $1->GetName() : std::string();
	    $$ = new CFEUserDefinedExpression(s);
	    $$->m_sourceLoc = @$;
	    if ($1)
		delete $1;
	}
	| LPAREN expression RPAREN
	{
	    if ($2)
	    {
		$$ = new CFEPrimaryExpression(EXPR_PAREN, $2);
		$2->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	;

id_expression :
	  qualified_id
	| unqualified_id
	;

unqualified_id :
	  ID
	{
	    $$ = new CFEDeclarator(DECL_IDENTIFIER, *$1);
	}
	| operator_function_id
	{
	    $$ = NULL;
	}
	| conversion_function_id
	{
	    $$ = NULL;
	}
	/* to reduce conflicts we use type-name instead of class-name here */
	/* trick to avoid reduce conflicts: use empty rule in the middle */
	| TILDE see_typename type_name
	{
	    $$ = NULL;
	}
	/* explicetly use template syntax to reduce conflicts */
	/* do NOT use class_name here, because that will clash
	 * nested_name_specifier : nested_name_specifier unqualified_id with
	 * nested_name_specifier type_name (and type_name : class_name)
	 */
// 	| template_id
// 	{
// 	$$ = new CFEDeclarator(DECL_IDENTIFIER, *$1);
// 	}
	;

see_typename :
	  /* empty */
	;

qualified_id :
	  nested_name_specifier TEMPLATE unqualified_id
	{
	    $$ = $3;
	    $$->Prefix(" template ");
	    $$->Prefix(*$1);
	}
	| nested_name_specifier          unqualified_id
	{
	    $$ = $2;
	    $$->Prefix(*$1);
	}
	;

/* to avoid conflicts we explicetly use class_name and namespace_name instead
 * of class-or-namespace-name here */
nested_name_specifier :
	  CLASS_ID SCOPE
	{
	    $$ = $1;
	    $1->append("::");
	}
	| template_id SCOPE
	{
	    $$ = $1;
	    $1->append("::");
	}
	| NAMESPACE_ID SCOPE
	{
	    $$ = $1;
	    $1->append("::");
	}
	| nested_name_specifier CLASS_ID SCOPE
	{
	    $$ = $1;
	    $$->append(*$2);
	    $$->append("::");
	}
	| nested_name_specifier template_id SCOPE
	{
	    $$ = $1;
	    $$->append(*$2);
	    $$->append("::");
	}
	| nested_name_specifier NAMESPACE_ID SCOPE
	{
	    $$ = $1;
	    $$->append(*$2);
	    $$->append("::");
	}
	;

postfix_expression :
	  primary_expression
	| postfix_expression LBRACKET expression RBRACKET
	{ $$ = NULL; }
	| postfix_expression LPAREN expression RPAREN
	{ $$ = NULL; }
	| postfix_expression LEFT_RIGHT
	{ $$ = NULL; }
	| simple_type_specifier LPAREN expression RPAREN
	{ $$ = NULL; }
	| simple_type_specifier LPAREN RPAREN
	{ $$ = NULL; }
	| postfix_expression DOT TEMPLATE SCOPE id_expression
	{ $$ = NULL; }
	| postfix_expression DOT          SCOPE id_expression
	{ $$ = NULL; }
	| postfix_expression DOT TEMPLATE       id_expression
	{ $$ = NULL; }
	| postfix_expression DOT                id_expression
	{ $$ = NULL; }
	| postfix_expression PTR_OP TEMPLATE SCOPE id_expression
	{ $$ = NULL; }
	| postfix_expression PTR_OP          SCOPE id_expression
	{ $$ = NULL; }
	| postfix_expression PTR_OP TEMPLATE       id_expression
	{ $$ = NULL; }
	| postfix_expression PTR_OP                id_expression
	{ $$ = NULL; }
	| postfix_expression DOT pseudo_destructor_name
	{ $$ = NULL; }
	| postfix_expression PTR_OP pseudo_destructor_name
	{ $$ = NULL; }
	| postfix_expression INC_OP
	{ $$ = NULL; }
	| postfix_expression DEC_OP
	{ $$ = NULL; }
	| DYNAMIC_CAST LT type_id GT LPAREN expression RPAREN
	{ $$ = NULL; }
	| STATIC_CAST LT type_id GT LPAREN expression RPAREN
	{ $$ = NULL; }
	| REINTERPRET_CAST LT type_id GT LPAREN expression RPAREN
	{ $$ = NULL; }
	| CONST_CAST LT type_id GT LPAREN expression RPAREN
	{ $$ = NULL; }
	| TYPEID LPAREN expression RPAREN
	{ $$ = NULL; }
	| TYPEID LPAREN type_id RPAREN
	{ $$ = NULL; }
	/* C99 */
	| LPAREN type_id RPAREN LBRACE initializer_list COMMA RBRACE
	{ $$ = NULL; }
	| LPAREN type_id RPAREN LBRACE initializer_list       RBRACE
	{ $$ = NULL; }
	| LPAREN type_id RPAREN LBRACE                        RBRACE
	{ $$ = NULL; }
	;

expression :
	  assignment_expression
	| expression COMMA assignment_expression
	{ $$ = $1; }
	;

pseudo_destructor_name :
	  SCOPE nested_name_specifier type_name SCOPE TILDE type_name
	| SCOPE                       type_name SCOPE TILDE type_name
	|       nested_name_specifier type_name SCOPE TILDE type_name
	|                             type_name SCOPE TILDE type_name
	| SCOPE nested_name_specifier TILDE type_name
	| SCOPE                       TILDE type_name
	|       nested_name_specifier TILDE type_name
	|                             TILDE type_name
	;

unary_expression :
	  postfix_expression
	| INC_OP cast_expression
	{
	    if ($2)
	    {
		$$ = new CFEUnaryExpression(EXPR_UNARY, EXPR_INCR, $2);
		$2->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| DEC_OP cast_expression
	{
	    if ($2)
	    {
		$$ = new CFEUnaryExpression(EXPR_UNARY, EXPR_DECR, $2);
		$2->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	/* ASTERISK, BITAND, TILDE inlined to avoid reduce/reduce conflicts */
	| ASTERISK cast_expression
	{
	    if ($2)
	    {
		$$ = new CFEUnaryExpression(EXPR_UNARY, EXPR_MUL, $2);
		$2->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| BITAND cast_expression
	{
	    if ($2)
	    {
		$$ = new CFEUnaryExpression(EXPR_UNARY, EXPR_BITAND, $2);
		$2->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| TILDE cast_expression
	{
	    if ($2)
	    {
		$$ = new CFEUnaryExpression(EXPR_UNARY, EXPR_TILDE, $2);
		$2->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| unary_operator cast_expression
	{
	    if ($2)
	    {
		$$ = new CFEUnaryExpression(EXPR_UNARY, $1, $2);
		$2->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| SIZEOF unary_expression
	{
	    if ($2)
	    {
		$$ = new CFESizeOfExpression($2);
		$2->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| SIZEOF LPAREN type_id RPAREN
	{
	    if ($3)
	    {
		$$ = new CFESizeOfExpression($3);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| ALIGNOF unary_expression
	{
	    if ($2)
	    {
		$$ = new CFEAlignOfExpression($2);
		$2->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| ALIGNOF LPAREN type_id RPAREN
	{
	    if ($3)
	    {
		$$ = new CFEAlignOfExpression($3);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| new_expression
	{
	    $$ = NULL;
	}
	| delete_expression
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
	| EXCLAM
	{
	    $$ = EXPR_EXCLAM;
	}
	;

new_expression :
	  SCOPE NEW new_placement new_type_id new_initializer
	| SCOPE NEW               new_type_id new_initializer
	|       NEW new_placement new_type_id new_initializer
	|       NEW               new_type_id new_initializer
	| SCOPE NEW new_placement new_type_id
	| SCOPE NEW               new_type_id
	|       NEW new_placement new_type_id
	|       NEW               new_type_id
	| SCOPE NEW new_placement LPAREN type_id RPAREN new_initializer
	| SCOPE NEW               LPAREN type_id RPAREN new_initializer
	|       NEW new_placement LPAREN type_id RPAREN new_initializer
	|       NEW               LPAREN type_id RPAREN new_initializer
	| SCOPE NEW new_placement LPAREN type_id RPAREN
	| SCOPE NEW               LPAREN type_id RPAREN
	|       NEW new_placement LPAREN type_id RPAREN
	|       NEW               LPAREN type_id RPAREN
	;

new_placement :
	  LPAREN expression RPAREN
	{
	    $$ = $2;
	}
	;

new_initializer :
	  LPAREN expression RPAREN
	{
	    $$ = $2;
	}
	| LEFT_RIGHT
	{
	    $$ = NULL;
	}
	;

new_type_id :
	  type_specifier_seq ptr_operator_seq direct_new_declarator
	| type_specifier_seq                  direct_new_declarator
	| type_specifier_seq
	;

/* the rule new_declarator : ptr_operator* direct_new_declarator has been
 * merged into new_type_id with creation of ptr_operator_seq */
ptr_operator_seq :
	  ptr_operator
	{ $$ = $1; }
	| ptr_operator_seq ptr_operator
	{ $$ = $1 + $2; }
	;

direct_new_declarator :
	  LBRACKET expression RBRACKET
	/* replace constant_expression with conditional_expression */
	| direct_new_declarator LBRACKET conditional_expression RBRACKET
	;

delete_expression :
	  SCOPE DELETE                   cast_expression
	{
	    $$ = new CFEUnaryExpression(EXPR_UNARY, EXPR_DELETE, $3);
	    if ($3)
		$3->SetParent($$);
	}
	|       DELETE                   cast_expression
	{
	    $$ = new CFEUnaryExpression(EXPR_UNARY, EXPR_DELETE, $2);
	    if ($2)
		$2->SetParent($$);
	}
	| SCOPE DELETE LBRACKET RBRACKET cast_expression
	{
	    $$ = new CFEUnaryExpression(EXPR_UNARY, EXPR_DELETE_ARRAY, $5);
	    if ($5)
		$5->SetParent($$);
	}
	|       DELETE LBRACKET RBRACKET cast_expression
	{
	    $$ = new CFEUnaryExpression(EXPR_UNARY, EXPR_DELETE_ARRAY, $4);
	    if ($4)
		$4->SetParent($$);
	}
	;

cast_expression :
	  unary_expression
	| LPAREN type_id RPAREN cast_expression
	{
	    if ($4)
	    {
		if ($2)
		{
		    $$ = new CFEUnaryExpression($2, $4);
		    $2->SetParent($$);
		    $4->SetParent($$);
		}
		else
		    $$ = $4;
	    }
	    else
		$$ = NULL;
	}
	;

pm_expression :
	  cast_expression
	| pm_expression DOTSTAR_OP cast_expression
	{
	    $$ = NULL;
	}
	| pm_expression PTRSTAR_OP cast_expression
	{
	    $$ = NULL;
	}
	;

multiplicative_expression :
	  pm_expression
	| multiplicative_expression ASTERISK pm_expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MUL, $3);
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| multiplicative_expression DIV pm_expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_DIV, $3);
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| multiplicative_expression MOD pm_expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MOD, $3);
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	;

additive_expression :
	  multiplicative_expression
	| additive_expression PLUS multiplicative_expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_PLUS, $3);
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| additive_expression MINUS multiplicative_expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MINUS, $3);
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| additive_expression LIT_INT
	{
	    if ($2 >= 0)
	    {
		driver.error(@2, "Invalid expression.");
		YYABORT;
	    }
	    CFEExpression *t = new CFEPrimaryExpression(EXPR_INT, (long)-$2);
	    $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MINUS, t);
	    t->SetParent($$);
	    if ($1)
		$1->SetParent($$);
	}
	| additive_expression LIT_LONG
	{
	    if ($2 >= 0)
	    {
		driver.error(@2, "Invalid expression.");
		YYABORT;
	    }
	    CFEExpression *t = new CFEPrimaryExpression(EXPR_INT, -$2);
	    $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MINUS, t);
	    t->SetParent($$);
	    if ($1)
		$1->SetParent($$);
	}
	| additive_expression LIT_LLONG
	{
	    if ($2 >= 0)
	    {
		driver.error(@2, "Invalid expression.");
		YYABORT;
	    }
	    CFEExpression *t = new CFEPrimaryExpression(EXPR_LLONG, -$2);
	    $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MINUS, t);
	    t->SetParent($$);
	    if ($1)
		$1->SetParent($$);
	}
	;

shift_expression :
	  additive_expression
	| shift_expression LSHIFT additive_expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LSHIFT, $3);
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| shift_expression RSHIFT additive_expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_RSHIFT, $3);
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	;

relational_expression :
	  shift_expression
	| relational_expression LT shift_expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LT, $3);
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| relational_expression GT shift_expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_GT, $3);
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| relational_expression LTEQUAL shift_expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LTEQU, $3);
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| relational_expression GTEQUAL shift_expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_GTEQU, $3);
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	;

equality_expression :
	  relational_expression
	| equality_expression EQUAL relational_expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_EQUALS, $3);
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
	| equality_expression NOTEQUAL relational_expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_NOTEQUAL, $3);
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

and_expression :
	  equality_expression
	| and_expression BITAND equality_expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITAND, $3);
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	;

exclusive_or_expression :
	  and_expression
	| exclusive_or_expression BITXOR and_expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITXOR, $3);
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	;

inclusive_or_expression :
	  exclusive_or_expression
	| inclusive_or_expression BITOR exclusive_or_expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITOR, $3);
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	;

logical_and_expression :
	  inclusive_or_expression
	| logical_and_expression LOGICALAND inclusive_or_expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LOGAND, $3);
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	;

logical_or_expression :
	  logical_and_expression
	| logical_or_expression LOGICALOR logical_and_expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LOGOR, $3);
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	;

conditional_expression :
	  logical_or_expression
	| logical_or_expression QUESTION expression COLON assignment_expression
	{
	    $$ = new CFEConditionalExpression($1, $3, $5);
	    if ($1)
		$1->SetParent($$);
	    if ($3)
		$3->SetParent($$);
	    if ($5)
		$5->SetParent($$);
	}
	/* gnu99 */
	| logical_or_expression QUESTION COLON assignment_expression
	{
	    $$ = new CFEConditionalExpression($1, NULL, $4);
	    if ($1)
		$1->SetParent($$);
	    if ($4)
		$4->SetParent($$);
	}
	| logical_or_expression MIN expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MIN, $3);
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| logical_or_expression MAX expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MAX, $3);
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	;

assignment_expression :
	  conditional_expression
	| logical_or_expression assignment_operator assignment_expression
	{
	    if ($1 && $3)
	    {
		$$ = new CFEBinaryExpression(EXPR_BINARY, $1, $2, $3);
		$1->SetParent($$);
		$3->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	| throw_expression
	;

assignment_operator :
	  IS
	{
	    $$ = EXPR_ASSIGN;
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
	| ADD_ASSIGN
	{
	    $$ = EXPR_PLUS_ASSIGN;
	}
	| SUB_ASSIGN
	{
	    $$ = EXPR_MINUS_ASSIGN;
	}
	| LS_ASSIGN
	{
	    $$ = EXPR_LSHIFT_ASSIGN;
	}
	| RS_ASSIGN
	{
	    $$ = EXPR_RSHIFT_ASSIGN;
	}
	| AND_ASSIGN
	{
	    $$ = EXPR_AND_ASSIGN;
	}
	| OR_ASSIGN
	{
	    $$ = EXPR_OR_ASSIGN;
	}
	| XOR_ASSIGN
	{
	    $$ = EXPR_XOR_ASSIGN;
	}
	/* gnu99 */
	| MIN_ASSIGN
	{
	    $$ = EXPR_MIN_ASSIGN;
	}
	| MAX_ASSIGN
	{
	    $$ = EXPR_MAX_ASSIGN;
	}
	;

/* an expression_list is an expression (list of COMMA separated
 * assignment_expression-s */

/* constant_expression : conditional_expression ; inlined */

/* A.5 Statements */
statement :
	  labeled_statement
	| expression_statement
	| compound_statement
	| selection_statement
	| iteration_statement
	| jump_statement
	| declaration_statement
	| try_block
	;

labeled_statement :
	  ID                          COLON statement
	/* replace constant_expression with conditional_expression */
	| CASE conditional_expression COLON statement
	| DEFAULT                     COLON statement
	;

expression_statement :
	  expression SEMICOLON
	| SEMICOLON
	;

compound_statement :
	  LBRACE statement_seq RBRACE
	| LBRACE RBRACE
	;

statement_seq :
	  statement
	| statement_seq statement
	;

selection_statement :
	  IF LPAREN condition RPAREN statement
	| IF LPAREN condition RPAREN statement ELSE statement
	| SWITCH LPAREN condition RPAREN statement
	;

condition :
	  type_specifier_seq declarator asm_definition_bare maybeattribute IS assignment_expression
	| type_specifier_seq declarator                     maybeattribute IS assignment_expression
	| expression
	;

iteration_statement :
	  WHILE LPAREN condition RPAREN statement
	| DO statement WHILE LPAREN expression RPAREN SEMICOLON
	| FOR LPAREN for_init_statement condition SEMICOLON expression RPAREN statement
	| FOR LPAREN for_init_statement           SEMICOLON expression RPAREN statement
	| FOR LPAREN for_init_statement condition SEMICOLON            RPAREN statement
	| FOR LPAREN for_init_statement           SEMICOLON            RPAREN statement
	;

for_init_statement :
	  expression_statement
	| simple_declaration
	;

jump_statement :
	  BREAK SEMICOLON
	| CONTINUE SEMICOLON
	| RETURN expression SEMICOLON
	| RETURN SEMICOLON
	| GOTO ID SEMICOLON
	;

declaration_statement :
	  block_declaration
	;

/* A.6 Declarations */
declaration_seq :
	  declaration
	| declaration_seq declaration
	| declaration_seq SEMICOLON
	;

declaration :
	  block_declaration
	| function_definition
	| template_declaration
	/* explicit instantiation */
	| TEMPLATE
	/* explicit sepcialization */
	| TEMPLATE LT GT
	| linkage_specification
	| namespace_definition
	;

block_declaration :
	  simple_declaration
	| asm_definition
	| namespace_alias_definition
	| using_declaration
	| using_directive
	;

simple_declaration :
	  /* extra rule for TYPEDEF:
	   * - If a storage-class-specifier appears in a decl-specified-seq,
	   *   there can be no TYPEDEF specifier in the same decl-specified-seq.
	   *   -> This implies that no extra storage_class_specifier is
	   *   required before TYPEDEF.
	   * - Also: a TYPEDEF defines aliases for _one_ type, -> check it. We
	   *   need the sequence though, because something like "const void *"
	   *   should work (which would only produce "void*" type, btw.).
	   */
	  TYPEDEF decl_specifier_seq init_declarator_list SEMICOLON
	{
	    if ($3)
	    {
		std::vector<CFEDeclarator*>::iterator i;
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
	    // as type $2 is used
	    CFETypeSpec *type = NULL;
	    if ($2->size() > 0)
		type = *$2->begin();
	    // $4 contains the defined aliases
	    CFETypedDeclarator *t = new CFETypedDeclarator(TYPEDECL_TYPEDEF, type, $3);
	    t->m_sourceLoc = @$;
	    type->SetParent(t);

	    // add typedef to current context
	    CFEBase *pContext = driver.getCurrentContext();
	    CFEFile *pFile = dynamic_cast<CFEFile*>(pContext);
	    CFELibrary *pLib = dynamic_cast<CFELibrary*>(pContext);
	    CFEInterface *pInterface = dynamic_cast<CFEInterface*>(pContext);
	    if (pFile)
	    {
		pFile->m_Typedefs.Add(t);
		pFile->m_sourceLoc += @$;
	    }
	    else if (pLib)
	    {
		pLib->m_Typedefs.Add(t);
		pLib->m_sourceLoc += @$;
	    }
	    else if (pInterface)
	    {
		pInterface->m_Typedefs.Add(t);
		pInterface->m_sourceLoc += @$;
	    }
	    else
	    {
		// something else is context, which is not possible: throw error
		driver.error(@$, "Typedef declared in invalid context.");
		YYABORT;
	    }
	}
	| TYPEDEF decl_specifier_seq SEMICOLON
	{
	    // the declarator is the last element of the decl_specifier_seq
	    // and is probably a user defined type
	    CFEDeclarator *decl = NULL;
	    if ($2->size() >= 2) // at least the type and the alias
	    {
		CFEUserDefinedType *user = dynamic_cast<CFEUserDefinedType*>($2->back());
		if (user)
		{
		    decl = new CFEDeclarator(DECL_IDENTIFIER, user->GetName());
		    $2->erase($2->end()-1);
		    delete user;
		}
	    }
	    else
		driver.error(@2, "Either type or declarator are missing.\n");
	    if (decl)
	    {
		std::string sName = decl->GetName();
		if (driver.trace_scanning)
		    std::cerr << "C-Parser: TYPEDEF checking " << sName << "\n";
		// check if any of the aliases has been declared before (as type)
		if (driver.check_token(sName, dice::parser::CSymbolTable::TYPENAME))
		    driver.error(@2, "Type \"" + sName + "\" already defined.");
		// add aliases to symbol table
		driver.add_token(sName, dice::parser::CSymbolTable::TYPENAME, (CFEBase*)0,
		    *@2.begin.filename, @2.begin.line, @2.begin.column);
	    }
	    else
		driver.error(@2, "Scribble, scrabble, call the debb'l.\n");
	    vector<CFEDeclarator*> *tVecD = new vector<CFEDeclarator*>();
	    tVecD->push_back(decl);
	    // as type $2 is used
	    CFETypeSpec *type = NULL;
	    if ($2->size() > 0)
		type = *$2->begin();
	    // $4 contains the defined aliases
	    CFETypedDeclarator *t = new CFETypedDeclarator(TYPEDECL_TYPEDEF, type, tVecD);
	    t->m_sourceLoc = @$;
	    type->SetParent(t);
	    if (decl)
		decl->SetParent(t);

	    // add typedef to current context
	    CFEBase *pContext = driver.getCurrentContext();
	    CFEFile *pFile = dynamic_cast<CFEFile*>(pContext);
	    CFELibrary *pLib = dynamic_cast<CFELibrary*>(pContext);
	    CFEInterface *pInterface = dynamic_cast<CFEInterface*>(pContext);
	    if (pFile)
	    {
		pFile->m_Typedefs.Add(t);
		pFile->m_sourceLoc += @$;
	    }
	    else if (pLib)
	    {
		pLib->m_Typedefs.Add(t);
		pLib->m_sourceLoc += @$;
	    }
	    else if (pInterface)
	    {
		pInterface->m_Typedefs.Add(t);
		pInterface->m_sourceLoc += @$;
	    }
	    else
	    {
		// something else is context, which is not possible: throw error
		driver.error(@$, "Typedef declared in invalid context.");
		YYABORT;
	    }
	}
	| decl_specifier_seq init_declarator_list SEMICOLON
	|                    init_declarator_list SEMICOLON
	| decl_specifier_seq                      SEMICOLON
	{
	    CFEBase *pContext = driver.getCurrentContext();
	    CFEFile *pFile = dynamic_cast<CFEFile*>(pContext);
	    CFELibrary *pLib = dynamic_cast<CFELibrary*>(pContext);
	    CFEInterface *pInterface = dynamic_cast<CFEInterface*>(pContext);
	    // iterate types and add them to local scope
	    vector<CFETypeSpec*>::iterator i;
	    for (i = $1->begin(); i != $1->end(); i++)
	    {
		CFEConstructedType *t = dynamic_cast<CFEConstructedType*>(*i);
		if (!t)
		    continue;
		if (pFile)
		{
		    pFile->m_TaggedDeclarators.Add(t);
		    pFile->m_sourceLoc += @$;
		}
		else if (pLib)
		{
		    pLib->m_TaggedDeclarators.Add(t);
		    pLib->m_sourceLoc += @$;
		}
		else if (pInterface)
		{
		    pInterface->m_TaggedDeclarators.Add(t);
		    pInterface->m_sourceLoc += @$;
		}
		else
		{
		    // something else is context, which is not possible: throw error
		    driver.error(@$, "Type declared in invalid context.");
		    YYABORT;
		}
	    }
	}
	/* explicetly specify TYPEDEF here, so we can add it to the namespace
	 * and scope */
	;

decl_specifier_seq :
	  decl_specifier
	{
	    $$ = new vector<CFETypeSpec*>();
	    if ($1)
		$$->push_back($1);
	}
	| FRIEND decl_specifier
	{
	    $$ = new vector<CFETypeSpec*>();
	    if ($2)
		$$->push_back($2);
	}
	| decl_specifier_seq decl_specifier
	{
	    $$ = $1;
	    if ($2)
		$$->push_back($2);
	}
	| decl_specifier_seq FRIEND decl_specifier
	{
	    $$ = $1;
	    if ($3)
		$$->push_back($3);
	}
	;

decl_specifier :
	  decl_specifier0 maybeasm maybeattribute
	{
	    $$ = $1;
	}
	;

decl_specifier0 :
	  storage_class_specifier
	{
	    $$ = NULL;
	}
	| type_specifier
	{
	    $$ = $1;
	}
	| function_specifier
	{
	    $$ = NULL;
	}
	/* | FRIEND explicetly in decl_specifier_seq */
	/* | TYPEDEF explicit specification in simple_declaration */
	;

function_specifier :
	  INLINE
	{
	    $$ = new std::string("inline");
	}
	| VIRTUAL
	{
	    $$ = new std::string("virtual");
	}
	| EXPLICIT
	{
	    $$ = new std::string("explicit");
	}
	;

storage_class_specifier :
	  AUTO
	{
	    $$ = new std::string("auto");
	}
	| REGISTER
	{
	    $$ = new std::string("register");
	}
	| STATIC
	{
	    $$ = new std::string("static");
	}
	| EXTERN
	{
	    $$ = new std::string("extern");
	}
	| MUTABLE
	{
	    $$ = new std::string("mutable");
	}
	;

type_specifier :
	  simple_type_specifier
	/* helps to eliminate some shift/reduce conflicts */
	| simple_type_specifier BOGUS
	| class_specifier
	{ $$ = NULL; }
	| struct_specifier
	{ $$ = $1; }
	| union_specifier
	{ $$ = $1; }
	| enum_specifier
	{ $$ = $1; }
	| elaborated_type_specifier
	{ $$ = NULL; }
	| cv_qualifier
	{ $$ = NULL; }
	/* C99 */
	| IMAGPART
	{ $$ = NULL; }
	| REALPART
	{ $$ = NULL; }
	| UCOMPLEX
	{ $$ = NULL; }
	/* typedef_name is covered by elaborated_type_specifier -> type_name
	 * -> typedef_name */
	/* gnu99 */
	| TYPEOF LPAREN expression RPAREN
	{
	    $$ = new CFETypeOfType($3);
	    $$->m_sourceLoc = @$;
	}
	/* expression covers type_name */
	| TYPEOF LPAREN type_name RPAREN
	{
	    CFEExpression *t = new CFEUserDefinedExpression(*$3);
	    t->m_sourceLoc = @3;
	    $$ = new CFETypeOfType(t);
	    $$->m_sourceLoc = @$;
	}
	;

simple_type_specifier :
	  SCOPE nested_name_specifier see_typename type_name
	{
	    $$ = new CFEUserDefinedType(std::string("::") + *$2 + *$4);
	    $$->m_sourceLoc = @$;
	}
	|       nested_name_specifier see_typename type_name
	{
	    $$ = new CFEUserDefinedType(*$1 + *$3);
	    $$->m_sourceLoc = @$;
	}
	| SCOPE see_typename          type_name
	{
	    $$ = new CFEUserDefinedType(std::string("::") + *$3);
	    $$->m_sourceLoc = @$;
	}
	/* XXX if we eliminate type_name the last 9 reduce/reduce conflicts go
	 * away.*/
	| type_name
	{
	    $$ = new CFEUserDefinedType(*$1);
	    $$->m_sourceLoc = @$;
	}
	| char_type
	| BOOLEAN
	{
	    $$ = new CFESimpleType(TYPE_BOOLEAN);
	}
	| integer_type
	| floating_pt_type
	| VOID
	{
	    $$ = new CFESimpleType(TYPE_VOID);
	}
	;

elaborated_type_specifier :
	  TYPENAME SCOPE nested_name_specifier ID
	| TYPENAME       nested_name_specifier ID
	| TYPENAME SCOPE                       ID
	| TYPENAME                             ID
	| TYPENAME SCOPE nested_name_specifier ID LT template_argument_list GT
	| TYPENAME       nested_name_specifier ID LT template_argument_list GT
	| TYPENAME SCOPE                       ID LT template_argument_list GT
	;

char_type :
	  SIGNED CHAR
	{
	    $$ = new CFESimpleType(TYPE_CHAR);
	}
	| SIGNED WCHAR
	{
	    $$ = new CFESimpleType(TYPE_WCHAR);
	}
	| UNSIGNED CHAR
	{
	    $$ = new CFESimpleType(TYPE_CHAR, true);
	}
	| UNSIGNED WCHAR
	{
	    $$ = new CFESimpleType(TYPE_WCHAR, true);
	}
	| CHAR
	{
	    $$ = new CFESimpleType(TYPE_CHAR);
	}
	| WCHAR
	{
	    $$ = new CFESimpleType(TYPE_WCHAR);
	}
	;

integer_type :
	  SIGNED integer_size INT
	{
	    if ($2 == 4)
		$$ = new CFESimpleType(TYPE_LONG, false, false, $2);
	    else
		$$ = new CFESimpleType(TYPE_INTEGER, false, false, $2);
	}
	| SIGNED              INT
	{
	    $$ = new CFESimpleType(TYPE_INTEGER, false, false);
	}
	| SIGNED integer_size
	{
	    if ($2 == 4)
		$$ = new CFESimpleType(TYPE_LONG, false, false, $2, false);
	    else
		$$ = new CFESimpleType(TYPE_INTEGER, false, false, $2, false);
	}
	| SIGNED
	{
	    $$ = new CFESimpleType(TYPE_INTEGER, false);
	}
	| UNSIGNED integer_size INT
	{
	    if ($2 == 4)
		$$ = new CFESimpleType(TYPE_LONG, true, false, $2);
	    else
		$$ = new CFESimpleType(TYPE_INTEGER, true, false, $2);
	}
	| UNSIGNED              INT
	{
	    $$ = new CFESimpleType(TYPE_INTEGER, true, false);
	}
	| UNSIGNED integer_size
	{
	    if ($2 == 4)
		$$ = new CFESimpleType(TYPE_LONG, true, false, $2, false);
	    else
		$$ = new CFESimpleType(TYPE_INTEGER, true, false, $2, false);
	}
	| UNSIGNED
	{
	    $$ = new CFESimpleType(TYPE_INTEGER, true);
	}
	| integer_size INT
	{
	    if ($1 == 4)
		$$ = new CFESimpleType(TYPE_LONG, false, false, $1);
	    else
		$$ = new CFESimpleType(TYPE_INTEGER, false, false, $1);
	}
	|              INT
	{
	    $$ = new CFESimpleType(TYPE_INTEGER, false, false);
	}
	| integer_size
	{
	    if ($1 == 4)
		$$ = new CFESimpleType(TYPE_LONG, false, false, $1, false);
	    else
		$$ = new CFESimpleType(TYPE_INTEGER, false, false, $1, false);
	}
	;

integer_size :
	  LONG
	{
	    $$ = 4;
	}
	| LONGLONG
	{
	    $$ = 8;
	}
	| SHORT
	{
	    $$ = 2;
	}
	;

floating_pt_type :
	  FLOAT
	{
	    $$ = new CFESimpleType(TYPE_FLOAT);
	}
	| DOUBLE
	{
	    $$ = new CFESimpleType(TYPE_DOUBLE);
	}
	| LONG DOUBLE
	{
	    $$ = new CFESimpleType(TYPE_LONG_DOUBLE);
	}
	;

type_name :
	  CLASS_ID
	| template_id
	| ENUM_ID
	| TYPEDEF_ID
	;

enum_specifier :
	/* TODO: should set the enum type as scope for the enumerator_list_opt
	 */
	  ENUM ID LBRACE enumerator_list_opt RBRACE
	{
	    if (driver.trace_scanning)
		std::cerr << "C-Parser: ENUM checking " << *$2 << "\n";
	    // check if any of the aliases has been declared before (as type)
	    if (driver.check_token(*$2, dice::parser::CSymbolTable::TYPENAME))
		driver.error(@2, "Enum \"" + *$2 + "\" already defined.");
	    // add aliases to symbol table
	    driver.add_token(*$2, dice::parser::CSymbolTable::TYPENAME, (CFEBase*)0,
		*@2.begin.filename, @2.begin.line, @2.begin.column);

	    // create type
	    $$ = new CFEEnumType(*$2, $4);
	}
	| ENUM LBRACE enumerator_list_opt RBRACE
	{
	    $$ = new CFEEnumType(std::string(), $3);
	}
	/* elaborated_type_specifier */
	| ENUM SCOPE nested_name_specifier ID
	{
	    $$ = new CFEEnumType(std::string("::") + *$3 + *$4, NULL);
	}
	| ENUM SCOPE                       ID
	{
	    $$ = new CFEEnumType(std::string("::") + *$3, NULL);
	}
	| ENUM       nested_name_specifier ID
	{
	    $$ = new CFEEnumType(*$2 + *$3, NULL);
	}
	| ENUM ID
	{
	    $$ = new CFEEnumType(*$2, NULL);
	}
	| ENUM TYPEDEF_ID
	{
	    $$ = new CFEEnumType(*$2, NULL);
	}
	;

enumerator_list_opt :
	  enumerator_list
	| /* empty */
	{
	    $$ = NULL;
	}
	/* C99 */
	| enumerator_list COMMA
	;

enumerator_list :
	  enumerator_definition
	{
	    $$ = new std::vector<CFEIdentifier*>();
	    if ($1)
		$$->push_back($1);
	}
	| enumerator_list COMMA enumerator_definition
	{
	    $$ = $1;
	    if ($3)
		$$->push_back($3);
	}
	;

enumerator_definition :
	  enumerator
	{
	    $$ = new CFEEnumDeclarator(*$1);
	}
	/* replace constant_expression with conditional_expression */
	| enumerator IS conditional_expression
	{
	    $$ = new CFEEnumDeclarator(*$1, $3);
	    if ($3)
		$3->SetParent($$);
	}
	;

enumerator :
	  ID
	{
	    if (driver.trace_scanning)
		std::cerr << "C-Parser: Enumerator checking " << *$1 << "\n";
	    // check if identifier is already used as enumerator in current scope
	    if (driver.check_token(*$1, dice::parser::CSymbolTable::ENUM))
		driver.error(@1, "Enumerator \"" + *$1 + "\" already defined.");
	    // add aliases to symbol table
	    driver.add_token(*$1, dice::parser::CSymbolTable::ENUM, (CFEBase*)0,
		*@1.begin.filename, @1.begin.line, @1.begin.column);

	    // return the name
	    $$ = $1;
	}
	;

namespace_definition :
	  NAMESPACE ID
	{
	    if (driver.trace_scanning)
		std::cerr << "C-Parser: NAMESPACE checking " << *$2 << "\n";
	    // check if identifier is already used as namespace id
	    if (driver.check_token(*$2, dice::parser::CSymbolTable::NAMESPACE))
		driver.error(@2, "Namespace ID \"" + *$2 + "\" already defined.");
	    // add aliases to symbol table
	    driver.add_token(*$2, dice::parser::CSymbolTable::NAMESPACE, (CFEBase*)0,
		*@2.begin.filename, @2.begin.line, @2.begin.column);

	    // create a library (i.e., namespace) object and add it to the current
	    // context
	    CFELibrary *pFELibrary = new CFELibrary(*$2, NULL, driver.getCurrentContext());
	    driver.setCurrentContext(pFELibrary);
	    // becase the name is not known, this is the first library with this
	    // name in the current scope, so no need to set the same libarary
	} LBRACE namespace_body RBRACE
	{
	    driver.leaveCurrentContext();
	}
	| NAMESPACE NAMESPACE_ID
	{
	    if (driver.trace_scanning)
		std::cerr << "C-Parser: NAMESPACE checking " << *$2 << "\n";
	    // NAMESPACE_ID identicates that the identifier is already defined as
	    // namespace, so this is a namespace extension
	    CFEBase *pContext = driver.getCurrentContext();
	    CFELibrary *pFELibrary = new CFELibrary(*$2, NULL, pContext);
	    driver.setCurrentContext(pFELibrary);
	    // check the context for the library. The context can either be a file
	    // or a library itself
	    CFEFile *pFEFile = dynamic_cast<CFEFile*>(pContext);
	    while (pFEFile)
	    {
		CFELibrary *pSameLib = pFEFile->FindLibrary(*$2);
		if (pSameLib)
		{
		    pSameLib->AddSameLibrary(pFELibrary);
		    break;
		}
		pFEFile = pFEFile->GetSpecificParent<CFEFile>();
	    }
	    CFELibrary *pParentLib = dynamic_cast<CFELibrary*>(pContext);
	    if (!dynamic_cast<CFEFile*>(pContext))
	    {
		while (pParentLib)
		{
		    CFELibrary *pSameLib = pParentLib->FindLibrary(*$2);
		    if (pSameLib)
		    {
			pSameLib->AddSameLibrary(pFELibrary);
			break;
		    }
		    pParentLib = pParentLib->GetSpecificParent<CFELibrary>();
		}
	    }
	} LBRACE namespace_body RBRACE
	{
	    driver.leaveCurrentContext();
	}
	| NAMESPACE LBRACE namespace_body RBRACE
	;

/* original_namespace_definition : NAMESPACE ID LBRACE namespace_body RBRACE ;
 * inlined */

/* extension_namespace_definition : NAMESPACE NAMESPACE_ID LBRACE
 * namespace_body RBRACE ; inlined */

/* unnamed_namespace_definition : NAMESPACE LBRACE namespace_body RBRACE ;
 * inlined */

namespace_body :
	  declaration_seq
	| /* empty */
	;

namespace_alias_definition :
	  NAMESPACE ID IS qualified_namespace_specifier SEMICOLON
	{
	    // add ID as new namespace name, but keep the current context. This is
	    // a simple alias definition which allows to access the
	    // qualified_namespace_specifier namespace with the ID alias.
	    if (driver.trace_scanning)
		std::cerr << "C-Parser: NAMESPACE checking " << *$2 << "\n";
	    // check if identifier is already used as namespace id
	    if (driver.check_token(*$2, dice::parser::CSymbolTable::NAMESPACE))
		driver.error(@2, "Namespace ID \"" + *$2 + "\" already defined.");
	    // add aliases to symbol table
	    driver.add_token(*$2, dice::parser::CSymbolTable::NAMESPACE, (CFEBase*)0,
		*@2.begin.filename, @2.begin.line, @2.begin.column);
	}
	;

qualified_namespace_specifier :
	  SCOPE nested_name_specifier NAMESPACE_ID
	{
	    $$ = new std::string("::");
	    $$->append(*$2);
	    $$->append(*$3);
	}
	|       nested_name_specifier NAMESPACE_ID
	{
	    $$ = $1;
	    $$->append(*$2);
	}
	| SCOPE                       NAMESPACE_ID
	{
	    $$ = new std::string("::");
	    $$->append(*$2);
	}
	|                             NAMESPACE_ID
	;

using_declaration :
	/*  USING TYPENAME nested_name_specifier unqualified_id SEMICOLON
	| USING          nested_name_specifier unqualified_id SEMICOLON*/
	  USING TYPENAME SCOPE nested_name_specifier unqualified_id SEMICOLON
	| USING TYPENAME       nested_name_specifier unqualified_id SEMICOLON
	| USING          SCOPE nested_name_specifier unqualified_id SEMICOLON
	| USING                nested_name_specifier unqualified_id SEMICOLON
	| USING SCOPE unqualified_id SEMICOLON
	;

using_directive :
	  USING NAMESPACE qualified_namespace_specifier SEMICOLON
	;

asm_definition :
	  asm_definition_bare SEMICOLON
	;

asm_definition_bare :
	  ASM_KEYWORD LPAREN asm_definition_interior RPAREN
	/* gcc extension */
	| ASM_KEYWORD VOLATILE LPAREN asm_definition_interior RPAREN
	;

asm_definition_interior :
	  asm_code COLON asm_constraints COLON asm_constraints COLON asm_clobber_list
	| asm_code COLON asm_constraints COLON asm_constraints
	| asm_code COLON asm_constraints
	| asm_code
	| asm_code SCOPE asm_constraints COLON asm_clobber_list
	| asm_code SCOPE asm_constraints
	| asm_code COLON asm_constraints SCOPE asm_clobber_list
	;

asm_code :
	  string
	;

asm_constraints :
	  asm_constraint_list
	| /* empty */
	;

asm_constraint_list :
	  asm_constraint
	| asm_constraint_list COMMA asm_constraint
	;

asm_constraint :
	  LBRACKET ID RBRACKET string LPAREN expression RPAREN
	|                      string LPAREN expression RPAREN
	;

asm_clobber_list :
	  string
	| asm_clobber_list COMMA string
	;

linkage_specification :
	  EXTERN_LANG_STRING LBRACE declaration_seq RBRACE
	| EXTERN_LANG_STRING LBRACE RBRACE
	| EXTERN_LANG_STRING declaration
	;

/* A.7 Declarators */
init_declarator_list :
	  init_declarator
	{
	    $$ = new std::vector<CFEDeclarator*>();
	    if ($1)
		$$->push_back($1);
	}
	| init_declarator_list COMMA init_declarator
	{
	    $$ = $1;
	    if ($3)
		$$->push_back($3);
	}
	;

init_declarator :
	  declarator maybeasm maybeattribute
	{
	    $$ = $1;
	    // add attributes
	}
	/* initializer inlined */
	| declarator maybeasm maybeattribute IS initializer_clause
	{ $$ = $1; }
	| declarator maybeasm maybeattribute LPAREN expression RPAREN
	{ $$ = $1; }
	;

/* gcc extension: assign register */
maybeasm :
	  asm_definition_bare
	| /* empty */
	;

/* gcc extension: attributes */
maybeattribute :
	  attribute_seq
	| /* empty */
	{ $$ = NULL; }
	;

declarator :
	  direct_declarator
	| ptr_operator_seq direct_declarator
	{
	    $$ = $2;
	    $$->SetStars($1);
	}
	;

direct_declarator :
	  declarator_id
	| array_declarator
	{ $$ = $1; }
	| function_declarator
	{ $$ = $1; }
	| LPAREN declarator RPAREN
	{ $$ = $2; }
	;

function_declarator :
	  direct_declarator LPAREN parameter_declaration_clause RPAREN cv_qualifier_seq exception_specification maybeattribute
	{
	    $$ = new CFEFunctionDeclarator($1, $3);
	}
	| direct_declarator LPAREN parameter_declaration_clause RPAREN cv_qualifier_seq maybeattribute
	{
	    $$ = new CFEFunctionDeclarator($1, $3);
	}
	/* C99 */
	/* "direct_declarator LPAREN identifier_list RPAREN" covered by
	 * parameter_declaration_clause -> parameter_declaration_list ->
	 * parameter_declaration (COMMA parameter_declaration)* ->
	 * decl_specifier_seq ->  decl_specifier+ -> type_specifier ->
	 * qualified_namespace_specifier -> unqualified_id ->  ID
	 */
	| direct_declarator LEFT_RIGHT maybeattribute
	{
	    $$ = new CFEFunctionDeclarator($1, NULL);
	}
	/* Dice specific:
	 * struct foo {}; will define "foo" a class-id
	 * int foo(params); will recognize foo as class and add it to the
	 * decl_spec_seq, thus no function can be recognized properly
	 */
	| LPAREN parameter_declaration_clause RPAREN cv_qualifier_seq exception_specification maybeattribute
	{ $$ = NULL; }
	| LPAREN parameter_declaration_clause RPAREN cv_qualifier_seq maybeattribute
	{ $$ = NULL; }
	;

array_declarator :
	/* replace constant_expression with conditional_expression */
	  direct_declarator LBRACKET conditional_expression RBRACKET
	{
	    if ($1)
	    {
		if ($1->GetType() == DECL_ARRAY)
		    $$ = static_cast<CFEArrayDeclarator*>($1);
		else
		{
		    $$ = new CFEArrayDeclarator($1);
		    delete $1;
		}
	    }
	    $$->AddBounds(NULL, $3);
	    if ($3)
		$3->SetParent($$);
	}
	| direct_declarator LBRACKET RBRACKET
	{
	    if ($1 && $1->GetType() == DECL_ARRAY)
		$$ = static_cast<CFEArrayDeclarator*>($1);
	    else
	    {
		$$ = new CFEArrayDeclarator($1);
		delete $1;
	    }
	    $$->AddBounds(NULL, NULL);
	}
	/* C99 */
	| direct_declarator LBRACKET ASTERISK RBRACKET
	{
	    if ($1 && $1->GetType() == DECL_ARRAY)
		$$ = static_cast<CFEArrayDeclarator*>($1);
	    else
	    {
		$$ = new CFEArrayDeclarator($1);
		delete $1;
	    }
	    CFEExpression *bound = new CFEExpression(static_cast<signed char>('*'));
	    $$->AddBounds(NULL, bound);
	    bound->SetParent($$);
	}
	;

/* identifier_list : ID | identifier_list COMMA ID ; */

ptr_operator :
	  BITAND
	{ $$ = 0; }
	| SCOPE nested_name_specifier ASTERISK cv_qualifier_seq
	{ $$ = 1; }
	|       nested_name_specifier ASTERISK cv_qualifier_seq
	{ $$ = 1; }
	| SCOPE                       ASTERISK cv_qualifier_seq
	{ $$ = 1; }
	|                             ASTERISK cv_qualifier_seq
	{ $$ = 1; }
	;

cv_qualifier_seq :
	  cv_qualifier_seq cv_qualifier
	| /* empty */
	;

cv_qualifier :
	  CONST
	{
	    $$ = new std::string("const");
	}
	| VOLATILE
	{
	    $$ = new std::string("volatile");
	}
	/* C99 */
	| RESTRICT
	{
	    $$ = new std::string("restrict");
	}
	| BYCOPY
	{
	    $$ = new std::string("bycopy");
	}
	| BYREF
	{
	    $$ = new std::string("byref");
	}
	| IN
	{
	    $$ = new std::string("in");
	}
	| OUT
	{
	    $$ = new std::string("out");
	}
	| INOUT
	{
	    $$ = new std::string("inout");
	}
	| ONEWAY
	{
	    $$ = new std::string("oneway");
	}
	;

declarator_id :
	  SCOPE id_expression
	{
	    $$ = $2;
	    $$->Prefix("::");
	}
	|       id_expression
	| SCOPE nested_name_specifier type_name
	{
	    $$ = new CFEDeclarator(DECL_IDENTIFIER, std::string("::") + *$2 + *$3);
	}
	| SCOPE                       type_name
	{
	    $$ = new CFEDeclarator(DECL_IDENTIFIER, std::string("::") + *$2);
	}
	|       nested_name_specifier type_name
	{
	    $$ = new CFEDeclarator(DECL_IDENTIFIER, *$1 + *$2);
	}
	|                             type_name
	{
	    $$ = new CFEDeclarator(DECL_IDENTIFIER, *$1);
	}
	;

type_id :
	  type_specifier_seq abstract_declarator
	{
	    if ($1->size() > 0)
		$$ = *($1->begin());
	    if ($2)
		delete $2;
	}
	| type_specifier_seq
	{
	    if ($1->size() > 0)
		$$ = *($1->begin());
	}
	;

type_specifier_seq :
	  type_specifier_seq type_specifier
	{
	    $$ = $1;
	    if ($2)
		$$->push_back($2);
	}
	| type_specifier
	{
	    $$ = new vector<CFETypeSpec*>();
	    if ($1)
		$$->push_back($1);
	}
	;

abstract_declarator :
	  ptr_operator_seq direct_abstract_declarator
	{
	    $$ = $2;
	    $$->SetStars($1);
	}
	| ptr_operator_seq
	{ $$ = NULL; }
	|                  direct_abstract_declarator
	;

direct_abstract_declarator :
	  direct_abstract_declarator LPAREN parameter_declaration_clause RPAREN cv_qualifier_seq exception_specification
	| direct_abstract_declarator LPAREN parameter_declaration_clause RPAREN cv_qualifier_seq
	|                            LPAREN parameter_declaration_clause RPAREN cv_qualifier_seq exception_specification
	{
	    $$ = NULL;
	}
	|                            LPAREN parameter_declaration_clause RPAREN cv_qualifier_seq
	{
	    $$ = NULL;
	}
	/* replace constant_expression with conditional_expression */
	| direct_abstract_declarator LBRACKET conditional_expression RBRACKET
	|                            LBRACKET conditional_expression RBRACKET
	{
	    $$ = NULL;
	}
	| direct_abstract_declarator LBRACKET                        RBRACKET
	|                            LBRACKET                        RBRACKET
	{
	    $$ = NULL;
	}
	| LPAREN abstract_declarator RPAREN
	{
	    $$ = $2;
	}
	/* C99 */
	| direct_abstract_declarator LBRACKET ASTERISK RBRACKET
	/* gnu99 */
	| PAREN_STAR_PAREN
	{
	    $$ = NULL;
	}
	;

parameter_declaration_clause :
	  parameter_declaration_list ELLIPSIS
	| parameter_declaration_list
	|                            ELLIPSIS
	{
	    $$ = NULL;
	}
	| /* empty */
	{
	    $$ = NULL;
	}
	| parameter_declaration_list COMMA ELLIPSIS
	;

parameter_declaration_list :
	  parameter_declaration
	{
	    $$ = new std::vector<CFETypedDeclarator*>();
	    if ($1)
		$$->push_back($1);
	}
	| parameter_declaration_list COMMA parameter_declaration
	{
	    $$ = $1;
	    if ($3)
		$$->push_back($3);
	}
	;

parameter_declaration :
	  decl_specifier_seq declarator
	{
	    $$ = NULL;
	}
	| decl_specifier_seq declarator IS assignment_expression
	{
	    $$ = NULL;
	}
	| decl_specifier_seq abstract_declarator
	{
	    $$ = NULL;
	}
	| decl_specifier_seq
	{
	    $$ = NULL;
	}
	| decl_specifier_seq abstract_declarator IS assignment_expression
	{
	    $$ = NULL;
	}
	| decl_specifier_seq IS assignment_expression
	{
	    $$ = NULL;
	}
	;

function_definition :
	  decl_specifier_seq init_declarator ctor_initializer compound_statement
	|                    init_declarator ctor_initializer compound_statement
	| decl_specifier_seq init_declarator                  compound_statement
	|                    init_declarator                  compound_statement
	| decl_specifier_seq init_declarator function_try_block
	|                    init_declarator function_try_block
// 	| decl_specifier_seq init_declarator SEMICOLON
// 	|                    init_declarator SEMICOLON
	;

/* function_body : compound_statement replaced */

initializer_clause :
	  assignment_expression
	| LBRACE initializer_list COMMA RBRACE
	| LBRACE initializer_list       RBRACE
	| LBRACE                        RBRACE
	;

initializer_list :
	  initializer_clause
	| initializer_list COMMA initializer_clause
	/* C99 */
	| designation initializer_clause
	| initializer_list COMMA designation initializer_clause
	;

/* C99: 6.7.8 */
designation :
	  designator_list IS
	| designator_list
	;

designator_list :
	  designator
	| designator_list designator
	;

designator :
	  /* replace constant_expression with conditional_expression */
	  LBRACKET conditional_expression RBRACKET
	| DOT ID
	/* gnu99 */
	| ID COLON
	;

/* A.8 Classes */
class_specifier :
	  class_head LBRACE member_specification_opt RBRACE
	/* elaborated_type_specifier */
	| CLASS TYPEDEF_ID
	| class_head
	;

class_head :
	  CLASS SCOPE nested_name_specifier ID base_clause
	| CLASS       nested_name_specifier ID base_clause
	| CLASS SCOPE                       ID base_clause
	| CLASS                             ID base_clause
	| CLASS                                base_clause
	| CLASS SCOPE nested_name_specifier ID
	| CLASS       nested_name_specifier ID
	| CLASS SCOPE                       ID
	| CLASS                             ID
	| CLASS
	;

struct_specifier :
	  struct_head maybeattribute LBRACE
	{
	    std::string sName = $1->GetTag();

	    if (!sName.empty())
	    {
		if (driver.trace_scanning)
		    std::cerr << "C-Parser: STRUCT checking " << sName << "\n";
		// check if identifier is already used as class id (structs are
		// classes)
		if (driver.check_token(sName, dice::parser::CSymbolTable::CLASS))
		    driver.error(@1, "Struct ID \"" + sName + "\" already defined.");
		// add aliases to symbol table
		driver.add_token(sName, dice::parser::CSymbolTable::CLASS, (CFEBase*)0,
		    *@1.begin.filename, @1.begin.line, @1.begin.column);
	    }

	    // set "class" as new scope
	    driver.setCurrentContext($1);
	} member_specification_opt RBRACE
	{
	    $$ = $1;
	    $$->AddMembers($5);
	    driver.leaveCurrentContext();
	}
	/* elaborated_type_specifier */
	| STRUCT CLASS_ID
	{
	    $$ = new CFEStructType(*$2, NULL);
	}
	| STRUCT TYPEDEF_ID
	{
	    $$ = new CFEStructType(*$2, NULL);
	}
	| struct_head
	;

struct_head :
	  STRUCT SCOPE nested_name_specifier ID base_clause
	{
	    $$ = new CFEStructType(std::string("::") + *$3 + *$4, NULL, $5);
	}
	| STRUCT       nested_name_specifier ID base_clause
	{
	    $$ = new CFEStructType(*$2 + *$3, NULL, $4);
	}
	| STRUCT SCOPE                       ID base_clause
	{
	    $$ = new CFEStructType(std::string("::") + *$3, NULL, $4);
	}
	| STRUCT                             ID base_clause
	{
	    $$ = new CFEStructType(*$2, NULL, $3);
	}
	| STRUCT                                base_clause
	{
	    $$ = new CFEStructType(std::string(), NULL, $2);
	}
	| STRUCT SCOPE nested_name_specifier ID
	{
	    $$ = new CFEStructType(std::string("::") + *$3 + *$4, NULL);
	}
	| STRUCT       nested_name_specifier ID
	{
	    $$ = new CFEStructType(*$2 + *$3, NULL);
	}
	| STRUCT SCOPE                       ID
	{
	    $$ = new CFEStructType(std::string("::") + *$3, NULL);
	}
	| STRUCT                             ID
	{
	    $$ = new CFEStructType(*$2, NULL);
	}
	| STRUCT
	{
	    $$ = new CFEStructType(std::string(), NULL);
	}
	;

union_specifier :
	  union_head LBRACE
	{
	    std::string sName = $1->GetTag();

	    if (!sName.empty())
	    {
		if (driver.trace_scanning)
		    std::cerr << "C-Parser: UNION checking " << sName << "\n";
		// check if identifier is already used as class id (unions are
		// classes)
		if (driver.check_token(sName, dice::parser::CSymbolTable::CLASS))
		    driver.error(@1, "Union ID \"" + sName + "\" already defined.");
		// add aliases to symbol table
		driver.add_token(sName, dice::parser::CSymbolTable::CLASS, (CFEBase*)0,
		    *@1.begin.filename, @1.begin.line, @1.begin.column);
	    }

	    // set "class" as new scope
	    driver.setCurrentContext($1);
	} union_body_opt RBRACE
	{
	    $$ = $1;
	    $$->AddMembers($4);
	    driver.leaveCurrentContext();
	}
	/* elaborated_type_specifier */
	| union_head
	| UNION CLASS_ID
	{
	    $$ = new CFEUnionType(*$2, NULL);
	}
	| UNION TYPEDEF_ID
	{
	    $$ = new CFEUnionType(*$2, NULL);
	}
	;

union_head :
	  UNION SCOPE nested_name_specifier ID base_clause
	{
	    $$ = new CFEUnionType(std::string("::") + *$3 + *$4, NULL, $5);
	}
	| UNION       nested_name_specifier ID base_clause
	{
	    $$ = new CFEUnionType(*$2 + *$3, NULL, $4);
	}
	| UNION SCOPE                       ID base_clause
	{
	    $$ = new CFEUnionType(std::string("::") + *$3, NULL, $4);
	}
	| UNION                             ID base_clause
	{
	    $$ = new CFEUnionType(*$2, NULL, $3);
	}
	| UNION                                base_clause
	{
	    $$ = new CFEUnionType(std::string(), NULL, $2);
	}
	| UNION SCOPE nested_name_specifier ID
	{
	    $$ = new CFEUnionType(std::string("::") + *$3 + *$4, NULL);
	}
	| UNION       nested_name_specifier ID
	{
	    $$ = new CFEUnionType(*$2 + *$3, NULL);
	}
	| UNION SCOPE                       ID
	{
	    $$ = new CFEUnionType(std::string("::") + *$3, NULL);
	}
	| UNION                             ID
	{
	    $$ = new CFEUnionType(*$2, NULL);
	}
	| UNION
	{
	    $$ = new CFEUnionType(std::string(), NULL);
	}
	;

/* class_key : CLASS | STRUCT | UNION ; inlined */

member_specification_opt :
	  member_specification
	| /* empty */
	{
	    $$ = NULL;
	}
	;

union_body_opt :
	  union_body
	| /* empty */
	{
	    $$ = NULL;
	}
	;

union_body :
	  union_body union_arm
	{
	    $$ = $1;
	    if ($2)
		$$->push_back($2);
	}
	| union_body access_specifier COLON
	| union_arm
	{
	    $$ = new vector<CFEUnionCase*>();
	    if ($1)
		$$->push_back($1);
	}
	| access_specifier COLON
	{
	    $$ = new vector<CFEUnionCase*>();
	}
	;

union_arm :
	  member_declaration
	{
	    if ($1)
	    {
		$$ = new CFEUnionCase($1);
		$1->SetParent($$);
	    }
	    else
		$$ = NULL;
	}
	;

member_specification :
	  member_specification member_declaration
	{
	    $$ = $1;
	    if ($2)
		$$->push_back($2);
	}
	// we ignore the access specifier: it defines the scope of the members
	| member_specification access_specifier COLON
	| member_declaration
	{
	    $$ = new vector<CFETypedDeclarator*>();
	    if ($1)
		$$->push_back($1);
	}
	| access_specifier COLON
	{
	    $$ = new vector<CFETypedDeclarator*>();
	    // we ignore the access specifier: it defines the scope of the members
	}
	;

member_declaration :
	  decl_specifier_seq member_declarator_list SEMICOLON
	{
	    CFETypeSpec *t = NULL;
	    if ($1 && $1->size() > 0)
		t = *$1->begin();
	    if (t)
	    {
		$$ = new CFETypedDeclarator(TYPEDECL_FIELD, t, $2);
		$$->m_sourceLoc = @$;
	    }
	    else
		$$ = NULL;
	}
	/* decl_specifier_seq omitted in constructor, destructor, and
	 * conversion function decarations only */
	|                    member_declarator_list SEMICOLON
	{
	    if ($1)
		delete $1;
	    $$ = NULL;
	}
	/* member_declarator_list can be omitted only after a class-specifier,
	 * an enum-specifier, or a decl-specifier-seq of the form friend
	 * elaborated-type-specifier */
	| decl_specifier_seq                        SEMICOLON
	{
	    if ($1)
		delete $1;
	    $$ = NULL;
	}
	| SEMICOLON
	{
	    $$ = NULL;
	}
	| function_definition SEMICOLON
	{
	    $$ = NULL;
	}
	| function_definition
	{
	    $$ = NULL;
	}
	| qualified_id SEMICOLON
	{
	    if ($1)
		delete $1;
	    $$ = NULL;
	}
	| using_declaration
	{
	    $$ = NULL;
	}
	| template_declaration
	{
	    $$ = NULL;
	}
	;

member_declarator_list :
	  member_declarator
	{
	    $$ = new vector<CFEDeclarator*>();
	    if ($1)
		$$->push_back($1);
	}
	| member_declarator_list COMMA member_declarator
	{
	    $$ = $1;
	    if ($3)
		$$->push_back($3);
	}
	;

member_declarator :
	  declarator maybeasm maybeattribute
	/* constant_initializer covers pure_specifier */
	/* replace constant_expression with conditional_expression */
	| declarator maybeasm maybeattribute IS conditional_expression
	{
	    $$ = $1;
	    if ($5)
		delete $5;
	}
	/* bitfield specification */
	| ID COLON conditional_expression
	{
	    $$ = new CFEDeclarator(DECL_IDENTIFIER, *$1);
	    if ($3)
		$$->SetBitfields($3->GetIntValue());
	}
	/* unnamed bitfield */
	|    COLON conditional_expression
	{
	    $$ = NULL;
	}
	;

/* covered by constant_initializer:
pure_specifier : IS LIT_INT ;
*/

/* constant_initializer : IS constant_expression ; inlined */

/* A.9 Derived classes */
base_clause :
	  COLON base_specifier_list
	{
	    $$ = $2;
	}
	;

base_specifier_list :
	  base_specifier
	{
	    $$ = new vector<CFEIdentifier*>();
	    if ($1)
		$$->push_back($1);
	}
	| base_specifier_list COMMA base_specifier
	{
	    $$ = $1;
	    if ($3)
		$$->push_back($3);
	}
	;

/* to reduce conflicts all class_name-s have been replaced with type_name */
base_specifier :
	  qualified_namespace_specifier
	{
	    $$ = new CFEIdentifier(*$1);
	}
	| VIRTUAL access_specifier qualified_namespace_specifier
	{
	    $$ = new CFEIdentifier(*$3);
	}
	| VIRTUAL                  qualified_namespace_specifier
	{
	    $$ = new CFEIdentifier(*$2);
	}
	| access_specifier VIRTUAL qualified_namespace_specifier
	{
	    $$ = new CFEIdentifier(*$3);
	}
	| access_specifier         qualified_namespace_specifier
	{
	    $$ = new CFEIdentifier(*$2);
	}
	;

access_specifier :
	  PRIVATE
	| PROTECTED
	| PUBLIC
	;

/* A.10 Special member functions */
conversion_function_id :
	  OPERATOR type_specifier_seq conversion_declarator
	;

/* conversion_type_id :
          type_specifier_seq conversion_declarator
	| type_specifier_seq
	; inlined */

/* conversion_declarator : ptr_operator+ which is a ptr_operator_seq */
conversion_declarator :
	  conversion_declarator ptr_operator
	| /* empty */
	;

ctor_initializer :
	  COLON mem_initializer_list
	;

mem_initializer_list :
	  mem_initializer
	| mem_initializer_list COMMA mem_initializer
	;

mem_initializer :
	  mem_initializer_id LPAREN expression RPAREN
	| mem_initializer_id LEFT_RIGHT
	;

mem_initializer_id :
	  qualified_namespace_specifier
	/* ID covered by qualified_namespace_specifier */
	;

/* A.11 Overloading */
operator_function_id :
	  OPERATOR NEW
	{
	    $$ = new std::string("operator new");
	}
	| OPERATOR DELETE
	{
	    $$ = new std::string("operator delete");
	}
	| OPERATOR NEW LBRACKET RBRACKET
	{
	    $$ = new std::string("operator new[]");
	}
	| OPERATOR DELETE LBRACKET RBRACKET
	{
	    $$ = new std::string("operator delete[]");
	}
	| OPERATOR PLUS
	{
	    $$ = new std::string("operator+");
	}
	| OPERATOR MINUS
	{
	    $$ = new std::string("operator-");
	}
	| OPERATOR ASTERISK
	{
	    $$ = new std::string("operator*");
	}
	| OPERATOR DIV
	{
	    $$ = new std::string("operator/");
	}
	| OPERATOR MOD
	{
	    $$ = new std::string("operator%");
	}
	| OPERATOR BITXOR
	{
	    $$ = new std::string("operator^");
	}
	| OPERATOR BITAND
	{
	    $$ = new std::string("operator&");
	}
	| OPERATOR BITOR
	{
	    $$ = new std::string("operator|");
	}
	| OPERATOR TILDE
	{
	    $$ = new std::string("operator~");
	}
	| OPERATOR EXCLAM
	{
	    $$ = new std::string("operator!");
	}
	| OPERATOR IS
	{
	    $$ = new std::string("operator=");
	}
	| OPERATOR LT
	{
	    $$ = new std::string("operator<");
	}
	| OPERATOR GT
	{
	    $$ = new std::string("operator>");
	}
	| OPERATOR ADD_ASSIGN
	{
	    $$ = new std::string("operator+=");
	}
	| OPERATOR SUB_ASSIGN
	{
	    $$ = new std::string("operator-=");
	}
	| OPERATOR MUL_ASSIGN
	{
	    $$ = new std::string("operator*=");
	}
	| OPERATOR DIV_ASSIGN
	{
	    $$ = new std::string("operator/=");
	}
	| OPERATOR MOD_ASSIGN
	{
	    $$ = new std::string("operator%=");
	}
	| OPERATOR XOR_ASSIGN
	{
	    $$ = new std::string("operator^=");
	}
	| OPERATOR AND_ASSIGN
	{
	    $$ = new std::string("operator&=");
	}
	| OPERATOR OR_ASSIGN
	{
	    $$ = new std::string("operator|=");
	}
	| OPERATOR LSHIFT
	{
	    $$ = new std::string("operator<<");
	}
	| OPERATOR RSHIFT
	{
	    $$ = new std::string("operator>>");
	}
	| OPERATOR LS_ASSIGN
	{
	    $$ = new std::string("operator<<=");
	}
	| OPERATOR RS_ASSIGN
	{
	    $$ = new std::string("operator>>=");
	}
	| OPERATOR EQUAL
	{
	    $$ = new std::string("operator==");
	}
	| OPERATOR NOTEQUAL
	{
	    $$ = new std::string("operator!=");
	}
	| OPERATOR LTEQUAL
	{
	    $$ = new std::string("operator<=");
	}
	| OPERATOR GTEQUAL
	{
	    $$ = new std::string("operator>=");
	}
	| OPERATOR LOGICALAND
	{
	    $$ = new std::string("operator&&");
	}
	| OPERATOR LOGICALOR
	{
	    $$ = new std::string("operator||");
	}
	| OPERATOR INC_OP
	{
	    $$ = new std::string("operator++");
	}
	| OPERATOR DEC_OP
	{
	    $$ = new std::string("operator--");
	}
	| OPERATOR COMMA
	{
	    $$ = new std::string("operator,");
	}
	| OPERATOR PTRSTAR_OP
	{
	    $$ = new std::string("operator.*");
	}
	| OPERATOR PTR_OP
	{
	    $$ = new std::string("operator->");
	}
	| OPERATOR LEFT_RIGHT
	{
	    $$ = new std::string("operator()");
	}
	| OPERATOR LBRACKET RBRACKET
	{
	    $$ = new std::string("operator[]");
	}
	/* gnu99 */
	| OPERATOR LOGICALXOR
	{
	    $$ = new std::string("operator^^");
	}
	;

/* operator rule inlined */

/* A.12 Templates */
template_declaration :
	  EXPORT TEMPLATE LT template_parameter_list GT declaration
	|        TEMPLATE LT template_parameter_list GT declaration
	;

template_parameter_list :
	  template_parameter
	| template_parameter_list COMMA template_parameter
	;

template_parameter :
	  type_parameter
	| parameter_declaration
	;

type_parameter :
	  CLASS ID IS type_id
	| CLASS    IS type_id
	/* "CLASS | CLASS ID" covered by parameter_declaration */
	| TYPENAME ID IS type_id
	| TYPENAME    IS type_id
	/* "TYPENAME ID" covered by parameter_declaration */
	| TYPENAME
	| TEMPLATE LT template_parameter_list GT CLASS ID
	| TEMPLATE LT template_parameter_list GT CLASS
	| TEMPLATE LT template_parameter_list GT CLASS ID IS TEMPLATE_ID
	| TEMPLATE LT template_parameter_list GT CLASS    IS TEMPLATE_ID
	;

/* incorporate template_id into calling rules to reduce conflicts (give bison
 * more look-ahead tokens */
template_id :
	  TEMPLATE_ID LT template_argument_list GT
	;

template_argument_list :
	  template_argument
	| template_argument_list COMMA template_argument
	;

template_argument :
	  assignment_expression
	| type_id
	| TEMPLATE_ID
	;

/* explicit_instantiation : TEMPLATE declaration ; inlined */

/* explicit_specialization :  TEMPLATE LT GT declaration ; inlined */

/* A.13 exception handling */
try_block :
	  TRY compound_statement handler_seq
	;

function_try_block :
	  TRY ctor_initializer compound_statement handler_seq
	| TRY                  compound_statement handler_seq
	;

handler_seq :
	  handler
	| handler_seq handler
	;

handler :
	  CATCH LPAREN exception_declaration RPAREN compound_statement
	;

exception_declaration :
	  type_specifier_seq declarator
	| type_specifier_seq abstract_declarator
	| type_specifier_seq
	| ELLIPSIS
	;

throw_expression :
	  THROW assignment_expression
	{
	    $$ = NULL;
	}
	| THROW
	{
	    $$ = NULL;
	}
	;

exception_specification :
	  THROW LPAREN type_id_list RPAREN
	| THROW LEFT_RIGHT
	;

type_id_list :
	  type_id
	| type_id_list COMMA type_id
	;

/* A.14 preprocessing directives: handled in preprocessing step */


/* Dice internal helper rules */
string :
	  LIT_STR
	| string LIT_STR
	;

/* gcc extensions */

/* attributes */
attribute_seq :
	  attribute
	| asm_definition_bare
	{ $$ = new std::vector<CFEAttribute*>(); }
	| attribute_seq attribute
	{
	    $$ = $1;
	    $$->insert($$->end(), $2->begin(), $2->end());
	    delete $2;
	}
	| attribute_seq asm_definition_bare
	{ $$ = $1; }
	;

attribute :
	  ATTRIBUTE LPAREN LPAREN attribute_list RPAREN RPAREN
	{
	    $$ = $4;
	}
	| ATTRIBUTE VOLATILE LPAREN LPAREN attribute_list RPAREN RPAREN
	{
	    $$ = $5;
	}
	;

attribute_list :
	  attribute_parameter
	{
	    $$ = new std::vector<CFEAttribute*>();
	    if ($1)
		$$->push_back($1);
	}
	| attribute_list COMMA attribute_parameter
	{
	    $$ = $1;
	    if ($3)
		$$->push_back($3);
	}
	;

attribute_parameter :
	  /* empty */
	{
	    $$ = NULL;
	}
	| any_word
	{
	    $$ = new CFEStringAttribute(ATTR_C, *$1);
	}
	| any_word LPAREN ID RPAREN
	{
	    $1->append(std::string("(") + *$3 + ")");
	    $$ = new CFEStringAttribute(ATTR_C, *$1);
	}
	| any_word LPAREN ID COMMA expression RPAREN
	{
	    $1->append(std::string("(") + *$3 + ")");
	    $$ = new CFEStringAttribute(ATTR_C, *$1);
	}
	| any_word LPAREN expression RPAREN
	{
	    $1->append(std::string("(expr)"));
	    $$ = new CFEStringAttribute(ATTR_C, *$1);
	}
	;

any_word :
	  ID
	| storage_class_specifier
	| cv_qualifier
	;

%%

void
yy::c_parser::error (const yy::c_parser::location_type& l,
  const std::string& m)
{
  driver.error (l, m);
}

