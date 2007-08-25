%skeleton "lalr1.cc"
%require "2.1a"
%defines
%define "parser_class_name" "c_parser"

%{
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
};

%{
#include "c-parser-driver.hh"
%}

%token		EOF_TOKEN	0 "end of file"
%token		INVALID		1 "invalid token"
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
%token		BYTE		"byte"
%token		CASE		"case"
%token		CATCH		"catch"
%token		CHAR		"char"
%token		CHAR_PTR	"char*"
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
%token		DOTDOT		".."
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
%token	<sval>	FILENAME
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
%token		LOGICALXOR	"^^"
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
%token		QUOT		"\""
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
%token		SINGLEQUOT	"\'"
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
%token		VOID_PTR	"void*"
%token		VOLATILE	"volatile"
%token		WCHAR		"wchar_t"
%token		WHILE		"while"
%token		XOR_ASSIGN	"^="

%printer    { debug_stream () << *$$; } "identifier" "literal string"
%destructor { delete $$; } "identifier" "literal string"

%printer    { debug_stream () << $$; } "character" "float value" "integer value"

%left error

/* Add precedence rules to solve dangling else s/r conflict */
%nonassoc IF
%nonassoc ELSE

%left ID TYPENAME ENUM CLASS STRUCT UNION ELLIPSIS TYPEOF OPERATOR TYPEDEF_ID CLASS_ID NAMESPACE_ID ENUM_ID TEMPLATE_ID

%right SIGNED UNSIGNED
%left LBRACE COMMA SEMICOLON

%nonassoc THROW
%right COLON
%right ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN
%right AND_ASSIGN OR_ASSIGN  XOR_ASSIGN RS_ASSIGN  LS_ASSIGN
%right MIN_ASSIGN MAX_ASSIGN
%right QUESTION
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
%left MUL DIV MOD
%left PTRSTAR_OP DOTSTAR_OP
%right UNARY INC_OP DEC_OP TILDE
%left HYPERUNARY
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

class_name :
	  CLASS_ID
	| TEMPLATE_ID LT template_argument_list GT
	;

/* enum_name : ENUM_ID ; inlined */

/* template_name : TEMPLATE_ID ; inlined */

/* A.2 Lexical conventions: (mostly) covered by scanner */
literal :
	  LIT_INT
	| LIT_CHAR
	| LIT_FLOAT
	| LIT_LONG
	| LIT_ULONG
	| LIT_LLONG
	| LIT_ULLONG
	| LIT_STR
	| TRUE
	| FALSE
	;

/* A.3 Basic concepts */
file :
	  declaration_seq
	;

/* A.4 Expressions */
primary_expression :
	  literal
	| THIS
	/* SCOPE ID is covered by SCOPE unqualified_id */
	/* SCOPE operator_function_id is covered by SCOPE unqualified_id */
	/* SCOPE unqualified_id | SCOPE qualified_id is covered by SCOPE
	 * id_expression */
	/* SCOPE id_expression | id_expression is covered by declarator_id */
	/* | declarator_id  -> direct_declarator -> declarator */
	| declarator
	| LPAREN expression RPAREN
	;

id_expression :
	  qualified_id
	| unqualified_id
	;

unqualified_id :
	  ID
	| operator_function_id
        | OPERATOR type_specifier_seq ptr_operator_seq
	| OPERATOR type_specifier_seq
	/* to reduce conflicts we use type-name instead of class-name here */
	/* trick to avoid reduce conflicts: use empty rule in the middle */
	| TILDE see_typename type_name
	/* explicetly use template syntax to reduce conflicts */
	/* since template_id is only used here and in class_name, we use
	 * class_name instead of template_id to avoid ambiquities */
	/* to elimate a reduce/reduce conflict we use class_or_namespace_name
	 * here, which includes class_name */
	/* to eliminate more reduce/reduce conflicts we use type_name here,
	 * which includes class_or_namespace_name */
	| type_name %prec IS
	;

see_typename :
	  /* empty */
	;

qualified_id :
	  nested_name_specifier TEMPLATE unqualified_id
	| nested_name_specifier_list
	;

