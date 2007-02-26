%{
/**
 *    \file    dice/src/parser/corba/parser.yy
 *    \brief   contains the parser for the CORBA IDL
 *
 *    \date    06/01/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2004
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
#include <string>
#include <cassert>

#include "defines.h"
#include "Compiler.h"
#include "CParser.h"

void corbaerror(char *);
void corbaerror2(const char *fmt, ...);
void corbawarning(const char *fmt, ...);

#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "fe/FETypedDeclarator.h"

#include "fe/FEStructType.h"
#include "fe/FEIDLUnionType.h"
#include "fe/FEEnumType.h"
#include "fe/FESimpleType.h"
#include "fe/FEUserDefinedType.h"
#include "fe/FEArrayType.h"

#include "fe/FEArrayDeclarator.h"
#include "fe/FEConstDeclarator.h"

#include "fe/FEVersionAttribute.h"

#include "fe/FEConditionalExpression.h"
#include "fe/FEUserDefinedExpression.h"
#include "fe/FEUnionCase.h"

#include "fe/FEAttributeDeclarator.h"

#include "parser.h"

int corbalex(YYSTYPE*);

#define YYDEBUG    1

// collection for elements
extern CFEFileComponent *pCurFileComponent;

// error stuff
extern int errcount;
extern int warningcount;
extern int erroccured;

// #include/import special treatment
extern string sInFileName;
extern int gLineNumber;

int nParseErrorCORBA = 0;

%}

// we want a reentrant parser
%pure_parser

%union {
  char*          _id;
  string*        _str;
  char           _chr;
  long           _int;
  long double    _flt;

  CFEInterface*            _interface;
  CFELibrary*            _library;
  CFEExpression*        _expression;
  CFETypeSpec*            _type_spec;
  CFEIdentifier*        _identifier;
  CFETypedDeclarator*    _typed_decl;
  CFEDeclarator*        _decl;
  CFEAttribute*            _attribute;
  CFEOperation*            _operation;
  CFEEnumType*    _enum_type;
  CFEUnionType*    _union_type;
  CFEStructType*        _struct_type;
  CFEUnionCase*            _union_case;
  CFESimpleType*        _simple_type;
  CFEConstructedType*    _constr_type;
  CFEInterfaceComponent*    _i_component;
  CFEConstDeclarator*    _const_decl;
  CFEFileComponent*        _file_component;
  CFEArrayDeclarator*    _array_decl;

  vector<CFEIdentifier*>*       _vec_Identifier;
  vector<CFETypedDeclarator*>*  _vec_TypedDecl;
  vector<CFEUnionCase*>*        _vec_UnionCase;
  vector<CFEExpression*>*       _vec_Expression;
  vector<CFEDeclarator*>*       _vec_Declarator;
  vector<CFEAttribute*>*        _vec_Attribute;
  vector<CFEFileComponent*>*    _vec_FileComponent;
  vector<CFEInterfaceComponent*>* _vec_InterfaceComponent;
}

%token COMMA
%token SEMICOLON
%token COLON
%token SCOPE
%token LPAREN
%token RPAREN
%token LBRACE
%token RBRACE
%token LBRACKET
%token RBRACKET
%token LT
%token GT
%token IS
%token BITOR
%token BITXOR
%token BITAND
%token BITNOT
%token RSHIFT
%token LSHIFT
%token PLUS
%token MINUS
%token MUL
%token DIV
%token MOD
%token ABSTRACT
%token ANY
%token ATTRIBUTE
%token BOOLEAN
%token CASE
%token CHAR
%token CONST
%token CONTEXT
%token CUSTOM
%token DEFAULT
%token DOUBLE
%token ENUM
%token EXCEPTION
%token FALSE
%token FACTORY
%token FIXED
%token FLOAT
%token IN
%token INOUT
%token INTERFACE
%token MODULE
%token NATIVE
%token LONG
%token OBJECT
%token OCTET
%token ONEWAY
%token OUT
%token PRIVATE
%token PUBLIC
%token RAISES
%token READONLY
%token SEQUENCE
%token SHORT
%token STRING
%token STRUCT
%token SUPPORTS
%token SWITCH
%token TRUE
%token TRUNCABLE
%token TYPEDEF
%token UNION
%token UNSIGNED
%token VALUEBASE
%token VALUETYPE
%token VOID
%token WCHAR
%token WSTRING
%token FPAGE
%token REFSTRING
/* dice specific token */
%token EOF_TOKEN

%token    <_id>        ID
%token    <_int>        LIT_INT
%token    <_str>        LIT_STR
%token    <_str>        LIT_WSTR
%token    <_chr>        LIT_CHAR
%token    <_chr>        LIT_WCHAR
%token    <_flt>        LIT_FLOAT
%token    <_str>        FILENAME

%type    <_expression>        add_expr
%type    <_expression>        and_expr
%type    <_simple_type>        any_type
%type    <_array_decl>        array_declarator
%type    <_typed_decl>        attr_dcl
%type    <_type_spec>        base_type_spec
%type    <_type_spec>        boolean_type
%type    <_union_case>        case
%type    <_expression>        case_label
%type    <_vec_Expression>    case_label_list
%type    <_type_spec>         char_type
%type    <_const_decl>        const_dcl
%type    <_expression>        const_exp
%type    <_type_spec>         const_type
%type    <_constr_type>       constr_type_spec
%type    <_decl>              declarator
%type    <_vec_Declarator>    declarators
%type    <_typed_decl>        element_spec
%type    <_vec_Identifier>    enumerator_list
%type    <_enum_type>         enum_type
%type    <_i_component>       except_dcl
%type    <_i_component>       export
%type    <_vec_InterfaceComponent> export_list
%type    <_type_spec>         fixed_pt_const_type
%type    <_type_spec>         fixed_pt_type
%type    <_simple_type>       floating_pt_type
%type    <_typed_decl>        init_param_decl
%type    <_vec_TypedDecl>     init_param_decls
%type    <_type_spec>         integer_type
%type    <_interface>         interface
%type    <_interface>         interface_dcl
%type    <_vec_Identifier>    interface_inheritance_spec
%type    <_identifier>        interface_name
%type    <_vec_Identifier>    interface_name_list
%type    <_typed_decl>        member
%type    <_vec_TypedDecl>     member_list
%type    <_library>           module
%type    <_file_component>    module_element
%type    <_vec_FileComponent> module_element_list
%type    <_expression>        mult_expr
%type    <_simple_type>       object_type
%type    <_simple_type>       octet_type
%type    <_attribute>         op_attribute
%type    <_operation>         op_dcl
%type    <_type_spec>         op_type_spec
%type    <_expression>        or_expr
%type    <_vec_Attribute>     param_attribute
%type    <_typed_decl>        param_dcl
%type    <_vec_TypedDecl>     param_dcl_list
%type    <_type_spec>         param_type_spec
%type    <_vec_TypedDecl>     parameter_dcls
%type    <_expression>        positive_int_const
%type    <_simple_type>       predefined_type
%type    <_expression>        primary_expr
%type    <_vec_Identifier>    raises_expr
%type    <_identifier>        scoped_name
%type    <_vec_Identifier>    scoped_name_list
%type    <_type_spec>         sequence_type
%type    <_expression>        shift_expr
%type    <_decl>              simple_declarator
%type    <_vec_Declarator>    simple_declarator_list
%type    <_type_spec>         simple_type_spec
%type    <_vec_Identifier>    string_literal_list
%type    <_type_spec>         string_type
%type    <_struct_type>       struct_type
%type    <_vec_UnionCase>     switch_body
%type    <_type_spec>         switch_type_spec
%type    <_type_spec>         template_type_spec
%type    <_typed_decl>        type_dcl
%type    <_type_spec>         type_spec
%type    <_expression>        unary_expr
%type    <_union_type>        union_type
%type    <_type_spec>         value_base_type
%type    <_identifier>        value_name
%type    <_vec_Identifier>    value_name_list
%type    <_simple_type>       wide_char_type
%type    <_type_spec>         wide_string_type
%type    <_expression>        xor_expr

%%

specification    :
      definition_list
    ;

definition_list :
      definition_list definition
    | definition
    ;

definition :
      type_dcl semicolon
    {
        CFEFile *pFEFile = CParser::GetCurrentFile();
	assert(pFEFile);
        pFEFile->m_Typedefs.Add($1);
    }
    | const_dcl semicolon
    {
        CFEFile *pFEFile = CParser::GetCurrentFile();
	assert(pFEFile);
        pFEFile->m_Constants.Add($1);
    }
    | except_dcl semicolon
    {
    }
    | interface semicolon
    {
        if ($1 != NULL) {
            CFEFile *pFEFile = CParser::GetCurrentFile();
	    assert(pFEFile);
            pFEFile->m_Interfaces.Add($1);
        }
        // else: was forward_dcl
    }
    | module semicolon
    {
        CFEFile *pFEFile = CParser::GetCurrentFile();
	assert(pFEFile);
        pFEFile->m_Libraries.Add($1);
    }
    | value semicolon
    | EOF_TOKEN
    {
        YYACCEPT;
    }
    ;

module :
      MODULE ID
    {
        // check if we can find a library with this name
        CFEFile *pRoot = dynamic_cast<CFEFile*>(CParser::GetCurrentFile()->GetRoot());
        assert(pRoot);
	CFELibrary *pFEPrevLib = pRoot->FindLibrary($2);
	CFELibrary *pFELibrary = new CFELibrary(string($2), NULL, 
	    pCurFileComponent ? static_cast<CFEBase*>(pCurFileComponent) :
	    CParser::GetCurrentFile());
	if (pFEPrevLib)
	    pFEPrevLib->AddSameLibrary(pFELibrary);
	$<_library>$ = pFELibrary;
	pFELibrary->SetSourceLine(gLineNumber);
	// bookkeeping
	pCurFileComponent = pFELibrary;
    } LBRACE module_element_list rbrace
    {
    	$$ = $<_library>3;
	$$->AddComponents($5);
        delete $5;
	$$->SetSourceLineEnd(gLineNumber);

	// bookkeeping
	if (pCurFileComponent &&
	    pCurFileComponent->GetParent())
	{
	    pCurFileComponent =
		dynamic_cast<CFEFileComponent*>(pCurFileComponent->GetParent());
	}       
    }
    ;

module_element_list :
      module_element_list module_element
    {
	if ($2)
	    $1->push_back($2);
	$$ = $1;
    }
    | module_element
    {
        $$ = new vector<CFEFileComponent*>();
        if ($1)
            $$->push_back($1);
    }
    ;

module_element :
      type_dcl semicolon
    { $$ = $1; }
    | const_dcl semicolon
    { $$ = $1; }
    | except_dcl semicolon
    { $$ = $1; }
    | interface semicolon
    { $$ = $1; }
    | module semicolon
    { $$ = $1; }
    | value semicolon
    {
    }
    ;

interface :
      interface_dcl
    { $$ = $1; }
    | forward_dcl
    { $$ = NULL; }
    ;

interface_dcl    :
      ABSTRACT INTERFACE ID interface_inheritance_spec
    {
        // test base interfaces
        CFEFile *pRoot = dynamic_cast<CFEFile*>(CParser::GetCurrentFile()->GetRoot());
        assert(pRoot);
        vector<CFEIdentifier*>::iterator iter;
        for (iter = $4->begin(); iter != $4->end(); iter++) {
            if (*iter) {
                if (pRoot->FindInterface((*iter)->GetName()) == NULL)
                {
                    corbaerror2("Couldn't find base interface name \"%s\".",
		    (*iter)->GetName().c_str());
                    YYABORT;
                }
            }
        }
        if (pRoot->FindInterface($3) != NULL)
        {
            corbaerror2("Interface name \"%s\" already exists", $3);
            YYABORT;
        }
	// create abstract attribute
        CFEAttribute *pAttr = new CFEAttribute(ATTR_ABSTRACT);
        pAttr->SetSourceLine(gLineNumber);
        vector<CFEAttribute*> *tmpVA = new vector<CFEAttribute*>();
        tmpVA->push_back(pAttr);

	// create interface
	CFEInterface *pFEInterface = new CFEInterface(tmpVA, string($3), $4,
	    pCurFileComponent ? static_cast<CFEBase*>(pCurFileComponent) :
	    CParser::GetCurrentFile());
        pAttr->SetParent(pFEInterface);
        delete tmpVA;
        pFEInterface->SetSourceLine(gLineNumber);
	$<_interface>$ = pFEInterface;

	// bookkeeping
	pCurFileComponent = pFEInterface;
    } LBRACE export_list rbrace
    {
        if ($7 == NULL) {
            corbaerror2("no empty interface specification supported.");
            YYABORT;
        }
        // add elements to interface
	$$ = $<_interface>5;
	$$->AddComponents($7);
        delete $7;
        $$->SetSourceLineEnd(gLineNumber);

	// bookkeeping
	if (pCurFileComponent &&
	    pCurFileComponent->GetParent())
	{
	    pCurFileComponent =
		dynamic_cast<CFEFileComponent*>(pCurFileComponent->GetParent());
	}       
    }
    | ABSTRACT INTERFACE ID
    {
        CFEFile *pRoot = dynamic_cast<CFEFile*>(CParser::GetCurrentFile()->GetRoot());
        assert(pRoot);
        if (pRoot->FindInterface($3) != NULL)
        {
            // name already exists
            corbaerror2("Interface name \"%s\" already exists", $3);
            YYABORT;
        }
	// create abstract attribute
        CFEAttribute *pAttr = new CFEAttribute(ATTR_ABSTRACT);
        pAttr->SetSourceLine(gLineNumber);
        vector<CFEAttribute*> *tmpVA = new vector<CFEAttribute*>();
        tmpVA->push_back(pAttr);

	// create interface
	CFEInterface *pFEInterface = new CFEInterface(tmpVA, string($3), NULL,
	    pCurFileComponent ? static_cast<CFEBase*>(pCurFileComponent) :
	    CParser::GetCurrentFile());
        pAttr->SetParent(pFEInterface);
        delete tmpVA;
        pFEInterface->SetSourceLine(gLineNumber);
	$<_interface>$ = pFEInterface;

	// bookkeeping
	pCurFileComponent = pFEInterface;
    } LBRACE export_list rbrace
    {
        if ($6 == NULL) {
            corbaerror2("no empty interface specification supported.");
            YYABORT;
        }
        // add elements to interface
	$$ = $<_interface>4;
	$$->AddComponents($6);
        delete $6;
        $$->SetSourceLineEnd(gLineNumber);

	// bookkeeping
	if (pCurFileComponent &&
	    pCurFileComponent->GetParent())
	{
	    pCurFileComponent =
		dynamic_cast<CFEFileComponent*>(pCurFileComponent->GetParent());
	}       
    }
    | INTERFACE ID interface_inheritance_spec
    {
        // test base interfaces
        CFEFile *pRoot = dynamic_cast<CFEFile*>(CParser::GetCurrentFile()->GetRoot());
        assert(pRoot);
        vector<CFEIdentifier*>::iterator iter;
        for (iter = $3->begin(); iter != $3->end(); iter++)
        {
            if (*iter)
            {
                if (!(pRoot->FindInterface((*iter)->GetName())))
                {
                    corbaerror2("Couldn't find base interface name \"%s\".", 
		        (*iter)->GetName().c_str());
                    YYABORT;
                }
            }
        }
        if (pRoot->FindInterface($2) != NULL)
        {
            // name already exists
            corbaerror2("Interface name \"%s\" already exists", $2);
            YYABORT;
        }
	// create interface
	CFEInterface *pFEInterface = new CFEInterface(NULL, string($2), $3,
	    pCurFileComponent ? static_cast<CFEBase*>(pCurFileComponent) :
	    CParser::GetCurrentFile());
        pFEInterface->SetSourceLine(gLineNumber);
	$<_interface>$ = pFEInterface;

	// bookkeeping
	pCurFileComponent = pFEInterface;
    } LBRACE export_list rbrace
    {
        if ($6 == NULL) {
            corbaerror2("no empty interface specification supported.");
            YYABORT;
        }
        // add elements to interface
	$$ = $<_interface>4;
	$$->AddComponents($6);
        delete $6;
        $$->SetSourceLineEnd(gLineNumber);

	// bookkeeping
	if (pCurFileComponent &&
	    pCurFileComponent->GetParent())
	{
	    pCurFileComponent =
		dynamic_cast<CFEFileComponent*>(pCurFileComponent->GetParent());
	}       
    }
    | INTERFACE ID
    {
        CFEFile *pRoot = dynamic_cast<CFEFile*>(CParser::GetCurrentFile()->GetRoot());
        assert(pRoot);
        if (pRoot->FindInterface($2) != NULL)
        {
            // name already exists
            corbaerror2("Interface name \"%s\" already exists", $2);
            YYABORT;
        }
	// create interface
	CFEInterface *pFEInterface = new CFEInterface(NULL, string($2), NULL,
	    pCurFileComponent ? static_cast<CFEBase*>(pCurFileComponent) :
	    CParser::GetCurrentFile());
        pFEInterface->SetSourceLine(gLineNumber);
	$<_interface>$ = pFEInterface;

	// bookkeeping
	pCurFileComponent = pFEInterface;
    } LBRACE export_list rbrace
    {
        if ($5 == NULL) {
            corbaerror2("no empty interface specification supported.");
            YYABORT;
        }
        // add elements to interface
	$$ = $<_interface>3;
	$$->AddComponents($5);
        delete $5;
        $$->SetSourceLineEnd(gLineNumber);

	// bookkeeping
	if (pCurFileComponent &&
	    pCurFileComponent->GetParent())
	{
	    pCurFileComponent =
		dynamic_cast<CFEFileComponent*>(pCurFileComponent->GetParent());
	}       
    }
    ;

forward_dcl :
      ABSTRACT INTERFACE ID
    | INTERFACE ID
    ;

export_list :
      export_list export
    {
            if ($2)
                $1->push_back($2);
        $$ = $1;
    }
    | export_list attr_dcl
    {
        if ($2)
            $1->push_back($2);
        $$ = $1;
    }
    | export
    {
        $$ = new vector<CFEInterfaceComponent*>();
        $$->push_back($1);
    }
    | attr_dcl
    {
        $$ = new vector<CFEInterfaceComponent*>();
        $$->push_back($1);
    }
    ;

export :
      type_dcl semicolon
    { $$ = $1; }
    | const_dcl semicolon
    { $$ = $1; }
    | except_dcl semicolon
    { $$ = $1; }
    | op_dcl semicolon
    { $$ = $1; }
    ;

interface_inheritance_spec :
      COLON interface_name_list
    {
        $$ = $2;
    }
    ;

interface_name_list :
      interface_name_list COMMA interface_name
    {
            if ($3)
                $1->push_back($3);
        $$ = $1;
    }
    | interface_name
    {
        $$ = new vector<CFEIdentifier*>();
        $$->push_back($1);
    }
    ;

interface_name :
      scoped_name
    { $$ = $1; }
    ;

scoped_name :
      ID
    {
        $$ = new CFEIdentifier($1);
        $$->SetSourceLine(gLineNumber);
    }
    | SCOPE ID
    {
        $$ = new CFEIdentifier($2);
        $$->Prefix(string("::"));
        $$->SetSourceLine(gLineNumber);
    }
    | scoped_name SCOPE ID
    {
        $$ = $1;
        $$->Suffix(string("::"));
        $$->Suffix(string($3));
    }
    ;

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
    {
            if ($3)
                $1->push_back($3);
        $$ = $1;
    }
    | value_name
    {
        $$ = new vector<CFEIdentifier*>();
        $$->push_back($1);
    }
    ;

value_name :
      scoped_name
    { $$ = $1; }
    ;

value_element    :
      export
    { }
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
    {
            if ($3)
                $1->push_back($3);
        $$ = $1;
    }
    | init_param_decl
    {
        $$ = new vector<CFETypedDeclarator*>();
        $$->push_back($1);
    }
    ;

init_param_decl :
      IN param_type_spec simple_declarator
    {
        CFEAttribute *tmp = new CFEAttribute(ATTR_IN);
        tmp->SetSourceLine(gLineNumber);
        vector<CFEAttribute*> *tmpVA = new vector<CFEAttribute*>();
        tmpVA->push_back(tmp);
        vector<CFEDeclarator*> *tmpVD = new vector<CFEDeclarator*>();
        tmpVD->push_back($3);
        $$ = new CFETypedDeclarator(TYPEDECL_PARAM, $2, tmpVD, tmpVA);
        // set parent relationship
        $2->SetParent($$);
        $3->SetParent($$);
        tmp->SetParent($$);
        $$->SetSourceLine(gLineNumber);
        delete tmpVD;
        delete tmpVA;
    }
    ;

const_dcl :
      CONST const_type ID IS const_exp
    {
        CFETypeSpec *pType = $2;
            while (pType && (pType->GetType() == TYPE_USER_DEFINED))
        {
            string sTypeName = ((CFEUserDefinedType*)pType)->GetName();
            CFETypedDeclarator *pTypedef = CParser::GetCurrentFile()->FindUserDefinedType(sTypeName);
            if (!pTypedef)
                corbaerror2("Cannot find type for \"%s\".", sTypeName.c_str());
            pType = pTypedef->GetType();
        }
        if (!( $5->IsOfType(pType->GetType()) )) {
            corbaerror2("Const type of \"%s\" does not match with expression.",$3);
            YYABORT;
        }
        $$ = new CFEConstDeclarator($2, string($3), $5);
        $$->SetSourceLine(gLineNumber);
        // set parent relationship
        $2->SetParent($$);
        $5->SetParent($$);
    }
    ;

const_type :
      integer_type
    { $$ = $1; }
    | char_type
    { $$ = $1; }
    | wide_char_type
    { $$ = $1; }
    | boolean_type
    { $$ = $1; }
    | floating_pt_type
    { $$ = $1; }
    | string_type
    { $$ = $1; }
    | wide_string_type
    { $$ = $1; }
    | fixed_pt_const_type
    { $$ = $1; }
    | scoped_name
    {
        $$ = new CFEUserDefinedType($1->GetName());
        $$->SetSourceLine(gLineNumber);
    }
    | octet_type
    { $$ = $1; }
    ;

const_exp :
      or_expr
    { $$ = $1; }
    ;

or_expr :
      xor_expr
    { $$ = $1; }
    | or_expr BITOR xor_expr
    {
        $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITOR, $3);
        $$->SetSourceLine(gLineNumber);
        // set parent relationship
        $1->SetParent($$);
        $3->SetParent($$);
    }
    ;

