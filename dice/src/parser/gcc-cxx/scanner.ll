%{
/**
 *    \file    dice/src/parser/gcc-cxx/scanner.ll
 *    \brief   contains the scanner for the gcc C++ code
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
#include <cassert>
#include <string>
#include "fe/FEFile.h"

#include "parser.h"
#include "Compiler.h"
#include "CParser.h"
#include "CPreProcess.h"

#define YY_DECL int yylex(YYSTYPE* lvalp)

// current linenumber and filename
extern int gLineNumber;
extern int nOldLineNb;
extern string sInFileName;
extern string sInPathName;
extern string sTopLevelInFileName;


extern int gcc_cxxdebug;

// we don't need unput so "remove" additional code
#define YY_NO_UNPUT

// #include/import special treatment
int nNewLineNumberGccCxx;
string sNewFileNameGccCxx;
bool bNeedToSetFileGccCxx;

extern bool bExpectFileName;

int prevTokenCXX = 0;
%}

%option noyywrap nounput

/* some rules */
Id                [a-zA-Z_][a-zA-Z0-9_]*
Integer            (-)?[1-9][0-9]*
Filename        ("\""|"<")("<")?[a-zA-Z0-9_\./\\:\- ]*(">")?("\""|">")
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
                    sNewFileNameGccCxx = "";
                    bNeedToSetFileGccCxx = true;
                }
<line,line2>[ \t]+ /* eat white-spaces */
<line>[0-9]+    {
                    nNewLineNumberGccCxx = atoi(yytext);
                }
<line>{Filename} {
                    BEGIN(line2);
                       // check the filename
                       if (strlen (yytext) > 2)
                       {
                        sNewFileNameGccCxx = yytext;
                        while (sNewFileNameGccCxx[0] == '"')
                            sNewFileNameGccCxx.erase(sNewFileNameGccCxx.begin());
                        while (*(sNewFileNameGccCxx.end()-1) == '"')
                            sNewFileNameGccCxx.erase(sNewFileNameGccCxx.end()-1);
                       }
                       else
                           sNewFileNameGccCxx = sTopLevelInFileName;
                    if (sNewFileNameGccCxx == "<stdin>")
                        sNewFileNameGccCxx = sTopLevelInFileName;
                    if (sNewFileNameGccCxx == "<built-in>")
                        sNewFileNameGccCxx = sTopLevelInFileName;
                    if (sNewFileNameGccCxx == "<command line>")
                        sNewFileNameGccCxx = sTopLevelInFileName;
                }
<line2>[1-4]    {
                    // find path file file
                    int nFlags = atoi(yytext);
                    CParser *pParser = CParser::GetCurrentParser();
                    if (gcc_cxxdebug)
                        fprintf(stderr, "Cxx: # %d \"%s\" %d found\n", nNewLineNumberGccCxx,
                            sNewFileNameGccCxx.c_str(), nFlags);
                    switch(nFlags)
                    {
                    case 1:
                        {
                            // get path for file
                            CPreProcess *pPreProcess = CPreProcess::GetPreProcessor();
                            unsigned char nRet = 2;
                            // comment the following line to allow recursive self-inclusion
                            //if (sNewFileNameGccCxx != sInFileName)
                                // import this file
                                nRet = pParser->Import(sNewFileNameGccCxx);
                            switch (nRet)
                            {
                            case 0:
                                // error
                                yyterminate();
                                break;
                            case 1:
                                // ok, but create file
                                {
                                    string sPath = pPreProcess->FindPathToFile(sNewFileNameGccCxx, gLineNumber);
                                    string sOrigName = pPreProcess->GetOriginalIncludeForFile(sNewFileNameGccCxx, gLineNumber);
                                    bool bStdInc = pPreProcess->IsStandardInclude(sNewFileNameGccCxx, gLineNumber);
                                    // if path is set and origname is empty, then sNewFileNameGccC is with full path
                                    // and FindPathToFile returned an include path that matches the beginning of the
                                    // string. Now we get the original name by cutting off the beginning of the string,
                                    // which is the path
                                    if (!sPath.empty() && sOrigName.empty())
                                        sOrigName = sNewFileNameGccCxx.substr(sPath.length());
                                    if (sOrigName.empty())
                                        sOrigName = sNewFileNameGccCxx;
                                    // simply create new CFEFile and set it as current
                                    CFEFile *pFEFile = new CFEFile(sOrigName, sPath, gLineNumber, bStdInc);
                                    CParser::SetCurrentFile(pFEFile);
                                    sInFileName = sNewFileNameGccCxx;
                                    gLineNumber = nNewLineNumberGccCxx;
                                }
                                break;
                            case 2:
                                // ok do nothing
                                // created new parser and used it to set
                                // filename and linenumber
                                break;
                            }
                        }
                        bNeedToSetFileGccCxx = false;
                        break;
                    case 2:
                        // check if we have to switch the parsers back
                        // e.g. by sending an EOF (yyterminate)
                        if (pParser->DoEndImport())
                        {
                            // update state info for parent, so its has new line number
                            // Gcc might have eliminated some lines, so old line number can be incorrect
                            pParser->UpdateState(sNewFileNameGccCxx, nNewLineNumberGccCxx);
                            bNeedToSetFileGccCxx = false;
                            return EOF_TOKEN;
                        }
                        else
                            CParser::SetCurrentFileParent();
                        break;
                    case 3:
                    case 4:
                        break;
                    }
                }