/* to avoid conflicts we explicetly use class_name and namespace_name instead
 * of class-or-namespace-name here */
/* to further reduce conflicts, we use type_name instead of class-or-namespace
 * name */
/* more conflict reduction by replacing type_name with unqualified_id */
/* because nested_name_specifier usually appears with an optional SCOPE and
 * itself optional, we integrate that here, and place the original
 * nested_name_specifier into a _list */
nested_name_specifier :
	  SCOPE nested_name_specifier_list
	|       nested_name_specifier_list
	| SCOPE
	;

nested_name_specifier_list :
	  unqualified_id_scope
	| nested_name_specifier_list unqualified_id_scope
	;

unqualified_id_scope :
	  unqualified_id SCOPE
	;

/* class_or_namespace_name : class_name | namespace_name ; eliminated */

postfix_expression :
	  primary_expression
	| postfix_expression LBRACKET expression RBRACKET
	| postfix_expression LPAREN expression RPAREN
	| postfix_expression LEFT_RIGHT
	/* postfix_expression (DOT | PTR_OP) SCOPE? id_expression covered by:
	 * postfix_expression (DOT | PTR_OP) pseudo_destructor_name */
	/* SCOPE id_expression | id_expression covered by declarator_id */
	| postfix_expression DOT TEMPLATE declarator_id
	| postfix_expression PTR_OP TEMPLATE declarator_id
	| postfix_expression DOT pseudo_destructor_name
	| postfix_expression PTR_OP pseudo_destructor_name
	| postfix_expression INC_OP
	| postfix_expression DEC_OP
	| DYNAMIC_CAST LT type_id GT LPAREN expression RPAREN
	| STATIC_CAST LT type_id GT LPAREN expression RPAREN
	| REINTERPRET_CAST LT type_id GT LPAREN expression RPAREN
	| CONST_CAST LT type_id GT LPAREN expression RPAREN
	| TYPEID LPAREN expression RPAREN
	| TYPEID LPAREN type_id RPAREN
	/* C99 */
	| LPAREN type_id RPAREN LBRACE initializer_list COMMA RBRACE
	| LPAREN type_id RPAREN LBRACE initializer_list       RBRACE
	| LPAREN type_id RPAREN LBRACE                        RBRACE
	;

expression :
	  assignment_expression
	| expression COMMA assignment_expression
	;

pseudo_destructor_name :
	/* type_name is covered by unqualified_id */
	/* nested_name_specifier unqualified_id is covered by
	 * qualified_namespace_specifier */
	/* qualified_namespace_specifier SCOPE is covered by
	 * nested_name_specifier (list of unqualified_id separated by SCOPE
	 * and ending on SCOPE */
	/* TILDE type_name is covered by unqualified_id, "nested_name_specifier
	 * unqualified_id | unqualified_id" is covered by nested_name_specifier_list */
	  nested_name_specifier
	;

unary_expression :
	  postfix_expression
	| INC_OP cast_expression
	| DEC_OP cast_expression
	| ASTERISK cast_expression
	| BITAND   cast_expression %prec UNARY
	| PLUS     cast_expression
	| MINUS    cast_expression
	| EXCLAM   cast_expression
	| TILDE    cast_expression
	| SIZEOF unary_expression
	| SIZEOF LPAREN type_id RPAREN
	| new_expression
	| delete_expression
	;

/* to reduce conflicts we moved unary_operator into umary_expression */
/*unary_operator :
	  ASTERISK
	| BITAND
	| PLUS
	| MINUS
	| EXCLAM
	| TILDE
	;*/

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
	;

new_initializer :
	  LPAREN expression RPAREN
	| LEFT_RIGHT
	;

new_type_id :
	  type_specifier_seq ptr_operator_seq direct_new_declarator
	  type_specifier_seq                  direct_new_declarator
	| type_specifier_seq
	;

/* the rule new_declarator : ptr_operator* direct_new_declarator has been
 * merged into new_type_id with creation of ptr_operator_seq */
ptr_operator_seq :
	  ptr_operator
	| ptr_operator_seq ptr_operator
	;

