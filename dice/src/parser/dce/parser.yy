%{
/**
 *    \file    dice/src/parser/dce/parser.yy
 *    \brief   contains the parser for the DCE IDL
 *
 *    \date    06/01/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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
#include "Compiler.h"
#include "Messages.h"
#include "CParser.h"

#include <typeinfo>
#include <string>
#include <cassert>

void dceerror(const char *);
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
#include "fe/FEAttributeDeclarator.h"

#include "fe/FEStructType.h"
#include "fe/FEIDLUnionType.h"
#include "fe/FEEnumType.h"
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

#define YYDEBUG    1

// collection for elements
extern CFEFileComponent *pCurFileComponent;

// error stuff
extern int errcount;
extern int warningcount;
extern int erroccured;

extern int gLineNumber;

// indicate which TOKENS should be recognized as tokens
extern int c_inc;
extern int c_inc_old;

// global variables
int nParseErrorDCE = 0;

// helper functions
bool
DoInterfaceCheck(string *pIName, vector<CFEIdentifier*> *pBaseInterfaces, CFEInterface **pFEInterface);

bool
AddInterfaceToFileComponent(CFEInterface* pFEInterface, CFELibrary *pFELibrary);

%}

// we want a reentrant parser
%pure_parser

%union {
  string*            _id;
  string*            _str;
  long               _int;
  unsigned char      _byte;
  char               _char;
  float              _float;
  long double        _double;
  int                _bool;

  version_t          _version;

  CFEExpression*     _expr;
  CFEIdentifier*     _identifier;
  CFEDeclarator*     _decl;
  CFEArrayDeclarator*    _array_decl;
  CFEConstDeclarator*    _const_decl;
  CFETypedDeclarator*    _typed_decl;
  CFEFunctionDeclarator* _func_decl;
  CFEEnumDeclarator* _enum_decl;
  CFEAttributeDeclarator*_attr_decl;
  CFETypeSpec*       _type_spec;
  CFESimpleType*     _simple_type;
  CFEConstructedType*    _constructed_type;
  CFEStructType*     _tag_struct_type;
  CFEUnionType*      _tag_union_type;
  CFEUnionType*      _union_type;
  CFEUnionCase*      _union_case;
  CFEEnumType*       _enum_type;
  CFEAttribute*      _attr;
  CFETypeAttribute*  _type_attr;
  CFEVersionAttribute*   _version_attr;
  CFEIsAttribute*    _is_attr;
  CFEInterface*      _interface;
  CFEInterfaceComponent* _i_component;
  CFEOperation*      _operation;
  CFELibrary*        _library;
  CFEFileComponent*  _f_component;

  vector<PortSpec>*            _vec_PortSpec;
  vector<CFEIdentifier*>*      _vec_Identifier;
  vector<CFEDeclarator*>*      _vec_Declarator;
  vector<CFETypedDeclarator*>* _vec_TypedDecl;
  vector<CFEUnionCase*>*       _vec_UnionCase;
  vector<CFEExpression*>*      _vec_Expression;
  vector<CFEAttribute*>*       _vec_Attribute;
  vector<CFEFileComponent*>*   _vec_FileComponent;
  vector<CFEInterfaceComponent*>* _vec_InterfaceComponent;

  enum EXPT_OPERATOR _expr_operator;
}

%token LBRACE     RBRACE     LBRACKET    RBRACKET  COLON     COMMA
%token LPAREN     RPAREN     DOT         QUOT      ASTERISK  SINGLEQUOT
%token QUESTION   BITOR      BITXOR      BITAND    LT        GT
%token PLUS       MINUS      DIV         MOD       TILDE     EXCLAM
%token SEMICOLON  LOGICALOR  LOGICALAND  EQUAL     NOTEQUAL  LTEQUAL
%token GTEQUAL    LSHIFT     RSHIFT      DOTDOT    SCOPE

%token IS        BOOLEAN    BYTE       CASE      CHAR         CONST
%token DEFAULT   DOUBLE     ENUM       FALSE     FLOAT        HANDLE_T
%token HYPER     INT        INTERFACE  LONG      EXPNULL
%token PIPE      SHORT      SMALL      STRUCT    SWITCH       TRUE
%token TYPEDEF   UNION      UNSIGNED   SIGNED    VOID         ERROR_STATUS_T
%token FLEXPAGE  REFSTRING  OBJECT     IID_IS    ISO_LATIN_1  ISO_MULTI_LINGUAL
%token ISO_UCS   CHAR_PTR   VOID_PTR   LONGLONG

%token BROADCAST    CONTEXT_HANDLE     ENDPOINT    EXCEPTIONS   FIRST_IS      HANDLE
%token IDEMPOTENT   IGNORE             IN          LAST_IS      LENGTH_IS     LOCAL
%token MAX_IS       MAYBE              MIN_IS      OUT          PTR           POINTER_DEFAULT
%token REF          REFLECT_DELETIONS  SIZE_IS     STRING       SWITCH_IS
%token SWITCH_TYPE  TRANSMIT_AS        UNIQUE      UUID         VERSION_ATTR  RAISES
%token EXCEPTION    LIBRARY            CONTROL     HELPCONTEXT  HELPFILE      HELPSTRING
%token HIDDEN       LCID               RESTRICTED  AUTO_HANDLE  BINDING_CALLOUT
%token CODE         COMM_STATUS        CS_CHAR     CS_DRTAG     CS_RTAG       CS_STAG
%token TAG_RTN      ENABLE_ALLOCATE    EXTERN_EXCEPTIONS        EXPLICIT_HANDLE

/* DCE attribute tokens */
%token FAULT_STATUS      HEAP            IMPLICIT_HANDLE  NOCODE
%token REPRESENT_AS      USER_MARSHAL    WITHOUT_USING_EXCEPTIONS  SERVER_PARAMETER
%token DEFAULT_FUNCTION  ERROR_FUNCTION  ERROR_FUNCTION_CLIENT     ERROR_FUNCTION_SERVER
%token INIT_RCVSTRING    INIT_RCVSTRING_CLIENT       INIT_RCVSTRING_SERVER    INIT_WITH_IN
%token PREALLOC          ABSTRACT        INOUT       MODULE      READONLY     ALLOW_REPLY_ONLY
%token ONEWAY            CALLBACK	PREALLOC_CLIENT	PREALLOC_SERVER

/* dice specific token */
%token EOF_TOKEN    NOOPCODE    NOEXCEPTIONS
%token SCHED_DONATE DEDICATED_PARTNER DEFAULT_TIMEOUT

%token <_id>     ID
%token <_id>     TYPENAME
%token <_int>    LIT_INT
%token <_char>   LIT_CHAR
%token <_str>    LIT_STR
%token <_str>    UUID_STR
%token <_double> LIT_FLOAT
%token <_str>    PORTSPEC
%token <_str>    VERSION_STR

%type <_expr>                 additive_expr
%type <_expr>                 and_expr
%type <_expr>                 array_bound
%type <_array_decl>           array_declarator
%type <_decl>                 attr_var
%type <_vec_Declarator>       attr_var_list
%type <_attr_decl>            attribute_decl
%type <_vec_Identifier>       base_interface_list
%type <_id>                   base_interface_name
%type <_simple_type>          base_type_spec
%type <_simple_type>          boolean_type
%type <_expr>                 cast_expr
%type <_simple_type>          char_type
%type <_expr>                 conditional_expr
%type <_constructed_type>     constructed_type_spec
%type <_const_decl>           const_declarator
%type <_expr>                 const_expr
%type <_vec_Expression>       const_expr_list
%type <_type_spec>            const_type_spec
%type <_decl>                 declarator
%type <_vec_Declarator>       declarator_list
%type <_attr>                 directional_attribute
%type <_decl>                 direct_declarator
%type <_enum_decl>            enumeration_declarator
%type <_vec_Identifier>       enumeration_declarator_list
%type <_enum_type>            enumeration_type
%type <_enum_type>            tagged_enumeration_type
%type <_expr>                 equality_expr
%type <_typed_decl>           exception_declarator
%type <_vec_Identifier>       excep_name_list
%type <_expr>                 exclusive_or_expr
%type <_i_component>          export
%type <_attr>                 field_attribute
%type <_vec_Attribute>        field_attributes
%type <_vec_Attribute>        field_attribute_list
%type <_typed_decl>           field_declarator
%type <_simple_type>          floating_pt_type
%type <_func_decl>            function_declarator
%type <_vec_Identifier>       identifier_list
%type <_str>                  scoped_name
%type <_id>                   id_or_typename
%type <_expr>                 inclusive_or_expr
%type <_int>                  integer_size
%type <_simple_type>          integer_type
%type <_interface>            interface
%type <_attr>                 interface_attribute
%type <_vec_Attribute>        interface_attributes
%type <_vec_Attribute>        interface_attribute_list
%type <_attr>                 lib_attribute
%type <_vec_Attribute>        lib_attribute_list
%type <_f_component>          lib_definition
%type <_vec_FileComponent>    lib_definitions
%type <_library>              library
%type <_expr>                 logical_and_expr
%type <_expr>                 logical_or_expr
%type <_typed_decl>           member
%type <_vec_TypedDecl>        member_list
%type <_vec_TypedDecl>        member_list_1
%type <_expr>                 multiplicative_expr
%type <_attr>                 operation_attribute
%type <_vec_Attribute>        operation_attributes
%type <_vec_Attribute>        operation_attribute_list
%type <_operation>            op_declarator
%type <_attr>                 param_attribute
%type <_vec_Attribute>        param_attributes
%type <_vec_Attribute>        param_attribute_list
%type <_typed_decl>           param_declarator
%type <_vec_TypedDecl>        param_declarators
%type <_vec_TypedDecl>        param_declarator_list
%type <_int>                  pointer
%type <_vec_PortSpec>         port_specs
%type <_expr>                 postfix_expr
%type <_type_spec>            predefined_type_spec
%type <_expr>                 primary_expr
%type <_attr>                 ptr_attr
%type <_vec_Identifier>       raises_declarator
%type <_expr>                 relational_expr
%type <_expr>                 shift_expr
%type <_type_spec>            simple_type_spec
%type <_str>                  string
%type <_type_spec>            switch_type_spec
%type <_str>                  tag
%type <_constructed_type>     tagged_declarator
%type <_tag_struct_type>      tagged_struct_declarator
%type <_tag_union_type>       tagged_union_declarator
%type <_attr>                 type_attribute
%type <_vec_Attribute>        type_attributes
%type <_vec_Attribute>        type_attribute_list
%type <_typed_decl>           type_declarator
%type <_type_spec>            type_spec
%type <_expr>                 unary_expr
%type <_typed_decl>           union_arm
%type <_vec_UnionCase>        union_body
%type <_vec_UnionCase>        union_body_n_e
%type <_union_case>           union_case
%type <_expr>                 union_case_label
%type <_vec_Expression>       union_case_label_list
%type <_union_case>           union_case_n_e
%type <_is_attr>              union_instance_switch_attr
%type <_union_type>           union_type
%type <_union_type>           union_type_header
%type <_type_attr>            union_type_switch_attr
%type <_attr>                 usage_attribute
//%type <_str>                uuid_rep
%type <_version>              version_rep
%type <_expr_operator>        unary_operator

/******************************************************************************
 * GCC specific tokens
 ******************************************************************************/

%token    INC_OP    DEC_OP        TYPEOF    ALIGNOF
%token    ATTRIBUTE            SIZEOF

%token    RS_ASSIGN    LS_ASSIGN    ADD_ASSIGN    SUB_ASSIGN    MUL_ASSIGN    DIV_ASSIGN
%token    MOD_ASSIGN    AND_ASSIGN    XOR_ASSIGN    OR_ASSIGN    PTR_OP


/******************************************************************************
 * end GCC specific tokens
 ******************************************************************************/

%left COMMA SEMICOLON
%right PLUS MINUS
%left LOGICALAND LOGICALOR BITAND BITOR
%left LSHIFT RSHIFT

%right LPAREN
%left RPAREN

%start file
%%

file:
      file_component_list
    | /* a totally empty file */
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
            CMessages::GccError(NULL, 0, "Fatal Error: current file vanished (typedef)");
            YYABORT;
        }
        if (pCurFileComponent)
        {
            if (dynamic_cast<CFELibrary*>(pCurFileComponent))
                ((CFELibrary*)pCurFileComponent)->m_Typedefs.Add($1);
            else if (dynamic_cast<CFEInterface*>(pCurFileComponent))
                ((CFEInterface*)pCurFileComponent)->m_Typedefs.Add($1);
            else
            {
		CMessages::GccError(NULL, 0, 
		"current file component is unknown type: %s\n",
		typeid(*pCurFileComponent).name());
                assert(false);
            }
        }
        else
            CParser::GetCurrentFile()->m_Typedefs.Add($1);
    }
    | const_declarator semicolon
    {
        if (!CParser::GetCurrentFile())
        {
            CMessages::GccError(NULL, 0, "Fatal Error: current file vanished (const)");
            YYABORT;
        }
        else
            CParser::GetCurrentFile()->m_Constants.Add($1);
    }
    | tagged_declarator semicolon
    {
        if (!CParser::GetCurrentFile())
        {
            CMessages::GccError(NULL, 0, "Fatal Error: current file vanished (struct/union)");
            YYABORT;
        }
        else
            CParser::GetCurrentFile()->m_TaggedDeclarators.Add($1);
    }
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