xor_expr :
      and_expr
    { $$ = $1; }
    | xor_expr BITXOR and_expr
    {
        $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITXOR, $3);
        $$->SetSourceLine(gLineNumber);
        // set parent relationship
        $1->SetParent($$);
        $3->SetParent($$);
    }
    ;

and_expr :
      shift_expr
    { $$ = $1; }
    | and_expr BITAND shift_expr
    {
        $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITAND, $3);
        $$->SetSourceLine(gLineNumber);
        // set parent relationship
        $1->SetParent($$);
        $3->SetParent($$);
    }
    ;

shift_expr :
      add_expr
    { $$ = $1; }
    | shift_expr RSHIFT add_expr
    {
        $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_RSHIFT, $3);
        $$->SetSourceLine(gLineNumber);
        // set parent relationship
        $1->SetParent($$);
        $3->SetParent($$);
    }
    | shift_expr LSHIFT add_expr
    {
        $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LSHIFT, $3);
        $$->SetSourceLine(gLineNumber);
        // set parent relationship
        $1->SetParent($$);
        $3->SetParent($$);
    }
    ;

add_expr :
      mult_expr
    { $$ = $1; }
    | add_expr PLUS mult_expr
    {
        $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_PLUS, $3);
        $$->SetSourceLine(gLineNumber);
        // set parent relationship
        $1->SetParent($$);
        $3->SetParent($$);
    }
    | add_expr MINUS mult_expr
    {
        $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MINUS, $3);
        $$->SetSourceLine(gLineNumber);
        // set parent relationship
        $1->SetParent($$);
        $3->SetParent($$);
    }
    ;

