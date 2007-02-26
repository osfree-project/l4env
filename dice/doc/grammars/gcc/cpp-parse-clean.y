%token EXTENSION	ASM_KEYWORD	NAMESPACE	USING	NSNAME		SCOPE
%token EXTERN_LANG_STRING		TEMPLATE	TYPENAME_KEYWORD	ARITHCOMPARE
%token EMPTY		END_OF_SAVED_INPUT		SELFNAME			LEFT_RIGHT
%token RETURN_KEYWORD			IDENTIFIER	TYPENAME	PTYPENAME
%token IDENTIFIER_DEFN	TYPENAME_DEFN	PTYPENAME_DEFN	SCSPEC	RSHIFT
%token PLUSPLUS		MINUSMINUS	UNARY		ANDAND		SIZEOF	HYPERUNARY
%token ALIGNOF		REALPART	IMAGPART	POINTSAT_STAR		DOT_STAR
%token LSHIFT		EQCOMPARE	MIN_MAX		OROR		ASSIGN	THROW
%token PFUNCNAME	CONSTANT	THIS		CV_QUALIFIER		DYNAMIC_CAST
%token STATIC_CAST	REINTERPRET_CAST	CONST_CAST	TYPEID	TYPESPEC	NEW
%token DELETE	CXX_TRUE	CXX_FALSE	STRING	POINTSAT	TYPEOF	SIGOF
%token ATTRIBUTE	PRE_PARSED_FUNCTION_DECL	DEFARG_MARKER	ENUM
%token AGGR	COMMA	ID	TRY	PAREN_STAR_PAREN	LABEL	IF	ELSE	WHILE
%token DO	FOR	SWITCH	CASE	ELLIPSIS	DEFAULT	BREAK	CONTINUE	GOTO
%token CATCH	DEFARG	OPERATOR

%%
gcc_program:
	  /* empty */
	| gcc_extdefs
	;

gcc_extdefs:
	  gcc_lang_extdef
	| gcc_extdefs gcc_lang_extdef
	;

gcc_extdefs_opt:
	  gcc_extdefs
	| /* empty */
	;

.gcc_hush_warning:
	;
.gcc_warning_ok:
	;

gcc_extension:
	EXTENSION
	;

gcc_asm_keyword:
	  ASM_KEYWORD
	;

gcc_lang_extdef:
	  gcc_extdef
	;

gcc_extdef:
	  gcc_fndef gcc_eat_saved_input
	| gcc_datadef
	| gcc_template_def
	| gcc_asm_keyword '(' gcc_string ')' ';'
	| gcc_extern_lang_string '{' gcc_extdefs_opt '}'
	| gcc_extern_lang_string .gcc_hush_warning gcc_fndef .gcc_warning_ok gcc_eat_saved_input
	| gcc_extern_lang_string .gcc_hush_warning gcc_datadef .gcc_warning_ok
	| NAMESPACE gcc_identifier '{' gcc_extdefs_opt '}'
	| NAMESPACE '{' gcc_extdefs_opt '}'
	| gcc_namespace_alias
	| gcc_using_decl ';'
	| gcc_using_directive
	| gcc_extension gcc_extdef
	;

gcc_namespace_alias:
    NAMESPACE gcc_identifier '=' gcc_any_id ';'
	;

gcc_using_decl:
	  USING gcc_qualified_id
	| USING gcc_global_scope gcc_qualified_id
	| USING gcc_global_scope gcc_unqualified_id
	;

gcc_namespace_using_decl:
	  USING gcc_namespace_qualifier gcc_identifier
	| USING gcc_global_scope gcc_identifier
	| USING gcc_global_scope gcc_namespace_qualifier gcc_identifier
	;

gcc_using_directive:
	  USING NAMESPACE gcc_any_id ';'
	;

gcc_namespace_qualifier:
	  NSNAME SCOPE
	| gcc_namespace_qualifier NSNAME SCOPE
	;

gcc_any_id:
	  gcc_unqualified_id
	| gcc_qualified_id
	| gcc_global_scope gcc_qualified_id
	| gcc_global_scope gcc_unqualified_id
	;

gcc_extern_lang_string:
	EXTERN_LANG_STRING
	| gcc_extern_lang_string EXTERN_LANG_STRING
	;

gcc_template_header:
	  TEMPLATE '<' gcc_template_parm_list '>'
	| TEMPLATE '<' '>'
	;

gcc_template_parm_list:
	  gcc_template_parm
	| gcc_template_parm_list ',' gcc_template_parm
	;

gcc_maybe_identifier:
	  gcc_identifier
	|	/* empty */
	;		

gcc_template_type_parm:
	  gcc_aggr gcc_maybe_identifier
	| TYPENAME_KEYWORD gcc_maybe_identifier
	;

gcc_template_template_parm:
	  gcc_template_header gcc_aggr gcc_maybe_identifier
	;

gcc_template_parm:
	  gcc_template_type_parm
	| gcc_template_type_parm '=' gcc_type_id
	| gcc_parm
	| gcc_parm '=' gcc_expr_no_commas  %prec ARITHCOMPARE
	| gcc_template_template_parm
	| gcc_template_template_parm '=' gcc_template_arg
	;

gcc_template_def:
	  gcc_template_header gcc_template_extdef
	| gcc_template_header error  %prec EMPTY
	;

gcc_template_extdef:
	  gcc_fndef gcc_eat_saved_input
	| gcc_template_datadef
	| gcc_template_def
	| gcc_extern_lang_string .gcc_hush_warning gcc_fndef .gcc_warning_ok gcc_eat_saved_input
	| gcc_extern_lang_string .gcc_hush_warning gcc_template_datadef .gcc_warning_ok
	| gcc_extension gcc_template_extdef
	;

gcc_template_datadef:
	  gcc_nomods_initdecls ';'
	| gcc_declmods gcc_notype_initdecls ';'
	| gcc_typed_declspecs gcc_initdecls ';'
	| gcc_structsp ';'
	;

gcc_datadef:
	  gcc_nomods_initdecls ';'
	| gcc_declmods gcc_notype_initdecls ';'
	| gcc_typed_declspecs gcc_initdecls ';'
    | gcc_declmods ';'
	| gcc_explicit_instantiation ';'
	| gcc_typed_declspecs ';'
	| error ';'
	| error '}'
	| ';'
	;

gcc_ctor_initializer_opt:
	  gcc_nodecls
	| gcc_base_init
	;

gcc_maybe_return_init:
	  /* empty */
	| gcc_return_init
	| gcc_return_init ';'
	;

gcc_eat_saved_input:
	  /* empty */
	| END_OF_SAVED_INPUT
	;

gcc_fndef:
	  gcc_fn.gcc_def1 gcc_maybe_return_init gcc_ctor_initializer_opt gcc_compstmt_or_error
	| gcc_fn.gcc_def1 gcc_maybe_return_init gcc_function_try_block
	| gcc_fn.gcc_def1 gcc_maybe_return_init error
	;

