%{
/**
 *    \file    dice/src/parser/gcc-c/scanner.ll
 *    \brief   contains the scanner for the gcc C code
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
#include <ctype.h>
#include <string>
#include <algorithm>
using namespace std;

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

#include "parser/gcc-c/parser.h"
#include "Compiler.h"
#include "CParser.h"
#include "CPreProcess.h"

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#define YY_DECL int yylex(YYSTYPE* lvalp)

// current linenumber and filename
extern int gLineNumber;
extern string sInFileName;
extern string sInPathName;
extern string sTopLevelInFileName;

extern int gcc_cdebug;

// we don't need unput so "remove" additional code
#define YY_NO_UNPUT

// #include/import special treatment
int nNewLineNumberGccC;
string sNewFileNameGccC;
bool bNeedToSetFileGccC = true;

extern bool bExpectFileName;

// forward declaration
static int HexToInt (char *s);
static unsigned int HexToUInt (char *s);
static int OctToInt (char *s);
static unsigned int OctToUInt (char *s);
#if SIZEOF_LONG_LONG > 0
static long long HexToLLong (char *s);
static unsigned long long HexToULLong (char *s);
static long long OctToLLong (char *s);
static unsigned long long OctToULLong (char *s);
#endif

#define INTTYPE_INT     1
#define INTTYPE_UINT    2
#define INTTYPE_LLONG   3
#define INTTYPE_ULLONG  4

static unsigned char IntType(char *s);

// helper var to distinguish tagged typedef
bool bTaggedTypeAllowed = false;

%}

%option noyywrap

/* some rules */
Id                [a-zA-Z_][a-zA-Z0-9_]*
Integer_Suffix  ([uU][lL]?)|([uU]([lL]{2}))|([lL][uU]?)|([lL]{2}[uU]?)
Integer            [1-9][0-9]*{Integer_Suffix}?
Filename        (\"|"<")("<")?[a-zA-Z0-9_\./\\:\- ]*(">")?(\"|">")
string          \"([^\\\n\"]|\\[ntvbrfa\\\?\'\"\n])*\"
C99String       \"([^\"]|\\\")*\"
Char_lit        \'.\'
Fixed_point        [0-9]*"."[0-9]*

Float_literal1    [0-9]*"."[0-9]+((e|E)[+|-]?[0-9]+)?
Float_literal2    [0-9]+"."((e|E)[+|-]?[0-9]+)?
Float_literal3    [0-9]+((e|E)[+|-]?[0-9]+)
Float            ({Float_literal1}|{Float_literal2}|{Float_literal3})

Hexadec        0[xX][a-fA-F0-9]+{Integer_Suffix}?
Octal          0[0-7]*{Integer_Suffix}?
Escape         (L)?\'\\[ntvbrfa\\\?\'\"]\'
Oct_char       (L)?\'\\[0-7]{1,3}\'
Hex_char       (L)?\'\\(x|X)[a-fA-F0-9]{1,2}\'

/* Include support */
%x line
%x line2

%%

    /* removed rules: they collide with linux header files

    bycopy          { bTaggedTypeAllowed = false; return BYCOPY; }
    byref           { bTaggedTypeAllowed = false; return BYREF; }
    in              { bTaggedTypeAllowed = false; return IN; }
    out             { bTaggedTypeAllowed = false; return OUT; }
    inout           { bTaggedTypeAllowed = false; return INOUT; }
    oneway          { bTaggedTypeAllowed = false; return ONEWAY; }

    */

    /* ignore pragma directives */
^[ \t]*"#pragma".*("\r")?"\n"   /* ignore */

    /* since CPP ran over our IDL files, we have 'line' directives in the source code,
     * which have the following format
     * # linenum filename flags
     * flags is optional and can be 1, 2, 3, or 4
     * 1 - start of file
     * 2 - return to file (after an include)
     * 3 - start of system header file -> certain warnings should be surpressed
     * 4 - extern "C" start
     * There can be multiple flags, which are seperated by spaces
     */
^[ \t]*"#"      {
                    BEGIN(line);
                    sNewFileNameGccC = "";
                    bNeedToSetFileGccC = true;
                }
<line,line2>[ \t]+ /* eat white-spaces */
<line>[0-9]+    {
                    nNewLineNumberGccC = atoi(yytext);
                }
<line>{Filename} {
                    BEGIN(line2);
                    // check the filename
                    if (strlen (yytext) > 2)
                    {
                    sNewFileNameGccC = yytext;
                    while (sNewFileNameGccC[0] == '"')
                        sNewFileNameGccC.erase(sNewFileNameGccC.begin());
                    while (*(sNewFileNameGccC.end()-1) == '"')
                        sNewFileNameGccC.erase(sNewFileNameGccC.end()-1);
                    }
                    else
                        sNewFileNameGccC = sTopLevelInFileName;
                    if (sNewFileNameGccC == "<stdin>")
                        sNewFileNameGccC = sTopLevelInFileName;
                    if (sNewFileNameGccC == "<built-in>")
                        sNewFileNameGccC = sTopLevelInFileName;
                    if (sNewFileNameGccC == "<command line>")
                        sNewFileNameGccC = sTopLevelInFileName;
                }
<line2>[1-4]    {
                    // find path file file
                    int nFlags = atoi(yytext);
                    if (gcc_cdebug)
                        fprintf(stderr, "C: # %d \"%s\" %d found\n", nNewLineNumberGccC,
                            sNewFileNameGccC.c_str(), nFlags);
                    // this is a bad hack to avoid resetting of state
                    CParser *pParser = CParser::GetCurrentParser();
                    switch(nFlags)
                    {
                    case 1:
                    case 3:
                        {
                            // get path for file
                            CPreProcess *pPreProcess = CPreProcess::GetPreProcessor();
                            unsigned char nRet = 2;
                            // comment following line to allow recursive self-inclusion
                            //if (sNewFileNameGccC != sInFileName)
                                // import this file
                                nRet = pParser->Import(sNewFileNameGccC);
                            switch (nRet)
                            {
                            case 0:
                                // error
                                yyterminate();
                                break;
                            case 1:
                                // ok, but create file
                                // did not create a new parser, instead we change the
                                // current file only
                                {
                                    string sPath = pPreProcess->FindPathToFile(sNewFileNameGccC, gLineNumber);
                                    string sOrigName = pPreProcess->GetOriginalIncludeForFile(sNewFileNameGccC, gLineNumber);
                                    bool bStdInc = pPreProcess->IsStandardInclude(sNewFileNameGccC, gLineNumber);
                                    // if path is set and origname is empty, then sNewFileNameGccC is with full path
                                    // and FindPathToFile returned an include path that matches the beginning of the
                                    // string. Now we get the original name by cutting off the beginning of the string,
                                    // which is the path
                                    if (!sPath.empty() && sOrigName.empty())
                                        sOrigName = sNewFileNameGccC.substr(sPath.length());
                                    if (sOrigName.empty())
                                        sOrigName = sNewFileNameGccC;
                                    // simply create new CFEFile and set it as current
                                    CFEFile *pFEFile = new CFEFile(sOrigName, sPath, gLineNumber, bStdInc);
                                    CParser::SetCurrentFile(pFEFile);
                                    sInFileName = sNewFileNameGccC;
                                    gLineNumber = nNewLineNumberGccC;
                                    if (gcc_cdebug)
                                        fprintf(stderr, " gLineNumber: %d (%s)\n", gLineNumber, sInFileName.c_str());
                                }
                                // fall through
                            case 2:
                                // ok do nothing
                                break;
                            }
                        }
                        bNeedToSetFileGccC = false;
                        break;
                    case 2:
                        if (gcc_cdebug)
                            fprintf(stderr, "Gcc-C: End of file %s, new file: %s at line %d (update? %s)\n",
                                sInFileName.c_str(), sNewFileNameGccC.c_str(), nNewLineNumberGccC,
                                bNeedToSetFileGccC ? "yes" : "no");
                        // check if we have to switch the parsers back
                        // e.g. by sending an EOF (yyterminate)
                        if (pParser->DoEndImport())
                        {
                            // update state info for parent, so its has new line number
                            // Gcc might have eliminated some lines, so old line number can be incorrect
                            pParser->UpdateState(sNewFileNameGccC, nNewLineNumberGccC);
                            bTaggedTypeAllowed = false;
                            return EOF_TOKEN;
                        }
                        else
                            CParser::SetCurrentFileParent();
                        break;
                    case 4:
                        break;
                    }
                }
<line,line2>("\r")?"\n" {
                    // a line without the num, so only line number info
                    // is updated (# line "file")
                    BEGIN(INITIAL);
                    if (bNeedToSetFileGccC)
                    {
                        gLineNumber = nNewLineNumberGccC;
                        if (sNewFileNameGccC.empty())
                            sInFileName = sTopLevelInFileName;
                        else
                            sInFileName = sNewFileNameGccC;
                        if (gcc_cdebug)
                            fprintf(stderr, "Gcc-C: # %d \"%s\" (reset line number)\n", gLineNumber, sInFileName.c_str());
                    }
                }

"["     { bTaggedTypeAllowed = false; return LBRACKET; }
"]"        { bTaggedTypeAllowed = false; return RBRACKET; }
"("        { bTaggedTypeAllowed = false; return LPAREN; }
")"        { bTaggedTypeAllowed = false; return RPAREN; }
"{"        { bTaggedTypeAllowed = false; return LBRACE; }
"}"        { bTaggedTypeAllowed = false; return RBRACE; }
"."        { bTaggedTypeAllowed = false; return DOT; }
"->"        { bTaggedTypeAllowed = false; return POINTSAT; }
"++"        { bTaggedTypeAllowed = false; return INCR; }
"--"        { bTaggedTypeAllowed = false; return DECR; }
"&"            { bTaggedTypeAllowed = false; return BITAND; }
"*"            { bTaggedTypeAllowed = false; return MUL; }
"+"            { bTaggedTypeAllowed = false; return PLUS; }
"-"            { bTaggedTypeAllowed = false; return MINUS; }
"~"            { bTaggedTypeAllowed = false; return TILDE; }
"!"            { bTaggedTypeAllowed = false; return EXCLAM; }
"/"            { bTaggedTypeAllowed = false; return DIV; }
"%"            { bTaggedTypeAllowed = false; return MOD; }
">>"        { bTaggedTypeAllowed = false; return RSHIFT; }
"<<"        { bTaggedTypeAllowed = false; return LSHIFT; }
"<"            { bTaggedTypeAllowed = false; return LT; }
">"            { bTaggedTypeAllowed = false; return GT; }
"<="        { bTaggedTypeAllowed = false; return LTEQUAL; }
">="        { bTaggedTypeAllowed = false; return GTEQUAL; }
"=="        { bTaggedTypeAllowed = false; return EQUAL; }
"!="        { bTaggedTypeAllowed = false; return NOTEQUAL; }
"^"            { bTaggedTypeAllowed = false; return BITXOR; }
"|"            { bTaggedTypeAllowed = false; return BITOR; }
"&&"        { bTaggedTypeAllowed = false; return ANDAND; }
"||"        { bTaggedTypeAllowed = false; return OROR; }
"?"            { bTaggedTypeAllowed = false; return QUESTION; }
":"            { bTaggedTypeAllowed = false; return COLON; }
";"            { bTaggedTypeAllowed = false; return SEMICOLON; }
"..."       { bTaggedTypeAllowed = false; return ELLIPSIS; }
"="            { bTaggedTypeAllowed = false; return IS; }
"*="        { bTaggedTypeAllowed = false; return MUL_ASSIGN; }
"/="        { bTaggedTypeAllowed = false; return DIV_ASSIGN; }
"%="        { bTaggedTypeAllowed = false; return MOD_ASSIGN; }
"+="        { bTaggedTypeAllowed = false; return PLUS_ASSIGN; }
"-="        { bTaggedTypeAllowed = false; return MINUS_ASSIGN; }
"<<="        { bTaggedTypeAllowed = false; return LSHIFT_ASSIGN; }
">>="        { bTaggedTypeAllowed = false; return RSHIFT_ASSIGN; }
"&="        { bTaggedTypeAllowed = false; return AND_ASSIGN; }
"^="        { bTaggedTypeAllowed = false; return XOR_ASSIGN; }
"|="        { bTaggedTypeAllowed = false; return OR_ASSIGN; }
","            { bTaggedTypeAllowed = false; return COMMA; }
"::"        { bTaggedTypeAllowed = false; return SCOPE; }

auto        { bTaggedTypeAllowed = false; return AUTO; }
break        { bTaggedTypeAllowed = false; return BREAK; }
case        { bTaggedTypeAllowed = false; return CASE; }
char        { bTaggedTypeAllowed = false; return CHAR; }
const        { bTaggedTypeAllowed = false; return CONST; }
continue    { bTaggedTypeAllowed = false; return CONTINUE; }
default        { bTaggedTypeAllowed = false; return DEFAULT; }
do            { bTaggedTypeAllowed = false; return DO; }
double        { bTaggedTypeAllowed = false; return DOUBLE; }
else        { bTaggedTypeAllowed = false; return ELSE; }
enum        { bTaggedTypeAllowed = false; return ENUM; }
extern        { bTaggedTypeAllowed = false; return EXTERN; }
float        { bTaggedTypeAllowed = false; return FLOAT; }
for            { bTaggedTypeAllowed = false; return FOR; }
goto        { bTaggedTypeAllowed = false; return GOTO; }
if            { bTaggedTypeAllowed = false; return IF; }
inline        { bTaggedTypeAllowed = false; return INLINE; }
__inline    { bTaggedTypeAllowed = false; return INLINE; }
__inline__  { bTaggedTypeAllowed = false; return INLINE; }
int            { bTaggedTypeAllowed = false; return INT; }
long        { bTaggedTypeAllowed = false; return LONG; }
long[ \t]+long  { bTaggedTypeAllowed = false; return LONGLONG; }
register    { bTaggedTypeAllowed = false; return REGISTER; }
restrict    { bTaggedTypeAllowed = false; return RESTRICT; }
__restrict  { bTaggedTypeAllowed = false; return RESTRICT; }
return        { bTaggedTypeAllowed = false; return RETURN; }
short        { bTaggedTypeAllowed = false; return SHORT; }
signed        { bTaggedTypeAllowed = false; return SIGNED; }
__signed__  { bTaggedTypeAllowed = false; return SIGNED; }
sizeof        { bTaggedTypeAllowed = false; return SIZEOF; }
static        { bTaggedTypeAllowed = false; return STATIC; }
struct        { bTaggedTypeAllowed = false; return STRUCT; }
switch        { bTaggedTypeAllowed = false; return SWITCH; }
typedef        { bTaggedTypeAllowed = false; return TYPEDEF; }
union        { bTaggedTypeAllowed = false; return UNION; }
unsigned    { bTaggedTypeAllowed = false; return UNSIGNED; }
void        { bTaggedTypeAllowed = false; return VOID; }
volatile    { bTaggedTypeAllowed = false; return VOLATILE; }
__volatile__    { bTaggedTypeAllowed = false; return VOLATILE; }
__volatile  { bTaggedTypeAllowed = false; return VOLATILE; }
while        { bTaggedTypeAllowed = false; return WHILE; }
_Bool        { bTaggedTypeAllowed = false; return UBOOL; }
_Complex    { bTaggedTypeAllowed = false; return UCOMPLEX; }
_Imaginary    { bTaggedTypeAllowed = false; return UIMAGINARY; }

asm            { bTaggedTypeAllowed = false; return ASM_KEYWORD; }
__asm        { bTaggedTypeAllowed = false; return ASM_KEYWORD; }
__asm__        { bTaggedTypeAllowed = false; return ASM_KEYWORD; }
typeof      { bTaggedTypeAllowed = false; return TYPEOF; }
__typeof    { bTaggedTypeAllowed = false; return TYPEOF; }
__typeof__  { bTaggedTypeAllowed = false; return TYPEOF; }
alignof     { bTaggedTypeAllowed = false; return ALIGNOF; }
extension   /* eat extension keyword */
__extension__   /* eat extension keyword */
label       { bTaggedTypeAllowed = false; return LABEL; }
__real        { bTaggedTypeAllowed = false; return REALPART; }
__real__    { bTaggedTypeAllowed = false; return REALPART; }
__imag        { bTaggedTypeAllowed = false; return IMAGPART; }
__imag__    { bTaggedTypeAllowed = false; return IMAGPART; }
attribute    { bTaggedTypeAllowed = false; return ATTRIBUTE; }
__attribute    { bTaggedTypeAllowed = false; return ATTRIBUTE; }
__attribute__    { bTaggedTypeAllowed = false; return ATTRIBUTE; }
__const            { bTaggedTypeAllowed = false; return CONST; }
__const__        { bTaggedTypeAllowed = false; return CONST; }
__label         { bTaggedTypeAllowed = false; return LOCAL_LABEL; }
__label__       { bTaggedTypeAllowed = false; return LOCAL_LABEL; }

__builtin_va_list   {
                lvalp->_id = new string(yytext);
                bTaggedTypeAllowed = false;
                return TYPENAME;
            }

{Id}         {
                lvalp->_id = new string(yytext);
                // get top level file of current scope, which is the last import statement
                CParser *pCurrentParser = CParser::GetCurrentParser();
                CFEFile *pFile = pCurrentParser->GetTopFileInScope();
                assert (pFile);
                if (pFile->FindUserDefinedType (yytext))
                {
                    bTaggedTypeAllowed = false;
                    return TYPENAME;
                }
                // try tagged decl
                if (pFile->FindTaggedDecl(yytext) &&
                    bTaggedTypeAllowed)
                {
                    bTaggedTypeAllowed = false;
                    return TYPENAME;
                }
                // typename in scope not found, check else-where
                pFile = dynamic_cast<CFEFile*>(pFile->GetRoot());
                if (pFile && pFile->FindUserDefinedType(yytext))
                {
                    bTaggedTypeAllowed = false;
                    return TYPENAME;
                }
                if (pFile && pFile->FindTaggedDecl(yytext) &&
                    bTaggedTypeAllowed)
                {
                    bTaggedTypeAllowed = false;
                    return TYPENAME;
                }
                // typename not found, so it has to be an identifier
                if (gcc_cdebug)
                    fprintf(stderr,"ID(4): %s\n",yytext);
                bTaggedTypeAllowed = false;
                return IDENTIFIER;
            }
{Integer}    {
                bTaggedTypeAllowed = false;
                // check ending
                switch (IntType(yytext))
                {
                case INTTYPE_ULLONG:
#if HAVE_ATOLL
                    lvalp->_ullong = atoll(yytext);
                    return ULONGLONG_CONST;
#else
                    CCompiler::GccWarning(CParser::GetCurrentFile(), gLineNumber,
                        "'atoll' not provided on this architecture");
                    lvalp->_ulong = atol(yytext);
                    return ULONG_CONST;
#endif
                    break;
                case INTTYPE_LLONG:
#if HAVE_ATOLL
                    lvalp->_llong = atoll(yytext);
                    return LONGLONG_CONST;
#else
                    CCompiler::GccWarning(CParser::GetCurrentFile(), gLineNumber,
                        "'atoll' not provided on this architecture");
                    lvalp->_long = atol(yytext);
                    return LONG_CONST;
#endif
                    break;
                case INTTYPE_UINT:
                    lvalp->_uint = atol(yytext);
                    return UINTEGER_CONST;
                    break;
                case INTTYPE_INT:
                default:
                    lvalp->_int = atoi(yytext);
                    return INTEGER_CONST;
                break;
                }
            }
{Octal}     {
                bTaggedTypeAllowed = false;
                // check ending
                switch (IntType(yytext))
                {
                case INTTYPE_ULLONG:
#if HAVE_ATOLL
                    lvalp->_ullong = OctToULLong(&yytext[1]);
                    return ULONGLONG_CONST;
#else
                    CCompiler::GccWarning(CParser::GetCurrentFile, gLineNumber,
                        "'long long' not supported on this architecture");
                    lvalp->_ulong = OctoToULong(&yytext[1]);
                    return ULONG_ CONST;
#endif
                    break;
                case INTTYPE_LLONG:
#if HAVE_ATOLL
                    lvalp->_llong = OctToLLong(&yytext[1]);
                    return LONGLONG_CONST;
#else
                    CCompiler::GccWarning(CParser::GetCurrentFile, gLineNumber,
                        "'long long' not supported on this architecture");
                    lvalp->_long = OctoToLong(&yytext[1]);
                    return LONG_CONST;
#endif
                    break;
                case INTTYPE_UINT:
                    lvalp->_uint = OctToUInt(&yytext[1]);
                    return UINTEGER_CONST;
                    break;
                case INTTYPE_INT:
                default:
                    lvalp->_int = OctToInt(&yytext[1]);
                    return INTEGER_CONST;
                break;
                }
            }
{Hexadec}   {
                bTaggedTypeAllowed = false;
                // check ending
                switch (IntType(yytext))
                {
                case INTTYPE_ULLONG:
                    lvalp->_ullong = HexToULLong(&yytext[1]);
                    return ULONGLONG_CONST;
                    break;
                case INTTYPE_LLONG:
                    lvalp->_llong = HexToLLong(&yytext[1]);
                    return LONGLONG_CONST;
                    break;
                case INTTYPE_UINT:
                    lvalp->_uint = HexToUInt(&yytext[1]);
                    return UINTEGER_CONST;
                    break;
                case INTTYPE_INT:
                default:
                    lvalp->_int = HexToInt(&yytext[1]);
                    return INTEGER_CONST;
                break;
                }
            }
{Float}     {
                lvalp->_float = atof(yytext);
                bTaggedTypeAllowed = false;
                return FLOAT_CONST;
            }
{Char_lit}  {
                lvalp->_char = yytext[1];
                bTaggedTypeAllowed = false;
                return CHAR_CONST;
            }
{string}    {
                lvalp->_str = new string(&yytext[1]);
                while (*(lvalp->_str->end()-1) == '"')
                    lvalp->_str->erase(lvalp->_str->end()-1);
                bTaggedTypeAllowed = false;
                return STRING;
            }
{C99String} {
                // allows embedded newlines
                lvalp->_str = new string(&yytext[1]);
                while (*(lvalp->_str->end()-1) == '"')
                    lvalp->_str->erase(lvalp->_str->end()-1);
                // replace \n with \\n
                string::size_type pos;
                while ((pos = lvalp->_str->find("\n")) != string::npos)
                {
                    lvalp->_str->replace(pos, strlen("\n"), "\\n", strlen("\\n"));
                }
                bTaggedTypeAllowed = false;
                return STRING;
            }
"/*"        {
                register int c;
                for (;;) {
                    while ((c = yyinput()) != '*' && c != EOF && c != '\n') ; // eat up text of comment
                    if (c == '*')
                    {
                        while ((c = yyinput()) == '*') ; // eat up trailing *
                        if (c == '/') break; // found end
                    }
                    if (c == '\n')
                    {
                        gLineNumber++;
                        if (gcc_cdebug)
                            fprintf(stderr, " gLineNumber: %d (%s)\n", gLineNumber, sInFileName.c_str());
                    }
                    if (c == EOF) {
                        CCompiler::GccError(CParser::GetCurrentFile(), gLineNumber, "EOF in comment.");
                        yyterminate();
                        break;
                    }
                }
            }

"//".*        /* eat C++ style comments */

[ \t]*        /* eat whitespace */

("\r")?"\n"    { ++gLineNumber; if (gcc_cdebug) fprintf(stderr, " gLineNumber: %d (%s)\n", gLineNumber, sInFileName.c_str()); }
"\f"        { ++gLineNumber; if (gcc_cdebug) fprintf(stderr, " gLineNumber: %d (%s)\n", gLineNumber, sInFileName.c_str()); }

<<EOF>>     {
                if (gcc_cdebug)
                    fprintf(stderr, "Stop file %s\n", sInFileName.c_str());
                yyterminate(); // stops gcc_cparse
            }
.            {
                CCompiler::GccWarning(CParser::GetCurrentFile(), gLineNumber, "Unknown character \"%s\".",yytext);
            }

%%

#if SIZEOF_LONG_LONG > 0

// convert hexadecimal numbers to integers
static unsigned long long HexToULLong (char *s)
{
     // already removed 0x from beginning
    unsigned long long res = 0, lastres = 0;
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

static long long HexToLLong(char *s)
{
    return (long long)HexToULLong(s);
}

// convert octal numbers to integers
static unsigned long long OctToULLong (char *s)
{
     // already removed leading 0
     unsigned long long res = 0, lastres = 0;
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

static long long OctToLLong(char*s)
{
    return (long long)OctToULLong(s);
}

#endif

static unsigned int HexToUInt(char*s)
{
    return (unsigned int)HexToULLong(s);
}

static int HexToInt(char*s)
{
    return (int)HexToUInt(s);
}

static unsigned int OctToUInt(char*s)
{
    return (unsigned int)OctToULLong(s);
}

static int OctToInt(char*s)
{
    return (int)OctToUInt(s);
}

static unsigned char IntType(char *s)
{
    // check ending
    // if "ll" or "ull" -> longlong
    // could end on u,ul,ull,l,ll,lu,llu
    int length = strlen(yytext);
    if ((length > 3) &&
        ( (tolower(yytext[length-1]) == 'l') &&
        (tolower(yytext[length-2]) == 'l') &&
        (tolower(yytext[length-3]) == 'u') ) ||
        ( (tolower(yytext[length-1]) == 'u') &&
        (tolower(yytext[length-2]) == 'l') &&
        (tolower(yytext[length-3]) == 'l') ))
    {
        return INTTYPE_ULLONG;
    }
    if ((length > 2) &&
        (tolower(yytext[length-1]) == 'l') &&
        (tolower(yytext[length-2]) == 'l'))
    {
        return INTTYPE_LLONG;
    }
    if ((length > 2) &&
        ( (tolower(yytext[length-1]) == 'l') &&
        (tolower(yytext[length-2]) == 'u') ) ||
        ( (tolower(yytext[length-1]) == 'u') &&
        (tolower(yytext[length-2]) == 'l') ))
    {
        return INTTYPE_UINT;
    }
    if ((length > 1) &&
        (tolower(yytext[length-1]) == 'u'))
    {
        return INTTYPE_UINT;
    }
    return INTTYPE_INT;
}

#ifndef YY_CURRENT_BUFFER_LVALUE
#define YY_CURRENT_BUFFER_LVALUE YY_CURRENT_BUFFER
#endif

void* GetCurrentBufferGccC()
{
    if ( YY_CURRENT_BUFFER )
    {
        /* Flush out information for old buffer. */
        *yy_c_buf_p = yy_hold_char;
        YY_CURRENT_BUFFER_LVALUE->yy_buf_pos = yy_c_buf_p;
        YY_CURRENT_BUFFER_LVALUE->yy_n_chars = yy_n_chars;
        if (gcc_cdebug)
            fprintf(stderr, "GetCurrentBufferC: FILE: %p, buffer: %p, pos: %p, chars: %d\n",
                YY_CURRENT_BUFFER_LVALUE->yy_input_file, YY_CURRENT_BUFFER, yy_c_buf_p, yy_n_chars);
    }
    return YY_CURRENT_BUFFER;
}

void RestoreBufferGccC(void *buffer, bool bInit)
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
        // C Parser always starts with line context, because C files are not included by us,
        // but by GCC and it may leave some flags after '# 1 "filename" 1' -> to skip them
        // change into line2 context
        bNeedToSetFileGccC = false;
        BEGIN(line2);
        if (gcc_cdebug)
        {
            fprintf(stderr, "RestoreBufferGccC: FILE: %p, buffer: %p, pos: %p, chars: %d (chars left: %d)\n",
                YY_CURRENT_BUFFER_LVALUE->yy_input_file, YY_CURRENT_BUFFER,
                yy_c_buf_p, yy_n_chars, yy_n_chars-(int)(yy_c_buf_p-(char*)(YY_CURRENT_BUFFER_LVALUE->yy_ch_buf)));
            fprintf(stderr, "RestoreBufferGccC: continue with %s:%d\n", sInFileName.c_str(), gLineNumber);
        }
    }
}

void StartBufferGccC(FILE* fInput)
{
    yy_switch_to_buffer( yy_create_buffer(fInput, YY_BUF_SIZE) );
    if (gcc_cdebug)
        fprintf(stderr, "StartBufferGccC: FILE: %p, buffer: %p\n",
            YY_CURRENT_BUFFER_LVALUE->yy_input_file, YY_CURRENT_BUFFER);
}
