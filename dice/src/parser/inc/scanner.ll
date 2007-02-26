%{
/**
 *    \file    dice/src/parser/inc/scanner.ll
 *    \brief   contains the include scanner
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
#include <ctype.h>
#include <string>
#include <algorithm>
using namespace std;
#include "CPreProcess.h"
#include "fe/FEFile.h"
#include "Compiler.h"

extern int gLineNumber;
int nStartOfStringInc = 0;
extern string sInFileName;

bool bImport = false;
bool bStandard = false;

// for storing the previous buffer states
#define MAX_INCLUDE_DEPTH 20

typedef struct
{
    string sFilename;
    int nLineNumber;
    YY_BUFFER_STATE buffer;
} state_stack_location;

state_stack_location state_stack[MAX_INCLUDE_DEPTH];
unsigned state_stack_size = 0;

// we don't need unput so "remove" additional code
#define YY_NO_UNPUT
%}

%option noyywrap

%x include
%x comment
%x strings
%x line

Filename        ("<")?[a-zA-Z0-9_\./\\:\- ]*(">")?("\""|">")

%%

    /* this will only scan for the include and import statements
     * and remember their position.
     */

"\""                    {
                            BEGIN(strings);
                            ECHO;
                            nStartOfStringInc = gLineNumber;
                        }
<strings>[^\\\"]*        {
                            // write string content as is to target file
                            ECHO;
                        }
<strings>"\\\""            {
                            // nested escaped ""; still part of string
                            ECHO;
                        }
<strings>"\""            {
                            // closing quote
                            ECHO;
                            nStartOfStringInc = 0;
                            BEGIN(INITIAL);
                        }
<strings><<EOF>>            {
                            // unterminated string
                            fprintf(stderr, "%s:%d: Unterminated string at end of file. Could have started here: %d.\n",
                                sInFileName.c_str(), gLineNumber, nStartOfStringInc);
                            yyterminate();
                        }

"//"[^\r\n]*            ECHO;

"/*"                    BEGIN(comment);
<comment>[^*\n]*        /* eat anything that's not a '*' */
<comment>"*"+[^*/\n]*   /* eat up '*'s not followed by '/'s */
<comment>"*"+"/"        BEGIN(INITIAL);
<comment>(\r)?\n        {
                        gLineNumber++;
                        fprintf(yyout, "\n");
                    }

"#"[ \t]*"include"[ \t]+"\""    {
                                    BEGIN(include);
                                    bImport = false;
                                    bStandard = false;
                                }
"#"[ \t]*"include"[ \t]+"<"        {
                                    BEGIN(include);
                                    bImport = false;
                                    bStandard = true;
                                }
[ \t]*"import"[ \t]+"\""        {
                                    BEGIN(include);
                                    bImport = true;
                                    bStandard = false;
                                }
[ \t]*"import"[ \t]+"<"            {
                                    BEGIN(include);
                                    bImport = true;
                                    bStandard = true;
                                }
<include>[ \t]*                    /* do nothing */
<include>","                    /* do nothing */
<include>{Filename}                {
                                    /* set last char (" or >) to 0 */
                                    yytext[yyleng-1] = 0;
                                    CPreProcess *pPreProcess = CPreProcess::GetPreProcessor();
                                    assert(pPreProcess);
                                    string sFilename = yytext;
                                    // check if this an idl file (ends on '.idl')
                                    int iDot = sFilename.rfind('.');
                                    string sExt;
                                    if (iDot > 0)
                                        sExt = sFilename.substr(iDot + 1);
                                    transform(sExt.begin(), sExt.end(), sExt.begin(), tolower);
                                    // our own header dice-corba-types.h is an exception
                                    if (sFilename == "dice/dice-corba-types.h")
                                        sExt = "idl";
                                    // only open the input file if it is an IDL file and
                                    // if we did not include it at the same spot before (return value of AddInclude)
                                    FILE *fInput = 0;
                                    if (!pPreProcess->AddInclude(sFilename, sInFileName, gLineNumber, bImport, bStandard) &&
                                        (sExt == "idl"))
                                        fInput = pPreProcess->OpenFile(sFilename, bStandard, false, true);
                                    if (fInput)
                                    {
                                        // check if stack full
                                        if (state_stack_size+1 == MAX_INCLUDE_DEPTH)
                                        {
                                            CCompiler::Error("Include depth exceeds Dice's limit (%d)\n", MAX_INCLUDE_DEPTH);
                                            yyterminate();
                                        }
                                        // save state
                                        state_stack[state_stack_size].buffer = YY_CURRENT_BUFFER;
                                        state_stack[state_stack_size].nLineNumber = gLineNumber;
                                        state_stack[state_stack_size].sFilename = sInFileName;
                                        state_stack_size++;
                                        // set filename and line number
                                        sInFileName = sFilename;
                                        gLineNumber = 1; // starts right in the new file/buffer
                                        // print preprocessing line statement
                                        fprintf(yyout, "# 1 \"%s\" 1\n", yytext);
                                        // create new buffer
                                        yy_switch_to_buffer(yy_create_buffer(fInput, YY_BUF_SIZE));
                                        BEGIN(INITIAL);
                                    }
                                    else
                                    {
                                        // if we cannot open the file, we print the
                                        // statement as is, so the preprocessor can
                                        // take care of this.
                                        if (bStandard)
                                            fprintf(yyout, "#include <%s>", yytext);
                                        else
                                            fprintf(yyout, "#include \"%s\"", yytext);
                                    }
                                }
<include>(\r)?\n                {
                        BEGIN(INITIAL);
                        fprintf(yyout, "\n");
                        gLineNumber++;
                    }
"#line"             BEGIN(line);
<line>[ \t]+        /* eat whitespace */
<line>[0-9]+        {
                        // reset line number
                        gLineNumber = atoi(yytext);
                    }
<line>("\""|"<"){Filename}?   {
                        // reprint line directive
                        fprintf(yyout, "#line %d %s", gLineNumber, yytext);
                        gLineNumber--; // to skip newline at end of this directive
                        BEGIN(INITIAL);
                    }
<line>(\r)?\n       {
                        fprintf(yyout, "#line %d\n", gLineNumber);
                        BEGIN(INITIAL);
                    }
(\r)?\n                {
                        fprintf(yyout, "\n");
                        gLineNumber++;
                    }
\f                  {
                        gLineNumber++;
                        fprintf(yyout, "\n");
                    }

<<EOF>>                {
                        // restore previous buffer
                        if (state_stack_size > 0)
                        {
                            // close and delete current buffer
                            yy_delete_buffer(YY_CURRENT_BUFFER);
                            // get previous buffer
                            state_stack_size--;
                            yy_switch_to_buffer(state_stack[state_stack_size].buffer);
                            // restore linenumber and filename
                            gLineNumber = state_stack[state_stack_size].nLineNumber;
                            sInFileName = state_stack[state_stack_size].sFilename;
                            // the line directive states the line number after this directive,
                            // but we can't increase gLineNumber, because we got the newline
                            // at the end of the include statement before us, which will increment
                            // gLineNumber. This newline ahead will also be printed after this
                            // statement
                            fprintf(yyout, "# %d \"%s\" 2", gLineNumber+1, sInFileName.c_str());
                        }
                        else
                            yyterminate();
                    }

.                    ECHO;

%%