gcc_constructor_declarator:
	  gcc_nested_name_specifier SELFNAME '(' gcc_parmlist ')' gcc_cv_qualifiers gcc_exception_specification_opt
	| gcc_nested_name_specifier SELFNAME LEFT_RIGHT gcc_cv_qualifiers gcc_exception_specification_opt
	| gcc_global_scope gcc_nested_name_specifier SELFNAME '(' gcc_parmlist ')' gcc_cv_qualifiers gcc_exception_specification_opt
	| gcc_global_scope gcc_nested_name_specifier SELFNAME LEFT_RIGHT gcc_cv_qualifiers gcc_exception_specification_opt
	| gcc_nested_name_specifier gcc_self_template_type '(' gcc_parmlist ')' gcc_cv_qualifiers gcc_exception_specification_opt
	| gcc_nested_name_specifier gcc_self_template_type LEFT_RIGHT gcc_cv_qualifiers gcc_exception_specification_opt
	| gcc_global_scope gcc_nested_name_specifier gcc_self_template_type '(' gcc_parmlist ')' gcc_cv_qualifiers gcc_exception_specification_opt
	| gcc_global_scope gcc_nested_name_specifier gcc_self_template_type LEFT_RIGHT gcc_cv_qualifiers gcc_exception_specification_opt
	;

gcc_fn.gcc_def1:
	  gcc_typed_declspecs gcc_declarator
	| gcc_declmods gcc_notype_declarator
	| gcc_notype_declarator
	| gcc_declmods gcc_constructor_declarator
	| gcc_constructor_declarator
	;

gcc_component_constructor_declarator:
	  SELFNAME '(' gcc_parmlist ')' gcc_cv_qualifiers gcc_exception_specification_opt
	| SELFNAME LEFT_RIGHT gcc_cv_qualifiers gcc_exception_specification_opt
	| gcc_self_template_type '(' gcc_parmlist ')' gcc_cv_qualifiers gcc_exception_specification_opt
	| gcc_self_template_type LEFT_RIGHT gcc_cv_qualifiers gcc_exception_specification_opt
	;

gcc_fn.gcc_def2:
	  gcc_declmods gcc_component_constructor_declarator
	| gcc_component_constructor_declarator
	| gcc_typed_declspecs gcc_declarator
	| gcc_declmods gcc_notype_declarator
	| gcc_notype_declarator
	| gcc_declmods gcc_constructor_declarator
	| gcc_constructor_declarator
	;

gcc_return_id:
	  RETURN_KEYWORD IDENTIFIER
	;

gcc_return_init:
	  gcc_return_id gcc_maybe_init
	| gcc_return_id '(' gcc_nonnull_exprlist ')'
	| gcc_return_id LEFT_RIGHT
	;

gcc_base_init:
	  ':' .gcc_set_base_init gcc_member_init_list
	;

.gcc_set_base_init:
	  /* empty */
	;

gcc_member_init_list:
	  /* empty */
	| gcc_member_init
	| gcc_member_init_list ',' gcc_member_init
	| gcc_member_init_list error
	;

gcc_member_init:
	  '(' gcc_nonnull_exprlist ')'
	| LEFT_RIGHT
	| gcc_notype_identifier '(' gcc_nonnull_exprlist ')'
	| gcc_notype_identifier LEFT_RIGHT
	| gcc_nonnested_type '(' gcc_nonnull_exprlist ')'
	| gcc_nonnested_type LEFT_RIGHT
	| gcc_typename_sub '(' gcc_nonnull_exprlist ')'
	| gcc_typename_sub LEFT_RIGHT
	;

gcc_identifier:
	  IDENTIFIER
	| TYPENAME
	| SELFNAME
	| PTYPENAME
	| NSNAME
	;

gcc_notype_identifier:
	  IDENTIFIER
	| PTYPENAME 
	| NSNAME  %prec EMPTY
	;

gcc_identifier_defn:
	  IDENTIFIER_DEFN
	| TYPENAME_DEFN
	| PTYPENAME_DEFN
	;

gcc_explicit_instantiation:
	  TEMPLATE gcc_begin_explicit_instantiation gcc_typespec ';' gcc_end_explicit_instantiation
	| TEMPLATE gcc_begin_explicit_instantiation gcc_typed_declspecs gcc_declarator gcc_end_explicit_instantiation
	| TEMPLATE gcc_begin_explicit_instantiation gcc_notype_declarator gcc_end_explicit_instantiation
	| TEMPLATE gcc_begin_explicit_instantiation gcc_constructor_declarator gcc_end_explicit_instantiation
	| SCSPEC TEMPLATE gcc_begin_explicit_instantiation gcc_typespec ';' gcc_end_explicit_instantiation
	| SCSPEC TEMPLATE gcc_begin_explicit_instantiation gcc_typed_declspecs gcc_declarator gcc_end_explicit_instantiation
	| SCSPEC TEMPLATE gcc_begin_explicit_instantiation gcc_notype_declarator gcc_end_explicit_instantiation
	| SCSPEC TEMPLATE gcc_begin_explicit_instantiation gcc_constructor_declarator gcc_end_explicit_instantiation
	;

gcc_begin_explicit_instantiation: 

gcc_end_explicit_instantiation: 

gcc_template_type:
	  PTYPENAME '<' gcc_template_arg_list_opt gcc_template_close_bracket .gcc_finish_template_type
	| TYPENAME  '<' gcc_template_arg_list_opt gcc_template_close_bracket .gcc_finish_template_type
	| gcc_self_template_type
	;

gcc_apparent_template_type:
	  gcc_template_type
	| gcc_identifier '<' gcc_template_arg_list_opt '>' .gcc_finish_template_type
	;

gcc_self_template_type:
	  SELFNAME  '<' gcc_template_arg_list_opt gcc_template_close_bracket .gcc_finish_template_type
	;

.gcc_finish_template_type:

gcc_template_close_bracket:
	  '>'
	| RSHIFT 
	;

gcc_template_arg_list_opt:
	 /* empty */
	| gcc_template_arg_list
	;

gcc_template_arg_list:
        gcc_template_arg
	| gcc_template_arg_list ',' gcc_template_arg
	;

gcc_template_arg:
	  gcc_type_id
	| PTYPENAME
	| gcc_expr_no_commas  %prec ARITHCOMPARE
	;

gcc_unop:
	  '-'
	| '+'
	| PLUSPLUS
	| MINUSMINUS
	| '!'
	;

gcc_expr:
	  gcc_nontrivial_exprlist
	| gcc_expr_no_commas
	;

gcc_paren_expr_or_null:
	LEFT_RIGHT
	| '(' gcc_expr ')'
	;

gcc_paren_cond_or_null:
	LEFT_RIGHT
	| '(' gcc_condition ')'
	;

