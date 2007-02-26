%{

    /* Copyright (C) 1989-1991 James A. Roskind, All rights reserved. 
    This grammar was developed  and  written  by  James  A.  Roskind. 
    Copying  of  this  grammar  description, as a whole, is permitted 
    providing this notice is intact and applicable  in  all  complete 
    copies.   Translations as a whole to other parser generator input 
    languages  (or  grammar  description  languages)   is   permitted 
    provided  that  this  notice is intact and applicable in all such 
    copies,  along  with  a  disclaimer  that  the  contents  are   a 
    translation.   The reproduction of derived text, such as modified 
    versions of this grammar, or the output of parser generators,  is 
    permitted,  provided  the  resulting  work includes the copyright 
    notice "Portions Copyright (c)  1989,  1990  James  A.  Roskind". 
    Derived products, such as compilers, translators, browsers, etc., 
    that  use  this  grammar,  must also provide the notice "Portions 
    Copyright  (c)  1989,  1990  James  A.  Roskind"  in   a   manner 
    appropriate  to  the  utility,  and in keeping with copyright law 
    (e.g.: EITHER displayed when first invoked/executed; OR displayed 
    continuously on display terminal; OR via placement in the  object 
    code  in  form  readable in a printout, with or near the title of 
    the work, or at the end of the file).  No royalties, licenses  or 
    commissions  of  any  kind are required to copy this grammar, its 
    translations, or derivative products, when the copies are made in 
    compliance with this notice. Persons or corporations that do make 
    copies in compliance with this notice may charge  whatever  price 
    is  agreeable  to  a  buyer, for such copies or derivative works. 
    THIS GRAMMAR IS PROVIDED ``AS IS'' AND  WITHOUT  ANY  EXPRESS  OR 
    IMPLIED  WARRANTIES,  INCLUDING,  WITHOUT LIMITATION, THE IMPLIED 
    WARRANTIES  OF  MERCHANTABILITY  AND  FITNESS  FOR  A  PARTICULAR 
    PURPOSE.

    James A. Roskind
    Independent Consultant
    516 Latania Palm Drive
    Indialantic FL, 32903
    (407)729-4348
    jar@hq.ileaf.com


    ---end of copyright notice---

MOTIVATION-

My  goal  is  to  see  software  developers  adopt  this grammar as a 
standard until such time as a better  standard  is  accessible.   The 
only  way  to  get it to become a standard, is to be sure that people 
know that derivations are based on a specific work.   The  intent  of 
releasing  this  grammar is to provide a publicly accessible standard 
grammar for C++.  The intent of the  copyright  notice  is  to  allow 
arbitrary  commercial  and non-commercial use of the grammar, as long 
as reference is given to the original standard.  Without reference to 
a specific standard, many alternative  grammars  would  develop.   By 
referring  to  the  standard,  this grammar is given publicity, which 
should lead to further use in compatible products and  systems.   The 
benefits  of  such  a  standard  to  commercial  products  (browsers, 
beautifiers, translators, compilers, ...) should be  obvious  to  the 
developers,  in  that  other compatible products will emerge, and the 
value of all conforming products  will  rise.   Most  developers  are 
aware  of  the  value  of  acquiring  a fairly complete grammar for a 
language, and the copyright notice  (and  the  resulting  affiliation 
with my work) should not be too high a price to pay.  By copyrighting 
this  grammar,  I have some minor control over what this standard is, 
and I can (hopefully) keep it from degrading without my approval.   I 
will  consistently  attempt  to  provide  upgraded  grammars that are 
compliant  with  the  current  art,  and  the  ANSI   C++   Committee 
recommendation  in  particular.   A developer is never prevented from 
modifying the grammar to improve it in  whatever  way  is  seen  fit.  
There  is  also  no  restriction on the sale of copies, or derivative 
works, providing the requests in the copyright notice are satisfied.

If you are not "copying" my work, but  are  rather  only  abstracting 
some  of  the  standard,  an acknowledgment with references to such a 
standard would be appreciated.  Specifically,  agreements  with  this 
standard  as  to  the  resolution  of otherwise ambiguous constructs, 
should be noted.

Simply put: "make whatever use you would like  of  the  grammar,  but 
include  the  ``portions  Copyright  ...''  as  a  reference  to this 
standard."


*/


/* Last modified 7/4/91, Version 2.0  */

/* File CPP5.Y is translated by YACC to Y.TAB.C */

/*  ACKNOWLEDGMENT: Without Bjarne Stroustrup and his many co-workers 
at Bell Labs, there would be no C++ Language for which to  provide  a 
syntax  description. Bjarne has also been especially helpful and open 
in discussions, and by permitting me to review  his  texts  prior  to 
their publication, allowed me a wonderful vantage point of clarity.

Without  the effort expended by the ANSI C standardizing committee, I 
would have been lost.  Although the ANSI C standard does not  include 
a  fully disambiguated syntax description, the committee has at least 
provided most of the disambiguating rules in  narratives.   This  C++ 
grammar  is intended to be a superset of an ANSI C compatible grammar 
that is provided in an related file.

Several reviewers have also  recently  critiqued  this  grammar,  the 
related  C  grammar,  and  or  assisted  in  discussions  during it's 
preparation.  These reviewers are certainly not responsible  for  the 
errors  I  have committed here, but they are responsible for allowing 
me  to  provide  fewer  errors.   These  colleagues  include:   Bruce 
Blodgett,  Mark Langley, Joe Fialli, Greg Perkins, Ron Guilmette, and 
Eric Krohn. */

/* Required fixes from last release :

done: 0) Allow direct call to destructors

done: 1) Allow placement of declarations in labeled statements.   The 
easiest  fix involves using a larger variance from the C grammar, and 
simply making "statement" include declarations.  Note that it  should 
also  be  legal  for  declarations  to  be  in  the  branches  of  if 
statements, as long as there is no other code in the block (I think).  
Consider:

        ...
        {
           if (0 == a)
                int b=5;
           else
                int c=4;
         }

1) template support: Not  done:  pending  syntax  specification  from 
ANSI.  (This looks like a major effort, as ANSI has decided to extend 
the  "TYPEDEFname"-feedback-to-the-lexer-hack  to  support   template 
names as a new kind of terminal token.)

2)  exception  handling:  Not done: pending syntax specification from 
ANSI (but it doesn't look hard)

done: 3) Support nested types, including identifier::name,  where  we 
realize  that  identifier was a hidden type.  Force the lexer to keep 
pace in this situation.   This  will  require  an  extension  of  the 
yacc-lex feedback loop.

done: 4) Support nested types even when derivations are used in class 
definitions.

5)  Provide  advanced  tutorial  on  YACC  conflicts:  almost done in 
documentation about machine generated documentation.

done: 6) Allow declaration specifiers to be left out of  declarations 
at file and structure scope so that operator conversion functions can 
be  declared and/or defined.  Note that checking to see that it was a 
function type that does not require declaration_specifiers is  now  a 
constraint  check,  and  not  a  syntax  issue.  Within function body 
scopes, declaration specifiers are required, and this is critical  to 
distinguishing expressions.


*/

%}

/*

Interesting ambiguity:
Usually
        typename ( typename2 ) ...
or
        typename ( typename2 [4] ) ...
etc.
is a redeclaration of typename2.

Inside  a structure elaboration, it is sometimes the declaration of a 
constructor!  Note, this only  counts  if  typename  IS  the  current 
containing  class name. (Note this can't conflict with ANSI C because 
ANSI C would call it a redefinition, but  claim  it  is  semantically 
illegal because you can't have a member declared the same type as the 
containing struct!) Since the ambiguity is only reached when a ';' is 
found,   there  is  no  problem  with  the  fact  that  the  semantic 
interpretation  is  providing  the  true  resolution.   As  currently 
implemented, the constructor semantic actions must be able to process 
an  ordinary  declaration.  I may reverse this in the future, to ease 
semantic implementation.

*/



/*

INTRO TO ANSI C GRAMMAR (provided in a separate file):

The refined grammar resolves several typedef ambiguities in the draft 
proposed ANSI C standard syntax down to 1 shift/reduce  conflict,  as 
reported by a YACC process.  Note that the one shift reduce conflicts 
is  the  traditional  if-if-else conflict that is not resolved by the 
grammar.  This ambiguity can be removed using the method described in 
the Dragon Book (2nd edition), but this does  not  appear  worth  the 
effort.

There  was quite a bit of effort made to reduce the conflicts to this 
level, and an additional effort was made to make  the  grammar  quite 
similar  to  the  C++ grammar being developed in parallel.  Note that 
this grammar resolves the following ANSI C ambiguities:

ANSI C section 3.5.6, "If the [typedef  name]  is  redeclared  at  an 
inner  scope,  the  type specifiers shall not be omitted in the inner 
declaration".  Supplying type specifiers prevents consideration of  T 
as a typedef name in this grammar.  Failure to supply type specifiers 
forced the use of the TYPEDEFname as a type specifier.  This is taken 
to an (unnecessary) extreme by this implementation.  The ambiguity is 
only  a  problem  with  the first declarator in a declaration, but we 
restrict  ALL  declarators  whenever  the  users  fails  to   use   a 
type_specifier.

ANSI C section 3.5.4.3, "In a parameter declaration, a single typedef 
name  in  parentheses  is  taken  to  be  an abstract declarator that 
specifies a function  with  a  single  parameter,  not  as  redundant 
parentheses  around  the  identifier".  This is extended to cover the 
following cases:

typedef float T;
int noo(const (T[5]));
int moo(const (T(int)));
...

Where again the '(' immediately to the left of 'T' is interpreted  as 
being  the  start  of  a  parameter type list, and not as a redundant 
paren around a redeclaration of T.  Hence an equivalent code fragment 
is:

typedef float T;
int noo(const int identifier1 (T identifier2 [5]));
int moo(const int identifier1 (T identifier2 (int identifier3)));
...

*/


