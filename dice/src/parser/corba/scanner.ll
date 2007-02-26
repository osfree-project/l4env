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

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "fe/stdfe.h"
#include "parser/corba/parser.h"
#include "Compiler.h"
#include "CParser.h"

// current linenumber and filename
int nLineNbCORBA = 1;
extern int nOldLineNb;
extern String sInFileName;
extern String sInPathName;
extern String sTopLevelInFileName;

extern CFEFile *pCurFile;

// we don't need unput so "remove" additional code
#define YY_NO_UNPUT

// #include/import special treatment
int nIncludeLevelCORBA = 0;
int nStdIncCORBA = 0;
bool bReadFileForLineCORBA = false;

extern int c_inc;

// if we expect a file-name, return one
bool bExpectFileName = false;

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
Id				[a-zA-Z_][a-zA-Z0-9_]*
Integer			(-)?[1-9][0-9]*
String			"\""([^\n\"]|("\\\""))*[^\\]"\""
Char_lit		"'"."'"
Fixed_point		(-)?[0-9]*"."[0-9]*

Float_literal1	[0-9]*"."[0-9]+((e|E)[+|-]?[0-9]+)?
Float_literal2	[0-9]+"."((e|E)[+|-]?[0-9]+)?
Float_literal3	[0-9]+((e|E)[+|-]?[0-9]+)
Float			(-)?({Float_literal1}|{Float_literal2}|{Float_literal3})