gcc_xcond:
	  /* empty */
	| gcc_condition
	| error
	;

gcc_condition:
	  gcc_type_specifier_seq gcc_declarator gcc_maybeasm gcc_maybe_attribute '=' gcc_init
	| gcc_expr
	;

gcc_compstmtend:
	  '}'
	| gcc_maybe_label_decls gcc_stmts '}'
	| gcc_maybe_label_decls gcc_stmts error '}'
	| gcc_maybe_label_decls error '}'
	;

gcc_already_scoped_stmt:
	  '{'
	  gcc_compstmtend
	| gcc_simple_stmt
	;

gcc_nontrivial_exprlist:
	  gcc_expr_no_commas ',' gcc_expr_no_commas
	| gcc_expr_no_commas ',' error
	| gcc_nontrivial_exprlist ',' gcc_expr_no_commas
	| gcc_nontrivial_exprlist ',' error
	;

gcc_nonnull_exprlist:
	  gcc_expr_no_commas
	| gcc_nontrivial_exprlist
	;

gcc_unary_expr:
	  gcc_primary  %prec UNARY
	| gcc_extension gcc_cast_expr  	  %prec UNARY
	| '*' gcc_cast_expr   %prec UNARY
	| '&' gcc_cast_expr   %prec UNARY
	| '~' gcc_cast_expr
	| gcc_unop gcc_cast_expr  %prec UNARY
	| ANDAND gcc_identifier
	| SIZEOF gcc_unary_expr  %prec UNARY
	| SIZEOF '(' gcc_type_id ')'  %prec HYPERUNARY
	| ALIGNOF gcc_unary_expr  %prec UNARY
	| ALIGNOF '(' gcc_type_id ')'  %prec HYPERUNARY
	| gcc_new gcc_new_type_id  %prec EMPTY
	| gcc_new gcc_new_type_id gcc_new_initializer
	| gcc_new gcc_new_placement gcc_new_type_id  %prec EMPTY
	| gcc_new gcc_new_placement gcc_new_type_id gcc_new_initializer
	| gcc_new '(' .gcc_begin_new_placement gcc_type_id .gcc_finish_new_placement %prec EMPTY
	| gcc_new '(' .gcc_begin_new_placement gcc_type_id .gcc_finish_new_placement gcc_new_initializer
	| gcc_new gcc_new_placement '(' .gcc_begin_new_placement gcc_type_id .gcc_finish_new_placement   %prec EMPTY
	| gcc_new gcc_new_placement '(' .gcc_begin_new_placement gcc_type_id .gcc_finish_new_placement  gcc_new_initializer
	| gcc_delete gcc_cast_expr  %prec UNARY
	| gcc_delete '[' ']' gcc_cast_expr  %prec UNARY
	| gcc_delete '[' gcc_expr ']' gcc_cast_expr  %prec UNARY
	| REALPART gcc_cast_expr %prec UNARY
	| IMAGPART gcc_cast_expr %prec UNARY
	;

.gcc_finish_new_placement:
	  ')'

.gcc_begin_new_placement:

gcc_new_placement:
	  '(' .gcc_begin_new_placement gcc_nonnull_exprlist ')'
	| '{' .gcc_begin_new_placement gcc_nonnull_exprlist '}'
	;

gcc_new_initializer:
	  '(' gcc_nonnull_exprlist ')'
	| LEFT_RIGHT
	| '(' gcc_typespec ')'
	| '=' gcc_init
	;

gcc_regcast_or_absdcl:
	  '(' gcc_type_id ')'  %prec EMPTY
	| gcc_regcast_or_absdcl '(' gcc_type_id ')'  %prec EMPTY
	;

gcc_cast_expr:
	  gcc_unary_expr
	| gcc_regcast_or_absdcl gcc_unary_expr  %prec UNARY
	| gcc_regcast_or_absdcl '{' gcc_initlist gcc_maybecomma '}'  %prec UNARY
	;

gcc_expr_no_commas:
	  gcc_cast_expr
	| gcc_expr_no_commas POINTSAT_STAR gcc_expr_no_commas
	| gcc_expr_no_commas DOT_STAR gcc_expr_no_commas
	| gcc_expr_no_commas '+' gcc_expr_no_commas
	| gcc_expr_no_commas '-' gcc_expr_no_commas
	| gcc_expr_no_commas '*' gcc_expr_no_commas
	| gcc_expr_no_commas '/' gcc_expr_no_commas
	| gcc_expr_no_commas '%' gcc_expr_no_commas
	| gcc_expr_no_commas LSHIFT gcc_expr_no_commas
	| gcc_expr_no_commas RSHIFT gcc_expr_no_commas
	| gcc_expr_no_commas ARITHCOMPARE gcc_expr_no_commas
	| gcc_expr_no_commas '<' gcc_expr_no_commas
	| gcc_expr_no_commas '>' gcc_expr_no_commas
	| gcc_expr_no_commas EQCOMPARE gcc_expr_no_commas
	| gcc_expr_no_commas MIN_MAX gcc_expr_no_commas
	| gcc_expr_no_commas '&' gcc_expr_no_commas
	| gcc_expr_no_commas '|' gcc_expr_no_commas
	| gcc_expr_no_commas '^' gcc_expr_no_commas
	| gcc_expr_no_commas ANDAND gcc_expr_no_commas
	| gcc_expr_no_commas OROR gcc_expr_no_commas
	| gcc_expr_no_commas '?' gcc_xexpr ':' gcc_expr_no_commas
	| gcc_expr_no_commas '=' gcc_expr_no_commas
	| gcc_expr_no_commas ASSIGN gcc_expr_no_commas
	| THROW
	| THROW gcc_expr_no_commas
	;

gcc_notype_unqualified_id:
	  '~' gcc_see_typename gcc_identifier
	| '~' gcc_see_typename gcc_template_type
    | gcc_template_id
	| gcc_operator_name
	| IDENTIFIER
	| PTYPENAME
	| NSNAME  %prec EMPTY
	;

gcc_do_id:

gcc_template_id:
      PFUNCNAME '<' gcc_do_id gcc_template_arg_list_opt gcc_template_close_bracket 
    | gcc_operator_name '<' gcc_do_id gcc_template_arg_list_opt gcc_template_close_bracket
	;

gcc_object_template_id:
      TEMPLATE gcc_identifier '<' gcc_template_arg_list_opt gcc_template_close_bracket
    | TEMPLATE PFUNCNAME '<' gcc_template_arg_list_opt gcc_template_close_bracket
    | TEMPLATE gcc_operator_name '<' gcc_template_arg_list_opt gcc_template_close_bracket
    ;

gcc_unqualified_id:
	  gcc_notype_unqualified_id
	| TYPENAME
	| SELFNAME
	;

gcc_expr_or_declarator_intern:
	  gcc_expr_or_declarator
	| gcc_attributes gcc_expr_or_declarator
	;