%{
/*************** Includes and Defines *****************************/
#define YYDEBUG_LEXER_TEXT (yylval) /* our lexer loads this up each time.
				     We are telling the graphical debugger
				     where to find the spelling of the 
				     tokens.*/
#define YYDEBUG 1        /* get the pretty debugging code to compile*/
#define YYSTYPE  char *  /* interface with flex: should be in header file */

/*************** Standard ytab.c continues here *********************/
%}

/*************************************************************************/


/* This group is used by the C/C++ language parser */
%token AUTO            DOUBLE          INT             STRUCT
%token BREAK           ELSE            LONG            SWITCH
%token CASE            ENUM            REGISTER        TYPEDEF
%token CHAR            EXTERN          RETURN          UNION
%token CONST           FLOAT           SHORT           UNSIGNED
%token CONTINUE        FOR             SIGNED          VOID
%token DEFAULT         GOTO            SIZEOF          VOLATILE
%token DO              IF              STATIC          WHILE

/* The following are used in C++ only.  ANSI C would call these IDENTIFIERs */
%token NEW             DELETE
%token THIS
%token OPERATOR
%token CLASS
%token PUBLIC          PROTECTED       PRIVATE
%token VIRTUAL         FRIEND
%token INLINE          OVERLOAD

/* ANSI C Grammar suggestions */
%token IDENTIFIER              STRINGliteral
%token FLOATINGconstant        INTEGERconstant        CHARACTERconstant
%token OCTALconstant           HEXconstant

/* New Lexical element, whereas ANSI C suggested non-terminal */
%token TYPEDEFname

/* Multi-Character operators */
%token  ARROW            /*    ->                              */
%token  ICR DECR         /*    ++      --                      */
%token  LS RS            /*    <<      >>                      */
%token  LE GE EQ NE      /*    <=      >=      ==      !=      */
%token  ANDAND OROR      /*    &&      ||                      */
%token  ELLIPSIS         /*    ...                             */
                 /* Following are used in C++, not ANSI C        */
%token  CLCL             /*    ::                              */
%token  DOTstar ARROWstar/*    .*       ->*                    */

/* modifying assignment operators */
%token MULTassign  DIVassign    MODassign   /*   *=      /=      %=      */
%token PLUSassign  MINUSassign              /*   +=      -=              */
%token LSassign    RSassign                 /*   <<=     >>=             */
%token ANDassign   ERassign     ORassign    /*   &=      ^=      |=      */

/*************************************************************************/

%start translation_unit

/*************************************************************************/

%%

/*********************** CONSTANTS *********************************/
constant:
        INTEGERconstant
        | FLOATINGconstant
        /*  We  are not including ENUMERATIONconstant here because we 
          are treating it like a variable with a type of "enumeration 
          constant".  */
        | OCTALconstant
        | HEXconstant
        | CHARACTERconstant
        ;

string_literal_list:
                STRINGliteral
                | string_literal_list STRINGliteral
                ;


/************************* EXPRESSIONS ********************************/


    /* Note that I provide  a  "scope_opt_identifier"  that  *cannot* 
    begin  with ::.  This guarantees we have a viable declarator, and 
    helps to disambiguate :: based uses in the grammar.  For example:

            ...
            {
            int (* ::b()); // must be an expression
            int (T::b); // officially a declaration, which fails on constraint grounds

    This *syntax* restriction reflects the current syntax in the ANSI 
    C++ Working Papers.   This  means  that  it  is  *incorrect*  for 
    parsers to misparse the example:

            int (* ::b()); // must be an expression

    as a declaration, and then report a constraint error.

    In contrast, declarations such as:

        class T;
        class A;
        class B;
        main(){
              T( F());  // constraint error: cannot declare local function
              T (A::B::a); // constraint error: cannot declare member as a local value

    are  *parsed*  as  declarations,  and *then* given semantic error 
    reports.  It is incorrect for a parser to "change its mind" based 
    on constraints.  If your C++ compiler claims  that  the  above  2 
    lines are expressions, then *I* claim that they are wrong. */

paren_identifier_declarator:
        scope_opt_identifier
        | scope_opt_complex_name
        | '(' paren_identifier_declarator ')'
        ;


    /* Note that CLCL IDENTIFIER is NOT part of scope_opt_identifier, 
    but  it  is  part of global_opt_scope_opt_identifier.  It is ONLY 
    valid for referring to an identifier, and NOT valid for declaring 
    (or importing an external declaration of)  an  identifier.   This 
    disambiguates  the  following  code,  which  would  otherwise  be 
    syntactically and semantically ambiguous:

            class base {
                static int i; // element i;
                float member_function(void);
                };
            base i; // global i
            float base::member_function(void) {
                i; // refers to static int element "i" of base
                ::i; // refers to global "i", with type "base"
                    {
                    base :: i; // import of global "i", like "base (::i);"?
                                // OR reference to global??
                    }
                }
        */

primary_expression:
        global_opt_scope_opt_identifier
        | global_opt_scope_opt_complex_name
        | THIS   /* C++, not ANSI C */
        | constant
        | string_literal_list
        | '(' comma_expression ')'
        ;


    /* I had to disallow struct, union, or enum  elaborations  during 
    operator_function_name.   The  ANSI  C++  Working  paper is vague 
    about whether this should be part of the syntax, or a constraint.  
    The ambiguities that resulted were more than LALR  could  handle, 
    so  the  easiest  fix was to be more specific.  This means that I 
    had to in-line expand type_specifier_or_name far  enough  that  I 
    would  be  able to exclude elaborations.  This need is what drove 
    me to distinguish a whole series of tokens based on whether  they 
    include elaborations:

         struct A { ... }

    or simply a reference to an aggregate or enumeration:

         enum A

    The  latter,  as  well  an  non-aggregate  types are what make up 
    non_elaborating_type_specifier */

    /* Note that the following does not include  type_qualifier_list. 
    Hence,   whenever   non_elaborating_type_specifier  is  used,  an 
    adjacent rule is supplied containing type_qualifier_list.  It  is 
    not  generally  possible  to  know  immediately  (i_e., reduce) a 
    type_qualifier_list, as a TYPEDEFname that follows might  not  be 
    part of a type specifier, but might instead be "TYPEDEFname ::*".  
    */

non_elaborating_type_specifier:
        sue_type_specifier
        | basic_type_specifier
        | typedef_type_specifier

        | basic_type_name
        | TYPEDEFname
        | global_or_scoped_typedefname
        ;


    /*  The  following  introduces  MANY  conflicts.   Requiring  and 
    allowing '(' ')' around the `type' when the type is complex would 
    help a lot. */

operator_function_name:
        OPERATOR any_operator
        | OPERATOR type_qualifier_list            operator_function_ptr_opt
        | OPERATOR non_elaborating_type_specifier operator_function_ptr_opt
        ;


    /* The following causes several ambiguities on *  and  &.   These 
    conflicts  would also be removed if parens around the `type' were 
    required in the derivations for operator_function_name */

    /*  Interesting  aside:  The  use  of  right  recursion  in   the 
    production  for  operator_function_ptr_opt gives both the correct 
    parsing, AND removes a conflict!   Right  recursion  permits  the 
    parser  to  defer  reductions  (a.k.a.:  delay  resolution),  and 
    effectively make a second pass! */

operator_function_ptr_opt:
        /* nothing */
        | unary_modifier        operator_function_ptr_opt
        | asterisk_or_ampersand operator_function_ptr_opt
        ;


    /* List of operators we can overload */
any_operator:
        '+'
        | '-'
        | '*'
        | '/'
        | '%'
        | '^'
        | '&'
        | '|'
        | '~'
        | '!'
        | '<'
        | '>'
        | LS
        | RS
        | ANDAND
        | OROR
        | ARROW
        | ARROWstar
        | '.'
        | DOTstar
        | ICR
        | DECR
        | LE
        | GE
        | EQ
        | NE
        | assignment_operator
        | '(' ')'
        | '[' ']'
        | NEW
        | DELETE
        | ','
        ;


    /* The following production for type_qualifier_list was specially 
    placed BEFORE the definition of postfix_expression to  resolve  a 
    reduce-reduce    conflict    set    correctly.    Note   that   a 
    type_qualifier_list is only used  in  a  declaration,  whereas  a 
    postfix_expression is clearly an example of an expression.  Hence 
    we  are helping with the "if it can be a declaration, then it is" 
    rule.  The reduce conflicts are on ')', ',' and '='.  Do not move 
    the following productions */

type_qualifier_list_opt:
        /* Nothing */
        | type_qualifier_list
        ;


    /*  Note  that  the next set of productions in this grammar gives 
    post-increment a higher precedence that pre-increment.   This  is 
    not  clearly  stated  in  the  C++  Reference manual, and is only 
    implied by the grammar in the ANSI C Standard. */

    /* I *DON'T* use  argument_expression_list_opt  to  simplify  the 
    grammar  shown  below.   I am deliberately deferring any decision 
    until    *after*     the     closing     paren,     and     using 
    "argument_expression_list_opt" would commit prematurely.  This is 
    critical to proper conflict resolution. */

    /*  The  {}  in  the following rules allow the parser to tell the 
    lexer to search for the member name  in  the  appropriate  scope, 
    much the way the CLCL operator works.*/