interface:
      interface_attributes INTERFACE ID COLON base_interface_list LBRACE
    {
        // test interfaces
        CFEInterface* pFEInterface;
        if (!DoInterfaceCheck($3, $5, &pFEInterface))
            YYABORT;
        // create interface and set as file-component
        if (!pFEInterface)
            pFEInterface = new CFEInterface($1, *$3, $5, 
	        pCurFileComponent ? static_cast<CFEBase*>(pCurFileComponent) :
		CParser::GetCurrentFile());
        else
        {
            // add attributes, and base names
            pFEInterface->m_Attributes.Add($1);
            pFEInterface->m_BaseInterfaceNames.Add($5);
        }
        delete $1;
        pFEInterface->SetSourceLine(gLineNumber);
        if (!AddInterfaceToFileComponent(pFEInterface, NULL))
            YYABORT;
        pCurFileComponent = pFEInterface;
    } interface_component_list RBRACE
    {
        if (dynamic_cast<CFEInterface*>(pCurFileComponent))
            pCurFileComponent->SetSourceLineEnd(gLineNumber);
        // imported elements are already added to respective file object
        // adjust current file component
        if (pCurFileComponent &&
            pCurFileComponent->GetParent())
        {
	    pCurFileComponent = 
	        dynamic_cast<CFEFileComponent*>(pCurFileComponent->GetParent());
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
            pFEInterface = new CFEInterface($1, *$3, $5, 
	        pCurFileComponent ? static_cast<CFEBase*>(pCurFileComponent) :
		CParser::GetCurrentFile());
        else
        {
            // add attributes, and base names
            pFEInterface->m_Attributes.Add($1);
            pFEInterface->m_BaseInterfaceNames.Add($5);
        }
        delete $1;
        pFEInterface->SetSourceLine(gLineNumber);
        if (!AddInterfaceToFileComponent(pFEInterface, NULL))
            YYABORT;
    } RBRACE
    {
        if (dynamic_cast<CFEInterface*>(pCurFileComponent))
            pCurFileComponent->SetSourceLineEnd(gLineNumber);
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
            pFEInterface = new CFEInterface($1, *$3, NULL, 
	        pCurFileComponent ? static_cast<CFEBase*>(pCurFileComponent) :
		CParser::GetCurrentFile());
        else
        {
            // add attributes
            pFEInterface->m_Attributes.Add($1);
        }
        delete $1;
        pFEInterface->SetSourceLine(gLineNumber);
        if (!AddInterfaceToFileComponent(pFEInterface, NULL))
            YYABORT;
        pCurFileComponent = pFEInterface;
    } interface_component_list RBRACE
    {
        if (dynamic_cast<CFEInterface*>(pCurFileComponent))
            pCurFileComponent->SetSourceLineEnd(gLineNumber);
        // imported elements already added to respective file objects
        // adjust current file component
        if (pCurFileComponent &&
            pCurFileComponent->GetParent())
        {
            if (dynamic_cast<CFEFileComponent*>(pCurFileComponent->GetParent()))
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
            pFEInterface = new CFEInterface($1, *$3, NULL, 
	        pCurFileComponent ? static_cast<CFEBase*>(pCurFileComponent) :
		CParser::GetCurrentFile());
        else
        {
            // add attributes
            pFEInterface->m_Attributes.Add($1);
        }
        delete $1;
        pFEInterface->SetSourceLine(gLineNumber);
        if (!AddInterfaceToFileComponent(pFEInterface, NULL))
            YYABORT;
    } RBRACE
    {
        if (dynamic_cast<CFEInterface*>(pCurFileComponent))
            pCurFileComponent->SetSourceLineEnd(gLineNumber);
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
        CFEFile *pRoot = dynamic_cast<CFEFile*>(CParser::GetCurrentFile()->GetRoot());
        string::size_type nScopePos = string::npos;
        CFELibrary *pFELibrary = NULL;
        string sRest = *$3;
        while ((nScopePos = sRest.find("::")) != string::npos)
        {
            string sScope = sRest.substr(0, nScopePos);
            sRest = sRest.substr(nScopePos+2);
            if (!sScope.empty())
            {
                if (pFELibrary)
                    pFELibrary = pFELibrary->FindLibrary(sScope);
                else
                    pFELibrary = pRoot->FindLibrary(sScope);
                if (!pFELibrary)
                {
                    dceerror2("Cannot find library %s, which is used in forward declaration of %s.", sScope.c_str(), (*$3).c_str());
                    YYABORT;
                }
            }
        }
        if (pFEInterface)
            delete pFEInterface;
        pFEInterface = new CFEInterface(NULL, sRest, NULL, 
	    pCurFileComponent ? static_cast<CFEBase*>(pCurFileComponent) :
	    CParser::GetCurrentFile());
        pFEInterface->SetSourceLine(gLineNumber);
        pFEInterface->SetSourceLineEnd(gLineNumber);
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
        if (!pFEInterface)
            pFEInterface = new CFEInterface(NULL, *$2, $4, 
	        pCurFileComponent ? static_cast<CFEBase*>(pCurFileComponent) :
		CParser::GetCurrentFile());
        else
            // add base names
            pFEInterface->m_BaseInterfaceNames.Add($4);
        pFEInterface->SetSourceLine(gLineNumber);
        if (!AddInterfaceToFileComponent(pFEInterface, NULL))
            YYABORT;
        pCurFileComponent = pFEInterface;
    } interface_component_list RBRACE
    {
        if (dynamic_cast<CFEInterface*>(pCurFileComponent))
            pCurFileComponent->SetSourceLineEnd(gLineNumber);
        // imported elements are already added to respective file object
        // adjust current file component
        if (pCurFileComponent &&
            pCurFileComponent->GetParent())
        {
            if (dynamic_cast<CFEFileComponent*>(pCurFileComponent->GetParent()))
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
        if (!pFEInterface)
            pFEInterface = new CFEInterface(NULL, *$2, NULL, 
	        pCurFileComponent ? static_cast<CFEBase*>(pCurFileComponent) :
		CParser::GetCurrentFile());
        pFEInterface->SetSourceLine(gLineNumber);
        if (!AddInterfaceToFileComponent(pFEInterface, NULL))
            YYABORT;
        pCurFileComponent = pFEInterface;
    } interface_component_list RBRACE
    {
        if (dynamic_cast<CFEInterface*>(pCurFileComponent))
            pCurFileComponent->SetSourceLineEnd(gLineNumber);
        // imported elements already added to respective file objects
        // adjust current file component
        if (pCurFileComponent &&
            pCurFileComponent->GetParent())
        {
            if (dynamic_cast<CFEFileComponent*>(pCurFileComponent->GetParent()))
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
            pFEInterface = new CFEInterface(NULL, *$2, $4, 
	        pCurFileComponent ? static_cast<CFEBase*>(pCurFileComponent) :
		CParser::GetCurrentFile());
        else
        {
            // add attributes, and base names
            pFEInterface->m_BaseInterfaceNames.Add($4);
        }
        pFEInterface->SetSourceLine(gLineNumber);
        if (!AddInterfaceToFileComponent(pFEInterface, NULL))
            YYABORT;
    } RBRACE
    {
        if (dynamic_cast<CFEInterface*>(pCurFileComponent))
            pCurFileComponent->SetSourceLineEnd(gLineNumber);
        delete $2;
        $$ = NULL;
    }
    | INTERFACE ID LBRACE
    {
        CFEInterface* pFEInterface;
        if (!DoInterfaceCheck($2, NULL, &pFEInterface))
            YYABORT;
        // create interface and set as file-component
        if (!pFEInterface)
            pFEInterface = new CFEInterface(NULL, *$2, NULL, 
	        pCurFileComponent ? static_cast<CFEBase*>(pCurFileComponent) :
		CParser::GetCurrentFile());
        pFEInterface->SetSourceLine(gLineNumber);
        if (!AddInterfaceToFileComponent(pFEInterface, NULL))
            YYABORT;
    } RBRACE
    {
        if (dynamic_cast<CFEInterface*>(pCurFileComponent))
            pCurFileComponent->SetSourceLineEnd(gLineNumber);
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
        CFEFile *pRoot = dynamic_cast<CFEFile*>(CParser::GetCurrentFile()->GetRoot());
        string::size_type nScopePos = string::npos;
        CFELibrary *pFELibrary = NULL;
        string sRest = *$2;
        while ((nScopePos = sRest.find("::")) != string::npos)
        {
            string sScope = sRest.substr(0, nScopePos);
            sRest = sRest.substr(nScopePos+2);
            if (!sScope.empty())
            {
                if (pFELibrary)
                    pFELibrary = pFELibrary->FindLibrary(sScope);
                else
                    pFELibrary = pRoot->FindLibrary(sScope);
                if (!pFELibrary)
                {
                    dceerror2("Cannot find library %s, which is used in forward declaration of %s.", sScope.c_str(), (*$2).c_str());
                    YYABORT;
                }
            }
        }
        if (pFEInterface)
            delete pFEInterface;
        pFEInterface = new CFEInterface(NULL, sRest, NULL, 
	    pCurFileComponent ? static_cast<CFEBase*>(pCurFileComponent) :
	    CParser::GetCurrentFile());
        pFEInterface->SetSourceLine(gLineNumber);
        pFEInterface->SetSourceLineEnd(gLineNumber);
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
        $$ = new vector<CFEIdentifier*>();
        $$->push_back(tmp);
        delete $1;
    }
    | base_interface_list COMMA base_interface_name
    {
        CFEIdentifier *tmp = new CFEIdentifier(*$3);
        tmp->SetSourceLine(gLineNumber);
        $1->push_back(tmp);
        $$ = $1;
        delete $3;
    }
    ;

base_interface_name:
      scoped_name { $$ = $1; }
    | TYPENAME
    | error
    {
        dceerror2("expected interface name");
	YYABORT;
    }
    ;

interface_attributes :
      { if (c_inc == 0) c_inc = 2; } LBRACKET interface_attribute_list { if (c_inc == 2) c_inc = 0; } RBRACKET { $$ = $3; }
    | { if (c_inc == 0) c_inc = 2; } LBRACKET { if (c_inc == 2) c_inc = 0; } RBRACKET
    {
        $$ = new vector<CFEAttribute*>();
    }
    ;

interface_attribute_list :
      interface_attribute_list COMMA interface_attribute
    {
        if ($3)
            $1->push_back($3);
        $$ = $1;
    }
    | interface_attribute
    {
        $$ = new vector<CFEAttribute*>();
        if ($1)
            $$->push_back($1);
    }
    | error
    {
        dceerror2("unknown attribute");
        YYABORT;
    }
    ;

interface_attribute :
/*      UUID LPAREN uuid_rep rparen
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
	$$->SetSourceLine(gLineNumber);
	delete $3;
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
        dcewarning("Attribute [server_parameter] is deprecated.");
        $$ = NULL;
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
    | INIT_RCVSTRING_CLIENT LPAREN ID rparen
    {
        $$ = new CFEStringAttribute(ATTR_INIT_RCVSTRING_CLIENT, *$3);
        delete $3;
        $$->SetSourceLine(gLineNumber);
    }
    | INIT_RCVSTRING_SERVER LPAREN ID rparen
    {
        $$ = new CFEStringAttribute(ATTR_INIT_RCVSTRING_SERVER, *$3);
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
    | ERROR_FUNCTION_CLIENT LPAREN ID rparen
    {
        $$ = new CFEStringAttribute(ATTR_ERROR_FUNCTION_CLIENT, *$3);
        delete $3;
        $$->SetSourceLine(gLineNumber);
    }
    | ERROR_FUNCTION_SERVER LPAREN ID rparen
    {
        $$ = new CFEStringAttribute(ATTR_ERROR_FUNCTION_SERVER, *$3);
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
    | DEDICATED_PARTNER
    {
        $$ = new CFEAttribute(ATTR_DEDICATED_PARTNER);
        $$->SetSourceLine(gLineNumber);
    }
    | DEFAULT_TIMEOUT
    {
        $$ = new CFEAttribute(ATTR_DEFAULT_TIMEOUT);
	$$->SetSourceLine(gLineNumber);
    }
    ;

version_rep :
      LIT_INT DOT LIT_INT
    {
        $$.nMajor = $1;
        $$.nMinor = $3;
    }
    | LIT_INT COMMA LIT_INT
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
        $$.nMajor = atoi($1->substr(0, $1->find('.')).c_str());
        $$.nMinor = atoi($1->substr($1->find('.')).c_str());
        delete $1;
    }
    ;


port_specs :
      port_specs COMMA PORTSPEC
    {
        PortSpec tmp;
        tmp.sFamily = *$3;
        $1->push_back(tmp);
        $$ = $1;
    }
    | PORTSPEC
    {
        PortSpec tmp;
        tmp.sFamily = *$1;
        $$ = new vector<PortSpec>();
        $$->push_back(tmp);
    }
    ;

excep_name_list :
      excep_name_list COMMA ID
    {
        CFEIdentifier *tmp = new CFEIdentifier(*$3);
        tmp->SetSourceLine(gLineNumber);
        $1->push_back(tmp);
        $$ = $1;
        delete $3;
    }
    | ID
    {
        CFEIdentifier *tmp = new CFEIdentifier(*$1);
        tmp->SetSourceLine(gLineNumber);
    	$$ = new vector<CFEIdentifier*>();
    	$$->push_back(tmp);
        delete $1;
    }
    ;

interface_component_list :
      interface_component interface_component_list
    | interface_component
    ;

interface_component :
      export semicolon
    {
        // ignore argument:
	// typedef, const decl, tagged decl has been added
	// ignore excpetions for now
	if ($1)
	    delete $1;
    }
    | op_declarator semicolon
    {
        // should not return anything: already added
	if ($1)
	    delete $1;
    }
    | attribute_decl semicolon
    {
        // should not return anything: already added
	if ($1)
	    delete $1;
    }
    ;

export :
      type_declarator
    {
    	$$ = NULL;
        if (pCurFileComponent)
        {
            if (dynamic_cast<CFELibrary*>(pCurFileComponent))
                ((CFELibrary*)pCurFileComponent)->m_Typedefs.Add($1);
            else if (dynamic_cast<CFEInterface*>(pCurFileComponent))
                ((CFEInterface*)pCurFileComponent)->m_Typedefs.Add($1);
            else
            {
		CMessages::GccError(NULL, 0, 
		"current file component is unknown type: %s\n",
		    typeid(*pCurFileComponent).name());
                assert(false);
            }
        }
        else
            CParser::GetCurrentFile()->m_Typedefs.Add($1);
    }
    | const_declarator
    {
        $$ = NULL;
        if (pCurFileComponent)
        {
            if (dynamic_cast<CFELibrary*>(pCurFileComponent))
                ((CFELibrary*)pCurFileComponent)->m_Constants.Add($1);
            else if (dynamic_cast<CFEInterface*>(pCurFileComponent))
                ((CFEInterface*)pCurFileComponent)->m_Constants.Add($1);
            else
            {
		CMessages::GccError(NULL, 0, 
                    "current file component is unknown type: %s\n", 
		    typeid(*pCurFileComponent).name());
                assert(false);
            }
        }
        else
            CParser::GetCurrentFile()->m_Constants.Add($1);
    }
    | tagged_declarator
    {
        $$ = NULL;
        if (pCurFileComponent)
        {
            if (dynamic_cast<CFELibrary*>(pCurFileComponent))
                ((CFELibrary*)pCurFileComponent)->m_TaggedDeclarators.Add($1);
            else if (dynamic_cast<CFEInterface*>(pCurFileComponent))
                ((CFEInterface*)pCurFileComponent)->m_TaggedDeclarators.Add($1);
            else
            {
		CMessages::GccError(NULL, 0, 
		    "current file component is unknown type: %s\n",
		    typeid(*pCurFileComponent).name());
                assert(false);
            }
        }
        else
            CParser::GetCurrentFile()->m_TaggedDeclarators.Add($1);
    }
    | exception_declarator 
    { 
	$$ = NULL;
	if (pCurFileComponent && 
	    dynamic_cast<CFEInterface*>(pCurFileComponent))
		((CFEInterface*)pCurFileComponent)->m_Exceptions.Add($1);
	else
	{
	    CMessages::GccError(NULL, 0, 
		"current file component is unknown type: %s\n",
		typeid(*pCurFileComponent).name());
	    assert(false);
	}
    }
    ;

const_declarator :
      CONST const_type_spec id_or_typename IS const_expr
    {
        CFETypeSpec *pType = $2;
            while (pType && (pType->GetType() == TYPE_USER_DEFINED))
        {
            string sTypeName = ((CFEUserDefinedType*)pType)->GetName();
            CFETypedDeclarator *pTypedef = CParser::GetCurrentFile()->FindUserDefinedType(sTypeName);
            if (!pTypedef)
                dceerror2("Cannot find type for \"%s\".", sTypeName.c_str());
            pType = pTypedef->GetType();
        }
        if (!( $5->IsOfType($2->GetType()) )) {
            dceerror2("Const type of \"%s\" does not match with expression.", (*$3).c_str());
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

const_type_spec :
      integer_type { $$ = $1; }
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
            $1->push_back($3);
        $$ = $1;
    }
    | const_expr
    {
        $$ = new vector<CFEExpression*>();
        $$->push_back($1);
    }
    ;

const_expr :
      conditional_expr
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

conditional_expr :
      logical_or_expr
    /* gcc specifics */
    | logical_or_expr QUESTION const_expr_list COLON conditional_expr
    {
        // because the const_expr_list contains the conditional_expr
        // we have to evaluate the case, this can happend (and be IDL specific)
        // get first of const_expr_list
    vector<CFEExpression*>::iterator iter;
    CFEExpression *pExpr = NULL;
    if ($3)
    {
        iter = $3->begin();
        if (iter != $3->end())
            pExpr = *iter;
    }
        $$ = new CFEConditionalExpression($1, pExpr, $5);
        // set parent relationship
    pExpr->SetParent($$);
        $1->SetParent($$);
        $5->SetParent($$);
        $$->SetSourceLine(gLineNumber);
    delete $3;
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

logical_or_expr :
      logical_and_expr
    | logical_or_expr LOGICALOR logical_and_expr
    {
        $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LOGOR, $3);
        // set parent relationship
        $1->SetParent($$);
        $3->SetParent($$);
        $$->SetSourceLine(gLineNumber);
    }
    ;

logical_and_expr :
      inclusive_or_expr
    | logical_and_expr LOGICALAND inclusive_or_expr
    {
        $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_LOGAND, $3);
        // set parent relationship
        $1->SetParent($$);
        $3->SetParent($$);
        $$->SetSourceLine(gLineNumber);
    }
    ;

inclusive_or_expr :
      exclusive_or_expr
    | inclusive_or_expr BITOR exclusive_or_expr
    {
        $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITOR, $3);
        // set parent relationship
        $1->SetParent($$);
        $3->SetParent($$);
        $$->SetSourceLine(gLineNumber);
    }
    ;

exclusive_or_expr :
      and_expr
    | exclusive_or_expr BITXOR and_expr
    {
        $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITXOR, $3);
        // set parent relationship
        $1->SetParent($$);
        $3->SetParent($$);
        $$->SetSourceLine(gLineNumber);
    }
    ;

and_expr :
      equality_expr
    | and_expr BITAND equality_expr
    {
        $$ = new CFEBinaryExpression(EXPR_BINARY, $1, EXPR_BITAND, $3);
        // set parent relationship
        $1->SetParent($$);
        $3->SetParent($$);
        $$->SetSourceLine(gLineNumber);
    }
    ;

equality_expr :
      relational_expr
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
      shift_expr
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

shift_expr :
      additive_expr
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

additive_expr :
      multiplicative_expr
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
    /* unary_expr */
      cast_expr
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
    | unary_operator cast_expr
    {
        $$ = new CFEUnaryExpression(EXPR_UNARY, $1, $2);
        // set parent relationship
        $2->SetParent($$);
        $$->SetSourceLine(gLineNumber);
    }
    /* gcc specifics */
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
    ;

unary_operator :
      PLUS
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
    | postfix_expr LBRACKET const_expr_list RBRACKET
    {
        $$ = NULL;
    delete $3;
    }
    | postfix_expr LPAREN const_expr_list RPAREN
    {
        $$ = NULL;
    delete $3;
    }
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

primary_expr :
      LIT_INT
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
        $$ = new CFEPrimaryExpression(EXPR_FLOAT, atol($1->c_str()));
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
        CFEExpression *pExpr = NULL;
    if ($2)
    {
        vector<CFEExpression*>::iterator iter = $2->begin();
        if (iter != $2->end())
            pExpr = *iter;
    }
        if (pExpr != NULL)
    {
            $$ = new CFEPrimaryExpression(EXPR_PAREN, pExpr);
            pExpr->SetParent($$);
            $$->SetSourceLine(gLineNumber);
        }
    else
            $$ = NULL;
    }
    ;

