%token IDENTIFIER
%token TYPENAME
%token SCSPEC
%token TYPESPEC
%token TYPE_QUAL
%token CONSTANT
%token STRING
%token ELLIPSIS

%token SIZEOF ENUM STRUCT UNION IF ELSE WHILE DO FOR SWITCH CASE DEFAULT
%token BREAK CONTINUE RETURN GOTO ASM_KEYWORD TYPEOF ALIGNOF
%token ATTRIBUTE EXTENSION LABEL
%token REALPART IMAGPART

%nonassoc IF
%nonassoc ELSE

%right ASSIGN IS
%right QUESTION COLON
%left OROR
%left ANDAND
%left BITOR
%left BITXOR
%left BITAND
%left EQCOMPARE
%left ARITHCOMPARE
%left LSHIFT RSHIFT
%left PLUS MINUS
%left ASTERISK DIV MOD
%right UNARY PLUSPLUS MINUSMINUS
%left HYPERUNARY
%left POINTSAT DOT LPAREN LBRACKET

%%
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
	| ASM_KEYWORD LPAREN gcc_nonnull_exprlist RPAREN SEMICOLON
	| EXTENSION gcc_extdef
	;

gcc_datadef:
					gcc_notype_initdecls SEMICOLON
	| gcc_declmods  gcc_notype_initdecls SEMICOLON
	| gcc_typed_declspecs  gcc_initdecls SEMICOLON
    | gcc_declmods SEMICOLON
	| gcc_typed_declspecs SEMICOLON
	| SEMICOLON
	;

gcc_fndef:
	  gcc_typed_declspecs  gcc_declarator gcc_old_style_parm_decls gcc_compstmt
	| gcc_declmods  gcc_notype_declarator gcc_old_style_parm_decls gcc_compstmt
	|				gcc_notype_declarator gcc_old_style_parm_decls gcc_compstmt
	;

gcc_identifier:
	IDENTIFIER
	| TYPENAME
	;

gcc_unop: 
	  BITAND
	| MINUS
	| PLUS
	| PLUSPLUS
	| MINUSMINUS
	| TILDE
	| EXCLAM
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
	| ASTERISK gcc_cast_expr   %prec UNARY
	| EXTENSION gcc_cast_expr	  %prec UNARY
	| gcc_unop gcc_cast_expr  %prec UNARY
	| ANDAND gcc_identifier
	| SIZEOF gcc_unary_expr  %prec UNARY
	| SIZEOF LPAREN gcc_typename RPAREN  %prec HYPERUNARY
	| ALIGNOF gcc_unary_expr  %prec UNARY
	| ALIGNOF LPAREN gcc_typename RPAREN  %prec HYPERUNARY
	| REALPART gcc_cast_expr %prec UNARY
	| IMAGPART gcc_cast_expr %prec UNARY
	;

gcc_cast_expr:
	gcc_unary_expr
	| LPAREN gcc_typename RPAREN gcc_cast_expr  %prec UNARY
	| LPAREN gcc_typename RPAREN LBRACE gcc_initlist_maybe_comma RBRACE  %prec UNARY
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
	| gcc_expr_no_commas ARITHCOMPARE gcc_expr_no_commas
	| gcc_expr_no_commas EQCOMPARE gcc_expr_no_commas
	| gcc_expr_no_commas BITAND gcc_expr_no_commas
	| gcc_expr_no_commas BITOR gcc_expr_no_commas
	| gcc_expr_no_commas BITXOR gcc_expr_no_commas
	| gcc_expr_no_commas ANDAND gcc_expr_no_commas
	| gcc_expr_no_commas OROR gcc_expr_no_commas
	| gcc_expr_no_commas QUESTION gcc_nonnull_exprlist COLON gcc_expr_no_commas
	| gcc_expr_no_commas QUESTION COLON gcc_expr_no_commas
	| gcc_expr_no_commas IS gcc_expr_no_commas
	| gcc_expr_no_commas ASSIGN gcc_expr_no_commas
	;

gcc_primary:
	IDENTIFIER
	| CONSTANT
	| gcc_string
	| LPAREN gcc_nonnull_exprlist RPAREN
	| LPAREN gcc_compstmt RPAREN
	| gcc_primary LPAREN gcc_exprlist RPAREN   %prec DOT
	| gcc_primary LBRACKET gcc_nonnull_exprlist RBRACKET   %prec DOT
	| gcc_primary DOT gcc_identifier
	| gcc_primary POINTSAT gcc_identifier
	| gcc_primary PLUSPLUS
	| gcc_primary MINUSMINUS
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