mult_expr :
      unary_expr
    { $$ = $1; }
    | mult_expr MUL unary_expr
    {
        $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MUL, $3);
        $$->SetSourceLine(gLineNumber);
        // set parent relationship
        $1->SetParent($$);
        $3->SetParent($$);
    }
    | mult_expr DIV unary_expr
    {
        $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_DIV, $3);
        $$->SetSourceLine(gLineNumber);
        // set parent relationship
        $1->SetParent($$);
        $3->SetParent($$);
    }
    | mult_expr MOD unary_expr
    {
        $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_MOD, $3);
        $$->SetSourceLine(gLineNumber);
        // set parent relationship
        $1->SetParent($$);
        $3->SetParent($$);
    }
    ;

unary_expr :
      MINUS primary_expr
    {
        $$ = new CFEUnaryExpression(EXPR_UNARY, EXPR_SMINUS, $2);
        $$->SetSourceLine(gLineNumber);
        // set parent relationship
        $2->SetParent($$);
    }
    | PLUS primary_expr
    {
        $$ = new CFEUnaryExpression(EXPR_UNARY, EXPR_SPLUS, $2);
        $$->SetSourceLine(gLineNumber);
        // set parent relationship
        $2->SetParent($$);
    }
    | BITNOT primary_expr
    {
        $$ = new CFEUnaryExpression(EXPR_UNARY, EXPR_TILDE, $2);
        $$->SetSourceLine(gLineNumber);
        // set parent relationship
        $2->SetParent($$);
    }
    | primary_expr
    { $$ = $1; }
    ;

