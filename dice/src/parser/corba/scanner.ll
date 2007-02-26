%{
/**
 *    \file    dice/src/parser/corba/scanner.ll
 *    \brief   contains the scanner for the CORBA IDL
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
using namespace std;

#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "fe/FETypedDeclarator.h"

#include "fe/FETaggedStructType.h"
#include "fe/FETaggedUnionType.h"
#include "fe/FETaggedEnumType.h"
#include "fe/FESimpleType.h"
#include "fe/FEUserDefinedType.h"
#include "fe/FEArrayType.h"

#include "fe/FEArrayDeclarator.h"
#include "fe/FEConstDeclarator.h"

#include "fe/FEVersionAttribute.h"

#include "fe/FEConditionalExpression.h"
#include "fe/FEUserDefinedExpression.h"
#include "fe/FEUnionCase.h"

#include "parser/corba/parser.h"
#include "Compiler.h"
#include "CParser.h"
#include "CPreProcess.h"

#define YY_DECL int yylex(YYSTYPE* lvalp)

// current linenumber and filename
extern int gLineNumber;
extern string sInFileName;
extern string sInPathName;
extern string sTopLevelInFileName;

// we don't need unput so "remove" additional code
#define YY_NO_UNPUT

// #include/import special treatment
int nNewLineNumberCORBA;
string sNewFileNameCORBA;
bool bNeedToSetFileCORBA = true;
extern int corbadebug;

extern bool bExpectFileName;

// number conversion
static unsigned int OctToInt(char*);
static unsigned int HexToInt(char*);
// character conversion
static int OctToChar(char*);
static int HexToChar(char*);
static int EscapeToChar(char*);
%}

%option noyywrap

/* some rules */
Id                [a-zA-Z_][a-zA-Z0-9_]*
Integer            (-)?[1-9][0-9]*
string            "\""([^\\\n\"]|"\\"[ntvbrfa\\\?\'\"\n])*"\""
Char_lit        "'"."'"
Fixed_point        (-)?[0-9]*"."[0-9]*

Float_literal1    [0-9]*"."[0-9]+((e|E)[+|-]?[0-9]+)?
Float_literal2    [0-9]+"."((e|E)[+|-]?[0-9]+)?
Float_literal3    [0-9]+((e|E)[+|-]?[0-9]+)
Float            (-)?({Float_literal1}|{Float_literal2}|{Float_literal3})

Hexadec        0[xX][a-fA-F0-9]+
Octal          0[0-7]*
Escape         (L)?"'""\\"[ntvbrfa\\\?\'\"]"'"
Oct_char       (L)?"'""\\"[0-7]{1,3}"'"
Hex_char       (L)?"'""\\"(x|X)[a-fA-F0-9]{1,2}"'"

Filename        ("\""|"<")("<")?[a-zA-Z0-9_\./\\:\- ]*(">")?("\""|">")

/* Include support */
%x line
%x line2

%%

    /* since CPP ran over our IDL files, we have 'line' directives in the source code,
     * which have the following format
     * # linenum filename flags
     * flags is optional and can be 1, 2, 3, or 4
     * 1 - start of file
     * 2 - return to file (after an include)
     * 3 - start of system header file -> certain warnings should be surpressed
     * 4 - return to file (from system header file)
     * There can be multiple flags, which are seperated by spaces
     */
"#"             {
                    BEGIN(line);
                    sNewFileNameCORBA = "";
                    bNeedToSetFileCORBA = true;
                }
<line,line2>[ \t]+ /* eat white-spaces */
<line>[0-9]+    {
                    nNewLineNumberCORBA = atoi(yytext);
                }
<line>{Filename} {
                    BEGIN(line2);
                       // check the filename
                       if (strlen (yytext) > 2)
                       {
                        sNewFileNameCORBA = yytext;
                        while (sNewFileNameCORBA[0] == '"')
                            sNewFileNameCORBA.erase(sNewFileNameCORBA.begin());
                        while (*(sNewFileNameCORBA.end()-1) == '"')
                            sNewFileNameCORBA.erase(sNewFileNameCORBA.end()-1);
                       }
                       else
                           sNewFileNameCORBA = sTopLevelInFileName;
                    if (sNewFileNameCORBA == "<stdin>")
                        sNewFileNameCORBA = sTopLevelInFileName;
                    if (sNewFileNameCORBA == "<built-in>")
                        sNewFileNameCORBA = sTopLevelInFileName;
                    if (sNewFileNameCORBA == "<command line>")
                        sNewFileNameCORBA = sTopLevelInFileName;
                }
<line2>[1-4]    {
                    // find path file file
                    int nFlags = atoi(yytext);
                    CParser *pParser = CParser::GetCurrentParser();
                    if (corbadebug)
                        fprintf(stderr, "CORBA: # %d \"%s\" %d found\n", nNewLineNumberCORBA,
                            sNewFileNameCORBA.c_str(), nFlags);
                    switch(nFlags)
                    {
                    case 1:
                    case 3:
                        {
                            // get path for file
                            CPreProcess *pPreProcess = CPreProcess::GetPreProcessor();
                            unsigned char nRet = 2;
                            // comment the following line to allow recursive self-inclusion
                            if (sNewFileNameCORBA != sInFileName)
                                // import this file
                                nRet = pParser->Import(sNewFileNameCORBA);
                            switch (nRet)
                            {
                            case 0:
                                // error
                                yyterminate();
                                break;
                            case 1:
                                // ok, but create file
                                {
                                    string sPath = pPreProcess->FindPathToFile(sNewFileNameCORBA, gLineNumber);
                                    string sOrigName = pPreProcess->GetOriginalIncludeForFile(sNewFileNameCORBA, gLineNumber);
                                    bool bStdInc = pPreProcess->IsStandardInclude(sNewFileNameCORBA, gLineNumber);
                                    // IDL files should be included in the search list of the preprocessor.
                                    // nonetheless, we do this additional check, just to make sure...

                                    // if path is set and origname is empty, then sNewFileNameGccC is with full path
                                    // and FindPathToFile returned an include path that matches the beginning of the
                                    // string. Now we get the original name by cutting off the beginning of the string,
                                    // which is the path
                                    // ITS DEACTIVATED BUT THERE
                                    //if (!sPath.empty() && sOrigName.empty())
                                    //    sOrigName = sNewFileNameCORBA.substr(sPath.length());
                                    if (sOrigName.empty())
                                        sOrigName = sNewFileNameCORBA;
                                    // simply create new CFEFile and set it as current
                                    CFEFile *pFEFile = new CFEFile(sOrigName, sPath, gLineNumber, bStdInc);
                                    CParser::SetCurrentFile(pFEFile);
                                    sInFileName = sNewFileNameCORBA;
                                    gLineNumber = nNewLineNumberCORBA;
                                    if (corbadebug)
                                        fprintf(stderr, " gLineNumber: %d (%s)\n", gLineNumber, sInFileName.c_str());
                                }
                                // fall through
                            case 2:
                                // ok do nothing
                                break;
                            }
                        }
                        bNeedToSetFileCORBA = false;
                        break;
                    case 2:
                        // check if we have to switch the parsers back
                        // e.g. by sending an EOF (yyterminate)
                        if (pParser->DoEndImport())
                        {
                            // update state info for parent, so its has new line number
                            // Gcc might have eliminated some lines, so old line number can be incorrect
                            pParser->UpdateState(sNewFileNameCORBA, nNewLineNumberCORBA);
                            if (corbadebug)
                                fprintf(stderr, "CORBA: Update line info for %s to %d\n", sNewFileNameCORBA.c_str(), nNewLineNumberCORBA);
                            bNeedToSetFileCORBA = false;
                            return EOF_TOKEN;
                        }
                        else
                            CParser::SetCurrentFileParent();
                        fprintf(stderr, "CORBA: End of file %s, new file: %s at line %d (update? %s)\n",
                            sInFileName.c_str(), sNewFileNameCORBA.c_str(), nNewLineNumberCORBA,
                            bNeedToSetFileCORBA ? "yes" : "no");
                        break;
                    case 4:
                        break;
                    }
                }
<line,line2>("\r")?"\n" {
                    BEGIN(INITIAL);
                    if (bNeedToSetFileCORBA)
                    {
                        gLineNumber = nNewLineNumberCORBA;
                        if (sNewFileNameCORBA.empty())
                            sInFileName = sTopLevelInFileName;
                        else
                            sInFileName = sNewFileNameCORBA;
                        if (corbadebug)
                            fprintf(stderr, "CORBA: # %d \"%s\"\n", gLineNumber, sInFileName.c_str());
                    }
                }


","            return COMMA;
";"            return SEMICOLON;
":"            return COLON;
"::"        return SCOPE;
"("            return LPAREN;
")"            return RPAREN;
"{"            return LBRACE;
"}"            return RBRACE;
"["            return LBRACKET;
"]"            return RBRACKET;
"<"            return LT;
">"            return GT;
"="            return IS;
"|"            return BITOR;
"^"            return BITXOR;
"&"            return BITAND;
"~"            return BITNOT;
">>"        return RSHIFT;
"<<"        return LSHIFT;
"+"            return PLUS;
"-"            return MINUS;
"*"            return MUL;
"/"            return DIV;
"%"            return MOD;

abstract    return ABSTRACT;
any            return ANY;
attribute    return ATTRIBUTE;
boolean        return BOOLEAN;
case        return CASE;
char        return CHAR;
const        return CONST;
context        return CONTEXT;
custom        return CUSTOM;
default        return DEFAULT;
double        return DOUBLE;
enum        return ENUM;
exception    return EXCEPTION;
FALSE        return FALSE;
factory        return FACTORY;
fixed        return FIXED;
float        return FLOAT;
in            return IN;
inout        return INOUT;
interface    return INTERFACE;
module        return MODULE;
library        return MODULE; // supports KA IDLs
native        return NATIVE;
long        return LONG;
Object        return OBJECT;
octet        return OCTET;
oneway        return ONEWAY;
out            return OUT;
private        return PRIVATE;
public        return PUBLIC;
raises        return RAISES;
readonly    return READONLY;
sequence    return SEQUENCE;
short        return SHORT;
string        return STRING;
struct        return STRUCT;
supports    return SUPPORTS;
switch        return SWITCH;
TRUE        return TRUE;
truncatable    return TRUNCABLE;
typedef        return TYPEDEF;
union        return UNION;
unsigned    return UNSIGNED;
ValueBase    return VALUEBASE;
valuetype    return VALUETYPE;
void        return VOID;
wchar        return WCHAR;
wstring        return WSTRING;
fpage        return FPAGE;
refstring    return REFSTRING;

{Id}        {
                lvalp->_id = strdup(yytext);
                return ID;
            }
{Integer}    {
                lvalp->_int = atoi(yytext);
                return LIT_INT;
            }
L{Char_lit}    {
                lvalp->_chr = yytext[3];
                return LIT_WCHAR;
            }
{Char_lit}    {
                lvalp->_chr = yytext[2];
                return LIT_CHAR;
            }
L{string}    {
                lvalp->_str = new string(&yytext[1]);
                while (*(lvalp->_str->end()-1) == '"')
                    lvalp->_str->erase(lvalp->_str->end()-1);
                return LIT_WSTR;
            }
{Float}        {
                lvalp->_flt = atof(yytext);
                return LIT_FLOAT;
            }
{Fixed_point} {
                lvalp->_flt = atof(yytext);
                return LIT_FLOAT;
            }
{Hexadec}    {
                lvalp->_int = HexToInt (yytext + 2);    /* strip off the "0x" */
                return LIT_INT;
            }
{Octal}        {
                 lvalp->_int = OctToInt (yytext);    /* first 0 doesn't matter */
                 return LIT_INT;
            }
{Escape}    {
                 lvalp->_chr = EscapeToChar (yytext);
                 return LIT_CHAR;
            }
{Oct_char}    {
                 lvalp->_chr = OctToChar (yytext);
                 return LIT_CHAR;
            }
{Hex_char}    {
                 lvalp->_chr = HexToChar (yytext);
                 return LIT_CHAR;
            }
{Filename} {
                if (bExpectFileName)
                {
                    lvalp->_str = new string(&yytext[1]);
                    while ((*(lvalp->_str->end()-1) == '"') ||
                           (*(lvalp->_str->end()-1) == '>'))
                        lvalp->_str->erase(lvalp->_str->end()-1);
                    return FILENAME;
                }
                else
                {
                    // sequence type
                    REJECT;
                }
            }
{string}    {
                lvalp->_str = new string(&yytext[1]);
                while (*(lvalp->_str->end()-1) == '"')
                    lvalp->_str->erase(lvalp->_str->end()-1);
                return LIT_STR;
            }

"/*"        {
                register int c;
                for (;;) {
                    while ((c = yyinput()) != '*' && c != EOF && c != '\n') ; // eat up text of comment
                    if (c == '*') {
                        while ((c = yyinput()) == '*') ; // eat up trailing *
                        if (c == '/') break; // found end
                    }
                    if (c == '\n') gLineNumber++;
                    if (c == EOF) {
                        CCompiler::GccError(CParser::GetCurrentFile(), gLineNumber, "EOF in comment.");
                        yyterminate();
                        break;
                    }
                }
            }

"//".*        /* eat C++ style comments */

[ \t]*        /* eat whitespace */

("\r")?"\n"    { ++gLineNumber; }
"\f"        { ++gLineNumber; }

<<EOF>>     {
                yyterminate(); // stops corbaparse()
            }
.            {
                CCompiler::GccWarning(CParser::GetCurrentFile(), gLineNumber, "Unknown character \"%s\".",yytext);
            }

%%

// convert hexadecimal numbers to integers
static unsigned int HexToInt(char *s)
{
    // already removed 0x from beginning
    unsigned long res = 0, lastres = 0;
    while (((*s >= '0' && *s <= '9') ||
            (*s >= 'a' && *s <= 'f') ||
            (*s >= 'A' && *s <= 'F')) &&
           (lastres <= res)) {
        lastres = res;
        if (*s >= '0' && *s <= '9')
            res = res * 16 + *s - '0';
        else
            res = res * 16 + ((*s & 0x4f) - 'A' + 10);
        s++;
    }
    if (lastres > res)
        printf("%s:%d: Hexadecimal number overflow.\n",sInFileName.c_str(), gLineNumber);
    return res;
}

// convert octal numbers to integers
static unsigned int OctToInt(char *s)
{
    // already removed leading 0
    unsigned long res = 0, lastres = 0;
    while (*s >= '0' && *s < '8' && lastres <= res) {
        lastres = res;
        res = res * 8 + *s - '0';
        s++;
    }
    if (lastres > res)
        printf("%s:%d: Octal number overflow.\n",sInFileName.c_str(), gLineNumber);
    return res;
}

// convert octal coded characters to ASCII char
static int OctToChar(char *s)
{
    // check for leading 'L'
    if (s[0] == 'L') s++;
    // skip the first ' and backslash
    s+=2;
    // now we can use the integer converter to obtain ASCII code
    return (int)OctToInt(s);
}

// convert hexadecimal coded characters to ASCII char
static int HexToChar(char *s)
{
    // check for leading 'L'
    if (s[0] == 'L') s++;
    // skip the first ' and backslash
    s+=2;
    // now we can use the integer converter to obtain ASCII code
    return (int)HexToInt(s);
}

// converts escape sequences to character
static int EscapeToChar(char *s)
{
    // check for leading 'L'
    if (s[0] == 'L') s++;
    // skip the first ' and backslash
    s+=2;
    switch (s[0]) {
    case 'n' :
        return '\n';
        break;
    case 't' :
        return '\t';
        break;
    case 'v' :
        return '\v';
        break;
    case 'b' :
        return '\b';
        break;
    case 'r' :
        return '\r';
        break;
    case 'f' :
        return '\f';
        break;
    case 'a' :
        return '\a';
        break;
    case '\\' :
        return '\\';
        break;
    case '?' :
        return '?';
        break;
    case '\'' :
        return '\'';
        break;
    case '"' :
        return '"';
        break;
    }
    return 0;
}

#ifndef YY_CURRENT_BUFFER_LVALUE
#define YY_CURRENT_BUFFER_LVALUE YY_CURRENT_BUFFER
#endif

void* GetCurrentBufferCorba()
{
    if ( YY_CURRENT_BUFFER )
    {
        /* Flush out information for old buffer. */
        *yy_c_buf_p = yy_hold_char;
        YY_CURRENT_BUFFER_LVALUE->yy_buf_pos = yy_c_buf_p;
        YY_CURRENT_BUFFER_LVALUE->yy_n_chars = yy_n_chars;
        if (corbadebug)
            fprintf(stderr, "GetCurrentBufferCorba: FILE: %p, buffer: %p, pos: %p, chars: %d\n",
                YY_CURRENT_BUFFER_LVALUE->yy_input_file, YY_CURRENT_BUFFER, yy_c_buf_p, yy_n_chars);
    }
    return YY_CURRENT_BUFFER;
}

void RestoreBufferCorba(void *buffer, bool bInit)
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
        if (corbadebug)
        {
            fprintf(stderr, "RestoreBufferCorba: FILE: %p, buffer: %p, pos: %p, chars: %d (chars left: %d) init:%s\n",
                YY_CURRENT_BUFFER_LVALUE->yy_input_file, YY_CURRENT_BUFFER,
                yy_c_buf_p, yy_n_chars, yy_n_chars-(int)(yy_c_buf_p-(char*)(YY_CURRENT_BUFFER_LVALUE->yy_ch_buf)),
                bInit?"true":"false");
            fprintf(stderr, "RestoreBufferCorba: continue with %s:%d\n", sInFileName.c_str(), gLineNumber);
        }
    }
}

void StartBufferCorba(FILE* fInput)
{
    yy_switch_to_buffer( yy_create_buffer(fInput, YY_BUF_SIZE) );
    if (corbadebug)
        fprintf(stderr, "StartBufferCorba: FILE: %p, buffer: %p\n",
            YY_CURRENT_BUFFER_LVALUE->yy_input_file, YY_CURRENT_BUFFER);
}