gcc_datadecls:
	  gcc_datadecl
	| gcc_datadecls gcc_datadecl
	;

gcc_datadecl:
	  gcc_typed_declspecs_no_prefix_attr gcc_initdecls SEMICOLON
	| gcc_typed_declspecs_no_prefix_attr SEMICOLON
	| gcc_declmods_no_prefix_attr gcc_notype_initdecls SEMICOLON
	| gcc_declmods_no_prefix_attr SEMICOLON
	;

gcc_decls:
	  gcc_decl
	| gcc_decls gcc_decl
	;

gcc_decl:
	gcc_typed_declspecs  gcc_initdecls SEMICOLON
	| gcc_declmods  gcc_notype_initdecls SEMICOLON
	| gcc_typed_declspecs  gcc_nested_function
	| gcc_declmods  gcc_notype_nested_function
	| gcc_typed_declspecs SEMICOLON
	| gcc_declmods SEMICOLON
	| EXTENSION gcc_decl
	;

gcc_typed_declspecs:
	  gcc_typespec gcc_reserved_declspecs
	| gcc_declmods gcc_typespec gcc_reserved_declspecs
	;

gcc_reserved_declspecs:  /* empty */
	| gcc_reserved_declspecs gcc_typespecqual_reserved
	| gcc_reserved_declspecs SCSPEC
	| gcc_reserved_declspecs gcc_attributes
	;

gcc_typed_declspecs_no_prefix_attr:
								  gcc_typespec gcc_reserved_declspecs_no_prefix_attr
	| gcc_declmods_no_prefix_attr gcc_typespec gcc_reserved_declspecs_no_prefix_attr
	;

gcc_reserved_declspecs_no_prefix_attr:
	  /* gcc_empty */
	| gcc_reserved_declspecs_no_prefix_attr gcc_typespecqual_reserved
	| gcc_reserved_declspecs_no_prefix_attr SCSPEC
	;

gcc_declmods:
	  gcc_declmods_no_prefix_attr
	| gcc_attributes
	| gcc_declmods gcc_declmods_no_prefix_attr
	| gcc_declmods gcc_attributes
	;

gcc_declmods_no_prefix_attr:
	  TYPE_QUAL
	| SCSPEC
	| gcc_declmods_no_prefix_attr TYPE_QUAL
	| gcc_declmods_no_prefix_attr SCSPEC
	;

gcc_typed_typespecs:
	  gcc_typespec gcc_reserved_typespecquals
	| gcc_nonempty_type_quals gcc_typespec gcc_reserved_typespecquals
	;

gcc_reserved_typespecquals:  /* empty */
	| gcc_reserved_typespecquals gcc_typespecqual_reserved
	;

gcc_typespec: TYPESPEC
	| gcc_structsp
	| TYPENAME
	| TYPEOF LPAREN gcc_nonnull_exprlist RPAREN
	| TYPEOF LPAREN gcc_typename RPAREN
	;

gcc_typespecqual_reserved: TYPESPEC
	| TYPE_QUAL
	| gcc_structsp
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
	| gcc_any_word LPAREN IDENTIFIER RPAREN
	| gcc_any_word LPAREN IDENTIFIER COMMA gcc_nonnull_exprlist RPAREN
	| gcc_any_word LPAREN gcc_exprlist RPAREN
	;

gcc_any_word:
	  gcc_identifier
	| SCSPEC
	| TYPESPEC
	| TYPE_QUAL
	;

gcc_init:
	gcc_expr_no_commas
	| LBRACE gcc_initlist_maybe_comma RBRACE
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
	  gcc_after_type_declarator
	| gcc_notype_declarator
	;

gcc_after_type_declarator:
	  LPAREN gcc_after_type_declarator RPAREN
	| gcc_after_type_declarator LPAREN gcc_parmlist_or_identifiers  %prec DOT
	| gcc_after_type_declarator LBRACKET gcc_nonnull_exprlist RBRACKET  %prec DOT
	| gcc_after_type_declarator LBRACKET RBRACKET  %prec DOT
	| ASTERISK gcc_type_quals gcc_after_type_declarator  %prec UNARY
	| gcc_attributes  gcc_after_type_declarator
	| TYPENAME
	;