direct_new_declarator :
	  LBRACKET expression RBRACKET
	/* replace constant_expression with conditional_expression */
	| direct_new_declarator LBRACKET conditional_expression RBRACKET
	;

delete_expression :
	  SCOPE DELETE                   cast_expression
	|       DELETE                   cast_expression
	| SCOPE DELETE LBRACKET RBRACKET cast_expression
	|       DELETE LBRACKET RBRACKET cast_expression
	;

cast_expression :
	  unary_expression
	| LPAREN type_id RPAREN cast_expression
	;

pm_expression :
	  cast_expression
	| pm_expression DOTSTAR_OP cast_expression
	| pm_expression PTRSTAR_OP cast_expression
	;

multiplicative_expression :
	  pm_expression
	| multiplicative_expression ASTERISK pm_expression
	| multiplicative_expression DIV pm_expression
	| multiplicative_expression MOD pm_expression
	;

additive_expression :
	  multiplicative_expression
	| additive_expression PLUS multiplicative_expression
	| additive_expression MINUS multiplicative_expression
	;

shift_expression :
	  additive_expression
	| shift_expression LSHIFT additive_expression
	| shift_expression RSHIFT additive_expression
	;

relational_expression :
	  shift_expression
	| relational_expression LT shift_expression
	| relational_expression GT shift_expression
	| relational_expression LTEQUAL shift_expression
	| relational_expression GTEQUAL shift_expression
	;

equality_expression :
	  relational_expression
	| equality_expression EQUAL relational_expression
	| equality_expression NOTEQUAL relational_expression
	;

and_expression :
	  equality_expression
	| and_expression BITAND equality_expression
	;

exclusive_or_expression :
	  and_expression
	| exclusive_or_expression BITXOR and_expression
	;

inclusive_or_expression :
	  exclusive_or_expression
	| inclusive_or_expression BITOR exclusive_or_expression
	;

logical_and_expression :
	  inclusive_or_expression
	| logical_and_expression LOGICALAND inclusive_or_expression
	;

logical_or_expression :
	  logical_and_expression
	| logical_or_expression LOGICALOR logical_and_expression
	;

conditional_expression :
	  logical_or_expression
	| logical_or_expression QUESTION expression COLON assignment_expression
	;

assignment_expression :
	  conditional_expression
	| logical_or_expression assignment_operator assignment_expression
	| throw_expression
	;

assignment_operator :
	  IS
	| MUL_ASSIGN
	| DIV_ASSIGN
	| MOD_ASSIGN
	| ADD_ASSIGN
	| SUB_ASSIGN
	| LS_ASSIGN
	| RS_ASSIGN
	| AND_ASSIGN
	| OR_ASSIGN
	| XOR_ASSIGN
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
	  expression
	| type_specifier_seq declarator IS assignment_expression
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
	;

declaration :
	  block_declaration
	| function_definition
	| template_declaration
	| TEMPLATE declaration
	| TEMPLATE LT GT declaration
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
	  decl_specifier_seq init_declarator_list SEMICOLON
	|                    init_declarator_list SEMICOLON
	| decl_specifier_seq                      SEMICOLON
	;

decl_specifier_seq :
	  decl_specifier
	| decl_specifier_seq decl_specifier
	;

decl_specifier :
	  storage_class_specifier
	| type_specifier
	| function_specifier
	| FRIEND
	| TYPEDEF
	;

function_specifier :
	  INLINE
	| VIRTUAL
	| EXPLICIT
	;

storage_class_specifier :
	  AUTO
	| REGISTER
	| STATIC
	| EXTERN
	| MUTABLE
	;

type_specifier :
	  simple_type_specifier
	| class_specifier
	| enum_specifier
	| elaborated_type_specifier
	| cv_qualifier
	/* C99 */
	| IMAGPART
	| ENUM ID LBRACE enumerator_list COMMA RBRACE
	| ENUM    LBRACE enumerator_list COMMA RBRACE
	/* typedef_name is covered by elaborated_type_specifier -> type_name
	 * -> typedef_name */
	;

simple_type_specifier :
	  qualified_namespace_specifier
	| char_type
	| BOOLEAN
	| integer_type
	| floating_pt_type
	| VOID
	;