gcc_expr_or_declarator:
	  gcc_notype_unqualified_id
	| '*' gcc_expr_or_declarator_intern  %prec UNARY
	| '&' gcc_expr_or_declarator_intern  %prec UNARY
	| '(' gcc_expr_or_declarator_intern ')'
	;

gcc_notype_template_declarator:
	  IDENTIFIER '<' gcc_template_arg_list_opt gcc_template_close_bracket
	| NSNAME '<' gcc_template_arg_list gcc_template_close_bracket
	;
gcc_direct_notype_declarator:
	  gcc_complex_direct_notype_declarator
	| gcc_notype_unqualified_id %prec '('
	| gcc_notype_template_declarator
	| '(' gcc_expr_or_declarator_intern ')'
	;

gcc_primary:
	  gcc_notype_unqualified_id
	| CONSTANT
	| gcc_boolean.gcc_literal
	| gcc_string
	| '(' gcc_expr ')'
	| '(' gcc_expr_or_declarator_intern ')'
	| '(' error ')'
	| '(' gcc_compstmt ')'
    | gcc_notype_unqualified_id '(' gcc_nonnull_exprlist ')'
    | gcc_notype_unqualified_id LEFT_RIGHT
	| gcc_primary '(' gcc_nonnull_exprlist ')'
	| gcc_primary LEFT_RIGHT
	| gcc_primary '[' gcc_expr ']'
	| gcc_primary PLUSPLUS
	| gcc_primary MINUSMINUS
	| THIS
	| CV_QUALIFIER '(' gcc_nonnull_exprlist ')'
	| gcc_functional_cast
	| DYNAMIC_CAST '<' gcc_type_id '>' '(' gcc_expr ')'
	| STATIC_CAST '<' gcc_type_id '>' '(' gcc_expr ')'
	| REINTERPRET_CAST '<' gcc_type_id '>' '(' gcc_expr ')'
	| CONST_CAST '<' gcc_type_id '>' '(' gcc_expr ')'
	| TYPEID '(' gcc_expr ')'
	| TYPEID '(' gcc_type_id ')'
	| gcc_global_scope IDENTIFIER
	| gcc_global_scope gcc_template_id
	| gcc_global_scope gcc_operator_name
	| gcc_overqualified_id  %prec HYPERUNARY
	| gcc_overqualified_id '(' gcc_nonnull_exprlist ')'
	| gcc_overqualified_id LEFT_RIGHT
    | gcc_object gcc_object_template_id %prec UNARY
    | gcc_object gcc_object_template_id '(' gcc_nonnull_exprlist ')'
	| gcc_object gcc_object_template_id LEFT_RIGHT
	| gcc_object gcc_unqualified_id  %prec UNARY
	| gcc_object gcc_overqualified_id  %prec UNARY
	| gcc_object gcc_unqualified_id '(' gcc_nonnull_exprlist ')'
	| gcc_object gcc_unqualified_id LEFT_RIGHT
	| gcc_object gcc_overqualified_id '(' gcc_nonnull_exprlist ')'
	| gcc_object gcc_overqualified_id LEFT_RIGHT
	| gcc_object '~' TYPESPEC LEFT_RIGHT
	| gcc_object TYPESPEC SCOPE '~' TYPESPEC LEFT_RIGHT
	| gcc_object error
	;

gcc_new:
	  NEW
	| gcc_global_scope NEW
	;

gcc_delete:
	  DELETE
	| gcc_global_scope gcc_delete
	;

gcc_boolean.gcc_literal:
	  CXX_TRUE
	| CXX_FALSE
	;

gcc_string:
	  STRING
	| gcc_string STRING
	;

gcc_nodecls:
	  /* empty */
	;

gcc_object:
	  gcc_primary '.'
	| gcc_primary POINTSAT
	;

gcc_decl:
	  gcc_typespec gcc_initdecls ';'
	| gcc_typed_declspecs gcc_initdecls ';'
	| gcc_declmods gcc_notype_initdecls ';'
	| gcc_typed_declspecs ';'
	| gcc_declmods ';'
	| gcc_extension gcc_decl
	;

gcc_declarator:
	  gcc_after_type_declarator  %prec EMPTY
	| gcc_notype_declarator  %prec EMPTY
	;

gcc_fcast_or_absdcl:
	  LEFT_RIGHT  %prec EMPTY
	| gcc_fcast_or_absdcl LEFT_RIGHT  %prec EMPTY
	;

gcc_type_id:
	  gcc_typed_typespecs gcc_absdcl
	| gcc_nonempty_cv_qualifiers gcc_absdcl
	| gcc_typespec gcc_absdcl
	| gcc_typed_typespecs  %prec EMPTY
	| gcc_nonempty_cv_qualifiers  %prec EMPTY
	;

gcc_typed_declspecs:
	  gcc_typed_typespecs  %prec EMPTY
	| gcc_typed_declspecs1
	;

gcc_typed_declspecs1:
	  gcc_declmods gcc_typespec
	| gcc_typespec gcc_reserved_declspecs  %prec HYPERUNARY
	| gcc_typespec gcc_reserved_typespecquals gcc_reserved_declspecs
	| gcc_declmods gcc_typespec gcc_reserved_declspecs
	| gcc_declmods gcc_typespec gcc_reserved_typespecquals
	| gcc_declmods gcc_typespec gcc_reserved_typespecquals gcc_reserved_declspecs
	;

gcc_reserved_declspecs:
	  SCSPEC
	| gcc_reserved_declspecs gcc_typespecqual_reserved
	| gcc_reserved_declspecs SCSPEC
	| gcc_reserved_declspecs gcc_attributes
	| gcc_attributes
	;

gcc_declmods:
	  gcc_nonempty_cv_qualifiers  %prec EMPTY
	| SCSPEC
	| gcc_declmods CV_QUALIFIER
	| gcc_declmods SCSPEC
	| gcc_declmods gcc_attributes
	| gcc_attributes  %prec EMPTY
	;

gcc_typed_typespecs:
	  gcc_typespec  %prec EMPTY
	| gcc_nonempty_cv_qualifiers gcc_typespec
	| gcc_typespec gcc_reserved_typespecquals
	| gcc_nonempty_cv_qualifiers gcc_typespec gcc_reserved_typespecquals
	;

gcc_reserved_typespecquals:
	  gcc_typespecqual_reserved
	| gcc_reserved_typespecquals gcc_typespecqual_reserved
	;

gcc_typespec:
	  gcc_structsp
	| TYPESPEC  %prec EMPTY
	| gcc_complete_type_name
	| TYPEOF '(' gcc_expr ')'
	| TYPEOF '(' gcc_type_id ')'
	| SIGOF '(' gcc_expr ')'
	| SIGOF '(' gcc_type_id ')'
	;

gcc_typespecqual_reserved:
	  TYPESPEC
	| CV_QUALIFIER
	| gcc_structsp
	;