gcc_parm_declarator:
	  gcc_parm_declarator LPAREN gcc_parmlist_or_identifiers  %prec DOT
	| gcc_parm_declarator LBRACKET ASTERISK RBRACKET  %prec DOT
	| gcc_parm_declarator LBRACKET gcc_nonnull_exprlist RBRACKET  %prec DOT
	| gcc_parm_declarator LBRACKET RBRACKET  %prec DOT
	| ASTERISK gcc_type_quals gcc_parm_declarator  %prec UNARY
	| gcc_attributes  gcc_parm_declarator
	| TYPENAME
	;

gcc_notype_declarator:
	  gcc_notype_declarator LPAREN gcc_parmlist_or_identifiers  %prec DOT
	| LPAREN gcc_notype_declarator RPAREN
	| ASTERISK gcc_type_quals gcc_notype_declarator  %prec UNARY
	| gcc_notype_declarator LBRACKET ASTERISK RBRACKET  %prec DOT
	| gcc_notype_declarator LBRACKET gcc_nonnull_exprlist RBRACKET  %prec DOT
	| gcc_notype_declarator LBRACKET RBRACKET  %prec DOT
	| gcc_attributes  gcc_notype_declarator
	| IDENTIFIER
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
	| gcc_enum_head gcc_identifier LBRACE gcc_enumlist gcc_maybecomma RBRACE gcc_maybe_attribute
	| gcc_enum_head LBRACE gcc_enumlist gcc_maybecomma RBRACE gcc_maybe_attribute
	| gcc_enum_head gcc_identifier
	;

gcc_maybecomma:
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
	  gcc_typed_typespecs  gcc_components
	| gcc_typed_typespecs
	| gcc_nonempty_type_quals  gcc_components
	| gcc_nonempty_type_quals
	| EXTENSION gcc_component_decl
	;

gcc_components:
	  gcc_component_declarator
	| gcc_components COMMA gcc_component_declarator
	;

gcc_component_declarator:
	    gcc_declarator gcc_maybe_attribute
	|   gcc_declarator COLON gcc_expr_no_commas gcc_maybe_attribute
	|   COLON gcc_expr_no_commas gcc_maybe_attribute
	;

gcc_enumlist:
	  gcc_enumerator
	| gcc_enumlist COMMA gcc_enumerator
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
	  TYPE_QUAL
	| gcc_nonempty_type_quals TYPE_QUAL
	;

gcc_type_quals:
	  /* empty */
	| gcc_type_quals TYPE_QUAL
	;

gcc_absdcl1:  /* gcc_a gcc_nonempty gcc_absolute gcc_declarator */
	  LPAREN gcc_absdcl1 RPAREN
	| ASTERISK gcc_type_quals gcc_absdcl1  %prec UNARY
	| ASTERISK gcc_type_quals  %prec UNARY
	| gcc_absdcl1 LPAREN gcc_parmlist  %prec DOT
	| gcc_absdcl1 LBRACKET gcc_nonnull_exprlist RBRACKET  %prec DOT
	| gcc_absdcl1 LBRACKET RBRACKET  %prec DOT
	| LPAREN gcc_parmlist  %prec DOT
	| LBRACKET gcc_nonnull_exprlist RBRACKET  %prec DOT
	| LBRACKET RBRACKET  %prec DOT
	| gcc_attributes  gcc_absdcl1
	;

gcc_stmts:
	gcc_lineno_stmt_or_labels
	;

gcc_lineno_stmt_or_labels:
	  gcc_stmt_or_label
	| gcc_lineno_stmt_or_labels gcc_stmt_or_label
	| gcc_lineno_stmt_or_labels 
	;

gcc_xstmts:
	/* empty */
	| gcc_stmts
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

gcc_compstmt: LBRACE RBRACE
	| LBRACE  gcc_maybe_label_decls gcc_decls gcc_xstmts RBRACE
	| LBRACE  gcc_maybe_label_decls gcc_stmts RBRACE
	;

gcc_simple_if:
	  gcc_if_prefix gcc_lineno_labeled_stmt
	;

gcc_if_prefix:
	  IF LPAREN gcc_nonnull_exprlist RPAREN
	;