elaborated_type_specifier :
	/* replaced "nested_name_specifier ID" with qualified_namespace_specifier */
	  class_key qualified_namespace_specifier
	| ENUM      qualified_namespace_specifier
	| TYPENAME  qualified_namespace_specifier
	| TYPENAME  qualified_namespace_specifier LT template_argument_list GT
	;  

char_type :
	  SIGNED CHAR
	| SIGNED WCHAR
	| UNSIGNED CHAR
	| UNSIGNED WCHAR
	| CHAR
	| WCHAR
	;

integer_type :
	  SIGNED integer_size INT
	| SIGNED              INT
	| SIGNED integer_size
	| SIGNED
	| UNSIGNED integer_size INT
	| UNSIGNED              INT
	| UNSIGNED integer_size
	| UNSIGNED
	| integer_size INT
	|              INT
	| integer_size
	;

integer_size :
	  LONG
	| LONGLONG
	| SHORT
	;

floating_pt_type :
	  FLOAT
	| DOUBLE
	| LONG DOUBLE
	;

type_name :
	  class_name
	| ENUM_ID
	| TYPEDEF_ID
	;

enum_specifier :
	  ENUM ID LBRACE enumerator_list RBRACE
	| ENUM    LBRACE enumerator_list RBRACE
	| ENUM ID LBRACE                 RBRACE
	| ENUM    LBRACE                 RBRACE
	;

enumerator_list :
	  enumerator_definition
	| enumerator_list COMMA enumerator_definition
	;

enumerator_definition :
	  enumerator
	/* replace constant_expression with conditional_expression */
	| enumerator IS conditional_expression
	;

enumerator :
	  ID
	;

namespace_definition :
	  NAMESPACE ID LBRACE namespace_body RBRACE
	| NAMESPACE NAMESPACE_ID LBRACE namespace_body RBRACE
	| NAMESPACE LBRACE namespace_body RBRACE
	;

/* named_namespace_definition :
	  NAMESPACE ID LBRACE namespace_body RBRACE
	| NAMESPACE NAMESPACE_ID LBRACE namespace_body RBRACE
	; inlined */

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
	;

qualified_namespace_specifier :
	  nested_name_specifier NAMESPACE_ID
	|                       NAMESPACE_ID
	;

using_declaration :
	  USING TYPENAME nested_name_specifier unqualified_id SEMICOLON
	| USING          nested_name_specifier unqualified_id SEMICOLON
	;

using_directive :
	  USING NAMESPACE qualified_namespace_specifier SEMICOLON
	;

asm_definition :
	  ASM_KEYWORD LPAREN asm_definition_interior RPAREN SEMICOLON
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
	  EXTERN string LBRACE declaration_seq RBRACE
	| EXTERN string LBRACE RBRACE
	| EXTERN string declaration
	;

/* A.7 Declarators */
init_declarator_list :
	  init_declarator
	| init_declarator_list COMMA init_declarator
	;

init_declarator :
	  declarator maybeasm maybeattribute
	/* initializer inlined */
	| declarator maybeasm maybeattribute IS initializer_clause
	| declarator maybeasm maybeattribute LPAREN expression RPAREN
	;

/* gcc extension: assign register */
maybeasm :
	  asm_definition
	| /* empty */
	;

/* gcc extension: attributes */
maybeattribute :
	  attribute_seq
	| /* empty */
	;

declarator :
	                   direct_declarator
	| ptr_operator_seq direct_declarator
	;

direct_declarator :
	  declarator_id
	| direct_declarator LPAREN parameter_declaration_clause RPAREN cv_qualifier_seq exception_specification
	| direct_declarator LPAREN parameter_declaration_clause RPAREN cv_qualifier_seq
	/* replace constant_expression with conditional_expression */
	| direct_declarator LBRACKET conditional_expression RBRACKET
	| direct_declarator LBRACKET RBRACKET
	| LPAREN declarator RPAREN
	/* C99 */
	| direct_declarator LBRACKET ASTERISK RBRACKET
	/* "direct_declarator LPAREN identifier_list RPAREN" covered by
	 * parameter_declaration_clause -> parameter_declaration_list ->
	 * parameter_declaration (COMMA parameter_declaration)* ->
	 * decl_specifier_seq ->  decl_specifier+ -> type_specifier ->
	 * qualified_namespace_specifier -> unqualified_id ->  ID
	 */
	| direct_declarator LEFT_RIGHT
	;

