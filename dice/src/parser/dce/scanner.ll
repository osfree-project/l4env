%{
/**
 *    \file    dice/src/parser/dce/scanner.ll
 *    \brief   contains the scanner for the DCE IDL
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

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <cassert>

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
#include "Compiler.h"
#include "CParser.h"
#include "CPreProcess.h"

#define YY_DECL int yylex(YYSTYPE* lvalp)

// current linenumber and filename
extern int gLineNumber;
extern string sInFileName;
extern string sInPathName;
extern string sTopLevelInFileName;

int nNewLineNumberDCE;
string sNewFileNameDCE;
bool bReadFileForLineDCE = false;
bool bNeedToSetFileDCE = true;
bool bAllowInterfaceAsType = false;

extern int dcedebug;


// number conversion
static unsigned int OctToInt (char *);
static unsigned int HexToInt (char *);
// character conversion
static int OctToChar (char *);
static int HexToChar (char *);
static int EscapeToChar (char *);

extern int c_inc; // set to attribute

// helper macros
// if c_inc is set to 'attribute' (2)
#define RETURN_IF_ATTR(token) { \
    if (c_inc == 2) { return token; } \
    else { lvalp->_id = new string(yytext); if (dcedebug) fprintf(stderr,"( ID(3): %s (%d) )",yytext,c_inc); return ID; } \
    }

// we don't need unput so "remove" additional code
#define YY_NO_UNPUT

%}

%option noyywrap nounput
%option never-interactive
/* %option reentrant-bison */

/* some rules */
Float_literal1 [0-9]*"."[0-9]+((e|E)[+|-]?[0-9]+)?
Float_literal2 [0-9]+"."((e|E)[+|-]?[0-9]+)?
Float_literal3 [0-9]+((e|E)[+|-]?[0-9]+)
Id             [a-zA-Z_][a-zA-Z0-9_]*
Integer        [1-9][0-9]*(((u|U)(l|L)?)|((l|L)(u|U)?))?
Hexadec        0[xX][a-fA-F0-9]+
Octal          0[0-7]*
Float         ({Float_literal1}|{Float_literal2}|{Float_literal3})(f|l|F|L)?
Char_lit       (L)?\'.\'
Escape         (L)?\'\\[ntvbrfa\\\?\'\"]\'
Oct_char       (L)?\'\\[0-7]{1,3}\'
Hex_char       (L)?\'\\(x|X)[a-fA-F0-9]{1,2}\'
Filename        (\"|"<")("<")?[a-zA-Z0-9_\./\\:\- ]*(">")?(\"|">")
string         (L)?(\"([^\\\n\"]|\\[ntvbrfa\\\?\'\"\n])*\")
Portspec       \"[^\n\\][ \t]*:[ \t]*\[[^\n\\][ \t]*\][ \t]*\"
Uuid       [a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12}
VersionRep      [1-9][0-9]*("."[0-9]+)?

/* Include support */
%x line
%x line2

%%

    /* since CPP ran over our IDL files, we have 'line' directives in the 
     * source code, which have the following format
     * # linenum filename flags
     * flags is optional and can be 1, 2, 3, or 4
     * 1 - start of file
     * 2 - return to file (after an include)
     * 3 - start of system header file -> certain warnings should be surpressed
     * 4 - extern "C" start
     * There can be multiple flags, which are seperated by spaces
     */
"#"             {
                    BEGIN(line);
                    sNewFileNameDCE = "";
                    bNeedToSetFileDCE = true;
                }
<line,line2>[ \t]+ /* eat white-spaces */
<line>[0-9]+    {
                    nNewLineNumberDCE = atoi(yytext);
                }
<line>{Filename} {
                    BEGIN(line2);
                    // check the filename
                    if (strlen (yytext) > 2)
                    {
                        sNewFileNameDCE = yytext;
                        while (sNewFileNameDCE[0] == '"')
                            sNewFileNameDCE.erase(sNewFileNameDCE.begin());
                        while (*(sNewFileNameDCE.end()-1) == '"')
                            sNewFileNameDCE.erase(sNewFileNameDCE.end()-1);
                    }
                    else
                        sNewFileNameDCE = sTopLevelInFileName;
                    if (sNewFileNameDCE == "<stdin>")
                        sNewFileNameDCE = sTopLevelInFileName;
                    if (sNewFileNameDCE == "<built-in>")
                        sNewFileNameDCE = sTopLevelInFileName;
                    if (sNewFileNameDCE == "<command line>")
                        sNewFileNameDCE = sTopLevelInFileName;
                }
<line2>[1-4]    {
		    // if line directive is without these flags only new line
		    // numbers are defined.  find path file
                    int nFlags = atoi(yytext);
                    if (dcedebug)
                        fprintf(stderr, "DCE: # %d \"%s\" %d found\n", 
			    nNewLineNumberDCE, sNewFileNameDCE.c_str(), nFlags);
                    CParser *pParser = CParser::GetCurrentParser();
                    switch(nFlags)
                    {
                    case 1:
                    {
                        // get path for file
                        CPreProcess *pPreProcess = 
			    CPreProcess::GetPreProcessor();
                        unsigned char nRet = 2;
                        // comment following line to allow recursive 
			// self-inclusion
                        if (sNewFileNameDCE != sInFileName)
                            // import this file
                            nRet = pParser->Import(sNewFileNameDCE);
                        switch (nRet)
                        {
                        case 0:
                            // error
                            yyterminate();
                            break;
                        case 1:
                            // ok, but create file
                        {
                            // simply create new CFEFile and set it as current
                            string sPath = pPreProcess->FindPathToFile(
			        sNewFileNameDCE, gLineNumber);
                            string sOrigName = 
			        pPreProcess->GetOriginalIncludeForFile(
				    sNewFileNameDCE, gLineNumber);
                            bool bStdInc = pPreProcess->IsStandardInclude(
			        sNewFileNameDCE, gLineNumber);
			    // The returned path should never be empty. If it
			    // is, then the file name sNewFileNameDCE did not
			    // contain it. The preprocessor, however, stored
			    // it in an additional vector, so go check there.
			    if (sPath.empty())
			        sPath =
				pPreProcess->GetIncludePath(sNewFileNameDCE);
                            // IDL files should be included in the search 
			    // list of the preprocessor.  nonetheless, we do
			    // this additional check, just to make sure...

			    // if path is set and origname is empty, then
			    // sNewFileNameGccC is with full path and
			    // FindPathToFile returned an include path that
			    // matches the beginning of the string. Now we get
			    // the original name by cutting off the beginning
			    // of the string, which is the path
                            // ITS DEACTIVATED BUT THERE
			    //if (!sPath.empty() && sOrigName.empty())
			    //sOrigName =
			    //sNewFileNameDCE.Right(
			    //    sNewFileNameDCE.GetLength()-
			    //    sPath.GetLength());
			    if (sOrigName.empty())
                                sOrigName = sNewFileNameDCE;
                            CFEFile *pFEFile = new CFEFile(sOrigName, sPath, 
			        gLineNumber, bStdInc);
                            CParser::SetCurrentFile(pFEFile);
                            sInFileName = sNewFileNameDCE;
                            gLineNumber = nNewLineNumberDCE;
                            if (dcedebug)
                                fprintf(stderr, " gLineNumber: %d (%s)\n", 
				    gLineNumber, sInFileName.c_str());
                        }
                        // fall through
                        case 2:
                            // ok do nothing
                            break;
                        }
                    }
                        bNeedToSetFileDCE = false;
                        break;
                    case 2:
                        // check if we have to switch the parsers back
                        if (pParser->DoEndImport())
                        {
			    // update state info for parent, so its has new
			    // line number Gcc might have eliminated some
			    // lines, so old line number can be incorrect
                            pParser->UpdateState(sNewFileNameDCE, 
			        nNewLineNumberDCE);
                            bNeedToSetFileDCE = false;
                            return EOF_TOKEN;
                        }
                        else
                            CParser::SetCurrentFileParent();
                        break;
                    case 3:
                    case 4:
		    	/* ignore 3 and 4, because they only provide
			 * additional meaning to the line statement
			 */
                        break;
                    }
                }
<line,line2>("\r")?"\n" {
                    // a line without the num, so only line number info
                    // is updated (# line "file")
                    BEGIN(INITIAL);
                    if (bNeedToSetFileDCE)
                    {
                        gLineNumber = nNewLineNumberDCE;
                        if (sNewFileNameDCE.empty())
                            sInFileName = sTopLevelInFileName;
                        else
                            sInFileName = sNewFileNameDCE;
                        if (dcedebug)
                            fprintf(stderr, "DCE: # %d \"%s\" (reset line)\n", 
			        gLineNumber, sInFileName.c_str());
                    }
                }

("{"|"<%")      return LBRACE;
("}"|"%>")      return RBRACE;
("["|"<:")      return LBRACKET;
("]"|":>")      return RBRACKET;
":"             return COLON;
"::"            return SCOPE;
","             return COMMA;
"("             return LPAREN;
")"             return RPAREN;
"."             return DOT;
"\""            return QUOT;
"*"             return ASTERISK;
"'"             return SINGLEQUOT;
"?"             return QUESTION;
"|"             return BITOR;
"^"             return BITXOR;
"&"             return BITAND;
"<"             return LT;
">"             return GT;
"+"             return PLUS;
"-"             return MINUS;
"/"             return DIV;
"%"             return MOD;
"~"             return TILDE;
"!"             return EXCLAM;
";"             return SEMICOLON;
"||"            return LOGICALOR;
"&&"            return LOGICALAND;
"=="            return EQUAL;
"!="            return NOTEQUAL;
"<="            return LTEQUAL;
">="            return GTEQUAL;
"<<"            return LSHIFT;
">>"            return RSHIFT;
".."            return DOTDOT;
"="             return IS;

boolean         return BOOLEAN;
byte            return BYTE;
case            return CASE;
char            return CHAR;
const           return CONST;
default         return DEFAULT;
double          return DOUBLE;
enum            return ENUM;
false           return FALSE;
FALSE           return FALSE;
float           return FLOAT;
handle_t        return HANDLE_T;
hyper           return HYPER;
int             return INT;
interface       return INTERFACE;
long            return LONG;
"long"[ \t]+"long"    return LONGLONG;
null            return EXPNULL;
pipe            return PIPE;
short           return SHORT;
small           return SMALL;
struct          return STRUCT;
switch          return SWITCH;
true            return TRUE;
TRUE            return TRUE;
typedef         return TYPEDEF;
union           return UNION;
unsigned        return UNSIGNED;
signed          return SIGNED;
void            return VOID;
error_status_t  return ERROR_STATUS_T;

flexpage        return FLEXPAGE;
fpage           return FLEXPAGE;
ISO_LATIN_1     return ISO_LATIN_1;
ISO_MULTI_LINGUAL     return ISO_MULTI_LINGUAL;
ISO_UCS         return ISO_UCS;
refstring       return REFSTRING;
broadcast       RETURN_IF_ATTR(BROADCAST)
context_handle  RETURN_IF_ATTR(CONTEXT_HANDLE)
endpoint        RETURN_IF_ATTR(ENDPOINT)
exceptions      RETURN_IF_ATTR(EXCEPTIONS)
first_is        RETURN_IF_ATTR(FIRST_IS)
handle          RETURN_IF_ATTR(HANDLE)
idempotent      RETURN_IF_ATTR(IDEMPOTENT)
ignore          RETURN_IF_ATTR(IGNORE)
in              return IN;
last_is         RETURN_IF_ATTR(LAST_IS)
length_is       RETURN_IF_ATTR(LENGTH_IS)
local           RETURN_IF_ATTR(LOCAL)
max_is          RETURN_IF_ATTR(MAX_IS)
maybe           RETURN_IF_ATTR(MAYBE)
min_is          RETURN_IF_ATTR(MIN_IS)
out             return OUT;
inout           return INOUT;
ptr             RETURN_IF_ATTR(PTR)
pointer_default RETURN_IF_ATTR(POINTER_DEFAULT)
ref             RETURN_IF_ATTR(REF)
reflect_deletions     RETURN_IF_ATTR(REFLECT_DELETIONS)
size_is         RETURN_IF_ATTR(SIZE_IS)
string          RETURN_IF_ATTR(STRING)
switch_is       RETURN_IF_ATTR(SWITCH_IS)
switch_type     RETURN_IF_ATTR(SWITCH_TYPE)
transmit_as     RETURN_IF_ATTR(TRANSMIT_AS)
unique          RETURN_IF_ATTR(UNIQUE)
uuid            RETURN_IF_ATTR(UUID)
version         RETURN_IF_ATTR(VERSION_ATTR)
object          RETURN_IF_ATTR(OBJECT)
abstract        RETURN_IF_ATTR(ABSTRACT)
iid_is          RETURN_IF_ATTR(IID_IS)
raises          return RAISES;
exception       return EXCEPTION;
library         return LIBRARY;
module          return MODULE;
control         return CONTROL;
helpcontext     RETURN_IF_ATTR(HELPCONTEXT)
helpfile        RETURN_IF_ATTR(HELPFILE)
helpstring      RETURN_IF_ATTR(HELPSTRING)
default_function      RETURN_IF_ATTR(DEFAULT_FUNCTION)
error_function  RETURN_IF_ATTR(ERROR_FUNCTION)
error_function_client RETURN_IF_ATTR(ERROR_FUNCTION_CLIENT)
errfunc_client  RETURN_IF_ATTR(ERROR_FUNCTION_CLIENT)
error_function_server RETURN_IF_ATTR(ERROR_FUNCTION_SERVER)
errfunc_server  RETURN_IF_ATTR(ERROR_FUNCTION_SERVER)
server_parameter      RETURN_IF_ATTR(SERVER_PARAMETER)
init_with_in    RETURN_IF_ATTR(INIT_WITH_IN)
prealloc        RETURN_IF_ATTR(PREALLOC)
prealloc_client RETURN_IF_ATTR(PREALLOC_CLIENT)
prealloc_server RETURN_IF_ATTR(PREALLOC_SERVER)
allow_reply_only      RETURN_IF_ATTR(ALLOW_REPLY_ONLY)
hidden          RETURN_IF_ATTR(HIDDEN)
lcid            RETURN_IF_ATTR(LCID)
restricted      RETURN_IF_ATTR(RESTRICTED)
auto_handle     RETURN_IF_ATTR(AUTO_HANDLE)
binding_callout RETURN_IF_ATTR(BINDING_CALLOUT)
code            RETURN_IF_ATTR(CODE)
comm_status     RETURN_IF_ATTR(COMM_STATUS)
cs_char         RETURN_IF_ATTR(CS_CHAR)
cs_drtag        RETURN_IF_ATTR(CS_DRTAG)
cs_rtag         RETURN_IF_ATTR(CS_RTAG)
cs_stag         RETURN_IF_ATTR(CS_STAG)
cs_tag_rtn      RETURN_IF_ATTR(TAG_RTN)
enable_allocate RETURN_IF_ATTR(ENABLE_ALLOCATE)
extern_exceptions     RETURN_IF_ATTR(EXTERN_EXCEPTIONS)
explicit_handle RETURN_IF_ATTR(EXPLICIT_HANDLE)
fault_status    RETURN_IF_ATTR(FAULT_STATUS)
heap            RETURN_IF_ATTR(HEAP)
implicit_handle RETURN_IF_ATTR(IMPLICIT_HANDLE)
nocode          RETURN_IF_ATTR(NOCODE)
represent_as    RETURN_IF_ATTR(REPRESENT_AS)
user_marshal    RETURN_IF_ATTR(USER_MARSHAL)
without_using_exceptions    RETURN_IF_ATTR(WITHOUT_USING_EXCEPTIONS)
init_rcvstring  RETURN_IF_ATTR(INIT_RCVSTRING)
init_rcvstring_client  RETURN_IF_ATTR(INIT_RCVSTRING_CLIENT)
init_rcvstring_server  RETURN_IF_ATTR(INIT_RCVSTRING_SERVER)
sizeof          return SIZEOF;
oneway          RETURN_IF_ATTR(ONEWAY);
callback        RETURN_IF_ATTR(CALLBACK);
noopcode        RETURN_IF_ATTR(NOOPCODE);
noexceptions    RETURN_IF_ATTR(NOEXCEPTIONS);
l4_schedule_deceit     RETURN_IF_ATTR(SCHED_DONATE);
l4_sched_deceit        RETURN_IF_ATTR(SCHED_DONATE);
schedule_donate        RETURN_IF_ATTR(SCHED_DONATE);
sched_donate	       RETURN_IF_ATTR(SCHED_DONATE);
dedicated_partner      RETURN_IF_ATTR(DEDICATED_PARTNER);
attribute       return ATTRIBUTE;
__attribute__	return ATTRIBUTE;
">>="           return RS_ASSIGN;
"<<="           return LS_ASSIGN;
"+="            return ADD_ASSIGN;
"-="            return SUB_ASSIGN;
"*="            return MUL_ASSIGN;
"/="            return DIV_ASSIGN;
"%="            return MOD_ASSIGN;
"&="            return AND_ASSIGN;
"^="            return XOR_ASSIGN;
"|="            return OR_ASSIGN;
"++"            return INC_OP;
"--"            return DEC_OP;
"->"            return PTR_OP;
"void"[ \t]*"*" return VOID_PTR;
"char"[ \t]*"*" return CHAR_PTR;

{Id}         {
        lvalp->_id = new string(yytext);
        // get top level file of current scope, which is the last import statement
        CParser *pCurrentParser = CParser::GetCurrentParser();
        CFEFile *pFile = pCurrentParser->GetTopFileInScope();
        assert (pFile);
        // test for already defined type
        if (pFile->FindUserDefinedType (yytext))
            return TYPENAME;
        // a type can also be an interface (as object)
//        if (lvalp->_id->find("::") != string::npos)
//        {
            if (pFile->FindInterface(yytext))
                return TYPENAME;
//        }
//        else
//        {
//          // lookup in current scop
//        }
        // typename not found, so it has to be an identifier
        if (dcedebug)
            fprintf(stderr,"( ID(4): %s )",yytext);
        return ID;
    }
{Integer}    {
        lvalp->_int = atoi (yytext);
        return LIT_INT;
    }
{Hexadec}    {
        lvalp->_int = HexToInt (yytext + 2);    /* strip off the "0x" */
        return LIT_INT;
    }
{Octal}        {
        lvalp->_int = OctToInt (yytext);    /* first 0 doesn't matter */
        return LIT_INT;
    }
{VersionRep} {
        lvalp->_str = new string(yytext);
        return VERSION_STR;
    }
{Float}        {
        lvalp->_double = atof (yytext);
        return LIT_FLOAT;
    }
{Char_lit}    {
        if (yytext[0] == 'L')
            lvalp->_char = yytext[2];
        else
            lvalp->_char = yytext[1];
        return LIT_CHAR;
    }
{Escape}    {
        lvalp->_char = EscapeToChar (yytext);
        return LIT_CHAR;
    }
{Oct_char}    {
        lvalp->_char = OctToChar (yytext);
        return LIT_CHAR;
    }
{Hex_char}    {
        lvalp->_char = HexToChar (yytext);
        return LIT_CHAR;
    }
{string}    {
        // check if first is 'L'
        if (yytext[0] == 'L')
        {
            lvalp->_str = new string (&yytext[2]);
            while (*(lvalp->_str->end()-1) == '"')
                lvalp->_str->erase(lvalp->_str->end()-1);
        }
        else
        {
            lvalp->_str = new string (&yytext[1]);
            while (*(lvalp->_str->end()-1) == '"')
                lvalp->_str->erase(lvalp->_str->end()-1);
        }
        return LIT_STR;
    }
{Portspec}    return PORTSPEC;
{Uuid}        {
        lvalp->_str = new string (yytext);
        return UUID_STR;
    }

"/*"        {
        register int c;
        for (;;)
        {
            while ((c = yyinput ()) != '*' && c != EOF && c != '\n');    // eat up text of comment
            if (c == '*')
            {
                while ((c = yyinput ()) == '*');    // eat up trailing *
                if (c == '/') break;    // found end
            }
            if (c == '\n') { gLineNumber++; if (dcedebug) fprintf(stderr, " gLineNumber: %d (%s)\n", gLineNumber, sInFileName.c_str()); }
            if (c == EOF)
            {
                CCompiler::GccError (CParser::GetCurrentFile(), gLineNumber, "EOF in comment.");
                yyterminate ();
                break;
            }
        }
    }

"//".*          /* eat C++ style comments */
[ \t]*          /* eat whitespace */
(\r)?\n         { ++gLineNumber; if (dcedebug) fprintf(stderr, " gLineNumber: %d (%s)\n", gLineNumber, sInFileName.c_str()); }
\f              { ++gLineNumber; if (dcedebug) fprintf(stderr, " gLineNumber: %d (%s)\n", gLineNumber, sInFileName.c_str()); }
<<EOF>>         {
                    if (dcedebug)
                        fprintf(stderr, "Stop file %s\n", sInFileName.c_str());
                    yyterminate(); // stops dceparse()
                }
.               {
                    CCompiler::GccWarning (CParser::GetCurrentFile(), gLineNumber, "Unknown character \"%s\" (0x%x).", yytext, (yytext)?yytext[0]:0);
                }


%%
// convert hexadecimal numbers to integers
static unsigned int HexToInt (char *s)
{
     // already removed 0x from beginning
     unsigned long res = 0, lastres = 0;
     while (((*s >= '0' && *s <= '9') ||
         (*s >= 'a' && *s <= 'f') ||
         (*s >= 'A' && *s <= 'F')) && (lastres <= res))
     {
         lastres = res;
         if (*s >= '0' && *s <= '9')
            res = res * 16 + *s - '0';
         else
          {
             res = res * 16 + ((*s & 0x4f) - 'A' + 10);
         }
          s++;
     }
     if (lastres > res)
         CCompiler::GccWarning (CParser::GetCurrentFile(), gLineNumber,
                "Hexadecimal number overflow.");
     return res;
}

// convert octal numbers to integers
static unsigned int OctToInt (char *s)
{
     // already removed leading 0
     unsigned long res = 0, lastres = 0;
     while (*s >= '0' && *s < '8' && lastres <= res)
     {
         lastres = res;
        res = res * 8 + *s - '0';
        s++;
     }
     if (lastres > res)
         CCompiler::GccWarning (CParser::GetCurrentFile(), gLineNumber,
                "Octal number overflow.");
     return res;
}

// convert octal coded characters to ASCII char
static int OctToChar (char *s)
{
     // check for leading 'L'
     if (s[0] == 'L') s++;
     // skip the first ' and backslash
     s += 2;
     // now we can use the integer converter to obtain ASCII code
     return (int) OctToInt (s);
}

// convert hexadecimal coded characters to ASCII char
static int HexToChar (char *s)
{
     // check for leading 'L'
     if (s[0] == 'L') s++;
     // skip the first ' and backslash
     s += 2;
     // now we can use the integer converter to obtain ASCII code
     return (int) HexToInt (s);
}

// converts escape sequences to character
static int EscapeToChar (char *s)
{
     // check for leading 'L'
     if (s[0] == 'L') s++;
     // skip the first ' and backslash
     s += 2; switch (s[0])
     {
     case 'n':
        return '\n';
        break;
     case 't':
        return '\t';
        break;
     case 'v':
        return '\v';
        break;
     case 'b':
        return '\b';
        break;
     case 'r':
        return '\r';
        break;
     case 'f':
        return '\f';
        break;
     case 'a':
        return '\a';
        break;
     case '\\':
        return '\\';
        break;
     case '?':
        return '?';
        break;
     case '\'':
        return '\'';
        break;
     case '"':
         return '"';
        break;
     }
     return 0;
}

#ifndef YY_CURRENT_BUFFER_LVALUE
#define YY_CURRENT_BUFFER_LVALUE YY_CURRENT_BUFFER
#endif

void* GetCurrentBufferDce()
{
    if ( YY_CURRENT_BUFFER )
    {
        /* Flush out information for old buffer. */
        *yy_c_buf_p = yy_hold_char;
        YY_CURRENT_BUFFER_LVALUE->yy_buf_pos = yy_c_buf_p;
        YY_CURRENT_BUFFER_LVALUE->yy_n_chars = yy_n_chars;
        if (dcedebug)
            fprintf(stderr, "GetCurrentBufferDce: FILE: %p, buffer: %p, pos: %p, chars: %d\n",
                YY_CURRENT_BUFFER_LVALUE->yy_input_file, YY_CURRENT_BUFFER, yy_c_buf_p, yy_n_chars);
    }
    return YY_CURRENT_BUFFER;
}

void RestoreBufferDce(void *buffer, bool bInit)
{
    if (buffer)
    {
        if (buffer != YY_CURRENT_BUFFER)
            yy_switch_to_buffer((YY_BUFFER_STATE)buffer);
        else
            yy_load_buffer_state();
        // check input file
        if (!yyin)
            yyin = ((YY_BUFFER_STATE)buffer)->yy_input_file;
        if (bInit)
            BEGIN(INITIAL);
        else
            BEGIN(line2);
        if (dcedebug)
        {
            fprintf(stderr, "RestoreBufferDce: FILE: %p, buffer: %p, pos: %p, chars: %d (chars left: %d)\n",
                YY_CURRENT_BUFFER_LVALUE->yy_input_file, YY_CURRENT_BUFFER,
                yy_c_buf_p, yy_n_chars, yy_n_chars-(int)(yy_c_buf_p-(char*)(YY_CURRENT_BUFFER_LVALUE->yy_ch_buf)));
            fprintf(stderr, "RestoreBufferDce: continue with %s:%d\n", sInFileName.c_str(), gLineNumber);
        }
    }
}

void StartBufferDce(FILE* fInput)
{
    yy_switch_to_buffer( yy_create_buffer(fInput, YY_BUF_SIZE) );
    if (dcedebug)
        fprintf(stderr, "StartBufferDce: FILE: %p, buffer: %p\n",
            YY_CURRENT_BUFFER_LVALUE->yy_input_file, YY_CURRENT_BUFFER);
}