gcc_do_stmt_start:
	  DO gcc_lineno_labeled_stmt WHILE
	;

gcc_lineno_labeled_stmt:
	    gcc_stmt
	|   gcc_label gcc_lineno_labeled_stmt
	;

gcc_stmt_or_label:
	  gcc_stmt
	| gcc_label
	;

gcc_stmt:
	  gcc_compstmt
    | gcc_all_iter_stmt 
	| gcc_nonnull_exprlist SEMICOLON
	| gcc_simple_if ELSE gcc_lineno_labeled_stmt
	| gcc_simple_if %prec IF
	| WHILE LPAREN gcc_nonnull_exprlist RPAREN gcc_lineno_labeled_stmt
	| gcc_do_stmt_start LPAREN gcc_nonnull_exprlist RPAREN SEMICOLON
	| FOR LPAREN gcc_xexpr SEMICOLON gcc_xexpr SEMICOLON gcc_xexpr RPAREN gcc_lineno_labeled_stmt
	| SWITCH LPAREN gcc_nonnull_exprlist RPAREN gcc_lineno_labeled_stmt
	| BREAK SEMICOLON
	| CONTINUE SEMICOLON
	| RETURN SEMICOLON
	| RETURN gcc_nonnull_exprlist SEMICOLON
	| ASM_KEYWORD gcc_maybe_type_qual LPAREN gcc_nonnull_exprlist RPAREN SEMICOLON
	| ASM_KEYWORD gcc_maybe_type_qual LPAREN gcc_nonnull_exprlist COLON gcc_asm_operands RPAREN SEMICOLON
	| ASM_KEYWORD gcc_maybe_type_qual LPAREN gcc_nonnull_exprlist COLON gcc_asm_operands COLON gcc_asm_operands RPAREN SEMICOLON
	| ASM_KEYWORD gcc_maybe_type_qual LPAREN gcc_nonnull_exprlist COLON gcc_asm_operands COLON gcc_asm_operands COLON gcc_asm_clobbers RPAREN SEMICOLON
	| GOTO gcc_identifier SEMICOLON
	| GOTO ASTERISK gcc_nonnull_exprlist SEMICOLON
	| SEMICOLON
	;

gcc_all_iter_stmt:
	  gcc_all_iter_stmt_simple
	;

gcc_all_iter_stmt_simple:
	  FOR LPAREN gcc_primary RPAREN gcc_lineno_labeled_stmt

gcc_label:	  
	  CASE gcc_expr_no_commas COLON
	| CASE gcc_expr_no_commas ELLIPSIS gcc_expr_no_commas COLON
	| DEFAULT COLON
	| gcc_identifier COLON gcc_maybe_attribute
	;

gcc_maybe_type_qual:
	/* empty */
	| TYPE_QUAL
	;

gcc_xexpr:
	/* empty */
	| gcc_nonnull_exprlist
	;

gcc_asm_operands: /* empty */
	| gcc_nonnull_asm_operands
	;

gcc_nonnull_asm_operands:
	  gcc_asm_operand
	| gcc_nonnull_asm_operands COMMA gcc_asm_operand
	;

gcc_asm_operand:
	  STRING LPAREN gcc_nonnull_exprlist RPAREN
	;

gcc_asm_clobbers:
	  gcc_string
	| gcc_asm_clobbers COMMA gcc_string
	;

gcc_parmlist:
	  gcc_parmlist_2 RPAREN
	| gcc_parms SEMICOLON gcc_parmlist
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
	  gcc_typed_declspecs  gcc_parm_declarator gcc_maybe_attribute
	| gcc_typed_declspecs  gcc_notype_declarator gcc_maybe_attribute
	| gcc_typed_declspecs  gcc_absdcl gcc_maybe_attribute
	| gcc_declmods  gcc_notype_declarator gcc_maybe_attribute
	| gcc_declmods  gcc_absdcl gcc_maybe_attribute
	;

gcc_parmlist_or_identifiers:
	  gcc_parmlist
	| gcc_identifiers RPAREN
	;

gcc_identifiers:
	IDENTIFIER
	| gcc_identifiers COMMA IDENTIFIER
	;

gcc_identifiers_or_typenames:
	gcc_identifier
	| gcc_identifiers_or_typenames COMMA gcc_identifier
	;

%%