gcc_initdecls:
	  gcc_initdcl0
	| gcc_initdecls ',' gcc_initdcl
	;

gcc_notype_initdecls:
	  gcc_notype_initdcl0
	| gcc_notype_initdecls ',' gcc_initdcl
	;

gcc_nomods_initdecls:
	  gcc_nomods_initdcl0
	| gcc_nomods_initdecls ',' gcc_initdcl
	;

gcc_maybeasm:
	  /* empty */
	| gcc_asm_keyword '(' gcc_string ')'
	;

gcc_initdcl:
	  gcc_declarator gcc_maybeasm gcc_maybe_attribute '=' gcc_init
	| gcc_declarator gcc_maybeasm gcc_maybe_attribute
	;

gcc_initdcl0_innards:
	  gcc_maybe_attribute '=' gcc_init
	| gcc_maybe_attribute
  	;

gcc_initdcl0:
	  gcc_declarator gcc_maybeasm gcc_initdcl0_innards
	;

gcc_notype_initdcl0:
      gcc_notype_declarator gcc_maybeasm gcc_initdcl0_innards
    ;

gcc_nomods_initdcl0:
      gcc_notype_declarator gcc_maybeasm gcc_initdcl0_innards 
	| gcc_constructor_declarator gcc_maybeasm gcc_maybe_attribute
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
      ATTRIBUTE '(' '(' gcc_attribute_list ')' ')'
	;

gcc_attribute_list:
      gcc_attrib
	| gcc_attribute_list ',' gcc_attrib
	;
gcc_attrib:
	  /* empty */
	| gcc_any_word
	| gcc_any_word '(' IDENTIFIER ')'
	| gcc_any_word '(' IDENTIFIER ',' gcc_nonnull_exprlist ')'
	| gcc_any_word '(' gcc_nonnull_exprlist ')'
	;

gcc_any_word:
	  gcc_identifier
	| SCSPEC
	| TYPESPEC
	| CV_QUALIFIER
	;

gcc_identifiers_or_typenames:
	  gcc_identifier
	| gcc_identifiers_or_typenames ',' gcc_identifier
	;

gcc_maybe_init:
	  /* empty */  %prec EMPTY
	| '=' gcc_init

gcc_init:
	  gcc_expr_no_commas  %prec '='
	| '{' '}'
	| '{' gcc_initlist '}'
	| '{' gcc_initlist ',' '}'
	| error
	;

gcc_initlist:
	  gcc_init
	| gcc_initlist ',' gcc_init
	| '[' gcc_expr_no_commas ']' gcc_init
	| gcc_identifier ':' gcc_init
	| gcc_initlist ',' gcc_identifier ':' gcc_init
	;

gcc_fn.gcc_defpen:
	PRE_PARSED_FUNCTION_DECL

gcc_pending_inline:
	  gcc_fn.gcc_defpen gcc_maybe_return_init gcc_ctor_initializer_opt gcc_compstmt_or_error
	| gcc_fn.gcc_defpen gcc_maybe_return_init gcc_function_try_block
	| gcc_fn.gcc_defpen gcc_maybe_return_init error
	;

gcc_pending_inlines:
	/* empty */
	| gcc_pending_inlines gcc_pending_inline gcc_eat_saved_input
	;

gcc_defarg_again:
	DEFARG_MARKER gcc_expr_no_commas END_OF_SAVED_INPUT
	| DEFARG_MARKER error END_OF_SAVED_INPUT

gcc_pending_defargs:
	  /* empty */ %prec EMPTY
	| gcc_pending_defargs gcc_defarg_again
	| gcc_pending_defargs error
	;

gcc_structsp:
	  ENUM gcc_identifier '{'
	  gcc_enumlist gcc_maybecomma_warn '}'
	| ENUM gcc_identifier '{' '}'
	| ENUM '{'
	  gcc_enumlist gcc_maybecomma_warn '}'
	| ENUM '{' '}'
	| ENUM gcc_identifier
	| ENUM gcc_complex_type_name
	| TYPENAME_KEYWORD gcc_typename_sub
	| gcc_named_class_head '{' gcc_components '}' gcc_maybe_attribute gcc_pending_defargs gcc_pending_inlines
	| gcc_named_class_head  %prec EMPTY
	;

gcc_maybecomma:
	  /* empty */
	| ','
	;

gcc_maybecomma_warn:
	  /* empty */
	| ','
	;

gcc_aggr:
	  AGGR
	| gcc_aggr SCSPEC
	| gcc_aggr TYPESPEC
	| gcc_aggr CV_QUALIFIER
	| gcc_aggr AGGR
	| gcc_aggr gcc_attributes
	;

gcc_named_class_head_sans_basetype:
	  gcc_aggr gcc_identifier
	;

gcc_named_class_head_sans_basetype_defn:
	  gcc_aggr gcc_identifier_defn  %prec EMPTY
	| gcc_named_class_head_sans_basetype '{'
	| gcc_named_class_head_sans_basetype ':'
	;

gcc_named_complex_class_head_sans_basetype:
	  gcc_aggr gcc_nested_name_specifier gcc_identifier
	| gcc_aggr gcc_global_scope gcc_nested_name_specifier gcc_identifier
	| gcc_aggr gcc_global_scope gcc_identifier
	| gcc_aggr gcc_apparent_template_type
	| gcc_aggr gcc_nested_name_specifier gcc_apparent_template_type
	;

gcc_named_class_head:
	  gcc_named_class_head_sans_basetype  %prec EMPTY
	| gcc_named_class_head_sans_basetype_defn gcc_maybe_base_class_list  %prec EMPTY
	| gcc_named_complex_class_head_sans_basetype gcc_maybe_base_class_list
	;

gcc_maybe_base_class_list:
	/* empty */
	| gcc_base_class_list
	;

gcc_base_class_list:
	gcc_base_class
	| gcc_base_class_list COMMA gcc_base_class
	;

gcc_base_class:
	ID
	;

gcc_unnamed_class_head:
	  gcc_aggr '{'
	| gcc_fn.gcc_def2 ':' /* gcc_base_init gcc_compstmt */
	| gcc_fn.gcc_def2 TRY /* gcc_base_init gcc_compstmt */
	| gcc_fn.gcc_def2 RETURN_KEYWORD /* gcc_base_init gcc_compstmt */
	| gcc_fn.gcc_def2 '{' /* gcc_nodecls gcc_compstmt */
	| ';'
	| gcc_extension gcc_component_declarator
    | gcc_template_header gcc_component_declarator
	| gcc_template_header gcc_typed_declspecs ';'
	;