postfix_expression:
        primary_expression
        | postfix_expression '[' comma_expression ']'
        | postfix_expression '(' ')'
        | postfix_expression '(' argument_expression_list ')'
        | postfix_expression {} '.'   member_name
        | postfix_expression {} ARROW member_name
        | postfix_expression ICR
        | postfix_expression DECR

                /* The next 4 rules are the source of cast ambiguity */
        | TYPEDEFname                  '(' ')'
        | global_or_scoped_typedefname '(' ')'
        | TYPEDEFname                  '(' argument_expression_list ')'
        | global_or_scoped_typedefname '(' argument_expression_list ')'
        | basic_type_name '(' assignment_expression ')'
            /* If the following rule is added to the  grammar,  there 
            will  be 3 additional reduce-reduce conflicts.  They will 
            all be resolved in favor of NOT using the following rule, 
            so no harm will be done.   However,  since  the  rule  is 
            semantically  illegal  we  will  omit  it  until  we  are 
            enhancing the grammar for error recovery */
/*      | basic_type_name '(' ')'  /* Illegal: no such constructor*/
        ;


    /* The last two productions in the next set are questionable, but 
    do not induce any conflicts.  I need to ask X3J16 :  Having  them 
    means that we have complex member function deletes like:

          const unsigned int :: ~ const unsigned int
    */

member_name:
        scope_opt_identifier
        | scope_opt_complex_name
        | basic_type_name CLCL '~' basic_type_name  /* C++, not ANSI C */

        | declaration_qualifier_list  CLCL '~'   declaration_qualifier_list
        | type_qualifier_list         CLCL '~'   type_qualifier_list
        ;

argument_expression_list:
        assignment_expression
        | argument_expression_list ',' assignment_expression
        ;

unary_expression:
        postfix_expression
        | ICR  unary_expression
        | DECR unary_expression
        | asterisk_or_ampersand cast_expression
        | '-'                   cast_expression
        | '+'                   cast_expression
        | '~'                   cast_expression
        | '!'                   cast_expression
        | SIZEOF unary_expression
        | SIZEOF '(' type_name ')'
        | allocation_expression
        ;


    /* Note that I could have moved the  newstore  productions  to  a 
    lower  precedence  level  than  multiplication  (binary '*'), and 
    lower than bitwise AND (binary '&').  These moves  are  the  nice 
    way  to  disambiguate a trailing unary '*' or '&' at the end of a 
    freestore expression.  Since the freestore expression (with  such 
    a  grammar  and  hence  precedence  given)  can never be the left 
    operand of a binary '*' or '&', the ambiguity would  be  removed. 
    These  problems  really  surface when the binary operators '*' or 
    '&' are overloaded, but this must be syntactically  disambiguated 
    before the semantic checking is performed...  Unfortunately, I am 
    not  creating  the language, only writing a grammar that reflects 
    its specification, and  hence  I  cannot  change  its  precedence 
    assignments.   If  I  had  my  druthers,  I would probably prefer 
    surrounding the type with parens all the time, and  avoiding  the 
    dangling * and & problem all together.*/

       /* Following are C++, not ANSI C */
allocation_expression:
        global_opt_scope_opt_operator_new                                    '(' type_name ')'
                operator_new_initializer_opt

        | global_opt_scope_opt_operator_new '(' argument_expression_list ')' '(' type_name ')'
                operator_new_initializer_opt

                /* next two rules are the source of * and & ambiguities */
        | global_opt_scope_opt_operator_new                                  operator_new_type
        | global_opt_scope_opt_operator_new '(' argument_expression_list ')' operator_new_type
        ;


       /* Following are C++, not ANSI C */
global_opt_scope_opt_operator_new:
        NEW
        | global_or_scope NEW
        ;

operator_new_type:
        type_qualifier_list              operator_new_declarator_opt
                        operator_new_initializer_opt

        | non_elaborating_type_specifier operator_new_declarator_opt
                        operator_new_initializer_opt
        ;

    
    /*  Right  recursion  is critical in the following productions to 
    avoid a conflict on TYPEDEFname */

operator_new_declarator_opt:
        /* Nothing */
        | operator_new_array_declarator
        | asterisk_or_ampersand operator_new_declarator_opt
        | unary_modifier        operator_new_declarator_opt
        ;

operator_new_array_declarator:
                                        '['                  ']'
        |                               '[' comma_expression ']'
        | operator_new_array_declarator '[' comma_expression ']'
        ;

operator_new_initializer_opt:
        /* Nothing */
        | '('                          ')'
        | '(' argument_expression_list ')'
        ;

cast_expression:
        unary_expression
        | '(' type_name ')' cast_expression
        ;


    /* Following are C++, not ANSI C */
deallocation_expression:
        cast_expression
        | global_opt_scope_opt_delete deallocation_expression
        | global_opt_scope_opt_delete '[' comma_expression ']' deallocation_expression  /* archaic C++, what a concept */
        | global_opt_scope_opt_delete '[' ']' deallocation_expression
        ;


    /* Following are C++, not ANSI C */
global_opt_scope_opt_delete:
        DELETE
        | global_or_scope DELETE
        ;


    /* Following are C++, not ANSI C */
point_member_expression:
        deallocation_expression
        | point_member_expression DOTstar  deallocation_expression
        | point_member_expression ARROWstar  deallocation_expression
        ;

multiplicative_expression:
        point_member_expression
        | multiplicative_expression '*' point_member_expression
        | multiplicative_expression '/' point_member_expression
        | multiplicative_expression '%' point_member_expression
        ;

additive_expression:
        multiplicative_expression
        | additive_expression '+' multiplicative_expression
        | additive_expression '-' multiplicative_expression
        ;

shift_expression:
        additive_expression
        | shift_expression LS additive_expression
        | shift_expression RS additive_expression
        ;

relational_expression:
        shift_expression
        | relational_expression '<' shift_expression
        | relational_expression '>' shift_expression
        | relational_expression LE  shift_expression
        | relational_expression GE  shift_expression
        ;

equality_expression:
        relational_expression
        | equality_expression EQ relational_expression
        | equality_expression NE relational_expression
        ;

AND_expression:
        equality_expression
        | AND_expression '&' equality_expression
        ;

exclusive_OR_expression:
        AND_expression
        | exclusive_OR_expression '^' AND_expression
        ;

inclusive_OR_expression:
        exclusive_OR_expression
        | inclusive_OR_expression '|' exclusive_OR_expression
        ;

logical_AND_expression:
        inclusive_OR_expression
        | logical_AND_expression ANDAND inclusive_OR_expression
        ;

logical_OR_expression:
        logical_AND_expression
        | logical_OR_expression OROR logical_AND_expression
        ;

conditional_expression:
        logical_OR_expression

        | logical_OR_expression '?' comma_expression ':'
                conditional_expression
        ;

assignment_expression:
        conditional_expression
        | unary_expression assignment_operator assignment_expression
        ;

assignment_operator:
        '='
        | MULTassign
        | DIVassign
        | MODassign
        | PLUSassign
        | MINUSassign
        | LSassign
        | RSassign
        | ANDassign
        | ERassign
        | ORassign
        ;

comma_expression:
        assignment_expression
        | comma_expression ',' assignment_expression
        ;

constant_expression:
        conditional_expression
        ;


    /* The following was used for clarity */
comma_expression_opt:
        /* Nothing */
        | comma_expression
        ;


/******************************* DECLARATIONS *********************************/


    /*  The  following are notably different from the ANSI C Standard 
    specified grammar, but  are  present  in  my  ANSI  C  compatible 
    grammar.  The changes were made to disambiguate typedefs presence 
    in   declaration_specifiers   (vs.    in   the   declarator   for 
    redefinition); to allow struct/union/enum/class tag  declarations 
    without  declarators,  and  to  better  reflect  the  parsing  of 
    declarations    (declarators    must     be     combined     with 
    declaration_specifiers  ASAP, so that they can immediately become 
    visible in the current scope). */

declaration:
        declaring_list ';'
        | default_declaring_list ';'
        | sue_declaration_specifier ';' { /* this is constraint error, as it
                                        includes a storage class!?!*/ }
        | sue_type_specifier ';'
        | sue_type_specifier_elaboration ';'
        ;


    /* Note that if a typedef were  redeclared,  then  a  declaration 
    specifier  must be supplied (re: ANSI C spec).  The following are 
    declarations wherein no declaration_specifier  is  supplied,  and 
    hence the 'default' must be used.  An example of this is

        const a;

    which by default, is the same as:

        const int a;

    `a' must NOT be a typedef in the above example. */


    /*  The  presence of `{}' in the following rules indicates points 
    at which the symbol table MUST be updated so that  the  tokenizer 
    can  IMMEDIATELY  continue  to  maintain  the  proper distinction 
    between a TYPEDEFname and an IDENTIFIER. */

default_declaring_list:  /* Can't  redeclare typedef names */
        declaration_qualifier_list   identifier_declarator {} initializer_opt
        | type_qualifier_list        identifier_declarator {} initializer_opt
        | default_declaring_list ',' identifier_declarator {} initializer_opt

        | declaration_qualifier_list constructed_identifier_declarator
        | type_qualifier_list        constructed_identifier_declarator
        | default_declaring_list ',' constructed_identifier_declarator
        ;


    /* Note how type_qualifier_list is  NOT  used  in  the  following 
    productions.    Qualifiers   are   NOT   sufficient  to  redefine 
    typedef-names (as prescribed by the ANSI C standard).*/

