%{
/*
 * Copyright (C) 2001-2002
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

#include "CString.h"
#include "CParser.h"
#include "fe/FEFile.h"

int nLineNbInc = 1;
int nStartOfStringInc = 0;
extern String sInFileName;

bool bImport = false;
bool bStandard = false;

// we don't need unput so "remove" additional code
#define YY_NO_UNPUT
%}

%option noyywrap

%x import
%x include
%x comment
%x string

Filename		[a-zA-Z0-9_./\\:\-]*("\""|">")

%%

	/* this will only scan for the include and import statements
	 * and remember their position.
	 */

"\""					{
							BEGIN(string);
							fprintf(yyout, yytext);
							nStartOfStringInc = nLineNbInc;
						}
<string>[^\\\"]*		{
							// write string content as is to target file
							fprintf(yyout, yytext);
						}
<string>"\\\""			{
							// nested escaped ""; still part of string
							fprintf(yyout, yytext);
						}
<string>"\""			{
							// closing quote
							fprintf(yyout, yytext);
							nStartOfStringInc = 0;
							BEGIN(INITIAL);
						}
<string><<EOF>>			{
							// unterminated string
                            fprintf(stderr, "%s:%d: Unterminated string at end of file. Could have started here: %d.\n",
								(const char*)sInFileName, nLineNbInc, nStartOfStringInc);
							yyterminate();
						}

"//"[^\r\n]*			fprintf(yyout, yytext);

"/*"					BEGIN(comment);
<comment>[^*\n]*        /* eat anything that's not a '*' */
<comment>"*"+[^*/\n]*   /* eat up '*'s not followed by '/'s */
<comment>"*"+"/"        BEGIN(INITIAL);
<comment>(\r)?\n		{
						nLineNbInc++;
						fprintf(yyout, "\n");
					}

"#"[ \t]*"include"[ \t]+"\""	{
                                    BEGIN(include);
                                    bImport = false;
                                    bStandard = false;
                                    fprintf(yyout, yytext);
								}
"#"[ \t]*"include"[ \t]+"<"		{
                                    BEGIN(include);
                                    bImport = false;
                                    bStandard = true;
                                    fprintf(yyout, yytext);
								}
[ \t]*"import"[ \t]+"\""		{
                                    BEGIN(include);
                                    bImport = true;
                                    bStandard = false;
                                    fprintf(yyout, yytext);
								}
[ \t]*"import"[ \t]+"<"			{
                                    BEGIN(include);
                                    bImport = true;
                                    bStandard = true;
                                    fprintf(yyout, yytext);
								}
<include>[ \t]*					fprintf(yyout, yytext);
<include>","					fprintf(yyout, yytext);
<include>{Filename}				{
                                    fprintf(yyout, yytext);
                                    /* set last char (" or >) to 0 */
                                    yytext[yyleng-1] = 0;
                                    CParser *pParser = CParser::GetCurrentParser();
                                    if (pParser != 0)
										pParser->AddInclude(String(yytext), sInFileName, nLineNbInc, bImport, bStandard);
								}
<include>(\r)?\n				{
									BEGIN(INITIAL);
									nLineNbInc++;
									fprintf(yyout, "\n");
								}
(\r)?\n		    	{
						nLineNbInc++;
						fprintf(yyout, "\n");
					}

<<EOF>>				yyterminate();

.					fprintf(yyout, yytext);

%%