string :
      LIT_STR
    | QUOT QUOT
    { $$ = NULL; }
    /* gcc specifics */
    | string LIT_STR
    {
        if ($1 != NULL)
        {
            $1->append(*$2);
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

type_declarator :
      TYPEDEF type_attribute_list type_spec declarator_list
    {
        // check if type_names already exist
        CParser *pCurParser = CParser::GetCurrentParser();
        // we can only test the import scope
        CFEFile *pRoot = pCurParser->GetTopFileInScope();
        assert(pRoot);
        vector<CFEDeclarator*>::iterator iter;
        for (iter = $4->begin(); iter != $4->end(); iter++)
        {
            if (*iter)
            {
                if (!(*iter)->GetName().empty())
                {
                    if (pRoot->FindUserDefinedType((*iter)->GetName()) != NULL)
                    {
                        dceerror2("\"%s\" has already been defined as type.",
			    (*iter)->GetName().c_str());
                        YYABORT;
                    }
                }
            }
        }
        $$ = new CFETypedDeclarator(TYPEDECL_TYPEDEF, $3, $4, $2);
        delete $2;
        $3->SetParent($$);
        delete $4;
        $$->SetSourceLine(gLineNumber);
    }
    | TYPEDEF type_spec declarator_list
    {
        // check if type_names already exist
        CParser *pCurParser = CParser::GetCurrentParser();
        // we can only test the import scope
        CFEFile *pRoot = pCurParser->GetTopFileInScope();
        assert(pRoot);
        vector<CFEDeclarator*>::iterator iter;
        for (iter = $3->begin(); iter != $3->end(); iter++)
        {
            if (*iter)
            {
                if (!(*iter)->GetName().empty())
                {
                    if (pRoot->FindUserDefinedType((*iter)->GetName()) != NULL)
                    {
                        dceerror2("\"%s\" has already been defined as type.",(*iter)->GetName().c_str());
                        YYABORT;
                    }
                }
            }
        }
        $$ = new CFETypedDeclarator(TYPEDECL_TYPEDEF, $2, $3);
        // set parent relationship
        $2->SetParent($$);
        delete $3;
        $$->SetSourceLine(gLineNumber);
    }
    ;

type_attribute_list :
      { if (c_inc == 0) c_inc = 2; } LBRACKET type_attributes { if (c_inc == 2) c_inc = 0; } rbracket { $$ = $3; }
    ;

type_spec :
      simple_type_spec
    | constructed_type_spec { $$ = $1; }
    ;

simple_type_spec :
      base_type_spec { $$ = $1; }
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
            $1->push_back($3);
        $$ = $1;
    }
    | declarator
    {
        $$ = new vector<CFEDeclarator*>();
        $$->push_back($1);
    }
    ;

declarator :
      pointer attribute_list direct_declarator
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

direct_declarator :
      ID
    {
        $$ = new CFEDeclarator(DECL_IDENTIFIER, *$1);
        delete $1;
        $$->SetSourceLine(gLineNumber);
    }
    | LPAREN declarator rparen
    { $$ = $2; }
    | array_declarator { $$ = $1; }
    | function_declarator { $$ = $1; }
    ;

tagged_declarator :
      tagged_struct_declarator { $$ = $1; }
    | tagged_union_declarator { $$ = $1; }
    | tagged_enumeration_type { $$ = $1; }
    ;

base_type_spec :
      floating_pt_type
    | integer_type
    | char_type
    | boolean_type
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
    | TYPEOF LPAREN const_expr_list RPAREN
    {
        if ($3)
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

integer_size :
      LONG { $$ = 4; }
    | LONGLONG { $$ = 8; }
    | SHORT { $$ = 2; }
    | SMALL { $$ = 1; }
    | HYPER { $$ = 8; }
    ;

char_type :
      UNSIGNED CHAR
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

boolean_type :
      BOOLEAN
    {
        $$ = new CFESimpleType(TYPE_BOOLEAN);
        $$->SetSourceLine(gLineNumber);
    }
    ;

constructed_type_spec :
      attribute_list STRUCT LBRACE member_list RBRACE
    {
        // the attribute list is GCC specific
        $$ = new CFEStructType(string(), $4);
        $$->SetSourceLine(gLineNumber);
        if ($4)
            delete $4;
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

tagged_struct_declarator :
      attribute_list STRUCT tag LBRACE member_list RBRACE
    {
        $$ = new CFEStructType(*$3, $5);
        delete $3;
        if ($5)
            delete $5;
        $$->SetSourceLine(gLineNumber);
    }
    | attribute_list STRUCT tag
    {
        $$ = new CFEStructType(*$3, 0);
        delete $3;
        $$->SetSourceLine(gLineNumber);
    }
    ;

tag :
      id_or_typename { $$ = $1; }
    ;

member_list :
      /* empty */
    { $$ = new vector<CFETypedDeclarator*>(); }
    | member_list_1
    | member_list_1 member /* no semicolon here; if semi: member SEMICOLON -> member_list_1 */
    {
        if ($2)
            $1->push_back($2);
        $$ = $1;
    }
    ;

member_list_1 :
      member_list_1 member SEMICOLON
    {
        if ($2)
            $1->push_back($2);
        $$ = $1;
    }
    | member SEMICOLON
    {
        $$ = new vector<CFETypedDeclarator*>();
        $$->push_back($1);
    }
    ;

member :
      field_declarator
    ;

field_declarator :
      field_attribute_list type_spec declarator_list
    {
        $$ = new CFETypedDeclarator(TYPEDECL_FIELD, $2, $3, $1);
        delete $1;
        $2->SetParent($$);
        delete $3;
        $$->SetSourceLine(gLineNumber);
    }
    | type_spec declarator_list
    {
        $$ = new CFETypedDeclarator(TYPEDECL_FIELD, $1, $2);
        // set parent relationship
        $1->SetParent($$);
        delete $2;
        $$->SetSourceLine(gLineNumber);
    }
    ;

field_attribute_list :
      { if (c_inc == 0) c_inc = 2; } LBRACKET field_attributes { if (c_inc == 2) c_inc = 0; } rbracket { $$ = $3; }
    ;

tagged_union_declarator :
      UNION tag
    {
        $$ = new CFEUnionType(*$2, 0);
        delete $2;
        $$->SetSourceLine(gLineNumber);
    }
    | UNION tag union_type_header
    {
        $$ = $3;
	$$->SetTag(*$2);
	delete $2;
        $$->SetSourceLine(gLineNumber);
    }
    ;

union_type :
      UNION union_type_header { $$ = $2; }
    ;

union_type_header :
     SWITCH LPAREN switch_type_spec id_or_typename rparen id_or_typename LBRACE union_body RBRACE
    {
        $$ = new CFEIDLUnionType(string(), $8, $3, *$4, *$6);
        // set parent relationship
        $3->SetParent($$);
        delete $4;
        delete $6;
        if ($8)
            delete $8;
        $$->SetSourceLine(gLineNumber);
    }
    | SWITCH LPAREN switch_type_spec id_or_typename rparen LBRACE union_body RBRACE
    {
        $$ = new CFEIDLUnionType(string(), $7, $3, *$4, string());
        // set parent relationship
        $3->SetParent($$);
        delete $4;
        if ($7)
            delete $7;
        $$->SetSourceLine(gLineNumber);
    }
    | LBRACE union_body_n_e RBRACE
    {
        $$ = new CFEUnionType(string(), $2);
        $$->SetSourceLine(gLineNumber);
        if ($2)
            delete $2;
    }
    ;


switch_type_spec :
      integer_type { $$ = $1; }
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
            $1->push_back($2);
        $$ = $1;
    }
    | union_case
    {
        $$ = new vector<CFEUnionCase*>();
        $$->push_back($1);
    }
    ;

union_body_n_e :
      union_body_n_e union_case_n_e
    {
        if ($2)
            $1->push_back($2);
        $$ = $1;
    }
    | union_case_n_e
    {
        $$ = new vector<CFEUnionCase*>();
        $$->push_back($1);
    }
    ;


union_case :
      union_case_label_list union_arm
    {
        $$ = new CFEUnionCase($2, $1);
        // set parent relationship
        $2->SetParent($$);
    delete $1;
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

union_case_n_e :
      LBRACKET CASE LPAREN const_expr_list rparen rbracket union_arm
    {
        $$ = new CFEUnionCase($7, $4);
        // set parent relationship
        $7->SetParent($$);
        delete $4;
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
            $1->push_back($2);
        $$ = $1;
    }
    | union_case_label
    {
        $$ = new vector<CFEExpression*>();
        $$->push_back($1);
    }
    ;

union_case_label :
      CASE const_expr colon { $$ = $2; }
    ;

union_arm :
      field_declarator semicolon { $$ = $1; }
    | SEMICOLON
    {
        $$ = new CFETypedDeclarator(TYPEDECL_VOID, 0, 0);
        $$->SetSourceLine(gLineNumber);
    }
    ;

union_type_switch_attr :
      SWITCH_TYPE LPAREN switch_type_spec rparen
    {
        $$ = new CFETypeAttribute(ATTR_SWITCH_TYPE, $3);
        // set parent relationship
        $3->SetParent($$);
        $$->SetSourceLine(gLineNumber);
    }
    ;

union_instance_switch_attr :
      SWITCH_IS LPAREN attr_var rparen
    {
        vector<CFEDeclarator*> *tmp = new vector<CFEDeclarator*>();
        tmp->push_back($3);
        $$ = new CFEIsAttribute(ATTR_SWITCH_IS, tmp);
        delete tmp;
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
        $1->push_back(tmp);
        $$ = $1;
        delete $3;
    }
    | scoped_name
    {
        CFEIdentifier *tmp = new CFEIdentifier(*$1);
        tmp->SetSourceLine(gLineNumber);
    $$ = new vector<CFEIdentifier*>();
    $$->push_back(tmp);
        delete $1;
    }
    ;

scoped_name :
      scoped_name SCOPE ID
    {
        $1->append("::");
        $1->append(*$3);
        $$ = $1;
        delete $3;
    }
    | scoped_name SCOPE TYPENAME
    {
        $1->append("::");
        $1->append(*$3);
        $$ = $1;
        delete $3;
    }
    | SCOPE ID { $$ = $2; }
    | SCOPE TYPENAME { $$ = $2; }
    | ID { $$ = $1; }
    ;


enumeration_type :
      ENUM LBRACE enumeration_declarator_list RBRACE
    {
        $$ = new CFEEnumType(string(), $3);
        $$->SetSourceLine(gLineNumber);
        delete $3;
    }
    ;

tagged_enumeration_type :
      ENUM id_or_typename LBRACE enumeration_declarator_list RBRACE
    {
        $$ = new CFEEnumType(*$2, $4);
        delete $2;
        $$->SetSourceLine(gLineNumber);
        delete $4;
    }
    | ENUM id_or_typename
    {
        $$ = new CFEEnumType(*$2, NULL);
        delete $2;
        $$->SetSourceLine(gLineNumber);
    }
    ;

enumeration_declarator_list :
      enumeration_declarator
    {
        $$ = new vector<CFEIdentifier*>();
        $$->push_back($1);
    }
    | enumeration_declarator_list COMMA enumeration_declarator
    {
        if ($3)
        $1->push_back($3);
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

array_declarator :
      direct_declarator LBRACKET RBRACKET
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

array_bound :
      ASTERISK
    {
        $$ = new CFEExpression(EXPR_CHAR, '*');
        $$->SetSourceLine(gLineNumber);
    }
    | const_expr
    ;

type_attributes :
      type_attributes COMMA type_attribute
    {
            if ($3)
                $1->push_back($3);
        $$ = $1;
    }
    | type_attribute
    {
        $$ = new vector<CFEAttribute*>();
        $$->push_back($1);
    }
    | error
    {
        dceerror2("unknown attribute");
        YYABORT;
    }
    ;

type_attribute :
      TRANSMIT_AS LPAREN simple_type_spec RPAREN
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
    | usage_attribute
    | union_type_switch_attr { $$ = $1; }
    | ptr_attr
    ;

usage_attribute :
      STRING
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
            $1->push_back($3);
        $$ = $1;
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
        $$->SetSourceLine(gLineNumber);
        delete $3;
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
        $$->SetSourceLine(gLineNumber);
        delete $3;
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
        $$->SetSourceLine(gLineNumber);
        delete $3;
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
        $$->SetSourceLine(gLineNumber);
        delete $3;
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
        $$->SetSourceLine(gLineNumber);
        delete $3;
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
        $$->SetSourceLine(gLineNumber);
        delete $3;
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
    | usage_attribute
    | union_instance_switch_attr { $$ = $1; }
    | IGNORE
    {
        $$ = new CFEAttribute(ATTR_IGNORE);
        $$->SetSourceLine(gLineNumber);
    }
    | ptr_attr
    ;


attr_var_list :
      attr_var_list COMMA attr_var
    {
            if ($3)
                $1->push_back($3);
        $$ = $1;
    }
    | attr_var
    {
        $$ = new vector<CFEDeclarator*>();
        $$->push_back($1);
    }
    ;

attr_var :
      ASTERISK id_or_typename
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

ptr_attr :
      REF
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
        $$->SetSourceLine(gLineNumber);
        delete $3;
    }
    ;

pointer :
      ASTERISK pointer
    { $$ = $2 + 1; }
    | ASTERISK
    { $$ = 1; }
    ;

op_declarator :
      operation_attributes simple_type_spec id_or_typename LPAREN param_declarators RPAREN raises_declarator
    {
        $$ = NULL;
	CFEOperation *pOp = new CFEOperation($2, *$3, $5, $1, $7);
        // set parent relationship
        $2->SetParent(pOp);
        if ($5)
            delete $5;
        delete $3;
        delete $1;
        delete $7;
        pOp->SetSourceLine(gLineNumber);
        if (pCurFileComponent)
        {
            if (dynamic_cast<CFEInterface*>(pCurFileComponent))
                ((CFEInterface*)pCurFileComponent)->m_Operations.Add(pOp);
            else
            {
		CMessages::GccError(NULL, 0,
                    "current file component is unknown type: %s\n",
		    typeid(*pCurFileComponent).name());
                assert(false);
            }
        }
        else
        {
            dceerror2("Cannot add operation to file without interface\n");
            YYABORT;
        }
    }
    | operation_attributes simple_type_spec id_or_typename LPAREN param_declarators RPAREN
    {
        $$ = NULL;
	CFEOperation *pOp = new CFEOperation($2, *$3, $5, $1);
        // set parent relationship
        $2->SetParent(pOp);
        if ($5)
            delete $5;
        delete $3;
        delete $1;
        pOp->SetSourceLine(gLineNumber);
        if (pCurFileComponent)
        {
            if (dynamic_cast<CFEInterface*>(pCurFileComponent))
                ((CFEInterface*)pCurFileComponent)->m_Operations.Add(pOp);
            else
            {
	    	CMessages::GccError(NULL, 0, 
		    "current file component is unknown type: %s\n",
		    typeid(*pCurFileComponent).name());
                assert(false);
            }
        }
        else
        {
            dceerror2("Cannot add operation to file without interface\n");
            YYABORT;
        }
    }
    | simple_type_spec id_or_typename LPAREN param_declarators RPAREN raises_declarator
    {
        $$ = NULL;
	CFEOperation *pOp = new CFEOperation($1, *$2, $4, 0, $6);
        // set parent relationship
        $1->SetParent(pOp);
        if ($4)
            delete $4;
        delete $2;
        delete $6;
        pOp->SetSourceLine(gLineNumber);
        if (pCurFileComponent)
        {
            if (dynamic_cast<CFEInterface*>(pCurFileComponent))
                ((CFEInterface*)pCurFileComponent)->m_Operations.Add(pOp);
            else
            {
		CMessages::GccError(NULL, 0, 
		    "current file component is unknown type: %s\n",
		    typeid(*pCurFileComponent).name());
                assert(false);
            }
        }
        else
        {
            dceerror2("Cannot add operation to file without interface\n");
            YYABORT;
        }
    }
    | simple_type_spec id_or_typename LPAREN param_declarators RPAREN
    {
        $$ = NULL;
	CFEOperation *pOp = new CFEOperation($1, *$2, $4);
        // set parent relationship
        $1->SetParent(pOp);
        if ($4)
            delete $4;
        delete $2;
        pOp->SetSourceLine(gLineNumber);
        if (pCurFileComponent)
        {
            if (dynamic_cast<CFEInterface*>(pCurFileComponent))
                ((CFEInterface*)pCurFileComponent)->m_Operations.Add(pOp);
            else
            {
		CMessages::GccError(NULL, 0,
		    "current file component is unknown type: %s\n",
		    typeid(*pCurFileComponent).name());
                assert(false);
            }
        }
        else
        {
            dceerror2("Cannot add operation to file without interface\n");
            YYABORT;
        }
    }
    ;

id_or_typename:
      ID
    | TYPENAME
    ;

raises_declarator :
      RAISES LPAREN identifier_list rparen { $$ = $3; }
    ;

operation_attributes :
      { if (c_inc == 0) c_inc = 2; } LBRACKET operation_attribute_list { if (c_inc == 2) c_inc = 0; } rbracket { $$ = $3; }
    ;

operation_attribute_list :
      operation_attribute_list COMMA operation_attribute
    {
            if ($3)
                $1->push_back($3);
        $$ = $1;
    }
    | operation_attribute
    {
        $$ = new vector<CFEAttribute*>();
        $$->push_back($1);
    }
    ;

operation_attribute :
      IDEMPOTENT
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
    | ptr_attr
    | directional_attribute
    | ONEWAY
    {
        $$ = new CFEAttribute(ATTR_IN);
        $$->SetSourceLine(gLineNumber);
    }
    | CALLBACK
    {
        $$ = new CFEAttribute(ATTR_OUT);
        $$->SetSourceLine(gLineNumber);
    }
    | NOOPCODE
    {
        $$ = new CFEAttribute(ATTR_NOOPCODE);
        $$->SetSourceLine(gLineNumber);
    }
    | NOEXCEPTIONS
    {
        $$ = new CFEAttribute(ATTR_NOEXCEPTIONS);
        $$->SetSourceLine(gLineNumber);
    }
    | ALLOW_REPLY_ONLY
    {
        $$ = new CFEAttribute(ATTR_ALLOW_REPLY_ONLY);
        $$->SetSourceLine(gLineNumber);
    }
    /* L4 scheduling attributes */
    | SCHED_DONATE
    {
        $$ = new CFEAttribute(ATTR_SCHED_DONATE);
        $$->SetSourceLine(gLineNumber);
    }
    | DEFAULT_TIMEOUT
    {
    	$$ = new CFEAttribute(ATTR_DEFAULT_TIMEOUT);
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
    ;

param_declarator_list :
      param_declarator
    {
	if ($1)
	{
	    $$ = new vector<CFETypedDeclarator*>();
	    $$->push_back($1);
	}
	else
	{
	    dcewarning("invalid parameter (probably C style parameter)");
	    $$ = new vector<CFETypedDeclarator*>();
	}
    }
    | param_declarator_list COMMA param_declarator
    {
        if ($3)
            $1->push_back($3);
        else
            dcewarning("invalid parameter (probably C style parameter)");
        $$ = $1;
    }
    ;

param_declarator :
      param_attributes type_spec declarator
    {
	if ($2 != 0)
	{
	    vector<CFEDeclarator*> *tmp = new vector<CFEDeclarator*>();
	    tmp->push_back($3);
	    $$ = new CFETypedDeclarator(TYPEDECL_PARAM, $2, tmp, $1);
	    delete $1;
	    $2->SetParent($$);
	    $3->SetParent($$);
	    delete tmp;
	    $$->SetSourceLine(gLineNumber);
	}
	else
	    $$ = 0;
    }
    | type_spec declarator
    {
        if ($1 != 0)
        {
            CFEAttribute *tmpA = new CFEAttribute(ATTR_NONE);
            tmpA->SetSourceLine(gLineNumber);
            vector<CFEAttribute*> *tmpVA = new vector<CFEAttribute*>();
            tmpVA->push_back(tmpA);
            vector<CFEDeclarator*> *tmpVD = new vector<CFEDeclarator*>();
            tmpVD->push_back($2);
            $$ = new CFETypedDeclarator(TYPEDECL_PARAM, $1, tmpVD, tmpVA);
            tmpA->SetParent($$);
            $1->SetParent($$);
            $2->SetParent($$);
            delete tmpVA;
            delete tmpVD;
            $$->SetSourceLine(gLineNumber);
        }
        else
            $$ = 0;
    }
    ;

param_attributes :
      { if (c_inc == 0) c_inc = 2; } LBRACKET param_attribute_list { if (c_inc == 2) c_inc = 0; } rbracket { $$ = $3; }
    | { if (c_inc == 0) c_inc = 2; } LBRACKET { if (c_inc == 2) c_inc = 0; } RBRACKET
    {
        CFEAttribute *tmp = new CFEAttribute(ATTR_NONE);
        tmp->SetSourceLine(gLineNumber);
        $$ = new vector<CFEAttribute*>();
        $$->push_back(tmp);
    }
    ;

param_attribute_list :
      param_attribute_list COMMA param_attribute
    {
        if ($3)
            $1->push_back($3);
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
        $1->push_back(pAttr);
        // OUT attribute
        pAttr = new CFEAttribute(ATTR_OUT);
        pAttr->SetSourceLine(gLineNumber);
        $1->push_back(pAttr);
        // return vector
        $$ = $1;
    }
    | param_attribute
    {
	$$ = new vector<CFEAttribute*>();
	if ($1)
	    $$->push_back($1);
    }
    | INOUT
    {
        // have to place it here, because we have to return
        // a vector of two attributes
        dcewarning("Usage of [inout] is discouraged. Use [in, out] instead.");
        // IN attribute
        CFEAttribute *pAttr =  new CFEAttribute(ATTR_IN);
        pAttr->SetSourceLine(gLineNumber);
        $$ = new vector<CFEAttribute*>();
        $$->push_back(pAttr);
        // OUT attribute
        pAttr = new CFEAttribute(ATTR_OUT);
        pAttr->SetSourceLine(gLineNumber);
        $$->push_back(pAttr);
    }
    | error
    {
        dceerror2("unknown attribute");
        YYABORT;
    }
    ;

param_attribute :
      directional_attribute
    | field_attribute
    | INIT_WITH_IN
    {
    	dceerror2("[init_with_in] is deprecated. Use [prealloc_client] and/or [prealloc_server].");
	YYABORT;
    }
    | PREALLOC
    {
    	dceerror2("[prealloc] is deprecated. Use [prealloc_client] and/or [prealloc_server].");
	YYABORT;
    }
    | PREALLOC_CLIENT
    {
    	$$ = new CFEAttribute(ATTR_PREALLOC_CLIENT);
	$$->SetSourceLine(gLineNumber);
    }
    | PREALLOC_SERVER
    {
    	$$ = new CFEAttribute(ATTR_PREALLOC_SERVER);
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

directional_attribute :
      IN
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

function_declarator :
      direct_declarator LPAREN param_declarators RPAREN
    {
	$$ = new CFEFunctionDeclarator($1, $3);
	// set parent relationship
	$1->SetParent($$);
	if ($3)
	    delete $3;
	$$->SetSourceLine(gLineNumber);
    }
    ;

predefined_type_spec :
      ERROR_STATUS_T
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

exception_declarator :
      EXCEPTION ID LBRACE member_list RBRACE
    {
        vector<CFEDeclarator*> *vd = new vector<CFEDeclarator*>();
        vd->push_back(new CFEDeclarator(DECL_IDENTIFIER, *$2));
        CFEStructType *st = new CFEStructType(*$2, $4);
        st->SetSourceLine(gLineNumber);
        delete $2;
        delete $4;
        $$ = new CFETypedDeclarator(TYPEDECL_EXCEPTION, st, vd);
        // set parent relationship
        st->SetParent($$);
        $$->SetSourceLine(gLineNumber);
    }
    | EXCEPTION ID LBRACE RBRACE
    {
    	CFESimpleType *st = new CFESimpleType(TYPE_INTEGER, false, true, 4/*value for LONG*/, false);
	st->SetSourceLine(gLineNumber);

        vector<CFEDeclarator*> *vd = new vector<CFEDeclarator*>();
        vd->push_back(new CFEDeclarator(DECL_IDENTIFIER, *$2));
        delete $2;

        $$ = new CFETypedDeclarator(TYPEDECL_EXCEPTION, st, vd);
        // set parent relationship
        st->SetParent($$);
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
        // if name is scoped, start at root, if not, start at current file
        // component
	CFELibrary *pFEPrevLib = 0;
        if (($7->find("::") != string::npos) || (!pCurFileComponent))
        {
            CFEFile *pRoot = dynamic_cast<CFEFile*>(CParser::GetCurrentFile()->GetRoot());
            assert(pRoot);
            pFEPrevLib = pRoot->FindLibrary(*$7);
        }
        else if (pCurFileComponent)
        {
            if (dynamic_cast<CFELibrary*>(pCurFileComponent))
                pFEPrevLib = ((CFELibrary*)pCurFileComponent)->FindLibrary(*$7);
        }
        // create library and set as current file-component
        CFELibrary *pFELibrary = new CFELibrary(*$7, $3, 0);
        pFELibrary->SetSourceLine(gLineNumber);
        if (pFEPrevLib != 0)
            pFEPrevLib->AddSameLibrary(pFELibrary);
        delete $3;
        if (pCurFileComponent)
        {
            if (dynamic_cast<CFELibrary*>(pCurFileComponent))
            {
                ((CFELibrary*)pCurFileComponent)->m_Libraries.Add(pFELibrary);
                pCurFileComponent = pFELibrary;
            }
            else if (dynamic_cast<CFEInterface*>(pCurFileComponent))
            {
                dceerror2("Cannot nest library %s into interface %s.", 
		    $7->c_str(),
		    ((CFEInterface*)pCurFileComponent)->GetName().c_str());
                delete $7;
                YYABORT;
            }
        }
        else
        {
            pCurFileComponent = pFELibrary;
            CParser::GetCurrentFile()->m_Libraries.Add(pFELibrary);
        }
    } LBRACE lib_definitions RBRACE
    {
        // adjust current file component
        if (pCurFileComponent &&
            pCurFileComponent->GetParent())
        {
            if (dynamic_cast<CFEFileComponent*>(pCurFileComponent->GetParent()))
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
        // if name is scoped, start at root, if not, start at current file
        // component
        CFELibrary *pFEPrevLib  = 0;
        if (($2->find("::") != string::npos) || (!pCurFileComponent))
        {
            CFEFile *pRoot = dynamic_cast<CFEFile*>(CParser::GetCurrentFile()->GetRoot());
            assert(pRoot);
            pFEPrevLib = pRoot->FindLibrary(*$2);
        }
        else if (pCurFileComponent)
        {
            if (dynamic_cast<CFELibrary*>(pCurFileComponent))
                pFEPrevLib = ((CFELibrary*)pCurFileComponent)->FindLibrary(*$2);
        }
        // create library and set as current file-component
        CFELibrary *pFELibrary = new CFELibrary(*$2, 0, 
	    pCurFileComponent ? static_cast<CFEBase*>(pCurFileComponent) :
	    CParser::GetCurrentFile());
        pFELibrary->SetSourceLine(gLineNumber);
        if (pFEPrevLib != 0)
            pFEPrevLib->AddSameLibrary(pFELibrary);
        if (pCurFileComponent)
        {
            if (dynamic_cast<CFELibrary*>(pCurFileComponent))
            {
                ((CFELibrary*)pCurFileComponent)->m_Libraries.Add(pFELibrary);
                pCurFileComponent = pFELibrary;
            }
            else if (dynamic_cast<CFEInterface*>(pCurFileComponent))
            {
                dceerror2("Cannot nest library %s into interface %s.",
		    $2->c_str(),
		    ((CFEInterface*)pCurFileComponent)->GetName().c_str());
                delete $2;
                YYABORT;
            }
        }
        else
        {
            pCurFileComponent = pFELibrary;
            CParser::GetCurrentFile()->m_Libraries.Add(pFELibrary);
        }
    } LBRACE lib_definitions RBRACE
    {
        // imported elements already belong to lib_definitions
        // adjust current file component
        if (pCurFileComponent &&
            pCurFileComponent->GetParent())
        {
            if (dynamic_cast<CFEFileComponent*>(pCurFileComponent->GetParent()))
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
        // if name is scoped, start at root, if not, start at current file
        // component
        CFEFile *pRoot = dynamic_cast<CFEFile*>(CParser::GetCurrentFile()->GetRoot());
        assert(pRoot);
        CFELibrary *pFEPrevLib = 0;
        if (($2->find("::") != string::npos) || (!pCurFileComponent))
        {
            pFEPrevLib = pRoot->FindLibrary(*$2);
        }
        else if (pCurFileComponent)
        {
            if (dynamic_cast<CFELibrary*>(pCurFileComponent))
                pFEPrevLib = ((CFELibrary*)pCurFileComponent)->FindLibrary(*$2);
        }
        // create library but do net set as current, simply add it
        // if scoped name: find libraries first
        string::size_type nScopePos = string::npos;
        CFELibrary *pFEScopeLibrary = NULL;
        string sRest = *$2;
        while ((nScopePos = sRest.find("::")) != string::npos)
        {
            string sScope = sRest.substr(0, nScopePos);
            sRest = sRest.substr(nScopePos+2);
            if (!sScope.empty())
            {
                if (pFEScopeLibrary)
                    pFEScopeLibrary = pFEScopeLibrary->FindLibrary(sScope);
                else
                    pFEScopeLibrary = pRoot->FindLibrary(sScope);
                if (!pFEScopeLibrary)
                {
                    dceerror2("Cannot find library %s, which is used in forward declaration of %s.",sScope.c_str(), $2->c_str());
                    delete $2;
                    YYABORT;
                }
            }
        }
        CFELibrary *pFELibrary = new CFELibrary(sRest, 0, 
	    pCurFileComponent ? static_cast<CFEBase*>(pCurFileComponent) :
	    CParser::GetCurrentFile());
        pFELibrary->SetSourceLine(gLineNumber);
        if (pFEPrevLib != 0)
            pFEPrevLib->AddSameLibrary(pFELibrary);
        if (pCurFileComponent)
        {
            if (dynamic_cast<CFELibrary*>(pCurFileComponent))
            {
                if (pFEScopeLibrary)
                    pFEScopeLibrary->m_Libraries.Add(pFELibrary);
                else
                    ((CFELibrary*)pCurFileComponent)->m_Libraries.Add(pFELibrary);
            }
            else if (dynamic_cast<CFEInterface*>(pCurFileComponent))
            {
                dceerror2("Cannot declare library %s within interface %s.", $2->c_str(), ((CFEInterface*)pCurFileComponent)->GetName().c_str());
                delete $2;
                YYABORT;
            }
        }
        else
        {
            if (pFEScopeLibrary)
                pFEScopeLibrary->m_Libraries.Add(pFELibrary);
            else
                CParser::GetCurrentFile()->m_Libraries.Add(pFELibrary);
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
	    $1->push_back($3);
	$$ = $1;
    }
    | lib_attribute
    {
	$$ = new vector<CFEAttribute*>();
	$$->push_back($1);
    }
    | error
    {
        dceerror2("unknown attribute");
        YYABORT;
    }
    ;

lib_attribute :
/*      UUID LPAREN uuid_rep rparen
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
      /* empty */
    {	
        $$ = NULL;
    }
    | lib_definitions lib_definition
    {
    	if ($1)
	    $$ = $1;
	else
	    $$ = new vector<CFEFileComponent*>();
        if ($2)
            $$->push_back($2);
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
            if (dynamic_cast<CFELibrary*>(pCurFileComponent))
                ((CFELibrary*)pCurFileComponent)->m_Typedefs.Add($1);
            else if (dynamic_cast<CFEInterface*>(pCurFileComponent))
                ((CFEInterface*)pCurFileComponent)->m_Typedefs.Add($1);
            else
            {
		CMessages::GccError(NULL, 0,
                    "current file component is unknown type: %s\n",
		    typeid(*pCurFileComponent).name());
                assert(false);
            }
        }
        else
	    // should be error, since current-file component should exist
            CParser::GetCurrentFile()->m_Typedefs.Add($1);
    }
    | const_declarator semicolon
    {
        $$ = 0;
        if (pCurFileComponent)
        {
            if (dynamic_cast<CFELibrary*>(pCurFileComponent))
                ((CFELibrary*)pCurFileComponent)->m_Constants.Add($1);
            else if (dynamic_cast<CFEInterface*>(pCurFileComponent))
                ((CFEInterface*)pCurFileComponent)->m_Constants.Add($1);
            else
            {
		CMessages::GccError(NULL, 0,
                    "current file component is unknown type: %s\n",
		    typeid(*pCurFileComponent).name());
                assert(false);
            }
        }
    }
    | tagged_declarator semicolon
    {
        $$ = NULL;
        if (pCurFileComponent)
        {
            if (dynamic_cast<CFELibrary*>(pCurFileComponent))
                ((CFELibrary*)pCurFileComponent)->m_TaggedDeclarators.Add($1);
            else if (dynamic_cast<CFEInterface*>(pCurFileComponent))
                ((CFEInterface*)pCurFileComponent)->m_TaggedDeclarators.Add($1);
            else
            {
		CMessages::GccError(NULL, 0, 
		    "current file component is unknown type: %s\n",
		    typeid(*pCurFileComponent).name());
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

attribute_decl :
      READONLY ATTRIBUTE type_spec declarator
    {
    	$$ = NULL;
        CFEAttribute *tmp = new CFEAttribute(ATTR_READONLY);
        vector<CFEDeclarator*> *tmpVD = new vector<CFEDeclarator*>();
        tmpVD->push_back($4);
        vector<CFEAttribute*> *tmpVA = new vector<CFEAttribute*>();
        tmpVA->push_back(tmp);
        CFEAttributeDeclarator *pAttr = new CFEAttributeDeclarator($3, tmpVD, tmpVA);
        $3->SetParent(pAttr);
        $4->SetParent(pAttr);
        tmp->SetParent(pAttr);
        delete tmpVD;
        delete tmpVA;
        pAttr->SetSourceLine(gLineNumber);
        if (pCurFileComponent)
        {
            if (dynamic_cast<CFEInterface*>(pCurFileComponent))
                ((CFEInterface*)pCurFileComponent)->m_AttributeDeclarators.Add(pAttr);
            else
            {
		CMessages::GccError(NULL, 0, 
		    "current file component is unknown type: %s\n",
		    typeid(*pCurFileComponent).name());
                assert(false);
            }
        }
        else
        {
            dceerror2("Cannot add attribute declarator to file without interface\n");
            YYABORT;
        }
    }
    | ATTRIBUTE type_spec declarator
    {
    	$$ = NULL;
        vector<CFEDeclarator*> *tmpVD = new vector<CFEDeclarator*>();
        tmpVD->push_back($3);
        CFEAttributeDeclarator *pAttr = new CFEAttributeDeclarator($2, tmpVD);
        $2->SetParent(pAttr);
        $3->SetParent(pAttr);
        pAttr->SetSourceLine(gLineNumber);
        delete tmpVD;
        if (pCurFileComponent)
        {
            if (dynamic_cast<CFEInterface*>(pCurFileComponent))
                ((CFEInterface*)pCurFileComponent)->m_AttributeDeclarators.Add(pAttr);
            else
            {
		CMessages::GccError(NULL, 0, 
		    "current file component is unknown type: %s\n",
		    typeid(*pCurFileComponent).name());
                assert(false);
            }
        }
        else
        {
            dceerror2("Cannot add attribute declarator to file without interface\n");
            YYABORT;
        }
    }
    ;

/* helper definitions */
/*uuid_rep
    : QUOT UUID_STR QUOT
    {
        $$ = $2;
    }
    | UUID_STR
    | error
    {
        dceerror2("expected a UUID representation");
        YYABORT;
    }
    ;
*/

semicolon :
      SEMICOLON
    | error
    {
        dceerror2("expecting ';'");
//        yyclearin;
//        YYBACKUP(SEMICOLON, yylval);
//        dcelex(&yylval);
        YYABORT;
    }
    ;

rbracket :
      RBRACKET
    | error
    {
        dceerror2("expecting ']'");
        YYABORT;
    }
    ;

rparen :
      RPAREN
    | error
    {
        dceerror2("expecting ')'");
        YYABORT;
    }
    ;

colon :
      COLON
    | error
    {
        dceerror2("expecting ':'");
        YYABORT;
    }
    ;

/*********************************************************************************
 * start GCC specific rules
 *********************************************************************************/

initializer_list:
      initializer
    | initializer_list COMMA initializer
    ;

initializer:
      const_expr
    {
        if ($1 != 0) delete $1;
    }
    | LBRACE initializer_list RBRACE
    | LBRACE initializer_list COMMA RBRACE
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

void
dceerror(const char* s)
{
    // ignore this functions -> it's called by bison code, which is not controlled by us
    nParseErrorDCE = 1;
}

void
dceerror2(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    CMessages::GccErrorVL(CParser::GetCurrentFile(), gLineNumber, fmt, args);
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
    CMessages::GccWarningVL(CParser::GetCurrentFile(), gLineNumber, fmt, args);
    va_end(args);
    nParseErrorDCE = 0;
}

bool
DoInterfaceCheck(string *pIName, vector<CFEIdentifier*> *pBaseInterfaces, CFEInterface **pFEInterface)
{
    // test base interfaces
    CFEFile *pRoot = dynamic_cast<CFEFile*>(CParser::GetCurrentFile()->GetRoot());
    assert(pRoot);
    if (pBaseInterfaces)
    {
        vector<CFEIdentifier*>::iterator iter;
        for (iter = pBaseInterfaces->begin(); iter != pBaseInterfaces->end(); iter++)
        {
            CFEInterface *pInterface = NULL;
            if ((*iter)->GetName().find("::") != string::npos)
                // scoped name: always lookup from root
                pInterface = pRoot->FindInterface((*iter)->GetName());
            else
            {
                // not scoped: look in current scope or at root if no current
                // scope
                if (dynamic_cast<CFELibrary*>(pCurFileComponent))
                    pInterface = ((CFELibrary*)pCurFileComponent)->FindInterface((*iter)->GetName());
                else if (!pCurFileComponent)
                    pInterface = pRoot->FindInterface((*iter)->GetName());
            }
            if (pInterface == NULL)
            {
                dceerror2("Couldn't find base interface name \"%s\".", 
		    (*iter)->GetName().c_str());
                return false;
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
            dceerror2("Interface \"%s\" already exists", pIName->c_str());
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
        if (dynamic_cast<CFELibrary*>(pCurFileComponent))
        {
            if (pFELibrary)
                pFELibrary->m_Interfaces.Add(pFEInterface);
            else
                ((CFELibrary*)pCurFileComponent)->m_Interfaces.Add(pFEInterface);
        }
        else if (dynamic_cast<CFEInterface*>(pCurFileComponent))
        {
            dceerror2("Cannot nest interface %s into interface %s\n",
                pFEInterface->GetName().c_str(),
                ((CFEInterface*)pCurFileComponent)->GetName().c_str());
            return false;
        }
    }
    else
    {
        if (pFELibrary)
            pFELibrary->m_Interfaces.Add(pFEInterface);
        else
            CParser::GetCurrentFile()->m_Interfaces.Add(pFEInterface);
    }
    return true;
}