gcc_component_decl_1:
	  gcc_typed_declspecs gcc_components
	| gcc_declmods gcc_notype_components
	| gcc_notype_declarator gcc_maybeasm gcc_maybe_attribute gcc_maybe_init
	| gcc_constructor_declarator gcc_maybeasm gcc_maybe_attribute gcc_maybe_init
	| ':' gcc_expr_no_commas
	| error
	| gcc_declmods gcc_component_constructor_declarator gcc_maybeasm gcc_maybe_attribute gcc_maybe_init
	| gcc_component_constructor_declarator gcc_maybeasm gcc_maybe_attribute gcc_maybe_init
	| gcc_using_decl

gcc_components:
	  /* empty: gcc_possibly gcc_anonymous */
	| gcc_component_declarator0
	| gcc_components ',' gcc_component_declarator
	;

gcc_notype_components:
	  /* empty: gcc_possibly gcc_anonymous */
	| gcc_notype_component_declarator0
	| gcc_notype_components ',' gcc_notype_component_declarator
	;

gcc_component_declarator0:
	  gcc_after_type_component_declarator0
	| gcc_notype_component_declarator0
	;

gcc_component_declarator:
	  gcc_after_type_component_declarator
	| gcc_notype_component_declarator
	;

gcc_after_type_component_declarator0:
	  gcc_after_type_declarator gcc_maybeasm gcc_maybe_attribute gcc_maybe_init
	| TYPENAME ':' gcc_expr_no_commas gcc_maybe_attribute
	;

gcc_notype_component_declarator0:
	  gcc_notype_declarator gcc_maybeasm gcc_maybe_attribute gcc_maybe_init
	| gcc_constructor_declarator gcc_maybeasm gcc_maybe_attribute gcc_maybe_init
	| IDENTIFIER ':' gcc_expr_no_commas gcc_maybe_attribute
	| ':' gcc_expr_no_commas gcc_maybe_attribute
	;

gcc_after_type_component_declarator:
	  gcc_after_type_declarator gcc_maybeasm gcc_maybe_attribute gcc_maybe_init
	| TYPENAME ':' gcc_expr_no_commas gcc_maybe_attribute
	;

gcc_notype_component_declarator:
	  gcc_notype_declarator gcc_maybeasm gcc_maybe_attribute gcc_maybe_init
	| IDENTIFIER ':' gcc_expr_no_commas gcc_maybe_attribute
	| ':' gcc_expr_no_commas gcc_maybe_attribute
	;

gcc_enumlist:
	  gcc_enumerator
	| gcc_enumlist ',' gcc_enumerator
	;

gcc_enumerator:
	  gcc_identifier
	| gcc_identifier '=' gcc_expr_no_commas
	;

gcc_new_type_id:
	  gcc_type_specifier_seq gcc_new_declarator
	| gcc_type_specifier_seq  %prec EMPTY
	| '(' .gcc_begin_new_placement gcc_type_id .gcc_finish_new_placement '[' gcc_expr ']'
	;

gcc_cv_qualifiers:
	  /* empty */  %prec EMPTY
	| gcc_cv_qualifiers CV_QUALIFIER
	;

gcc_nonempty_cv_qualifiers:
	  CV_QUALIFIER
	| gcc_nonempty_cv_qualifiers CV_QUALIFIER
	;

gcc_suspend_mom:
	  /* empty */
	;

gcc_nonmomentary_expr:
	  gcc_suspend_mom gcc_expr
	;

gcc_maybe_parmlist:
	  gcc_suspend_mom '(' gcc_nonnull_exprlist ')'
	| gcc_suspend_mom '(' gcc_parmlist ')'
	| gcc_suspend_mom LEFT_RIGHT
	| gcc_suspend_mom '(' error ')'
	;

gcc_after_type_declarator_intern:
	  gcc_after_type_declarator
	| gcc_attributes gcc_after_type_declarator
	;

gcc_after_type_declarator:
	  '*' gcc_nonempty_cv_qualifiers gcc_after_type_declarator_intern  %prec UNARY
	| '&' gcc_nonempty_cv_qualifiers gcc_after_type_declarator_intern  %prec UNARY
	| '*' gcc_after_type_declarator_intern  %prec UNARY
	| '&' gcc_after_type_declarator_intern  %prec UNARY
	| gcc_ptr_to_mem gcc_cv_qualifiers gcc_after_type_declarator_intern
	| gcc_direct_after_type_declarator
	;

gcc_direct_after_type_declarator:
	  gcc_direct_after_type_declarator gcc_maybe_parmlist gcc_cv_qualifiers gcc_exception_specification_opt  %prec '.'
	| gcc_direct_after_type_declarator '[' gcc_nonmomentary_expr ']'
	| gcc_direct_after_type_declarator '[' ']'
	| '(' gcc_after_type_declarator_intern ')'
	| gcc_nested_name_specifier gcc_type_name  %prec EMPTY
	| gcc_type_name  %prec EMPTY
	;

gcc_nonnested_type:
	  gcc_type_name  %prec EMPTY
	| gcc_global_scope gcc_type_name
	;

gcc_complete_type_name:
	  gcc_nonnested_type
	| gcc_nested_type
	| gcc_global_scope gcc_nested_type
	;

gcc_nested_type:
	  gcc_nested_name_specifier gcc_type_name  %prec EMPTY
	;

gcc_notype_declarator_intern:
	  gcc_notype_declarator
	| gcc_attributes gcc_notype_declarator
	;
gcc_notype_declarator:
	  '*' gcc_nonempty_cv_qualifiers gcc_notype_declarator_intern  %prec UNARY
	| '&' gcc_nonempty_cv_qualifiers gcc_notype_declarator_intern  %prec UNARY
	| '*' gcc_notype_declarator_intern  %prec UNARY
	| '&' gcc_notype_declarator_intern  %prec UNARY
	| gcc_ptr_to_mem gcc_cv_qualifiers gcc_notype_declarator_intern
	| gcc_direct_notype_declarator
	;

gcc_complex_notype_declarator:
	  '*' gcc_nonempty_cv_qualifiers gcc_notype_declarator_intern  %prec UNARY
	| '&' gcc_nonempty_cv_qualifiers gcc_notype_declarator_intern  %prec UNARY
	| '*' gcc_complex_notype_declarator  %prec UNARY
	| '&' gcc_complex_notype_declarator  %prec UNARY
	| gcc_ptr_to_mem gcc_cv_qualifiers gcc_notype_declarator_intern
	| gcc_complex_direct_notype_declarator
	;

gcc_complex_direct_notype_declarator:
	  gcc_direct_notype_declarator gcc_maybe_parmlist gcc_cv_qualifiers gcc_exception_specification_opt  %prec '.'
	| '(' gcc_complex_notype_declarator ')'
	| gcc_direct_notype_declarator '[' gcc_nonmomentary_expr ']'
	| gcc_direct_notype_declarator '[' ']'
	| gcc_notype_qualified_id
    | gcc_nested_name_specifier gcc_notype_template_declarator
	;

gcc_qualified_id:
	  gcc_nested_name_specifier gcc_unqualified_id
    | gcc_nested_name_specifier gcc_object_template_id
	;