declaring_list:
        declaration_specifier          declarator {} initializer_opt
        | type_specifier               declarator {} initializer_opt
        | basic_type_name              declarator {} initializer_opt
        | TYPEDEFname                  declarator {} initializer_opt
        | global_or_scoped_typedefname declarator {} initializer_opt
        | declaring_list ','           declarator {} initializer_opt

        | declaration_specifier        constructed_declarator
        | type_specifier               constructed_declarator
        | basic_type_name              constructed_declarator
        | TYPEDEFname                  constructed_declarator
        | global_or_scoped_typedefname constructed_declarator
        | declaring_list ','           constructed_declarator
        ;


    /* Declarators with  parenthesized  initializers  present  a  big 
    problem.  Typically  a  declarator  that looks like: "*a(...)" is 
    supposed to bind FIRST to the "(...)", and then to the "*".  This 
    binding  presumes  that  the  "(...)" stuff is a prototype.  With 
    constructed declarators, we must (officially) finish the  binding 
    to the "*" (finishing forming a good declarator) and THEN connect 
    with  the argument list. Unfortunately, by the time we realize it 
    is an argument list (and not a  prototype)  we  have  pushed  the 
    separate  declarator  tokens  "*"  and  "a"  onto  the yacc stack 
    WITHOUT combining them. The solution is to use odd productions to 
    carry  the  incomplete  declarator  along  with   the   "argument 
    expression  list" back up the yacc stack.  We would then actually 
    instantiate the symbol table after we have  fully  decorated  the 
    symbol  with all the leading "*" stuff.  Actually, since we don't 
    have all the type information in one spot till  we  reduce  to  a 
    declaring_list,  this delay is not a problem.  Note that ordinary 
    initializers REQUIRE (ANSI C Standard) that the symbol be  placed 
    into  the symbol table BEFORE its initializer is read, but in the 
    case of parenthesized initializers,  this  is  not  possible  (we 
    don't  even  know  we  have  an  initializer till have passed the 
    opening "(". ) */

constructed_declarator:
        nonunary_constructed_identifier_declarator
        | constructed_paren_typedef_declarator
        | simple_paren_typedef_declarator '(' argument_expression_list ')'

        | simple_paren_typedef_declarator postfixing_abstract_declarator
                                          '(' argument_expression_list ')'  /* constraint error */

        | constructed_parameter_typedef_declarator
        | asterisk_or_ampersand constructed_declarator
        | unary_modifier        constructed_declarator
        ;

constructed_paren_typedef_declarator:
        '(' paren_typedef_declarator ')'
                    '(' argument_expression_list ')'

        | '(' paren_typedef_declarator ')' postfixing_abstract_declarator
                   '(' argument_expression_list ')'

        | '(' simple_paren_typedef_declarator postfixing_abstract_declarator ')'
                   '(' argument_expression_list ')'

        | '(' TYPEDEFname postfixing_abstract_declarator ')'
                   '(' argument_expression_list ')'
        ;


constructed_parameter_typedef_declarator:
        TYPEDEFname    '(' argument_expression_list ')'

        | TYPEDEFname  postfixing_abstract_declarator
                       '(' argument_expression_list ')'  /* constraint error */

        | '(' clean_typedef_declarator ')'
                       '(' argument_expression_list ')'

        | '(' clean_typedef_declarator ')'  postfixing_abstract_declarator
                       '(' argument_expression_list ')'
        ;


constructed_identifier_declarator:
        nonunary_constructed_identifier_declarator
        | asterisk_or_ampersand constructed_identifier_declarator
        | unary_modifier        constructed_identifier_declarator
        ;


    /* The following are restricted to NOT  begin  with  any  pointer 
    operators.   This  includes both "*" and "T::*" modifiers.  Aside 
    from  this  restriction,   the   following   would   have   been: 
    identifier_declarator '(' argument_expression_list ')' */

nonunary_constructed_identifier_declarator:
        paren_identifier_declarator   '(' argument_expression_list ')'

        | paren_identifier_declarator postfixing_abstract_declarator
                       '(' argument_expression_list ')'  /* constraint error*/

        | '(' unary_identifier_declarator ')'
                       '(' argument_expression_list ')'

        | '(' unary_identifier_declarator ')' postfixing_abstract_declarator
                       '(' argument_expression_list ')'
        ;


declaration_specifier:
        basic_declaration_specifier          /* Arithmetic or void */
        | sue_declaration_specifier          /* struct/union/enum/class */
        | typedef_declaration_specifier      /* typedef*/
        ;

type_specifier:
        basic_type_specifier                 /* Arithmetic or void */
        | sue_type_specifier                 /* Struct/Union/Enum/Class */
        | sue_type_specifier_elaboration     /* elaborated Struct/Union/Enum/Class */
        | typedef_type_specifier             /* Typedef */
        ;

declaration_qualifier_list:  /* storage class and optional const/volatile */
        storage_class
        | type_qualifier_list storage_class
        | declaration_qualifier_list declaration_qualifier
        ;

type_qualifier_list:
        type_qualifier
        | type_qualifier_list type_qualifier
        ;

declaration_qualifier:
        storage_class
        | type_qualifier                  /* const or volatile */
        ;

type_qualifier:
        CONST
        | VOLATILE
        ;

basic_declaration_specifier:      /*Storage Class+Arithmetic or void*/
        declaration_qualifier_list    basic_type_name
        | basic_type_specifier        storage_class
        | basic_type_name             storage_class
        | basic_declaration_specifier declaration_qualifier
        | basic_declaration_specifier basic_type_name
        ;

basic_type_specifier:
        type_qualifier_list    basic_type_name /* Arithmetic or void */
        | basic_type_name      basic_type_name
        | basic_type_name      type_qualifier
        | basic_type_specifier type_qualifier
        | basic_type_specifier basic_type_name
        ;

sue_declaration_specifier:          /* Storage Class + struct/union/enum/class */
        declaration_qualifier_list       elaborated_type_name
        | declaration_qualifier_list     elaborated_type_name_elaboration
        | sue_type_specifier             storage_class
        | sue_type_specifier_elaboration storage_class
        | sue_declaration_specifier      declaration_qualifier
        ;

sue_type_specifier_elaboration:
        elaborated_type_name_elaboration     /* elaborated struct/union/enum/class */
        | type_qualifier_list elaborated_type_name_elaboration
        | sue_type_specifier_elaboration type_qualifier
        ;

sue_type_specifier:
        elaborated_type_name              /* struct/union/enum/class */
        | type_qualifier_list elaborated_type_name
        | sue_type_specifier type_qualifier
        ;

typedef_declaration_specifier:       /*Storage Class + typedef types */
        declaration_qualifier_list   TYPEDEFname
        | declaration_qualifier_list global_or_scoped_typedefname

        | typedef_type_specifier       storage_class
        | TYPEDEFname                  storage_class
        | global_or_scoped_typedefname storage_class

        | typedef_declaration_specifier declaration_qualifier
        ;

typedef_type_specifier:              /* typedef types */
        type_qualifier_list      TYPEDEFname
        | type_qualifier_list    global_or_scoped_typedefname

        | TYPEDEFname                  type_qualifier
        | global_or_scoped_typedefname type_qualifier

        | typedef_type_specifier type_qualifier
        ;


/*  There  are  really  several distinct sets of storage_classes. The 
sets vary depending on whether the declaration is at file scope, is a 
declaration within a struct/class, is within a function body, or in a 
function declaration/definition (prototype  parameter  declarations).  
They   are   grouped  here  to  simplify  the  grammar,  and  can  be 
semantically checked.  Note that this  approach  tends  to  ease  the 
syntactic restrictions in the grammar slightly, but allows for future 
language  development,  and tends to provide superior diagnostics and 
error recovery (i_e.: a syntax error does not disrupt the parse).


                File    File    Member  Member  Local   Local  Formal
                Var     Funct   Var     Funct   Var     Funct  Params
TYPEDEF         x       x       x       x       x       x
EXTERN          x       x                       x       x
STATIC          x       x       x       x       x
AUTO                                            x              x
REGISTER                                        x              x
FRIEND                                  x
OVERLOAD                x               x               x
INLINE                  x               x               x
VIRTUAL                                 x               x
*/

storage_class:
        EXTERN
        | TYPEDEF
        | STATIC
        | AUTO
        | REGISTER
        | FRIEND   /* C++, not ANSI C */
        | OVERLOAD /* C++, not ANSI C */
        | INLINE   /* C++, not ANSI C */
        | VIRTUAL  /* C++, not ANSI C */
        ;

basic_type_name:
        INT
        | CHAR
        | SHORT
        | LONG
        | FLOAT
        | DOUBLE
        | SIGNED
        | UNSIGNED
        | VOID
        ;

elaborated_type_name_elaboration:
        aggregate_name_elaboration
        | enum_name_elaboration
        ;

elaborated_type_name:
        aggregate_name
        | enum_name
        ;


    /* Since the expression "new type_name" MIGHT use  an  elaborated 
    type  and a derivation, it MIGHT have a ':'.  This fact conflicts 
    with the requirement that a new expression can be placed  between 
    a '?' and a ':' in a conditional expression (at least it confuses 
    LR(1)   parsers).   Hence   the   aggregate_name_elaboration   is 
    responsible for a series of SR conflicts on ':'.*/

    /* The intermediate actions {}  represent  points  at  which  the 
    database  of  typedef  names  must  be  updated  in C++.  This is 
    critical to the lexer, which must begin to tokenize based on this 
    new information. */

aggregate_name_elaboration:
        aggregate_name derivation_opt  '{' member_declaration_list_opt '}'
        | aggregate_key derivation_opt '{' member_declaration_list_opt '}'
        ;


    /* We distinguish between the above, which  support  elaboration, 
    and  this  set  of  productions  so  that  we can provide special 
    declaration specifiers for operator_new_type, and for  conversion 
    functions.  Note that without this restriction a large variety of 
    conflicts  appear  when  processing  operator_new and conversions 
    operators (which can be  followed  by  a  ':'  in  a  ternary  ?: 
    expression) */

    /*  Note that at the end of each of the following rules we should 
    be sure that the tag name is  in,  or  placed  in  the  indicated 
    scope.   If  no  scope  is  specified, then we must add it to our 
    current scope IFF it cannot  be  found  in  an  external  lexical 
    scope. */