primary_expr :
      scoped_name
    {
        $$ = new CFEUserDefinedExpression($1->GetName());
        $$->SetSourceLine(gLineNumber);
    }
    | LIT_INT
    {
        $$ = new CFEPrimaryExpression(EXPR_INT, $1);
        $$->SetSourceLine(gLineNumber);
    }
    | LIT_STR
    {
        $$ = new CFEExpression(EXPR_STRING, *$1);
        delete $1;
        $$->SetSourceLine(gLineNumber);
    }
    | LIT_WSTR
    {
        $$ = new CFEExpression(EXPR_STRING, *$1);
        delete $1;
        $$->SetSourceLine(gLineNumber);
    }
    | LIT_CHAR
    {
        $$ = new CFEExpression(EXPR_CHAR, $1);
        $$->SetSourceLine(gLineNumber);
    }
    | LIT_WCHAR
    {
        $$ = new CFEExpression(EXPR_CHAR, $1);
        $$->SetSourceLine(gLineNumber);
    }
    | LIT_FLOAT
    {
        $$ = new CFEPrimaryExpression(EXPR_FLOAT, $1);
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
    | LPAREN const_exp rparen
    {
        $$ = new CFEPrimaryExpression(EXPR_PAREN, $2);
        $$->SetSourceLine(gLineNumber);
        // set parent relationship
        $2->SetParent($$);
    }
    ;

positive_int_const :
      const_exp
    { $$ = $1; }
    ;

type_dcl :
      TYPEDEF type_spec declarators
    {
        // check if type_names already exist
        CFEFile *pRoot = dynamic_cast<CFEFile*>(CParser::GetCurrentFile()->GetRoot());
        assert(pRoot);
        vector<CFEDeclarator*>::iterator iter;
        for (iter = $3->begin(); iter != $3->end(); iter++)
        {
            if (*iter)
            {
                if (!(*iter)->GetName().empty()) {
                    if (pRoot->FindUserDefinedType((*iter)->GetName()) != NULL)
                    {
                        corbaerror2("\"%s\" has already been defined as type.",(*iter)->GetName().c_str());
                        YYABORT;
                    }
                }
            }
        }
        $$ = new CFETypedDeclarator(TYPEDECL_TYPEDEF, $2, $3);
        $$->SetSourceLine(gLineNumber);
        // set parent relationship
        $2->SetParent($$);
        delete $3;
    }
    | struct_type
    {
        // extract the name
        string sTag = $1->GetTag();
        CFEDeclarator *pDecl =  new CFEDeclarator(DECL_IDENTIFIER, 
	    "_struct_" + sTag);
        pDecl->SetSourceLine(gLineNumber);
        // modify tag, so compiler won't warn
        vector<CFEDeclarator*> *pDecls = new vector<CFEDeclarator*>();
        pDecls->push_back(pDecl);
        // create a new typed decl
        $$ = new CFETypedDeclarator(TYPEDECL_TYPEDEF, $1, pDecls);
        $$->SetSourceLine(gLineNumber);
        $1->SetParent($$);
        pDecl->SetParent($$);
        // return
    }
    | union_type
    {
        // extract the name
        string sTag = $1->GetTag();
        CFEDeclarator *pDecl =  new CFEDeclarator(DECL_IDENTIFIER, sTag);
        pDecl->SetSourceLine(gLineNumber);
        // modify tag, so compiler won't warn
        vector<CFEDeclarator*> *pDecls = new vector<CFEDeclarator*>();
        pDecls->push_back(pDecl);
        // create a new typed decl
        $$ = new CFETypedDeclarator(TYPEDECL_TYPEDEF, $1, pDecls);
        $$->SetSourceLine(gLineNumber);
        $1->SetParent($$);
        pDecl->SetParent($$);
        // return
    }
    | enum_type
    {
        // FIXME
        // extract the name
        // create a new typed decl
        // return
    }
    | NATIVE simple_declarator
    {
        // FIXME
        // use the name as type name
        //$$ = new CFEUserDefinedType($2);
    }
    ;

type_spec :
      simple_type_spec
    { $$ = $1; }
    | constr_type_spec
    { $$ = $1; }
    ;

simple_type_spec :
      base_type_spec
    { $$ = $1; }
    | template_type_spec
    { $$ = $1; }
    | scoped_name
    {
        $$ = new CFEUserDefinedType($1->GetName());
        $$->SetSourceLine(gLineNumber);
    }
    ;

base_type_spec :
      floating_pt_type
    { $$ = $1; }
    | integer_type
    { $$ = $1; }
    | char_type
    { $$ = $1; }
    | wide_char_type
    { $$ = $1; }
    | boolean_type
    { $$ = $1; }
    | octet_type
    { $$ = $1; }
    | any_type
    { $$ = $1; }
    | object_type
    { $$ = $1; }
    | value_base_type
    { $$ = $1; }
    ;

template_type_spec :
      sequence_type
    { $$ = $1; }
    | string_type
    { $$ = $1; }
    | wide_string_type
    { $$ = $1; }
    | fixed_pt_type
    { $$ = $1; }
    ;

constr_type_spec :
      struct_type
    { $$ = $1; }
    | union_type
    { $$ = $1; }
    | enum_type
    { $$ = $1; }
    ;

declarators :
      declarators COMMA declarator
    {
            if ($3)
                $1->push_back($3);
        $$ = $1;
    }
    | declarator
    {
        $$ = new vector<CFEDeclarator*>();
        $$->push_back($1);
    }
    ;

/* complex_declarator == array_declarator */
declarator :
      simple_declarator
    { $$ = $1; }
    | array_declarator
    { $$ = $1; }
    ;

array_declarator :
      declarator LBRACKET positive_int_const rbracket
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
    ;

simple_declarator :
      ID
    {
        $$ = new CFEDeclarator(DECL_IDENTIFIER, string($1));
        $$->SetSourceLine(gLineNumber);
    }
    | error
    {
        corbaerror2("expected a declarator");
        YYABORT;
    }
    ;

floating_pt_type :
      FLOAT
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
      SHORT
    {
        $$ = new CFESimpleType(TYPE_INTEGER, false, true, 2/*value for SHORT*/, false);
        $$->SetSourceLine(gLineNumber);
    }
    | LONG
    {
        $$ = new CFESimpleType(TYPE_INTEGER, false, true, 4/*value for LONG*/, false);
        $$->SetSourceLine(gLineNumber);
    }
    | LONG LONG
    {
        $$ = new CFESimpleType(TYPE_INTEGER, false, true, 8/*value for LONG LONG*/, false);
        $$->SetSourceLine(gLineNumber);
    }
    | UNSIGNED SHORT
    {
        $$ = new CFESimpleType(TYPE_INTEGER, true, true, 2/*value for SHORT*/, false);
        $$->SetSourceLine(gLineNumber);
    }
    | UNSIGNED LONG
    {
        $$ = new CFESimpleType(TYPE_INTEGER, true, true, 4/*value for LONG*/, false);
        $$->SetSourceLine(gLineNumber);
    }
    | UNSIGNED LONG LONG
    {
        $$ = new CFESimpleType(TYPE_INTEGER, true, true, 8/*value for LONG LONG*/, false);
        $$->SetSourceLine(gLineNumber);
    }
    ;

char_type :
      CHAR
    {
        $$ = new CFESimpleType(TYPE_CHAR);
        $$->SetSourceLine(gLineNumber);
    }
    ;

wide_char_type :
      WCHAR
    {
        $$ = new CFESimpleType(TYPE_WCHAR);
        $$->SetSourceLine(gLineNumber);
    }
    ;

boolean_type :
      BOOLEAN
    {
        $$ = new CFESimpleType(TYPE_BOOLEAN);
        $$->SetSourceLine(gLineNumber);
    }
    ;

octet_type :
      OCTET
    {
        $$ = new CFESimpleType(TYPE_OCTET);
        $$->SetSourceLine(gLineNumber);
    }
    ;

any_type :
      ANY
    {
        $$ = new CFESimpleType(TYPE_ANY);
        $$->SetSourceLine(gLineNumber);
    }
    ;

object_type :
      OBJECT
    {
        $$ = new CFESimpleType(TYPE_OBJECT);
        $$->SetSourceLine(gLineNumber);
    }
    ;

struct_type :
      STRUCT ID LBRACE member_list rbrace
    {
        $$ = new CFEStructType(string($2), $4);
        if ($4)
            delete $4;
        $$->SetSourceLine(gLineNumber);
    }
    | STRUCT LBRACE member_list RBRACE
    {
        $$ = new CFEStructType(string(), $3);
        $$->SetSourceLine(gLineNumber);
        if ($3)
            delete $3;
    }
    ;

member_list :
      member_list member
    {
            if ($2)
                $1->push_back($2);
        $$ = $1;
    }
    | member
    {
        $$ = new vector<CFETypedDeclarator*>();
        $$->push_back($1);
    }
    ;

member :
      type_spec declarators semicolon
    {
        $$ = new CFETypedDeclarator(TYPEDECL_FIELD, $1, $2);
        $$->SetSourceLine(gLineNumber);
        // set parent relationship
        $1->SetParent($$);
        delete $2;
    }
    ;

union_type :
      UNION ID SWITCH LPAREN switch_type_spec rparen LBRACE switch_body rbrace
    {
	$$ = new CFEIDLUnionType(string($2), $8, $5, string(), string());
	$$->SetSourceLine(gLineNumber);
	// set parent relationship
	$5->SetParent($$);
	if ($8)
	    delete $8;
    }
    ;

switch_type_spec :
      integer_type
    { $$ = $1; }
    | char_type
    { $$ = $1; }
    | boolean_type
    { $$ = $1; }
    | enum_type
    { $$ = $1; }
    | scoped_name
    {
        $$ = new CFEUserDefinedType($1->GetName());
        $$->SetSourceLine(gLineNumber);
    }
    ;

switch_body :
      switch_body case
    {
	if ($2)
	    $1->push_back($2);
	$$ = $1;
    }
    | case
    {
	$$ = new vector<CFEUnionCase*>();
	$$->push_back($1);
    }
    ;

case :
      case_label_list element_spec semicolon
    {
	$$ = new CFEUnionCase($2, $1);
	$$->SetSourceLine(gLineNumber);
	// set parent relationship
	$2->SetParent($$);
	delete $1;
    }
    | DEFAULT colon element_spec semicolon
    {
        $$ = new CFEUnionCase($3);
        $$->SetSourceLine(gLineNumber);
        // set parent relationship
        $3->SetParent($$);
    }
    ;

case_label_list :
      case_label_list case_label
    {
        if ($2)
            $1->push_back($2);
        $$ = $1;
    }
    | case_label
    {
	$$ = new vector<CFEExpression*>();
	$$->push_back($1);
    }
    ;

case_label :
      CASE const_exp colon
    {
        $$ = $2;
    }
    ;

element_spec :
      type_spec declarator
    {
        vector<CFEDeclarator*> *tmp = new vector<CFEDeclarator*>();
        tmp->push_back($2);
        $$ = new CFETypedDeclarator(TYPEDECL_FIELD, $1, tmp);
        $$->SetSourceLine(gLineNumber);
        // set parent relationship
        $1->SetParent($$);
        $2->SetParent($$);
        delete tmp;
    }
    ;

enum_type :
      ENUM ID LBRACE enumerator_list rbrace
    {
        $$ = new CFEEnumType(string($2), $4);
        $$->SetSourceLine(gLineNumber);
        delete $4;
    }
    ;

enumerator_list :
      enumerator_list COMMA ID
    {
        CFEIdentifier *tmp = new CFEIdentifier($3);
        tmp->SetSourceLine(gLineNumber);
        $1->push_back(tmp);
        $$ = $1;
    }
    | ID
    {
        CFEIdentifier *tmp = new CFEIdentifier($1);
        tmp->SetSourceLine(gLineNumber);
        $$ = new vector<CFEIdentifier*>();
        $$->push_back(tmp);
    }
    ;

sequence_type    :
      SEQUENCE LT simple_type_spec COMMA positive_int_const GT
    {
        // specifies a "bounded" array of that type
        // can be nested
        $$ = new CFEArrayType($3, $5);
        $$->SetSourceLine(gLineNumber);
        $3->SetParent($$);
    }
    | SEQUENCE LT simple_type_spec GT
    {
        // specifies an "unbounded" array of that type
        // can be nested
        $$ = new CFEArrayType($3);
        $$->SetSourceLine(gLineNumber);
        $3->SetParent($$);
    }
    ;

string_type :
      STRING LT positive_int_const GT
    {
        // according to
        // - CORBA C Language Mapping 1.12 and 1.13, and
        // - CORBA C++ Language Mapping 1.7 and 1.8
        // strings, no matter if bound or unbound
        // are mapped to char* and wchar* respectively
        $$ = new CFESimpleType(TYPE_STRING); // is replaced by 'char*'
        $$->SetSourceLine(gLineNumber);
    }
    | STRING
    {
        $$ = new CFESimpleType(TYPE_STRING);
        $$->SetSourceLine(gLineNumber);
    }
    ;

wide_string_type :
      WSTRING LT positive_int_const GT
    {
        // according to
        // - CORBA C Language Mapping 1.12 and 1.13, and
        // - CORBA C++ Language Mapping 1.7 and 1.8
        // strings, no matter if bound or unbound
        // are mapped to char* and wchar* respectively
        $$ = new CFESimpleType(TYPE_WSTRING);
        $$->SetSourceLine(gLineNumber);
    }
    | WSTRING
    {
        $$ = new CFESimpleType(TYPE_WSTRING);
        $$->SetSourceLine(gLineNumber);
    }
    ;

attr_dcl :
      READONLY ATTRIBUTE param_type_spec simple_declarator_list
    {
        CFEAttribute *tmp = new CFEAttribute(ATTR_READONLY);
        vector<CFEAttribute*> *tmpVA = new vector<CFEAttribute*>();
        tmpVA->push_back(tmp);
        $$ = new CFEAttributeDeclarator($3, $4, tmpVA);
        $3->SetParent($$);
        delete $4;
        delete tmpVA;
        tmp->SetParent($$);
        $$->SetSourceLine(gLineNumber);
    }
    | ATTRIBUTE param_type_spec simple_declarator_list
    {
        $$ = new CFEAttributeDeclarator($2, $3);
        $2->SetParent($$);
        delete $3;
        $$->SetSourceLine(gLineNumber);
    }
    ;

simple_declarator_list :
      simple_declarator_list COMMA simple_declarator
    {
            if ($3)
                $1->push_back($3);
        $$ = $1;
    }
    | simple_declarator
    {
        $$ = new vector<CFEDeclarator*>();
        $$->push_back($1);
    }
    ;

except_dcl :
      EXCEPTION ID LBRACE member_list rbrace
    {
        // defines an exception with name ID and members ... (represent as struct?)
        $$ = NULL;
        if ($4)
            delete $4;
    }
    | EXCEPTION ID LBRACE RBRACE
    {
        // defines an exception with name ID and no members
        $$ = NULL;
    }
    ;

op_dcl :
      op_attribute op_type_spec ID parameter_dcls raises_expr context_expr
    {
	// ignore the context
	vector<CFEAttribute*> *tmpVA = new vector<CFEAttribute*>();
	if ($1)
	    tmpVA->push_back($1);
	$$ = new CFEOperation($2, string($3), $4, tmpVA, $5);
	$$->SetSourceLine(gLineNumber);
	// set parent relationship
	$2->SetParent($$);
	if ($1 != 0)
	    $1->SetParent($$);
	if ($4)
	    delete $4;
	delete $5;
	delete $1;
	delete tmpVA;
    }
    | op_attribute op_type_spec ID parameter_dcls context_expr
    {
	// ignore the context
	vector<CFEAttribute*> *tmpVA = new vector<CFEAttribute*>();
	if ($1)
	    tmpVA->push_back($1);
	$$ = new CFEOperation($2, string($3), $4, tmpVA);
	$$->SetSourceLine(gLineNumber);
	// set parent relationship
	$2->SetParent($$);
	if ($1 != 0)
	    $1->SetParent($$);
	if ($4)
	    delete $4;
	delete tmpVA;
    }
    ;

op_attribute :
      ONEWAY
    {
        // we do not support oneway attribute -> represent appropriately (in)
        $$ = new CFEAttribute(ATTR_IN);
        $$->SetSourceLine(gLineNumber);
    }
    | /* is optional attribute */
    {
        $$ = 0;
    }
    ;

op_type_spec :
      param_type_spec
    {
        $$ = $1;
    }
    | VOID
    {
        $$ = new CFESimpleType(TYPE_VOID);
        $$->SetSourceLine(gLineNumber);
    }
    ;

parameter_dcls :
      LPAREN param_dcl_list rparen
    {
        $$ = $2;
    }
    | LPAREN RPAREN
    {
        $$ = 0;
    }
    ;

param_dcl_list :
      param_dcl_list COMMA param_dcl
    {
            if ($3)
                $1->push_back($3);
        $$ = $1;
    }
    | param_dcl
    {
        $$ = new vector<CFETypedDeclarator*>();
        $$->push_back($1);
    }
    ;

param_dcl    :
      param_attribute param_type_spec simple_declarator
    {
        vector<CFEDeclarator*> *tmp = new vector<CFEDeclarator*>();
        tmp->push_back($3);
        $$ = new CFETypedDeclarator(TYPEDECL_PARAM, $2, tmp, $1);
        $$->SetSourceLine(gLineNumber);
        // check if OUT and reference
        if ($$->m_Attributes.Find(ATTR_OUT) != 0) {
            // because CORBA knows no pointers we have to add a reference
            $3->SetStars(1);
        }
        // set parent relationship
        $2->SetParent($$);
        $3->SetParent($$);
        delete $1;
        delete tmp;
    }
    ;

param_attribute :
      IN
    {
        CFEAttribute *tmp = new CFEAttribute(ATTR_IN);
        tmp->SetSourceLine(gLineNumber);
        $$ = new vector<CFEAttribute*>();
        $$->push_back(tmp);
    }
    | OUT
    {
        CFEAttribute *tmp = new CFEAttribute(ATTR_OUT);
        tmp->SetSourceLine(gLineNumber);
        $$ = new vector<CFEAttribute*>();
        $$->push_back(tmp);
    }
    | INOUT
    {
        CFEAttribute *tmpI = new CFEAttribute(ATTR_IN);
        CFEAttribute *tmpO = new CFEAttribute(ATTR_OUT);
        tmpI->SetSourceLine(gLineNumber);
        tmpO->SetSourceLine(gLineNumber);
        $$ = new vector<CFEAttribute*>();
        $$->push_back(tmpI);
        $$->push_back(tmpO);
    }
    | error
    {
        corbaerror2("expected parameter attribute, such as 'in', 'out', or 'inout'.");
        YYABORT;
    }
    ;

raises_expr :
      RAISES LPAREN scoped_name_list rparen
    { $$ = $3; }
    ;

scoped_name_list :
      scoped_name_list COMMA scoped_name
    {
            if ($3)
                $1->push_back($3);
        $$ = $1;
    }
    | scoped_name
    {
        $$ = new vector<CFEIdentifier*>();
    $$->push_back($1);
    }
    ;

context_expr :
      CONTEXT LPAREN string_literal_list rparen
    | /* empty context */
    ;

string_literal_list :
      string_literal_list COMMA LIT_STR
    {
        CFEIdentifier *tmp = new CFEIdentifier(*$3);
        tmp->SetSourceLine(gLineNumber);
        $1->push_back(tmp);
        $$ = $1;
        delete $3;
    }
    | LIT_STR
    {
        CFEIdentifier *tmp = new CFEIdentifier(*$1);
        tmp->SetSourceLine(gLineNumber);
        $$ = new vector<CFEIdentifier*>();
        $$->push_back(tmp);
        delete $1;
    }
    ;

param_type_spec :
      base_type_spec
    { $$ = $1; }
    | string_type
    { $$ = $1; }
    | wide_string_type
    { $$ = $1; }
    | scoped_name
    {
        $$ = new CFEUserDefinedType($1->GetName());
        $$->SetSourceLine(gLineNumber);
        //$1->SetParent($$);
        delete $1;
    }
    | predefined_type
    { $$ = $1; }
    | error
    {
        corbaerror2("expected a parameter type");
    }
    ;

predefined_type :
      FPAGE
    {
        $$ = new CFESimpleType(TYPE_FLEXPAGE);
        $$->SetSourceLine(gLineNumber);
    }
    | REFSTRING
    {
        $$ = new CFESimpleType(TYPE_REFSTRING);
        $$->SetSourceLine(gLineNumber);
    }
    ;

fixed_pt_type :
      FIXED LT positive_int_const COMMA positive_int_const GT
    {
        // use a fixed point type
        // I can think of a more stupid implementation
        // see CORBA language mapping for exact translation
        $$ = new CFESimpleType(TYPE_FLOAT);
        $$->SetSourceLine(gLineNumber);
    }
    ;

fixed_pt_const_type :
      FIXED
    {

    }
    ;

value_base_type :
       VALUEBASE
    {
        // ???
    }
    ;

semicolon
    : SEMICOLON
    | error
    {
        corbaerror2("expecting ';'");
        YYABORT;
    }
    ;

rbrace
    : RBRACE
    | error
    {
        corbaerror2("expecting '}'");
        YYABORT;
    }
    ;

rbracket
    : RBRACKET
    | error
    {
        corbaerror2("expecting ']'");
        YYABORT;
    }
    ;

rparen
    : RPAREN
    | error
    {
        corbaerror2("expecting ')'");
        YYABORT;
    }
    ;

colon
    : COLON
    | error
    {
        corbaerror2("expecting ':'");
        YYABORT;
    }
    ;

%%


void
corbaerror(char* s)
{
    // ignore this function because it is called by bison code, which we cannot control
    nParseErrorCORBA = 1;
}

void
corbaerror2(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    CCompiler::GccErrorVL(CParser::GetCurrentFile(), gLineNumber, fmt, args);
    va_end(args);
    nParseErrorCORBA = 0;
    erroccured = 1;
    errcount++;
}

void
corbawarning(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    CCompiler::GccWarningVL(CParser::GetCurrentFile(), gLineNumber, fmt, args);
    va_end(args);
    nParseErrorCORBA = 0;
}