<line,line2>("\r")?"\n" {
                    BEGIN(INITIAL);
                    if (bNeedToSetFileGccCxx)
                    {
                        gLineNumber = nNewLineNumberGccCxx;
                        if (sNewFileNameGccCxx.empty())
                            sInFileName = sTopLevelInFileName;
                        else
                            sInFileName = sNewFileNameGccCxx;
                        if (gcc_cxxdebug)
                            fprintf(stderr, "Gcc-Cxx: # %d \"%s\"\n", gLineNumber, sInFileName.c_str());
                    }
                    prevTokenCXX = 0;
                }

"."            { prevTokenCXX = DOT; return DOT; }
","            { prevTokenCXX = COMMA; return COMMA; }
";"            { prevTokenCXX = SEMICOLON; return SEMICOLON; }
":"            { prevTokenCXX = COLON; return COLON; }
"::"        { prevTokenCXX = SCOPE; return SCOPE; }
"("            { prevTokenCXX = LPAREN; return LPAREN; }
")"            { prevTokenCXX = RPAREN; return RPAREN; }
"{"            { prevTokenCXX = LBRACE; return LBRACE; }
"}"            { prevTokenCXX = RBRACE; return RBRACE; }
"["            { prevTokenCXX = LBRACKET; return LBRACKET; }
"]"            { prevTokenCXX = RBRACKET; return RBRACKET; }
"<"            { prevTokenCXX = LT; return LT; }
">"            { prevTokenCXX = GT; return GT; }
"="            { prevTokenCXX = IS; return IS; }
"|"            { prevTokenCXX = BITOR; return BITOR; }
"^"            { prevTokenCXX = BITXOR; return BITXOR; }
"&"            { prevTokenCXX = BITAND; return BITAND; }
"~"            { prevTokenCXX = BITNOT; return BITNOT; }
">>"        { prevTokenCXX = RSHIFT; return RSHIFT; }
"<<"        { prevTokenCXX = LSHIFT; return LSHIFT; }
"+"            { prevTokenCXX = PLUS; return PLUS; }
"-"            { prevTokenCXX = MINUS; return MINUS; }
"*"            { prevTokenCXX = MUL; return MUL; }
"/"            { prevTokenCXX = DIV; return DIV; }
"%"            { prevTokenCXX = MOD; return MOD; }
[^\.]"..."[^\.] { prevTokenCXX = ELLIPSIS; return ELLIPSIS; }
"&&"        { prevTokenCXX = ANDAND; return ANDAND; }
"^^"        { prevTokenCXX = OROR; return OROR; }
">="        { prevTokenCXX = GTEQUAL; return GTEQUAL; }
"<="        { prevTokenCXX = LTEQUAL; return LTEQUAL; }
"=="        { prevTokenCXX = EQUAL; return EQUAL; }
"->"        { prevTokenCXX = POINTSAT; return POINTSAT; }
"->*"       { prevTokenCXX = POINTSAT_STAR; return POINTSAT_STAR; }
".*"        { prevTokenCXX = DOT_STAR; return DOT_STAR; }
"<?"        { prevTokenCXX = CPP_MIN; return CPP_MIN; }
">?"        { prevTokenCXX = CPP_MAX; return CPP_MAX; }
"+="        { prevTokenCXX = CPP_PLUS_EQ; return CPP_PLUS_EQ; }
"-="        { prevTokenCXX = CPP_MINUS_EQ; return CPP_MINUS_EQ; }
"*="        { prevTokenCXX = CPP_MULT_EQ; return CPP_MULT_EQ; }
"/="        { prevTokenCXX = CPP_DIV_EQ; return CPP_DIV_EQ; }
"%="        { prevTokenCXX = CPP_MOD_EQ; return CPP_MOD_EQ; }
"&="        { prevTokenCXX = CPP_AND_EQ; return CPP_AND_EQ; }
"|="        { prevTokenCXX = CPP_OR_EQ; return CPP_OR_EQ; }
"^="        { prevTokenCXX = CPP_XOR_EQ; return CPP_XOR_EQ; }
">>="       { prevTokenCXX = CPP_RSHIFT_EQ; return CPP_RSHIFT_EQ; }
"<<="       { prevTokenCXX = CPP_LSHIFT_EQ; return CPP_LSHIFT_EQ; }
"<?="       { prevTokenCXX = CPP_MIN_EQ; return CPP_MIN_EQ; }
">?="       { prevTokenCXX = CPP_MAX_EQ; return CPP_MAX_EQ; }