aggregate_name:
                             aggregate_key tag_name
        | global_scope scope aggregate_key tag_name
        | global_scope       aggregate_key tag_name
        | scope              aggregate_key tag_name
        ;

derivation_opt:
        /* nothing */
        | ':' derivation_list
        ;

derivation_list:
        parent_class
        | derivation_list ',' parent_class
        ;

parent_class:
                                       global_opt_scope_opt_typedefname
        | VIRTUAL access_specifier_opt global_opt_scope_opt_typedefname
        | access_specifier virtual_opt global_opt_scope_opt_typedefname
        ;

virtual_opt:
        /* nothing */
        | VIRTUAL
        ;

access_specifier_opt:
        /* nothing */
        | access_specifier
        ;

access_specifier:
        PUBLIC
        | PRIVATE
        | PROTECTED
        ;

aggregate_key:
        STRUCT
        | UNION
        | CLASS /* C++, not ANSI C */
        ;


    /* Note that an empty list is ONLY allowed under C++. The grammar 
    can  be modified so that this stands out.  The trick is to define 
    member_declaration_list, and have that referenced for non-trivial 
    lists. */

member_declaration_list_opt:
        /* nothing */
        | member_declaration_list_opt member_declaration
        ;

member_declaration:
        member_declaring_list ';'
        | member_default_declaring_list ';'

        | access_specifier ':'               /* C++, not ANSI C */

        | new_function_definition            /* C++, not ANSI C */
        | constructor_function_in_class      /* C++, not ANSI C */

        | sue_type_specifier             ';' /* C++, not ANSI C */
        | sue_type_specifier_elaboration ';' /* C++, not ANSI C */
        | identifier_declarator          ';' /* C++, not ANSI C
                                                access modification
                                                conversion functions,
                                                unscoped destructors */

        | typedef_declaration_specifier ';' /* friend T */       /* C++, not ANSI C */
        | sue_declaration_specifier ';'     /* friend class C*/  /* C++, not ANSI C */
        ;

member_default_declaring_list:        /* doesn't redeclare typedef*/
        type_qualifier_list
                identifier_declarator member_pure_opt

        | declaration_qualifier_list
                identifier_declarator member_pure_opt /* C++, not ANSI C */

        | member_default_declaring_list ','
                identifier_declarator member_pure_opt

        | type_qualifier_list                bit_field_identifier_declarator
        | declaration_qualifier_list         bit_field_identifier_declarator /* C++, not ANSI C */
        | member_default_declaring_list ','  bit_field_identifier_declarator
        ;


    /* There is a conflict when "struct A" is used as  a  declaration 
    specifier,  and  there  is a chance that a bit field name will be 
    provided.  To fix this syntactically would require distinguishing 
    non_elaborating_declaration_specifiers   the   way   I    handled 
    non_elaborating_type_specifiers.   I   think  this  should  be  a 
    constraint error anyway :-). */

member_declaring_list:        /* Can possibly redeclare typedefs */
        type_specifier                 declarator member_pure_opt
        | basic_type_name              declarator member_pure_opt

        | global_or_scoped_typedefname declarator member_pure_opt
        | member_conflict_declaring_item
        | member_declaring_list ','    declarator member_pure_opt

        | type_specifier                bit_field_declarator
        | basic_type_name               bit_field_declarator
        | TYPEDEFname                   bit_field_declarator
        | global_or_scoped_typedefname  bit_field_declarator
        | declaration_specifier         bit_field_declarator /* constraint violation: storage class used */
        | member_declaring_list ','     bit_field_declarator
        ;


    /* The following conflict with constructors-
      member_conflict_declaring_item:
        TYPEDEFname             declarator member_pure_opt
        | declaration_specifier declarator member_pure_opt /* C++, not ANSI C * /
        ;
    so we inline expand declarator to get the following productions...
    */
member_conflict_declaring_item:
        TYPEDEFname             identifier_declarator            member_pure_opt
        | TYPEDEFname           parameter_typedef_declarator     member_pure_opt
        | TYPEDEFname           simple_paren_typedef_declarator  member_pure_opt

        | declaration_specifier identifier_declarator            member_pure_opt
        | declaration_specifier parameter_typedef_declarator     member_pure_opt
        | declaration_specifier simple_paren_typedef_declarator  member_pure_opt

        | member_conflict_paren_declaring_item
        ;


    /* The following still conflicts with constructors-
      member_conflict_paren_declaring_item:
        TYPEDEFname             paren_typedef_declarator     member_pure_opt
        | declaration_specifier paren_typedef_declarator     member_pure_opt
        ;
    so paren_typedef_declarator is expanded inline to get...*/

member_conflict_paren_declaring_item:
        TYPEDEFname   asterisk_or_ampersand
                '(' simple_paren_typedef_declarator ')' member_pure_opt
        | TYPEDEFname unary_modifier
                '(' simple_paren_typedef_declarator ')' member_pure_opt
        | TYPEDEFname asterisk_or_ampersand
                '(' TYPEDEFname ')'                     member_pure_opt
        | TYPEDEFname unary_modifier
                '(' TYPEDEFname ')'                     member_pure_opt
        | TYPEDEFname asterisk_or_ampersand
                 paren_typedef_declarator               member_pure_opt
        | TYPEDEFname unary_modifier
                 paren_typedef_declarator               member_pure_opt

        | declaration_specifier asterisk_or_ampersand
                '(' simple_paren_typedef_declarator ')' member_pure_opt
        | declaration_specifier unary_modifier
                '(' simple_paren_typedef_declarator ')' member_pure_opt
        | declaration_specifier asterisk_or_ampersand
                '(' TYPEDEFname ')'                     member_pure_opt
        | declaration_specifier unary_modifier
                '(' TYPEDEFname ')'                     member_pure_opt
        | declaration_specifier asterisk_or_ampersand
                paren_typedef_declarator                member_pure_opt
        | declaration_specifier unary_modifier
                paren_typedef_declarator                member_pure_opt

        | member_conflict_paren_postfix_declaring_item
        ;


    /* but we still have the following conflicts with constructors-
   member_conflict_paren_postfix_declaring_item:
      TYPEDEFname             postfix_paren_typedef_declarator member_pure_opt
      | declaration_specifier postfix_paren_typedef_declarator member_pure_opt
      ;
    so we expand paren_postfix_typedef inline and get...*/

member_conflict_paren_postfix_declaring_item:
        TYPEDEFname     '(' paren_typedef_declarator ')'
                                                           member_pure_opt
        | TYPEDEFname   '(' simple_paren_typedef_declarator
                        postfixing_abstract_declarator ')' member_pure_opt
        | TYPEDEFname   '(' TYPEDEFname
                        postfixing_abstract_declarator ')' member_pure_opt
        | TYPEDEFname   '(' paren_typedef_declarator ')'
                        postfixing_abstract_declarator     member_pure_opt

        | declaration_specifier '(' paren_typedef_declarator ')'
                                                           member_pure_opt
        | declaration_specifier '(' simple_paren_typedef_declarator
                        postfixing_abstract_declarator ')' member_pure_opt
        | declaration_specifier '(' TYPEDEFname
                        postfixing_abstract_declarator ')' member_pure_opt
        | declaration_specifier '(' paren_typedef_declarator ')'
                        postfixing_abstract_declarator     member_pure_opt
        ;
    /* ...and we are done.  Now all  the  conflicts  appear  on  ';', 
    which can be semantically evaluated/disambiguated */


member_pure_opt:
        /* nothing */
        | '=' OCTALconstant /* C++, not ANSI C */ /* Pure function*/
        ;


    /*  Note  that  bit  field  names, where redefining TYPEDEFnames, 
    cannot be parenthesized in C++ (due to  ambiguities),  and  hence 
    this  part of the grammar is simpler than ANSI C. :-) The problem 
    occurs because:

         TYPEDEFname ( TYPEDEFname) : .....

    doesn't look like a bit field, rather it looks like a constructor 
    definition! */

bit_field_declarator:
        bit_field_identifier_declarator
        | TYPEDEFname {} ':' constant_expression
        ;


    /* The actions taken in the "{}" above and below are intended  to 
    allow  the  symbol  table  to  be  updated when the declarator is 
    complete.  It is critical for code like:

            foo : sizeof(foo + 1);
    */

bit_field_identifier_declarator:
                                   ':' constant_expression
        | identifier_declarator {} ':' constant_expression
        ;

enum_name_elaboration:
        global_opt_scope_opt_enum_key '{' enumerator_list '}'
        | enum_name                   '{' enumerator_list '}'
        ;


    /* As with structures, the distinction between "elaborating"  and 
    "non-elaborating"  enum  types  is  maintained.  In actuality, it 
    probably does not cause much in the way of conflicts, since a ':' 
    is not allowed.  For symmetry, we maintain the distinction.   The 
    {}  actions are intended to allow the symbol table to be updated.  
    These updates are significant to code such as:

        enum A { first=sizeof(A)};
    */

enum_name:
        global_opt_scope_opt_enum_key tag_name
        ;

global_opt_scope_opt_enum_key:
        ENUM
        | global_or_scope ENUM
        ;

enumerator_list:
        enumerator_list_no_trailing_comma
        | enumerator_list_no_trailing_comma ',' /* C++, not ANSI C */
        ;


    /* Note that we do not need to rush to add an enumerator  to  the 
    symbol  table  until  *AFTER* the enumerator_value_opt is parsed. 
    The enumerated value is only in scope  AFTER  its  definition  is 
    complete.   Hence the following is legal: "enum {a, b=a+10};" but 
    the following is (assuming no external matching of names) is  not 
    legal:  "enum {c, d=sizeof(d)};" ("d" not defined when sizeof was 
    applied.) This is  notably  contrasted  with  declarators,  which 
    enter scope as soon as the declarator is complete. */