gcc_notype_qualified_id:
	  gcc_nested_name_specifier gcc_notype_unqualified_id
    | gcc_nested_name_specifier gcc_object_template_id
	;

gcc_overqualified_id:
	  gcc_notype_qualified_id
	| gcc_global_scope gcc_notype_qualified_id
	;

gcc_functional_cast:
	  gcc_typespec '(' gcc_nonnull_exprlist ')'
	| gcc_typespec '(' gcc_expr_or_declarator_intern ')'
	| gcc_typespec gcc_fcast_or_absdcl  %prec EMPTY
	;
gcc_type_name:
	  TYPENAME
	| SELFNAME
	| gcc_template_type  %prec EMPTY
	;

gcc_nested_name_specifier:
	  gcc_nested_name_specifier_1
	| gcc_nested_name_specifier gcc_nested_name_specifier_1
	| gcc_nested_name_specifier TEMPLATE gcc_explicit_template_type SCOPE
	;

gcc_nested_name_specifier_1:
	  TYPENAME SCOPE
	| SELFNAME SCOPE
	| NSNAME SCOPE
	| gcc_template_type SCOPE
	;

gcc_typename_sub:
	  gcc_typename_sub0
	| gcc_global_scope gcc_typename_sub0
	;

gcc_typename_sub0:
	  gcc_typename_sub1 gcc_identifier %prec EMPTY
	| gcc_typename_sub1 gcc_template_type %prec EMPTY
	| gcc_typename_sub1 gcc_explicit_template_type %prec EMPTY
	| gcc_typename_sub1 TEMPLATE gcc_explicit_template_type %prec EMPTY
	;

gcc_typename_sub1:
	  gcc_typename_sub2
	| gcc_typename_sub1 gcc_typename_sub2
	| gcc_typename_sub1 gcc_explicit_template_type SCOPE
	| gcc_typename_sub1 TEMPLATE gcc_explicit_template_type SCOPE
	;

gcc_typename_sub2:
	  TYPENAME SCOPE
	| SELFNAME SCOPE
	| gcc_template_type SCOPE
	| PTYPENAME SCOPE
	| IDENTIFIER SCOPE
	| NSNAME SCOPE
	;

gcc_explicit_template_type:
	  gcc_identifier '<' gcc_template_arg_list_opt gcc_template_close_bracket
	;

gcc_complex_type_name:
	  gcc_global_scope gcc_type_name
	| gcc_nested_type
	| gcc_global_scope gcc_nested_type
	;

gcc_ptr_to_mem:
	  gcc_nested_name_specifier '*'
	| gcc_global_scope gcc_nested_name_specifier '*'
	;

gcc_global_scope:
	  SCOPE
	;

gcc_new_declarator:
	  '*' gcc_cv_qualifiers gcc_new_declarator
	| '*' gcc_cv_qualifiers  %prec EMPTY
	| '&' gcc_cv_qualifiers gcc_new_declarator  %prec EMPTY
	| '&' gcc_cv_qualifiers  %prec EMPTY
	| gcc_ptr_to_mem gcc_cv_qualifiers  %prec EMPTY
	| gcc_ptr_to_mem gcc_cv_qualifiers gcc_new_declarator
	| gcc_direct_new_declarator  %prec EMPTY
	;

gcc_direct_new_declarator:
	  '[' gcc_expr ']'
	| gcc_direct_new_declarator '[' gcc_nonmomentary_expr ']'
	;

gcc_absdcl_intern:
	  gcc_absdcl
	| gcc_attributes gcc_absdcl
	;

gcc_absdcl:
	  '*' gcc_nonempty_cv_qualifiers gcc_absdcl_intern
	| '*' gcc_absdcl_intern
	| '*' gcc_nonempty_cv_qualifiers  %prec EMPTY
	| '*'  %prec EMPTY
	| '&' gcc_nonempty_cv_qualifiers gcc_absdcl_intern
	| '&' gcc_absdcl_intern
	| '&' gcc_nonempty_cv_qualifiers  %prec EMPTY
	| '&'  %prec EMPTY
	| gcc_ptr_to_mem gcc_cv_qualifiers  %prec EMPTY
	| gcc_ptr_to_mem gcc_cv_qualifiers gcc_absdcl_intern
	| gcc_direct_abstract_declarator  %prec EMPTY
	;

gcc_direct_abstract_declarator:
	  '(' gcc_absdcl_intern ')'
	| PAREN_STAR_PAREN
	| gcc_direct_abstract_declarator '(' gcc_parmlist ')' gcc_cv_qualifiers gcc_exception_specification_opt  %prec '.'
	| gcc_direct_abstract_declarator LEFT_RIGHT gcc_cv_qualifiers gcc_exception_specification_opt  %prec '.'
	| gcc_direct_abstract_declarator '[' gcc_nonmomentary_expr ']'  %prec '.'
	| gcc_direct_abstract_declarator '[' ']'  %prec '.'
	| '(' gcc_complex_parmlist ')' gcc_cv_qualifiers gcc_exception_specification_opt  %prec '.'
	| gcc_regcast_or_absdcl gcc_cv_qualifiers gcc_exception_specification_opt  %prec '.'
	| gcc_fcast_or_absdcl gcc_cv_qualifiers gcc_exception_specification_opt  %prec '.'
	| '[' gcc_nonmomentary_expr ']'  %prec '.'
	| '[' ']'  %prec '.'
	;

gcc_stmts:
	  gcc_stmt
	| gcc_errstmt
	| gcc_stmts gcc_stmt
	| gcc_stmts gcc_errstmt
	;

gcc_errstmt:
	  error ';'
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
	  LABEL gcc_identifiers_or_typenames ';'
	;

gcc_compstmt_or_error:
	  gcc_compstmt
	| error gcc_compstmt
	;

gcc_compstmt:
	  '{' gcc_compstmtend 
	;

gcc_simple_if:
	  IF gcc_paren_cond_or_null gcc_implicitly_scoped_stmt
	;

gcc_implicitly_scoped_stmt:
	  gcc_compstmt
	| gcc_simple_stmt 
	;

gcc_stmt:
	  gcc_compstmt
	| gcc_simple_stmt
	;