"extern \""[cC]"\""     { prevTokenCXX = EXTERN_LANG_STRING; return EXTERN_LANG_STRING; }
"("[ \t]*")"            { prevTokenCXX = LEFT_RIGHT; return LEFT_RIGHT; }
"("[ \t]*"*"[ \t]*")"   { prevTokenCXX = PAREN_STAR_PAREN; return PAREN_STAR_PAREN; }

dynamic_cast        { prevTokenCXX = DYNAMIC_CAST; return DYNAMIC_CAST; }
static_cast         { prevTokenCXX = STATIC_CAST; return STATIC_CAST; }
const_cast          { prevTokenCXX = CONST_CAST; return CONST_CAST; }
reinterpret_cast    { prevTokenCXX = REINTERPRET_CAST; return REINTERPRET_CAST; }

extension   { prevTokenCXX = EXTENSION; return EXTENSION; }
asm         { prevTokenCXX = ASM_KEYWORD; return ASM_KEYWORD; }
namespace   { prevTokenCXX = NAMESPACE; return NAMESPACE; }
using       { prevTokenCXX = USING; return USING; }
alignof     { prevTokenCXX = ALIGNOF; return ALIGNOF; }
__real        { prevTokenCXX = REALPART; return REALPART; }
__real__    { prevTokenCXX = REALPART; return REALPART; }
__imag        { prevTokenCXX = IMAGPART; return IMAGPART; }
__imag__    { prevTokenCXX = IMAGPART; return IMAGPART; }
throw       { prevTokenCXX = THROW; return THROW; }
this        { prevTokenCXX = THIS; return THIS; }
new         { prevTokenCXX = NEW; return NEW; }
delete      { prevTokenCXX = DELETE; return DELETE; }
template    { prevTokenCXX = TEMPLATE; return TEMPLATE; }
class       { prevTokenCXX = CLASS; return CLASS; }
struct      { prevTokenCXX = STRUCT; return STRUCT; }
union       { prevTokenCXX = UNION; return UNION; }
enume       { prevTokenCXX = ENUM; return ENUM; }
return      { prevTokenCXX = RETURN_KEYWORD; return RETURN_KEYWORD; }
typeid      { prevTokenCXX = TYPEID; return TYPEID; }
__null      { prevTokenCXX = CONSTANT; return CONSTANT; }
operator        { prevTokenCXX = OPERATOR; return OPERATOR; }
label           { prevTokenCXX = LABEL; return LABEL; }
true            { prevTokenCXX = CXX_TRUE; return CXX_TRUE; }
false           { prevTokenCXX = CXX_FALSE; return CXX_FALSE; }
typeof          { prevTokenCXX = TYPEOF; return TYPEOF; }
attribute        { prevTokenCXX = ATTRIBUTE; return ATTRIBUTE; }
__attribute        { prevTokenCXX = ATTRIBUTE; return ATTRIBUTE; }
__attribute__    { prevTokenCXX = ATTRIBUTE; return ATTRIBUTE; }
enum            { prevTokenCXX = ENUM; return ENUM; }
try             { prevTokenCXX = TRY; return TRY; }
if              { prevTokenCXX = IF; return IF; }
else            { prevTokenCXX = ELSE; return ELSE; }
while           { prevTokenCXX = WHILE; return WHILE; }
do              { prevTokenCXX = DO; return DO; }
for             { prevTokenCXX = FOR; return FOR; }
switch          { prevTokenCXX = SWITCH; return SWITCH; }
case            { prevTokenCXX = CASE; return CASE; }
default         { prevTokenCXX = DEFAULT; return DEFAULT; }
break           { prevTokenCXX = BREAK; return BREAK; }
continue        { prevTokenCXX = CONTINUE; return CONTINUE; }
goto            { prevTokenCXX = GOTO; return GOTO; }
catch           { prevTokenCXX = CATCH; return CATCH; }