/* identifier_list : ID | identifier_list COMMA ID ; */

ptr_operator :
	  BITAND
	| nested_name_specifier ASTERISK cv_qualifier_seq
	|                       ASTERISK cv_qualifier_seq
	;

cv_qualifier_seq :
	  cv_qualifier_seq cv_qualifier
	| /* empty */
	;

cv_qualifier :
	  CONST
	| VOLATILE
	/* C99 */
	| RESTRICT
	| BYCOPY
	| BYREF
	| IN
	| OUT
	| INOUT
	| ONEWAY
	;

declarator_id :
	  SCOPE id_expression
	|       id_expression
	/* id_expression -> unqualified_id -> type_name contains SCOPE
	 * type_name | type_name | nested_name_specifier type_name */
	;

type_id :
	  type_specifier_seq abstract_declarator
	| type_specifier_seq
	;

type_specifier_seq :
	  type_specifier_seq type_specifier
	| type_specifier
	;

abstract_declarator :
	  ptr_operator_seq direct_abstract_declarator
	|                  direct_abstract_declarator
	;

direct_abstract_declarator :
	  direct_abstract_declarator LPAREN parameter_declaration_clause RPAREN cv_qualifier_seq exception_specification
	| direct_abstract_declarator LPAREN parameter_declaration_clause RPAREN cv_qualifier_seq
	|                            LPAREN parameter_declaration_clause RPAREN cv_qualifier_seq exception_specification
	|                            LPAREN parameter_declaration_clause RPAREN cv_qualifier_seq
	/* replace constant_expression with conditional_expression */
	| direct_abstract_declarator LBRACKET conditional_expression RBRACKET
	|                            LBRACKET conditional_expression RBRACKET
	| direct_abstract_declarator LBRACKET                        RBRACKET
	|                            LBRACKET                        RBRACKET
	| LPAREN abstract_declarator RPAREN
	/* C99 */
	| direct_abstract_declarator LBRACKET ASTERISK RBRACKET
	;

parameter_declaration_clause :
	  parameter_declaration_list ELLIPSIS
	| parameter_declaration_list
	|                            ELLIPSIS
	| /* empty */
	| parameter_declaration_list COMMA ELLIPSIS
	;

parameter_declaration_list :
	  parameter_declaration
	| parameter_declaration_list COMMA parameter_declaration
	;

parameter_declaration :
	  decl_specifier_seq declarator
	| decl_specifier_seq declarator IS assignment_expression
	| decl_specifier_seq abstract_declarator
	| decl_specifier_seq
	| decl_specifier_seq abstract_declarator IS assignment_expression
	| decl_specifier_seq IS assignment_expression
	;

function_definition :
	  decl_specifier_seq declarator ctor_initializer compound_statement
	|                    declarator ctor_initializer compound_statement 
	| decl_specifier_seq declarator                  compound_statement 
	|                    declarator                  compound_statement 
	| decl_specifier_seq declarator function_try_block
	|                    declarator function_try_block
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
	;

designator_list :
	  designator
	| designator_list designator
	;

designator :
	  /* replace constant_expression with conditional_expression */
	  LBRACKET conditional_expression RBRACKET
	| DOT ID
	;

/* A.8 Classes */
class_specifier :
	  /* inlined "class_head : class_key qualified_namespace_specifier?
	   * (COLON base_specifier_list)?" */
	  class_key                               COLON base_specifier_list LBRACE member_specification RBRACE
	| class_key                                                         LBRACE member_specification RBRACE
	| class_key qualified_namespace_specifier COLON base_specifier_list LBRACE member_specification RBRACE
	| class_key qualified_namespace_specifier                           LBRACE member_specification RBRACE
	| class_key                               COLON base_specifier_list LBRACE RBRACE
	| class_key                                                         LBRACE RBRACE
	| class_key qualified_namespace_specifier COLON base_specifier_list LBRACE RBRACE
	| class_key qualified_namespace_specifier                           LBRACE RBRACE
	;