gcc_simple_stmt:
	  gcc_decl
	| gcc_expr ';'
	| gcc_simple_if ELSE gcc_implicitly_scoped_stmt
	| gcc_simple_if  %prec IF
	| WHILE gcc_paren_cond_or_null gcc_already_scoped_stmt
	| DO gcc_implicitly_scoped_stmt WHILE gcc_paren_expr_or_null ';' 
	| FOR '(' gcc_for.gcc_init.gcc_statement gcc_xcond ';' gcc_xexpr ')' gcc_already_scoped_stmt
	| SWITCH '(' gcc_condition ')' gcc_implicitly_scoped_stmt
	| CASE gcc_expr_no_commas ':' gcc_stmt
	| CASE gcc_expr_no_commas ELLIPSIS gcc_expr_no_commas ':' gcc_stmt
	| DEFAULT ':' gcc_stmt
	| BREAK ';'
	| CONTINUE ';'
	| RETURN_KEYWORD ';'
	| RETURN_KEYWORD gcc_expr ';'
	| gcc_asm_keyword gcc_maybe_cv_qualifier '(' gcc_string ')' ';'
	| gcc_asm_keyword gcc_maybe_cv_qualifier '(' gcc_string ':' gcc_asm_operands ')' ';'
	| gcc_asm_keyword gcc_maybe_cv_qualifier '(' gcc_string ':' gcc_asm_operands ':' gcc_asm_operands ')' ';'
	| gcc_asm_keyword gcc_maybe_cv_qualifier '(' gcc_string ':' gcc_asm_operands ':' gcc_asm_operands ':' gcc_asm_clobbers ')' ';'
	| GOTO '*' gcc_expr ';'
	| GOTO gcc_identifier ';'
	| gcc_label_colon gcc_stmt
	| gcc_label_colon '}'
	| ';'
	| gcc_try_block
	| gcc_using_directive
	| gcc_namespace_using_decl
	| gcc_namespace_alias
	;

gcc_function_try_block:
	  TRY gcc_ctor_initializer_opt gcc_compstmt gcc_handler_seq
	;

gcc_try_block:
	  TRY gcc_compstmt gcc_handler_seq
	;

gcc_handler_seq:
	  gcc_handler
	| gcc_handler_seq gcc_handler
	;

gcc_handler:
	  CATCH gcc_handler_args gcc_compstmt
	;

gcc_type_specifier_seq:
	  gcc_typed_typespecs  %prec EMPTY
	| gcc_nonempty_cv_qualifiers  %prec EMPTY
	;

gcc_handler_args:
	  '(' ELLIPSIS ')'
	| '(' gcc_parm ')'
	;

gcc_label_colon:
	  IDENTIFIER ':'
	| PTYPENAME ':'
	| TYPENAME ':'
	| SELFNAME ':'
	;

gcc_for.gcc_init.gcc_statement:
	  gcc_xexpr ';'
	| gcc_decl
	| '{' gcc_compstmtend
	;

gcc_maybe_cv_qualifier:
	  /* empty */
	| CV_QUALIFIER
	;

gcc_xexpr:
	  /* empty */
	| gcc_expr
	| error
	;

gcc_asm_operands:
	  /* empty */
	| gcc_nonnull_asm_operands
	;

gcc_nonnull_asm_operands:
	  gcc_asm_operand
	| gcc_nonnull_asm_operands ',' gcc_asm_operand
	;

gcc_asm_operand:
	  STRING '(' gcc_expr ')'
	;

gcc_asm_clobbers:
	  STRING
	| gcc_asm_clobbers ',' STRING
	;

gcc_parmlist:
	  /* empty */
	| gcc_complex_parmlist
	| gcc_type_id
	;

gcc_complex_parmlist:
	  gcc_parms
	| gcc_parms_comma ELLIPSIS
	| gcc_parms ELLIPSIS
	| gcc_type_id ELLIPSIS
	| ELLIPSIS
	| gcc_parms ':'
	| gcc_type_id ':'
	;

gcc_defarg:
	  '=' gcc_defarg1
	;

gcc_defarg1:
	  DEFARG
	| gcc_init
	;

gcc_parms:
	  gcc_named_parm
	| gcc_parm gcc_defarg
	| gcc_parms_comma gcc_full_parm
	| gcc_parms_comma gcc_bad_parm
	| gcc_parms_comma gcc_bad_parm '=' gcc_init
	;

gcc_parms_comma:
	  gcc_parms ','
	| gcc_type_id ','
	;

gcc_named_parm:
	  gcc_typed_declspecs1 gcc_declarator
	| gcc_typed_typespecs gcc_declarator
	| gcc_typespec gcc_declarator
	| gcc_typed_declspecs1 gcc_absdcl
	| gcc_typed_declspecs1  %prec EMPTY
	| gcc_declmods gcc_notype_declarator
	;

gcc_full_parm:
	  gcc_parm
	| gcc_parm gcc_defarg
	;

gcc_parm:
	  gcc_named_parm
	| gcc_type_id
	;

gcc_see_typename:
	  /* empty */  %prec EMPTY
	;

gcc_bad_parm:
	  /* empty */ %prec EMPTY
	| gcc_notype_declarator 
	;

gcc_exception_specification_opt:
	  /* empty */  %prec EMPTY
	| THROW '(' gcc_ansi_raise_identifiers  ')'  %prec EMPTY
	| THROW LEFT_RIGHT  %prec EMPTY
	;

gcc_ansi_raise_identifier:
	  gcc_type_id
	;

gcc_ansi_raise_identifiers:
	  gcc_ansi_raise_identifier
	| gcc_ansi_raise_identifiers ',' gcc_ansi_raise_identifier
	;

gcc_conversion_declarator:
	  /* empty */  %prec EMPTY
	| '*' gcc_cv_qualifiers gcc_conversion_declarator 
	| '&' gcc_cv_qualifiers gcc_conversion_declarator
	| gcc_ptr_to_mem gcc_cv_qualifiers gcc_conversion_declarator
	;

gcc_operator:
	  OPERATOR
	;

gcc_operator_name:
	  gcc_operator '*'
	| gcc_operator '/'
	| gcc_operator '%'
	| gcc_operator '+'
	| gcc_operator '-'
	| gcc_operator '&'
	| gcc_operator '|'
	| gcc_operator '^'
	| gcc_operator '~'
	| gcc_operator ','
	| gcc_operator ARITHCOMPARE
	| gcc_operator '<'
	| gcc_operator '>'
	| gcc_operator EQCOMPARE
	| gcc_operator ASSIGN
	| gcc_operator '='
	| gcc_operator LSHIFT
	| gcc_operator RSHIFT
	| gcc_operator PLUSPLUS
	| gcc_operator MINUSMINUS
	| gcc_operator ANDAND
	| gcc_operator OROR
	| gcc_operator '!'
	| gcc_operator '?' ':'
	| gcc_operator MIN_MAX
	| gcc_operator POINTSAT  %prec EMPTY
	| gcc_operator POINTSAT_STAR  %prec EMPTY
	| gcc_operator LEFT_RIGHT
	| gcc_operator '[' ']'
	| gcc_operator NEW  %prec EMPTY
	| gcc_operator DELETE  %prec EMPTY
	| gcc_operator NEW '[' ']'
	| gcc_operator DELETE '[' ']'
	/* Ngcc_ames gcc_here gcc_should gcc_be gcc_looked gcc_up gcc_in gcc_class gcc_scope ALSO.  */
	| gcc_operator gcc_type_specifier_seq gcc_conversion_declarator
	| gcc_operator error

%%