{Id}         {
                lvalp->_id = new string(yytext);
                // get top level file of current scope, which is the last import statement
                CParser *pCurrentParser = CParser::GetCurrentParser();
                CFEFile *pFile = pCurrentParser->GetTopFileInScope();
                assert (pFile);
                if (pFile->FindUserDefinedType (yytext) != 0)
                    { prevTokenCXX = TYPENAME; return TYPENAME; }

                // check for C++ namespace name and return NSNAME
                // { prevTokenCXX = NSNAME; return NSNAME; }

                // the name of the current element is the SELFNAME, e.g. the name of the current class
                // { prevTokenCXX = SELFNAME; return SELFNAME; }

                // I don't know what to do whith those
                // IDENTIFIER_DEFN
                // TYPENAME_DEFN
                // PTYPENAME_DEFN
                // PFUNCNAME
                // PTYPENAME
                // SIGOF
                // they are supposed to be some sort of definition for names


                // typename not found, so it has to be an identifier
                if (gcc_cxxdebug)
                    fprintf(stderr,"( ID(4): %s )",yytext);
                prevTokenCXX = IDENTIFIER;
                return IDENTIFIER;
            }
{Integer}    {
                lvalp->_int = atoi(yytext);
                prevTokenCXX = CONSTANT;
                return CONSTANT;
            }
{string}    {
                lvalp->_str = new string(&yytext[1]);
                while (*(lvalp->_str->end()-1) == '"')
                    lvalp->_str->erase(lvalp->_str->end()-1);
                prevTokenCXX = STRING;
                return STRING;
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

<<EOF>>     {
                yyterminate(); // stops gcc_cxxparse
//                prevTokenCXX = END_OF_SAVED_INPUT;
//              return END_OF_SAVED_INPUT;
            }
.            {
                CCompiler::GccWarning(CParser::GetCurrentFile(), gLineNumber, "Unknown character \"%s\".",yytext);
            }

%%

#ifndef YY_CURRENT_BUFFER_LVALUE
#define YY_CURRENT_BUFFER_LVALUE YY_CURRENT_BUFFER
#endif

/** used in CParser::Import to save the buffer's state */
void* GetCurrentBufferGccCxx()
{
    if ( YY_CURRENT_BUFFER )
    {
        /* Flush out information for old buffer. */
        *yy_c_buf_p = yy_hold_char;
        YY_CURRENT_BUFFER_LVALUE->yy_buf_pos = yy_c_buf_p;
        YY_CURRENT_BUFFER_LVALUE->yy_n_chars = yy_n_chars;
        if (gcc_cxxdebug)
            fprintf(stderr, "GetCurrentBufferGccCxx: FILE: %p, buffer: %p, pos: %p, chars: %d\n",
                YY_CURRENT_BUFFER_LVALUE->yy_input_file, YY_CURRENT_BUFFER, yy_c_buf_p, yy_n_chars);
    }
    return YY_CURRENT_BUFFER;
}

/** used in CParser::Import to restore the buffer's state */
void RestoreBufferGccCxx(void *buffer, bool bInit)
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
        // C++ Parser always starts with line context, because C++ files are not included by us,
        // but by GCC and it may leave some flags after '# 1 "filename" 1' -> to skip them
        // change into line2 context
        bNeedToSetFileGccCxx = false;
        BEGIN(line2);
        if (gcc_cxxdebug)
        {
            fprintf(stderr, "RestoreBufferGccCxx: FILE: %p, buffer: %p, pos: %p, chars: %d (chars left: %d)\n",
                YY_CURRENT_BUFFER_LVALUE->yy_input_file, YY_CURRENT_BUFFER,
                yy_c_buf_p, yy_n_chars, yy_n_chars-(int)(yy_c_buf_p-(char*)(YY_CURRENT_BUFFER_LVALUE->yy_ch_buf)));
            fprintf(stderr, "RestoreBufferGccCxx: continue with %s:%d\n", sInFileName.c_str(), gLineNumber);
        }
    }
}

/** used in CParser::Parse to initialize the new buffer */
void StartBufferGccCxx(FILE* fInput)
{
    yy_switch_to_buffer( yy_create_buffer(fInput, YY_BUF_SIZE) );
    if (gcc_cxxdebug)
        fprintf(stderr, "StartBufferGccCxx: FILE: %p, buffer: %p\n",
            YY_CURRENT_BUFFER_LVALUE->yy_input_file, YY_CURRENT_BUFFER);
}