enumerator_list_no_trailing_comma:
        enumerator_name enumerator_value_opt
        | enumerator_list_no_trailing_comma ',' enumerator_name enumerator_value_opt
        ;

enumerator_name:
        IDENTIFIER
        | TYPEDEFname
        ;

enumerator_value_opt:
        /* Nothing */
        | '=' constant_expression
        ;


    /*  We special case the lone type_name which has no storage class 
    (even though it should be an example of  a  parameter_type_list). 
    This helped to disambiguate type-names in parenthetical casts.*/

parameter_type_list:
        '(' ')'                             type_qualifier_list_opt
        | '(' type_name ')'                 type_qualifier_list_opt
        | '(' type_name initializer ')'     type_qualifier_list_opt /* C++, not ANSI C */
        | '(' named_parameter_type_list ')' type_qualifier_list_opt
        ;


    /* The following are used in old style function definitions, when 
    a complex return type includes the "function returning" modifier. 
    Note  the  subtle  distinction  from  parameter_type_list.  These 
    parameters are NOT the parameters for the function being defined, 
    but are simply part of the type definition.  An example would be:

        int(*f(   a  ))(float) long a; {...}

    which is equivalent to the full new style definition:

        int(*f(long a))(float) {...}

    The   type   list    `(float)'    is    an    example    of    an 
    old_parameter_type_list.   The  bizarre point here is that an old 
    function definition declarator can be followed by  a  type  list, 
    which  can  start  with a qualifier `const'.  This conflicts with 
    the new syntactic construct for const member  functions!?!  As  a 
    result,  an  old  style function definition cannot be used in all 
    cases for a member function.  */

old_parameter_type_list:
        '(' ')'
        | '(' type_name ')'
        | '(' type_name initializer ')'  /* C++, not ANSI C */
        | '(' named_parameter_type_list ')'
        ;

named_parameter_type_list:  /* WARNING: excludes lone type_name*/
        parameter_list
        | parameter_list comma_opt_ellipsis
        | type_name comma_opt_ellipsis
        | type_name initializer comma_opt_ellipsis  /* C++, not ANSI C */
        | ELLIPSIS /* C++, not ANSI C */
        ;

comma_opt_ellipsis:
        ELLIPSIS       /* C++, not ANSI C */
        | ',' ELLIPSIS
        ;

parameter_list:
        non_casting_parameter_declaration
        | non_casting_parameter_declaration initializer /* C++, not ANSI C */
        | type_name             ',' parameter_declaration
        | type_name initializer ',' parameter_declaration  /* C++, not ANSI C */
        | parameter_list        ',' parameter_declaration
        ;


    /* There is some very subtle disambiguation going  on  here.   Do 
    not be tempted to make further use of the following production in 
    parameter_list,  or else the conflict count will grow noticeably. 
    Specifically, the next set  of  rules  has  already  been  inline 
    expanded for the first parameter in a parameter_list to support a 
    deferred disambiguation. The subtle disambiguation has to do with 
    contexts where parameter type lists look like old-style-casts. */

parameter_declaration:
        type_name
        | type_name                         initializer  /* C++, not ANSI C */
        | non_casting_parameter_declaration
        | non_casting_parameter_declaration initializer /* C++, not ANSI C */
        ;


    /* There is an LR ambiguity between old-style parenthesized casts 
    and parameter-type-lists.  This tends to happen in contexts where 
    either  an  expression or a parameter-type-list is possible.  For 
    example, assume that T is an  externally  declared  type  in  the 
    code:

           int (T ((int

    it might continue:

           int (T ((int)0));

    which would make it:

           (int) (T) (int)0 ;

    which  is  an  expression,  consisting  of  a  series  of  casts.  
    Alternatively, it could be:

           int (T ((int a)));

    which would make it the redeclaration of T, equivalent to:

           int T (dummy_name (int a));

    if we see a type that either has a named variable (in  the  above 
    case "a"), or a storage class like:

           int (T ((int register

    then  we  know  it  can't  be  a cast, and it is "forced" to be a 
    parameter_list.

    It is not yet clear that the ANSI C++ committee would  decide  to 
    place this disambiguation into the syntax, rather than leaving it 
    as  a  constraint check (i.e., a valid parser would have to parse 
    everything as though it were  a  parameter  list  (in  these  odd 
    contexts),  and  then  give an error if is to a following context 
    (like "0" above) that invalidated this syntax evaluation. */

    /* One big thing implemented here is that a TYPEDEFname CANNOT be 
    redeclared when we don't have declaration_specifiers! Notice that 
    when we do use a TYPEDEFname based declarator, only the "special" 
    (non-ambiguous  in  this  context)  typedef_declarator  is  used. 
    Everything else that is "missing" shows up as a type_name. */

non_casting_parameter_declaration: /*have names or storage classes */
        declaration_specifier
        | declaration_specifier abstract_declarator
        | declaration_specifier identifier_declarator
        | declaration_specifier parameter_typedef_declarator

        | declaration_qualifier_list
        | declaration_qualifier_list abstract_declarator
        | declaration_qualifier_list identifier_declarator

        | type_specifier identifier_declarator
        | type_specifier parameter_typedef_declarator

        | basic_type_name identifier_declarator
        | basic_type_name parameter_typedef_declarator

        | TYPEDEFname                   identifier_declarator
        | TYPEDEFname                   parameter_typedef_declarator

        | global_or_scoped_typedefname  identifier_declarator
        | global_or_scoped_typedefname  parameter_typedef_declarator

        | type_qualifier_list identifier_declarator
        ;

type_name:
        type_specifier
        | basic_type_name
        | TYPEDEFname
        | global_or_scoped_typedefname
        | type_qualifier_list

        | type_specifier               abstract_declarator
        | basic_type_name              abstract_declarator
        | TYPEDEFname                  abstract_declarator
        | global_or_scoped_typedefname abstract_declarator
        | type_qualifier_list          abstract_declarator
        ;

initializer_opt:
        /* nothing */
        | initializer
        ;

initializer:
        '=' initializer_group
        ;

initializer_group:
        '{' initializer_list '}'
        | '{' initializer_list ',' '}'
        | assignment_expression
        ;

initializer_list:
        initializer_group
        | initializer_list ',' initializer_group
        ;


/*************************** STATEMENTS *******************************/

statement:
        labeled_statement
        | compound_statement
        | expression_statement
        | selection_statement
        | iteration_statement
        | jump_statement
        | declaration /* C++, not ANSI C */
        ;

labeled_statement:
        label                      ':' statement
        | CASE constant_expression ':' statement
        | DEFAULT                  ':' statement
        ;


    /*  I sneak declarations into statement_list to support C++.  The 
    grammar is a little clumsy this  way,  but  the  violation  of  C 
    syntax is heavily localized */

compound_statement:
        '{' statement_list_opt '}'
        ;

declaration_list:
        declaration
        | declaration_list declaration
        ;

statement_list_opt:
        /* nothing */
        | statement_list_opt statement
        ;

expression_statement:
        comma_expression_opt ';'
        ;

selection_statement:
          IF '(' comma_expression ')' statement
        | IF '(' comma_expression ')' statement ELSE statement
        | SWITCH '(' comma_expression ')' statement
        ;

iteration_statement:
        WHILE '(' comma_expression_opt ')' statement
        | DO statement WHILE '(' comma_expression ')' ';'

        | FOR '(' comma_expression_opt ';' comma_expression_opt ';'
                comma_expression_opt ')' statement

        | FOR '(' declaration        comma_expression_opt ';'
                comma_expression_opt ')' statement  /* C++, not ANSI C */
        ;

jump_statement:
        GOTO label                     ';'
        | CONTINUE                     ';'
        | BREAK                        ';'
        | RETURN comma_expression_opt  ';'
        ;


    /*  The  following  actions should update the symbol table in the 
    "label" name space */

label:
        IDENTIFIER
        | TYPEDEFname
        ;


/***************************** EXTERNAL DEFINITIONS *****************************/

translation_unit:
        /* nothing */
        | translation_unit external_definition
        ;

external_definition:
        function_declaration                         /* C++, not ANSI C*/
        | function_definition
        | declaration
        | linkage_specifier function_declaration     /* C++, not ANSI C*/
        | linkage_specifier function_definition      /* C++, not ANSI C*/
        | linkage_specifier declaration              /* C++, not ANSI C*/
        | linkage_specifier '{' translation_unit '}' /* C++, not ANSI C*/
        ;

linkage_specifier:
        EXTERN STRINGliteral
        ;


    /* Note that declaration_specifiers are left out of the following 
    function declarations.  Such omission is illegal in ANSI C. It is 
    sometimes necessary in C++, in instances  where  no  return  type 
    should be specified (e_g., a conversion operator).*/

function_declaration:
        identifier_declarator ';'   /*  semantically  verify  it is a 
                                    function, and (if ANSI says  it's 
                                    the  law for C++ also...) that it 
                                    is something that  can't  have  a 
                                    return  type  (like  a conversion 
                                    function, or a destructor */

        | constructor_function_declaration ';'
        ;

