%{
/*
 * Copyright (C) 2001-2003
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

#define YY_DECL int yylex(YYSTYPE* lvalp)

// current linenumber and filename
extern int gLineNumber;
extern String sInFileName;
extern String sInPathName;
extern String sTopLevelInFileName;

extern int gcc_cdebug;

// we don't need unput so "remove" additional code
#define YY_NO_UNPUT

// #include/import special treatment
int nNewLineNumberGccC;
String sNewFileNameGccC;
bool bNeedToSetFileGccC = true;

extern bool bExpectFileName;

// forward declaration
static unsigned int HexToInt (char *s);
static unsigned int OctToInt (char *s);

%}

%option noyywrap

/* some rules */
Id				[a-zA-Z_][a-zA-Z0-9_]*
Integer_Suffix  ([uU][lL]?)|([uU]([lL]{2}))|([lL][uU]?)|([lL]{2}[uU]?)
Integer			(-)?[1-9][0-9]*{Integer_Suffix}?
Filename		("\""|"<")("<")?[a-zA-Z0-9_\./\\:\- ]*(">")?("\""|">")
String          "\""([^\\\n\"]|"\\"[ntvbrfa\\\?\'\"\n])*"\""
C99String       "\""([^\"]|("\\\""))*[^\\\"]"\""
Char_lit		"'"."'"
Fixed_point		(-)?[0-9]*"."[0-9]*

Float_literal1	[0-9]*"."[0-9]+((e|E)[+|-]?[0-9]+)?
Float_literal2	[0-9]+"."((e|E)[+|-]?[0-9]+)?
Float_literal3	[0-9]+((e|E)[+|-]?[0-9]+)
Float			(-)?({Float_literal1}|{Float_literal2}|{Float_literal3})

Hexadec        0[xX][a-fA-F0-9]+{Integer_Suffix}?
Octal          0[0-7]*{Integer_Suffix}?
Escape         (L)?"'""\\"[ntvbrfa\\\?\'\"]"'"
Oct_char	   (L)?"'""\\"[0-7]{1,3}"'"
Hex_char	   (L)?"'""\\"(x|X)[a-fA-F0-9]{1,2}"'"

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
^[ \t]*"#"      {
                    BEGIN(line);
					sNewFileNameGccC.Empty();
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
    					sNewFileNameGccC.TrimLeft('"');
    					sNewFileNameGccC.TrimRight('"');
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
						    (const char*)sNewFileNameGccC, nFlags);
					// this is a bad hack to avoid resetting of state
					CParser *pParser = CParser::GetCurrentParser();
					switch(nFlags)
					{
					case 1:
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
									String sPath = pPreProcess->FindPathToFile(sNewFileNameGccC, gLineNumber);
									String sOrigName = pPreProcess->GetOriginalIncludeForFile(sNewFileNameGccC, gLineNumber);
									bool bStdInc = pPreProcess->IsStandardInclude(sNewFileNameGccC, gLineNumber);
									// if path is set and origname is empty, then sNewFileNameGccC is with full path
									// and FindPathToFile returned an include path that matches the beginning of the
									// string. Now we get the original name by cutting off the beginning of the string,
									// which is the path
									if (!sPath.IsEmpty() && sOrigName.IsEmpty())
										sOrigName = sNewFileNameGccC.Right(sNewFileNameGccC.GetLength()-sPath.GetLength());
									if (sOrigName.IsEmpty())
										sOrigName = sNewFileNameGccC;
									// simply create new CFEFile and set it as current
									CFEFile *pFEFile = new CFEFile(sOrigName, sPath, gLineNumber, bStdInc);
									CParser::SetCurrentFile(pFEFile);
									sInFileName = sNewFileNameGccC;
									gLineNumber = nNewLineNumberGccC;
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
					    // check if we have to switch the parsers back
						// e.g. by sending an EOF (yyterminate)
						if (pParser->DoEndImport())
						{
							return EOF_TOKEN;
    						bNeedToSetFileGccC = false;
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
					if (bNeedToSetFileGccC)
					{
						gLineNumber = nNewLineNumberGccC;
						if (sNewFileNameGccC.IsEmpty())
							sInFileName = sTopLevelInFileName;
						else
							sInFileName = sNewFileNameGccC;
						if (gcc_cdebug)
							fprintf(stderr, "Gcc-C: # %d \"%s\"\n", gLineNumber, (const char*)sInFileName);
					}
                }

"["		return LBRACKET;
"]"		return RBRACKET;
"("		return LPAREN;
")"		return RPAREN;
"{"		return LBRACE;
"}"		return RBRACE;
"."		return DOT;
"->"		return POINTSAT;
"++"		return INCR;
"--"		return DECR;
"&"		return BITAND;
"*"		return MUL;
"+"		return PLUS;
"-"		return MINUS;
"~"		return TILDE;
"!"		return EXCLAM;
"/"		return DIV;
"%"		return MOD;
">>"		return RSHIFT;
"<<"		return LSHIFT;
"<"		return LT;
">"		return GT;
"<="		return LTEQUAL;
">="		return GTEQUAL;
"=="		return EQUAL;
"!="		return NOTEQUAL;
"^"		return BITXOR;
"|"		return BITOR;
"&&"		return ANDAND;
"||"		return OROR;
"?"		return QUESTION;
":"		return COLON;
";"		return SEMICOLON;
"..."   return ELLIPSIS;
"="		return IS;
"*="		return MUL_ASSIGN;
"/="		return DIV_ASSIGN;
"%="		return MOD_ASSIGN;
"+="		return PLUS_ASSIGN;
"-="		return MINUS_ASSIGN;
"<<="		return LSHIFT_ASSIGN;
">>="		return RSHIFT_ASSIGN;
"&="		return AND_ASSIGN;
"^="		return XOR_ASSIGN;
"|="		return OR_ASSIGN;
","		return COMMA;
"::"		return SCOPE;

auto		return AUTO;
break		return BREAK;
case		return CASE;
char		return CHAR;
const		return CONST;
continue	return CONTINUE;
default		return DEFAULT;
do		return DO;
double		return DOUBLE;
else		return ELSE;
enum		return ENUM;
extern		return EXTERN;
float		return FLOAT;
for		return FOR;
goto		return GOTO;
if		return IF;
inline		return INLINE;
__inline__  return INLINE;
int		return INT;
long		return LONG;
long[ \t]+long  return LONGLONG;
register	return REGISTER;
restrict	return RESTRICT;
__restrict  return RESTRICT;
return		return RETURN;
short		return SHORT;
signed		return SIGNED;
sizeof		return SIZEOF;
static		return STATIC;
struct		return STRUCT;
switch		return SWITCH;
typedef		return TYPEDEF;
union		return UNION;
unsigned	return UNSIGNED;
void		return VOID;
volatile	return VOLATILE;
__volatile__    return VOLATILE;
__volatile  return VOLATILE;
while		return WHILE;
_Bool		return UBOOL;
_Complex	return UCOMPLEX;
_Imaginary	return UIMAGINARY;

asm		return ASM_KEYWORD;
__asm		return ASM_KEYWORD;
__asm__		return ASM_KEYWORD;
typeof      	return TYPEOF;
alignof     	return ALIGNOF;
extension   	/* eat extension keyword */
__extension__   /* eat extension keyword */
label       	return LABEL;
__real		return REALPART;
__real__	return REALPART;
__imag		return IMAGPART;
__imag__	return IMAGPART;
attribute	return ATTRIBUTE;
__attribute	return ATTRIBUTE;
__attribute__	return ATTRIBUTE;
__const			return CONST;
__const__		return CONST;
bycopy          return BYCOPY;
byref           return BYREF;
in              return IN;
out             return OUT;
inout           return INOUT;
oneway          return ONEWAY;
__label         return LOCAL_LABEL;
__label__       return LOCAL_LABEL;

__builtin_va_list   {
                lvalp->_id = new String(yytext);
				return TYPENAME;
            }

{Id}	 	{
				lvalp->_id = new String(yytext);
				// get top level file of current scope, which is the last import statement
				CParser *pCurrentParser = CParser::GetCurrentParser();
				CFEFile *pFile = pCurrentParser->GetTopFileInScope();
				assert (pFile);
				if (pFile->FindUserDefinedType (yytext))
					return TYPENAME;
                // typename in scope not found, check else-where
				pFile = pFile->GetRoot();
				if (pFile && pFile->FindUserDefinedType(yytext))
					return TYPENAME;
				// typename not found, so it has to be an identifier
				if (gcc_cdebug)
					fprintf(stderr,"ID(4): %s\n",yytext);
				return IDENTIFIER;
			}
{Integer}	{
                // could end on u,ul,ull,l,ll,lu,llu
				lvalp->_int = atoi(yytext);
				return INTEGER_CONST;
			}
{Octal}     {
                lvalp->_int = OctToInt(&yytext[1]);
				return INTEGER_CONST;
            }
{Hexadec}   {
                lvalp->_int = HexToInt(&yytext[2]);
				return INTEGER_CONST;
            }
{Float}     {
                lvalp->_float = atof(yytext);
				return FLOAT_CONST;
            }
{Char_lit}  {
                lvalp->_char = yytext[1];
				return CHAR_CONST;
            }
{String}	{
				lvalp->_str = new String(&yytext[1]);
				lvalp->_str->TrimRight('\"');
				return STRING;
			}
{C99String} {
                // allows embedded newlines
				lvalp->_str = new String(&yytext[1]);
				lvalp->_str->TrimRight('\"');
				// replace \n with \\n
                lvalp->_str->Replace("\n", "\\n");
				return STRING;
            }
"/*"		{
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
						    fprintf(stderr, " gLineNumber: %d (%s)\n", gLineNumber, (const char*)sInFileName);
					}
					if (c == EOF) {
						CCompiler::GccError(CParser::GetCurrentFile(), gLineNumber, "EOF in comment.");
						yyterminate();
						break;
					}
				}
			}

"//".*		/* eat C++ style comments */

[ \t]*		/* eat whitespace */

("\r")?"\n"	{ ++gLineNumber; if (gcc_cdebug) fprintf(stderr, " gLineNumber: %d (%s)\n", gLineNumber, (const char*)sInFileName); }
"\f"        { ++gLineNumber; if (gcc_cdebug) fprintf(stderr, " gLineNumber: %d (%s)\n", gLineNumber, (const char*)sInFileName); }

<<EOF>>     {
                if (gcc_cdebug)
				    fprintf(stderr, "Stop file %s\n", (const char*)sInFileName);
				yyterminate(); // stops gcc_cparse
			}
.			{
				CCompiler::GccWarning(CParser::GetCurrentFile(), gLineNumber, "Unknown character \"%s\".",yytext);
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
		BEGIN(line2);
		if (gcc_cdebug)
			fprintf(stderr, "RestoreBufferGccC: FILE: %p, buffer: %p, pos: %p, chars: %d (chars left: %d)\n",
				YY_CURRENT_BUFFER_LVALUE->yy_input_file, YY_CURRENT_BUFFER,
				yy_c_buf_p, yy_n_chars, yy_n_chars-(int)(yy_c_buf_p-(char*)(YY_CURRENT_BUFFER_LVALUE->yy_ch_buf)));
	}
}

void StartBufferGccC(FILE* fInput)
{
    yy_switch_to_buffer( yy_create_buffer(fInput, YY_BUF_SIZE) );
	if (gcc_cdebug)
		fprintf(stderr, "StartBufferGccC: FILE: %p, buffer: %p\n",
			YY_CURRENT_BUFFER_LVALUE->yy_input_file, YY_CURRENT_BUFFER);
}