class_key :
	  CLASS
	| STRUCT
	| UNION
	;

member_specification :
	  member_declaration member_specification
	| member_declaration
	| access_specifier COLON member_specification
	| access_specifier COLON
	;

member_declaration :
	  decl_specifier_seq member_declarator_list SEMICOLON
	|                    member_declarator_list SEMICOLON
	| decl_specifier_seq                        SEMICOLON
	|                                           SEMICOLON
	| function_definition SEMICOLON
	| function_definition
	| qualified_id SEMICOLON
	| using_declaration
	| template_declaration
	;

member_declarator_list :
	  member_declarator
	| member_declarator_list COMMA member_declarator
	;

member_declarator :
	| declarator
	/* constant_initializer covers pure_specifier */
	/* replace constant_expression with conditional_expression */
	| declarator IS conditional_expression
	| ID COLON conditional_expression
	|    COLON conditional_expression
	;

/* covered by constant_initializer:
pure_specifier : IS LIT_INT ;
*/

/* constant_initializer : IS constant_expression ; inlined */

/* A.9 Derived classes */
/* integrated base_clause into class_head to resolve conflicts (allow bison to
 * have more look-ahead tokens */
/*base_clause :
	  COLON base_specifier_list
	;*/

base_specifier_list :
	  base_specifier
	| base_specifier_list COMMA base_specifier
	;

/* to reduce conflicts all class_name-s have been replaced with type_name */
base_specifier :
	  qualified_namespace_specifier
	| VIRTUAL access_specifier qualified_namespace_specifier
	| VIRTUAL                  qualified_namespace_specifier
	| access_specifier VIRTUAL qualified_namespace_specifier
	| access_specifier         qualified_namespace_specifier
	;

access_specifier :
	  PRIVATE
	| PROTECTED
	| PUBLIC
	;

/* A.10 Special member functions */
/* conversion_function_id : OPERATOR conversion_type_id ;  inlined */

/* conversion_type_id :
          type_specifier_seq conversion_declarator
	| type_specifier_seq
	; inlined */

/* conversion_declarator : ptr_operator+ which is a ptr_operator_seq */

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
	  OPERATOR operator
	;

operator :
	  NEW
	| DELETE
	| NEW LBRACKET RBRACKET
	| DELETE LBRACKET RBRACKET
	| PLUS
	| MINUS
	| ASTERISK
	| DIV
	| MOD
	| BITXOR
	| BITAND
	| BITOR
	| TILDE
	| EXCLAM
	| IS
	| LT
	| GT
	| ADD_ASSIGN
	| SUB_ASSIGN
	| MUL_ASSIGN
	| DIV_ASSIGN
	| MOD_ASSIGN
	| XOR_ASSIGN
	| AND_ASSIGN
	| OR_ASSIGN
	| LSHIFT
	| RSHIFT
	| LS_ASSIGN
	| RS_ASSIGN
	| EQUAL
	| NOTEQUAL
	| LTEQUAL
	| GTEQUAL
	| LOGICALAND
	| LOGICALOR
	| INC_OP
	| DEC_OP
	| COMMA
	| PTRSTAR_OP
	| PTR_OP
	| LEFT_RIGHT
	| LBRACKET RBRACKET
	;

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
	  class_key            IS type_id
	| class_key class_name IS type_id
	| class_key
	/* "class_key class_name" covered by parameter_declaration */
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
/*template_id :
	  template_name LT template_argument_list GT
	;*/

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

/* explicit_specialization :  TEMPLATE LT GT declaration ; inlined *

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
	| THROW
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
	| attribute_seq attribute
	;

attribute :
	  ATTRIBUTE LPAREN LPAREN attribute_list RPAREN RPAREN
	;

attribute_list :
	  attribute_parameter
	| attribute_list COMMA attribute_parameter
	;

attribute_parameter :
	  /* empty */
	| any_word
	| any_word LPAREN ID RPAREN
	| any_word LPAREN ID COMMA expression RPAREN
	| any_word LPAREN expression RPAREN
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