function_definition:
        new_function_definition
        | old_function_definition
        | constructor_function_definition
        ;


    /* Note that in ANSI C, function definitions *ONLY* are presented 
    at file scope.  Hence, if there is a typedefname  active,  it  is 
    illegal  to  redeclare  it  (there  is no enclosing scope at file 
    scope).

    In  contrast,  C++  allows   function   definitions   at   struct 
    elaboration scope, and allows tags that are defined at file scope 
    (and  hence  look like typedefnames) to be redeclared to function 
    calls.  Hence several of the rules are "partially C++  only".   I 
    could  actually  build separate rules for typedef_declarators and 
    identifier_declarators, and mention that  the  typedef_declarator 
    rules represent the C++ only features.

    In  some  sense,  this  is  haggling, as I could/should have left 
    these as constraints in the ANSI C grammar, rather than as syntax 
    requirements.  */

new_function_definition:
                                       identifier_declarator compound_statement
        | declaration_specifier                   declarator compound_statement /* partially C++ only */
        | type_specifier                          declarator compound_statement /* partially C++ only */
        | basic_type_name                         declarator compound_statement /* partially C++ only */
        | TYPEDEFname                             declarator compound_statement /* partially C++ only */
        | global_or_scoped_typedefname            declarator compound_statement /* partially C++ only */
        | declaration_qualifier_list   identifier_declarator compound_statement
        | type_qualifier_list          identifier_declarator compound_statement
        ;


    /* Note that I do not support redeclaration of TYPEDEFnames  into 
    function  names  as I did in new_function_definitions (see note). 
    Perhaps I should do it, but for now, ignore the issue. Note  that 
    this  is  a  non-problem  with  ANSI  C,  as  tag  names  are not 
    considered TYPEDEFnames. */

old_function_definition:
                                       old_function_declarator {} old_function_body
        | declaration_specifier        old_function_declarator {} old_function_body
        | type_specifier               old_function_declarator {} old_function_body
        | basic_type_name              old_function_declarator {} old_function_body
        | TYPEDEFname                  old_function_declarator {} old_function_body
        | global_or_scoped_typedefname old_function_declarator {} old_function_body
        | declaration_qualifier_list   old_function_declarator {} old_function_body
        | type_qualifier_list          old_function_declarator {} old_function_body
        ;

old_function_body:
        declaration_list compound_statement
        | compound_statement
        ;


    /*    Verify    via    constraints     that     the     following 
        declaration_specifier           is          really          a 
        typedef_declaration_specifier, consisting of:

        ... TYPEDEFname :: TYPEDEFname

    optionally *preceded* by a "inline" keyword.   Use  care  not  to 
    support "inline" as a postfix!

    Similarly, the global_or_scoped_typedefname must be:

        ... TYPEDEFname :: TYPEDEFname

    with matching names at the end of the list.

    We  use the more general form to prevent a syntax conflict with a 
    typical    function    definition    (which    won't    have    a 
    constructor_init_list) */

constructor_function_definition:
        global_or_scoped_typedefname parameter_type_list
                     constructor_init_list_opt compound_statement

        | declaration_specifier      parameter_type_list
                     constructor_init_list_opt compound_statement
        ;


    /*  Same  comments  as  seen  for constructor_function_definition 
    apply here */

constructor_function_declaration:
        global_or_scoped_typedefname parameter_type_list  /* wasteful redeclaration; used for friend decls.  */

        | declaration_specifier      parameter_type_list  /* request to inline, no definition */
        ;


    /* The following use of declaration_specifiers are made to  allow 
    for  a TYPEDEFname preceded by an INLINE modifier. This fact must 
    be verified semantically.  It should also be  verified  that  the 
    TYPEDEFname  is  ACTUALLY  the  class name being elaborated. Note 
    that we could break out typedef_declaration_specifier from within 
    declaration_specifier, and we  might  narrow  down  the  conflict 
    region a bit. A second alternative (to what is done) for cleaning 
    up  this  stuff  is  to  let the tokenizer specially identify the 
    current class being elaborated as a special token, and not just a 
    typedefname. Unfortunately, things would get very  confusing  for 
    the  lexer,  as  we may pop into enclosed tag elaboration scopes; 
    into function definitions; or into both recursively! */

    /* I should make the following  rules  easier  to  annotate  with 
    scope  entry  and exit actions.  Note how hard it is to establish 
    the scope when you don't even know what the decl_spec is!! It can 
    be done with $-1 hacking, but I should not encourage users to  do 
    this directly. */

constructor_function_in_class:
        declaration_specifier   constructor_parameter_list_and_body
        | TYPEDEFname           constructor_parameter_list_and_body
        ;


    /* The following conflicts with member declarations-
    constructor_parameter_list_and_body:
          parameter_type_list ';'
          | parameter_type_list constructor_init_list_opt compound_statement
          ;
    so parameter_type_list was expanded inline to get */

    /* C++, not ANSI C */
constructor_parameter_list_and_body:
          '('                           ')' type_qualifier_list_opt ';'
        | '(' type_name initializer     ')' type_qualifier_list_opt ';' 
        | '(' named_parameter_type_list ')' type_qualifier_list_opt ';'
        | '('                           ')' type_qualifier_list_opt
                constructor_init_list_opt compound_statement
        | '(' type_name initializer     ')' type_qualifier_list_opt
                constructor_init_list_opt compound_statement
        | '(' named_parameter_type_list ')' type_qualifier_list_opt
                constructor_init_list_opt compound_statement

        | constructor_conflicting_parameter_list_and_body
        ;


    /* The following conflicted with member declaration-
    constructor_conflicting_parameter_list_and_body:
        '('   type_name ')'                 type_qualifier_list_opt ';'
        | '(' type_name ')'                 type_qualifier_list_opt
                constructor_init_list_opt compound_statement
        ;
    so type_name was inline expanded to get the following... */


    /*  Note  that by inline expanding type_qualifier_opt in a few of 
    the following rules I can transform 3  RR  conflicts  into  3  SR 
    conflicts.  Since  all the conflicts have a look ahead of ';', it 
    doesn't  really  matter  (also,  there  are  no   bad   LALR-only 
    components in the conflicts) */

constructor_conflicting_parameter_list_and_body:
        '(' type_specifier                 ')' type_qualifier_list_opt
                ';'
        | '(' basic_type_name              ')' type_qualifier_list_opt
                ';'

        | '(' TYPEDEFname                  ')' type_qualifier_list_opt
                ';'

        | '(' global_or_scoped_typedefname ')' type_qualifier_list_opt
                ';'

        | '(' type_qualifier_list          ')' type_qualifier_list_opt
                ';'


        | '(' type_specifier               abstract_declarator ')' type_qualifier_list_opt
                ';'
        | '(' basic_type_name              abstract_declarator ')' type_qualifier_list_opt
                ';'

        /* missing entry posted below */

        | '(' global_or_scoped_typedefname abstract_declarator ')' type_qualifier_list_opt
                ';'
        | '(' type_qualifier_list          abstract_declarator ')' type_qualifier_list_opt
                ';'


        | '(' type_specifier               ')' type_qualifier_list_opt
                constructor_init_list_opt compound_statement

        | '(' basic_type_name              ')' type_qualifier_list_opt
                constructor_init_list_opt compound_statement

        | '(' TYPEDEFname                  ')' type_qualifier_list_opt
                constructor_init_list_opt compound_statement

        | '(' global_or_scoped_typedefname ')' type_qualifier_list_opt
                constructor_init_list_opt compound_statement

        | '(' type_qualifier_list           ')' type_qualifier_list_opt
                constructor_init_list_opt compound_statement


        | '(' type_specifier  abstract_declarator ')' type_qualifier_list_opt
                constructor_init_list_opt compound_statement

        | '(' basic_type_name abstract_declarator ')' type_qualifier_list_opt
                constructor_init_list_opt compound_statement

        /* missing entry posted below */

        | '(' global_or_scoped_typedefname abstract_declarator ')' type_qualifier_list_opt
                constructor_init_list_opt compound_statement

        | '(' type_qualifier_list          abstract_declarator ')' type_qualifier_list_opt
                constructor_init_list_opt compound_statement

        | constructor_conflicting_typedef_declarator
        ;


    /* The following have ambiguities with member declarations-
    constructor_conflicting_typedef_declarator:
      '(' TYPEDEFname abstract_declarator ')' type_qualifier_list_opt
                ';'
      |  '(' TYPEDEFname abstract_declarator ')' type_qualifier_list_opt
                constructor_init_list_opt compound_statement
      ;
    which can be deferred by expanding abstract_declarator, and in two
    cases parameter_qualifier_list, resulting in ...*/

constructor_conflicting_typedef_declarator:
        '(' TYPEDEFname unary_abstract_declarator          ')' type_qualifier_list_opt
                ';'

        | '(' TYPEDEFname unary_abstract_declarator       ')' type_qualifier_list_opt
                constructor_init_list_opt compound_statement

        | '(' TYPEDEFname postfix_abstract_declarator     ')' type_qualifier_list_opt
                ';'

        | '(' TYPEDEFname postfix_abstract_declarator     ')' type_qualifier_list_opt
                constructor_init_list_opt compound_statement


        | '(' TYPEDEFname postfixing_abstract_declarator  ')' type_qualifier_list_opt
                ';'

        | '(' TYPEDEFname postfixing_abstract_declarator  ')' type_qualifier_list_opt
                constructor_init_list_opt compound_statement
        ;


constructor_init_list_opt:
        /* nothing */
        | constructor_init_list
        ;

constructor_init_list:
        ':' constructor_init
        | constructor_init_list ',' constructor_init
        ;

constructor_init:
        IDENTIFIER   '(' argument_expression_list ')'
        | IDENTIFIER '('                          ')'

        | TYPEDEFname '(' argument_expression_list ')'
        | TYPEDEFname '('                          ')'
        | global_or_scoped_typedefname '(' argument_expression_list ')'
        | global_or_scoped_typedefname '('                          ')'

        | '(' argument_expression_list ')' /* Single inheritance ONLY*/
        | '(' ')' /* Is this legal? It might be default! */
        ;