Hexadec        0[xX][a-fA-F0-9]+
Octal          0[0-7]*
Escape         (L)?"'""\\"[ntvbrfa\\\?\'\"]"'"
Oct_char	   (L)?"'""\\"[0-7]{1,3}"'"
Hex_char	   (L)?"'""\\"(x|X)[a-fA-F0-9]{1,2}"'"

Filename		("<"|"\"")[a-zA-Z0-9_./\\:\-]*(">"|"\"")

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
"#"             { BEGIN(line); bReadFileForLineCORBA = false; }
<line,line2>[ \t]+ /* eat white-spaces */
<line>[0-9]+    {
					nOldLineNb = nLineNbCORBA;
					nLineNbCORBA = atoi(yytext);
				}
<line>{Filename} {
					BEGIN(line2);
   					// check the filename
   					if (strlen (yytext) > 2)
   					{
    					sInFileName = yytext;
    					sInFileName.TrimLeft('"');
    					sInFileName.TrimRight('"');
   					}
   					else
   						sInFileName = sTopLevelInFileName;
   					// check extension of file
   					int iDot = sInFileName.ReverseFind('.');
   					if (iDot > 0)
   					{
   						String sExt = sInFileName.Mid (iDot + 1);
						sExt.MakeLower();
   						if ((sExt == "h") || (sExt == "hh"))
   							c_inc = 1;
   						else
   							c_inc = 0;
   					}
   					else
   						c_inc = 0;
					bReadFileForLineCORBA = true;
				}
<line2>[1-4]    {
					// find path file file
					int nFlags = atoi(yytext);
					switch(nFlags)
					{
					case 1:
					case 3:
						{
							// get path for file
							CParser *pParser = CParser::GetCurrentParser();
							String sPath = pParser->FindPathToFile(sInFileName, nOldLineNb);
                            String sOrigName = pParser->GetOriginalIncludeForFile(sInFileName, nOldLineNb);
							// should create new CFEFile and set it as current
							nIncludeLevelCORBA++;
							CFEFile *pFEFile = new CFEFile(sOrigName, sPath, nIncludeLevelCORBA, (nFlags&2)>0);
							pCurFile->AddChild(pFEFile);
							pCurFile = pFEFile;
						}
						break;
					case 2:
					case 4:
						// should move current file up by one pos
						nIncludeLevelCORBA--;
						if (pCurFile->GetParent() != 0)
						    pCurFile = (CFEFile*)pCurFile->GetParent();
						break;
					}
				}
<line,line2>("\r")?"\n" { BEGIN(INITIAL); if (!bReadFileForLineCORBA) sInFileName = sTopLevelInFileName; }

","			return COMMA;
";"			return SEMICOLON;
":"			return COLON;
"::"		return SCOPE;
"("			return LPAREN;
")"			return RPAREN;
"{"			return LBRACE;
"}"			return RBRACE;
"["			return LBRACKET;
"]"			return RBRACKET;
"<"			return LT;
">"			return GT;
"="			return IS;
"|"			return BITOR;
"^"			return BITXOR;
"&"			return BITAND;
"~"			return BITNOT;
">>"		return RSHIFT;
"<<"		return LSHIFT;
"+"			return PLUS;
"-"			return MINUS;
"*"			return MUL;
"/"			return DIV;
"%"			return MOD;

abstract	return ABSTRACT;
any			return ANY;
attribute	return ATTRIBUTE;
boolean		return BOOLEAN;
case		return CASE;
char		return CHAR;
const		return CONST;
context		return CONTEXT;
custom		return CUSTOM;
default		return DEFAULT;
double		return DOUBLE;
enum		return ENUM;
exception	return EXCEPTION;
FALSE		return FALSE;
factory		return FACTORY;
fixed		return FIXED;
float		return FLOAT;
in			return IN;
inout		return INOUT;
interface	return INTERFACE;
module		return MODULE;
library		return MODULE; // supports KA IDLs
native		return NATIVE;
long		return LONG;
Object		return OBJECT;
octet		return OCTET;
oneway		return ONEWAY;
out			return OUT;
private		return PRIVATE;
public		return PUBLIC;
raises		return RAISES;
readonly	return READONLY;
sequence	return SEQUENCE;
short		return SHORT;
string		return STRING;
struct		return STRUCT;
supports	return SUPPORTS;
switch		return SWITCH;
TRUE		return TRUE;
truncatable	return TRUNCABLE;
typedef		return TYPEDEF;
union		return UNION;
unsigned	return UNSIGNED;
ValueBase	return VALUEBASE;
valuetype	return VALUETYPE;
void		return VOID;
wchar		return WCHAR;
wstring		return WSTRING;
fpage		return FPAGE;
refstring	return REFSTRING;

{Id}		{
				corbalval._id = strdup(yytext);
				return ID;
			}
{Integer}	{
				corbalval._int = atoi(yytext);
				return LIT_INT;
			}
L{Char_lit}	{
				corbalval._chr = yytext[3];
				return LIT_WCHAR;
			}
{Char_lit}	{
				corbalval._chr = yytext[2];
				return LIT_CHAR;
			}
L{String}	{
				corbalval._str = new String(&yytext[1]);
				corbalval._str->TrimRight('\"');
				return LIT_WSTR;
			}
{Float}		{
				corbalval._flt = atof(yytext);
				return LIT_FLOAT;
			}
{Fixed_point} {
				corbalval._flt = atof(yytext);
				return LIT_FLOAT;
			}
{Hexadec}	{
				corbalval._int = HexToInt (yytext + 2);	/* strip off the "0x" */
				return LIT_INT;
			}
{Octal}		{
				 corbalval._int = OctToInt (yytext);	/* first 0 doesn't matter */
				 return LIT_INT;
			}
{Escape}	{
				 corbalval._chr = EscapeToChar (yytext);
				 return LIT_CHAR;
			}
{Oct_char}	{
				 corbalval._chr = OctToChar (yytext);
				 return LIT_CHAR;
			}
{Hex_char}	{
				 corbalval._chr = HexToChar (yytext);
				 return LIT_CHAR;
			}
{Filename} {
				if (bExpectFileName)
				{
					corbalval._str = new String (&yytext[1]);
					corbalval._str->TrimRight('>');
					corbalval._str->TrimRight('"');
					return FILENAME;
				}
				else
				{
					// sequence type
					REJECT;
				}
			}
{String}	{
				corbalval._str = new String(&yytext[1]);
				corbalval._str->TrimRight('\"');
				return LIT_STR;
			}

"/*"		{
				register int c;
				for (;;) {
					while ((c = yyinput()) != '*' && c != EOF && c != '\n') ; // eat up text of comment
					if (c == '*') {
						while ((c = yyinput()) == '*') ; // eat up trailing *
						if (c == '/') break; // found end
					}
					if (c == '\n') nLineNbCORBA++;
					if (c == EOF) {
						CCompiler::GccError(pCurFile, nLineNbCORBA, "EOF in comment.");
						yyterminate();
						break;
					}
				}
			}

"//".*		/* eat C++ style comments */	

[ \t]*		/* eat whitespace */

("\r")?"\n"	{ ++nLineNbCORBA; }

<<EOF>>     {
				CParser *pCurrentParser = CParser::GetCurrentParser();
				if (pCurrentParser->EndImport())
					yyterminate();
			}
.			{
				CCompiler::GccWarning(pCurFile, nLineNbCORBA, "Unknown character \"%s\".",yytext);
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
		printf("%s:%d: Hexadecimal number overflow.\n",(const char*)sInFileName, nLineNbCORBA);
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
		printf("%s:%d: Octal number overflow.\n",(const char*)sInFileName, nLineNbCORBA);
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

void* SwitchToFileCorba(FILE *fNewFile)
{
	YY_BUFFER_STATE old_buffer = YY_CURRENT_BUFFER;
	YY_BUFFER_STATE buffer = yy_create_buffer(fNewFile, YY_BUF_SIZE);
	yy_switch_to_buffer(buffer);
	nIncludeLevelCORBA++;
	return old_buffer;
}

void RestoreBufferCorba(void *buffer)
{
	nIncludeLevelCORBA--;
	yy_delete_buffer(YY_CURRENT_BUFFER);
	yy_switch_to_buffer((YY_BUFFER_STATE)buffer);
	BEGIN(INITIAL);
}
	