declarator:
        identifier_declarator
        | typedef_declarator
        ;

typedef_declarator:
        paren_typedef_declarator          /* would be ambiguous as parameter*/
        | simple_paren_typedef_declarator /* also ambiguous */
        | parameter_typedef_declarator    /* not ambiguous as parameter*/
        ;

parameter_typedef_declarator:
        TYPEDEFname
        | TYPEDEFname postfixing_abstract_declarator
        | clean_typedef_declarator
        ;


    /*  The  following  have  at  least  one  '*'or '&'.  There is no 
    (redundant) '(' between the '*'/'&'  and  the  TYPEDEFname.  This 
    definition  is  critical  in  that  a redundant paren that it too 
    close to the TYPEDEFname (i.e.,  nothing  between  them  at  all) 
    would  make  the TYPEDEFname into a parameter list, rather than a 
    declarator.*/

clean_typedef_declarator:
        clean_postfix_typedef_declarator
        | asterisk_or_ampersand parameter_typedef_declarator
        | unary_modifier        parameter_typedef_declarator
        ;

clean_postfix_typedef_declarator:
        '('   clean_typedef_declarator ')'
        | '(' clean_typedef_declarator ')' postfixing_abstract_declarator
        ;


    /* The following have a redundant '(' placed immediately  to  the 
    left  of the TYPEDEFname.  This opens up the possibility that the 
    TYPEDEFname is really the start of a parameter list, and *not*  a 
    declarator*/

paren_typedef_declarator:
        postfix_paren_typedef_declarator
        | asterisk_or_ampersand '(' simple_paren_typedef_declarator ')'
        | unary_modifier        '(' simple_paren_typedef_declarator ')'
        | asterisk_or_ampersand '(' TYPEDEFname ')' /* redundant paren */
        | unary_modifier        '(' TYPEDEFname ')' /* redundant paren */
        | asterisk_or_ampersand paren_typedef_declarator
        | unary_modifier        paren_typedef_declarator
        ;

postfix_paren_typedef_declarator:
        '(' paren_typedef_declarator ')'
        | '(' simple_paren_typedef_declarator postfixing_abstract_declarator ')'
        | '(' TYPEDEFname postfixing_abstract_declarator ')'              /* redundant paren */
        | '(' paren_typedef_declarator ')' postfixing_abstract_declarator
        ;


    /*  The following excludes lone TYPEDEFname to help in a conflict 
    resolution.  We have special cased lone  TYPEDEFname  along  side 
    all uses of simple_paren_typedef_declarator */

simple_paren_typedef_declarator:
        '(' TYPEDEFname ')'
        | '(' simple_paren_typedef_declarator ')'
        ;

identifier_declarator:
        unary_identifier_declarator
        | paren_identifier_declarator
        ;


    /*  The  following  allows  "function return array of" as well as 
    "array of function returning".  It COULD be cleaned  up  the  way 
    abstract  declarators  have been.  This change might make it hard 
    to recover from user's syntax errors, whereas now they appear  as 
    simple constraint errors. */

unary_identifier_declarator:
        postfix_identifier_declarator
        | asterisk_or_ampersand identifier_declarator
        | unary_modifier        identifier_declarator
        ;

postfix_identifier_declarator:
        paren_identifier_declarator           postfixing_abstract_declarator
        | '(' unary_identifier_declarator ')'
        | '(' unary_identifier_declarator ')' postfixing_abstract_declarator
        ;

old_function_declarator:
        postfix_old_function_declarator
        | asterisk_or_ampersand old_function_declarator
        | unary_modifier      old_function_declarator
        ;


    /*  ANSI  C  section  3.7.1  states  "An identifier declared as a 
    typedef name shall not be redeclared as a parameter".  Hence  the 
    following is based only on IDENTIFIERs.

    Instead  of identifier_lists, an argument_expression_list is used 
    in  old  style  function   definitions.    The   ambiguity   with 
    constructors   required   the  use  of  argument  lists,  with  a 
    constraint verification of the list (e_g.: check to see that  the 
    "expressions" consisted of lone identifiers).

    An interesting ambiguity appeared:
        const constant=5;
        int foo(constant) ...

    Is  this an old function definition or constructor?  The decision 
    is made later by THIS grammar based on trailing context :-). This 
    ambiguity is probably what caused many parsers to give up on  old 
    style function definitions. */

postfix_old_function_declarator:
        paren_identifier_declarator '(' argument_expression_list ')'
        | '(' old_function_declarator ')'
        | '(' old_function_declarator ')' old_postfixing_abstract_declarator
        ;

old_postfixing_abstract_declarator:
        array_abstract_declarator /* array modifiers */
        | old_parameter_type_list  /* function returning modifiers */
        ;

abstract_declarator:
        unary_abstract_declarator
        | postfix_abstract_declarator
        | postfixing_abstract_declarator
        ;

postfixing_abstract_declarator:
        array_abstract_declarator
        | parameter_type_list
        ;

array_abstract_declarator:
        '[' ']'
        | '[' constant_expression ']'
        | array_abstract_declarator '[' constant_expression ']'
        ;

unary_abstract_declarator:
        asterisk_or_ampersand
        | unary_modifier
        | asterisk_or_ampersand abstract_declarator
        | unary_modifier        abstract_declarator
        ;

postfix_abstract_declarator:
        '(' unary_abstract_declarator ')'
        | '(' postfix_abstract_declarator ')'
        | '(' postfixing_abstract_declarator ')'
        | '(' unary_abstract_declarator ')' postfixing_abstract_declarator
        ;

asterisk_or_ampersand:
        '*'
        | '&'
        ;

unary_modifier:
        scope '*' type_qualifier_list_opt
        | asterisk_or_ampersand type_qualifier_list
        ;



/************************* NESTED SCOPE SUPPORT ******************************/


    /*  The  actions taken in the rules that follow involve notifying 
    the lexer that it should use the scope specified to determine  if 
    the  next  IDENTIFIER  token is really a TYPEDEFname token.  Note 
    that the actions must be taken before the parse has a  chance  to 
    "look-ahead" at the token that follows the "::", and hence should 
    be  done  during  a  reduction to "scoping_name" (which is always 
    followed by CLCL).  Since we are defining an  LR(1)  grammar,  we 
    are  assured  that  an action specified *before* the :: will take 
    place before the :: is shifted, and hence before the  token  that 
    follows the CLCL is scanned/lexed. */

    /*  Note that at the end of each of the following rules we should 
    be sure that the tag name is  in,  or  placed  in  the  indicated 
    scope.   If  no  scope  is  specified, then we must add it to our 
    current scope IFF it cannot  be  found  in  an  external  lexical 
    scope. */

scoping_name:
        tag_name
        | aggregate_key tag_name /* also update symbol table here by notifying it about a (possibly) new tag*/
        ;

scope:
        scoping_name CLCL
        | scope scoping_name  CLCL
        ;


    /*  Don't try to simplify the count of non-terminals by using one 
    of the other definitions of  "IDENTIFIER  or  TYPEDEFname"  (like 
    "label").   If you reuse such a non-terminal, 2 RR conflicts will 
    appear. The conflicts are LALR-only. The underlying cause of  the 
    LALR-only   conflict   is  that  labels,  are  followed  by  ':'.  
    Similarly, structure elaborations which provide a derivation have 
    have ':' just  after  tag_name  This  reuse,  with  common  right 
    context, is too much for an LALR parser. */

tag_name:
        IDENTIFIER
        | TYPEDEFname
        ;

global_scope:
        { /*scan for upcoming name in file scope */ } CLCL
        ;

global_or_scope:
        global_scope
        | scope
        | global_scope scope
        ;


    /*  The  following can be used in an identifier based declarator. 
    (Declarators  that  redefine  an  existing  TYPEDEFname   require 
    special  handling,  and are not included here).  In addition, the 
    following are valid "identifiers" in  an  expression,  whereas  a 
    TYPEDEFname is NOT.*/

scope_opt_identifier:
                IDENTIFIER
        | scope IDENTIFIER  /* C++ not ANSI C */
        ;

scope_opt_complex_name:
                complex_name
        | scope complex_name
        ;

complex_name:
        '~' TYPEDEFname
        | operator_function_name
        ;


    /*  Note that the derivations for global_opt_scope_opt_identifier 
    and global_opt_scope_opt_complex_name must be  placed  after  the 
    derivation:

       paren_identifier_declarator : scope_opt_identifier

    There  are several states with RR conflicts on "(", ")", and "[".  
    In these states we give up and assume a declaration, which  means 
    resolving   in  favor  of  paren_identifier_declarator.  This  is 
    basically the "If it can be  a  declaration  rule...",  with  our 
    finite cut off. */

global_opt_scope_opt_identifier:
        global_scope scope_opt_identifier
        |            scope_opt_identifier
        ;

global_opt_scope_opt_complex_name:
        global_scope scope_opt_complex_name
        |            scope_opt_complex_name
        ;


    /*  Note  that we exclude a lone TYPEDEFname.  When all alone, it 
    gets involved in a lot of ambiguities (re: function like cast  vs 
    declaration),   and  hence  must  be  special  cased  in  several 
    contexts. Note that generally every use of scoped_typedefname  is 
    accompanied by a parallel production using lone TYPEDEFname */

scoped_typedefname:
        scope TYPEDEFname
        ;

global_or_scoped_typedefname:
                       scoped_typedefname
        | global_scope scoped_typedefname
        | global_scope TYPEDEFname
        ;

global_opt_scope_opt_typedefname:
        TYPEDEFname
        | global_or_scoped_typedefname
        ;

%%
yyerror(string)
char*string;
{
    printf("parser error: %s\n", string);
}


main()
{
    yyparse();
}
