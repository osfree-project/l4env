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
#include "parser/dce/parser.h"
#include "Compiler.h"
#include "CParser.h"

// current linenumber and filename
int nLineNbDCE = 1;
int nOldLineNb;
extern String sInFileName;
extern String sInPathName;
extern String sTopLevelInFileName;

extern CFEFile *pCurFile;

// we don't need unput so "remove" additional code
#define YY_NO_UNPUT

// #include/import special treatment
int nIncludeLevelDCE = 0;

// number conversion
static unsigned int OctToInt (char *);
static unsigned int HexToInt (char *);
// character conversion
static int OctToChar (char *);
static int HexToChar (char *);
static int EscapeToChar (char *);

int c_inc = 2; // set to attribute
bool bReadFileForLineDCE = false;

// helper macros
// if c_inc is set to 'C-file' (1)
#define RETURN_IF_C(token) { \
	if (c_inc == 1) { return token; } \
	else { dcelval._id = new String(yytext); return ID; } \
	}

// if c_inc is set to 'IDL-file' (0) or 'attribute' (2)
#define RETURN_IF_IDL(token) { \
	if (c_inc != 1) { return token; } \
	else { dcelval._id = new String(yytext); return ID; } \
	}

// if c_inc is set to 'attribute' (2)
#define RETURN_IF_ATTR(token) { \
    if (c_inc == 2) { return token; } \
    else { dcelval._id = new String(yytext); return ID; } \
    }

// if c_inc is set to 'C-file' (1) return c-token
// if c_inc is set to 'IDL-file' (0) or 'attribute' (2) return idl-token
#define RETURN_IDL_OR_C(token_idl, token_c) { \
	if (c_inc == 1) { return token_c; } \
	else { return token_idl; } \
	}

%}

%option noyywrap
/* some rules */
Float_literal1 [0-9]*"."[0-9]+((e|E)[+|-]?[0-9]+)?
Float_literal2 [0-9]+"."((e|E)[+|-]?[0-9]+)?
Float_literal3 [0-9]+((e|E)[+|-]?[0-9]+)
Id             [a-zA-Z_][a-zA-Z0-9_]*
Integer        (-)?[1-9][0-9]*(((u|U)(l|L)?)|((l|L)(u|U)?))?
Hexadec        0[xX][a-fA-F0-9]+
Octal          0[0-7]*
Float         (-)?({Float_literal1}|{Float_literal2}|{Float_literal3})(f|l|F|L)?
Char_lit       (L)?"'"."'"
Escape         (L)?"'""\\"[ntvbrfa\\\?\'\"]"'"
Oct_char	   (L)?"'""\\"[0-7]{1,3}"'"
Hex_char	   (L)?"'""\\"(x|X)[a-fA-F0-9]{1,2}"'"
Filename		("\""|"<")[a-zA-Z0-9_./\\:\-]*("\""|">")
String         (L)?"\""([^\n\"]|("\\\""))*[^\\]"\""
Portspec       "\""[^\n\\][ \t]*":"[ \t]*"["[^\n\\][ \t]*"]"[ \t]*"\""
Uuid       [a-fA-F0-9]{8}"-"[a-fA-F0-9]{4}"-"[a-fA-F0-9]{4}"-"[a-fA-F0-9]{4}"-"[a-fA-F0-9]{12}

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
"#"             { BEGIN(line); bReadFileForLineDCE = false; }
<line,line2>[ \t]+ /* eat white-spaces */
<line>[0-9]+    {
					nOldLineNb = nLineNbDCE;
					nLineNbDCE = atoi(yytext);
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
   							c_inc = 2; // start off with attributes
   					}
   					else
   						c_inc = 2; // start off with attributes
					 bReadFileForLineDCE = true;
				}
<line2>[1-4]    {
					// if line directive is without these flags only new line numbers are defined.
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
							nIncludeLevelDCE++;
							CFEFile *pFEFile = new CFEFile(sOrigName, sPath, nIncludeLevelDCE, (nFlags&2)>0);
							pCurFile->AddChild(pFEFile);
							pCurFile = pFEFile;
						}
						break;
					case 2:
					case 4:
						// should move current file up by one pos
						nIncludeLevelDCE--;
						if (pCurFile->GetParent() != 0)
						    pCurFile = (CFEFile*)pCurFile->GetParent();
						break;
					}
				}
<line,line2>("\r")?"\n" { BEGIN(INITIAL); if (!bReadFileForLineDCE) sInFileName = sTopLevelInFileName;}

("{"|"<%")		return LBRACE;
("}"|"%>")		return RBRACE;
("["|"<:")		return LBRACKET;
("]"|":>")		return RBRACKET;
":"				return COLON;
"::"            return SCOPE;
","				return COMMA;
"("				return LPAREN;
")"				return RPAREN;
"."				return DOT;
"\""			return QUOT;
"*"				return ASTERISK;
"'"				return SINGLEQUOT;
"?"				return QUESTION;
"|"				return BITOR;
"^"				return BITXOR;
"&"				return BITAND;
"<"				return LT;
">"				return GT;
"+"				return PLUS;
"-"				return MINUS;
"/"				return DIV;
"%"				return MOD;
"~"				return TILDE;
"!"				return EXCLAM;
";"				return SEMICOLON;
"||"			return LOGICALOR;
"&&"			return LOGICALAND;
"=="			return EQUAL;
"!="			return NOTEQUAL;
"<="			return LTEQUAL;
">="			return GTEQUAL;
"<<"			return LSHIFT;
">>"			return RSHIFT;
".."			return DOTDOT;
"="				return IS;

boolean			return BOOLEAN;
byte			return BYTE;
case			return CASE;
char			return CHAR;
const			RETURN_IDL_OR_C(CONST, C_CONST)
default			return DEFAULT;
double			return DOUBLE;
enum			return ENUM;
false			return FALSE;
FALSE			return FALSE;
float			return FLOAT;
handle_t		RETURN_IF_IDL(HANDLE_T)
hyper			RETURN_IF_IDL(HYPER)
import			RETURN_IF_IDL(IMPORT);
int				return INT;
interface		RETURN_IF_IDL(INTERFACE)
long			return LONG;
"long"[ \t]+"long"	return LONGLONG;
0			return EXPNULL;
pipe			return PIPE;
short			return SHORT;
small			RETURN_IF_IDL(SMALL)
struct			return STRUCT;
switch			return SWITCH;
true			return TRUE;
TRUE			return TRUE;
typedef			return TYPEDEF;
union			return UNION;
unsigned		return UNSIGNED;
signed			return SIGNED;
void			return VOID;
error_status_t	return ERROR_STATUS_T;

flexpage 			RETURN_IF_IDL(FLEXPAGE)
fpage               RETURN_IF_IDL(FLEXPAGE)
ISO_LATIN_1 		RETURN_IF_IDL(ISO_LATIN_1)
ISO_MULTI_LINGUAL 	RETURN_IF_IDL(ISO_MULTI_LINGUAL)
ISO_UCS 			RETURN_IF_IDL(ISO_UCS)
refstring 			RETURN_IF_IDL(REFSTRING)
broadcast			RETURN_IF_ATTR(BROADCAST)
context_handle 		RETURN_IF_ATTR(CONTEXT_HANDLE)
endpoint 			RETURN_IF_ATTR(ENDPOINT)
exceptions			RETURN_IF_ATTR(EXCEPTIONS)
first_is 			RETURN_IF_ATTR(FIRST_IS)
handle 				RETURN_IF_ATTR(HANDLE)
idempotent 			RETURN_IF_ATTR(IDEMPOTENT)
ignore 				RETURN_IF_ATTR(IGNORE)
in 					return IN;
last_is 			RETURN_IF_ATTR(LAST_IS)
length_is 			RETURN_IF_ATTR(LENGTH_IS)
local 				RETURN_IF_ATTR(LOCAL)
max_is				RETURN_IF_ATTR(MAX_IS)
maybe 				RETURN_IF_ATTR(MAYBE)
min_is 				RETURN_IF_ATTR(MIN_IS)
out 				return OUT;
ptr 				RETURN_IF_ATTR(PTR)
pointer_default 	RETURN_IF_ATTR(POINTER_DEFAULT)
ref 				RETURN_IF_ATTR(REF)
reflect_deletions 	RETURN_IF_ATTR(REFLECT_DELETIONS)
size_is 			RETURN_IF_ATTR(SIZE_IS)
string 				RETURN_IF_ATTR(STRING)
switch_is 			RETURN_IF_ATTR(SWITCH_IS)
switch_type 		RETURN_IF_ATTR(SWITCH_TYPE)
transmit_as 		RETURN_IF_ATTR(TRANSMIT_AS)
unique 				RETURN_IF_ATTR(UNIQUE)
uuid 				RETURN_IF_ATTR(UUID)
version 			RETURN_IF_ATTR(VERSION)
object 				RETURN_IF_ATTR(OBJECT)
iid_is 				RETURN_IF_ATTR(IID_IS)
raises 				return RAISES;
exception			return EXCEPTION;
library				RETURN_IF_IDL(LIBRARY)
control				RETURN_IF_IDL(CONTROL)
helpcontext			RETURN_IF_ATTR(HELPCONTEXT)
helpfile			RETURN_IF_ATTR(HELPFILE)
helpstring			RETURN_IF_ATTR(HELPSTRING)
default_function    RETURN_IF_ATTR(DEFAULT_FUNCTION)
error_function      RETURN_IF_ATTR(ERROR_FUNCTION)
server_parameter    RETURN_IF_ATTR(SERVER_PARAMETER)
init_with_in        RETURN_IF_ATTR(INIT_WITH_IN)
hidden				RETURN_IF_ATTR(HIDDEN)
lcid				RETURN_IF_ATTR(LCID)
restricted			RETURN_IF_ATTR(RESTRICTED)
auto_handle			RETURN_IF_ATTR(AUTO_HANDLE)
binding_callout		RETURN_IF_ATTR(BINDING_CALLOUT)
code				RETURN_IF_ATTR(CODE)
comm_status			RETURN_IF_ATTR(COMM_STATUS)
cs_char				RETURN_IF_ATTR(CS_CHAR)
cs_drtag			RETURN_IF_ATTR(CS_DRTAG)
cs_rtag				RETURN_IF_ATTR(CS_RTAG)
cs_stag				RETURN_IF_ATTR(CS_STAG)
cs_tag_rtn			RETURN_IF_ATTR(TAG_RTN)
enable_allocate		RETURN_IF_ATTR(ENABLE_ALLOCATE)
extern_exceptions	RETURN_IF_ATTR(EXTERN_EXCEPTIONS)
explicit_handle		RETURN_IF_ATTR(EXPLICIT_HANDLE)
fault_status		RETURN_IF_ATTR(FAULT_STATUS)
heap				RETURN_IF_ATTR(HEAP)
implicit_handle		RETURN_IF_ATTR(IMPLICIT_HANDLE)
nocode				RETURN_IF_ATTR(NOCODE)
represent_as		RETURN_IF_ATTR(REPRESENT_AS)
user_marshal		RETURN_IF_ATTR(USER_MARSHAL)
without_using_exceptions	RETURN_IF_ATTR(WITHOUT_USING_EXCEPTIONS)
init_rcvstring      RETURN_IF_ATTR(INIT_RCVSTRING)
auto				RETURN_IF_C(AUTO)
break				RETURN_IF_C(BREAK)
bycopy				RETURN_IF_C(BYCOPY)
byref				RETURN_IF_C(BYREF)
__complex			RETURN_IF_C(COMPLEX)
__complex__			RETURN_IF_C(COMPLEX)
continue			RETURN_IF_C(CONTINUE)
do					RETURN_IF_C(DO)
else				RETURN_IF_C(ELSE)
extern				RETURN_IF_C(EXTERN)
inline				RETURN_IF_C(INLINE)
for					RETURN_IF_C(FOR)
goto				RETURN_IF_C(GOTO)
if					RETURN_IF_C(IF)
inout				RETURN_IF_C(INOUT)
iterator			RETURN_IF_C(ITERATOR)
register			RETURN_IF_C(REGISTER)
return				RETURN_IF_C(RETURN)
sizeof				RETURN_IF_C(SIZEOF)
static				RETURN_IF_C(STATIC)
oneway				return ONEWAY;
volatile			RETURN_IF_C(VOLATILE)
__volatile			RETURN_IF_C(VOLATILE)
__volatile__		RETURN_IF_C(VOLATILE)
restrict			RETURN_IF_C(RESTRICT)
__restrict			RETURN_IF_C(RESTRICT)
__restrict__		RETURN_IF_C(RESTRICT)
while				RETURN_IF_C(WHILE)
asm					RETURN_IF_C(ASM_KEYWORD)
__asm				RETURN_IF_C(ASM_KEYWORD)
__asm__				RETURN_IF_C(ASM_KEYWORD)
__const				RETURN_IF_C(C_CONST)
__const__			RETURN_IF_C(C_CONST)
__real				RETURN_IF_C(REALPART)
__real__			RETURN_IF_C(REALPART)
__imag				RETURN_IF_C(IMAGPART)
__imag__			RETURN_IF_C(IMAGPART)
attribute			RETURN_IF_C(ATTRIBUTE)
__attribute			RETURN_IF_C(ATTRIBUTE)
__attribute__		RETURN_IF_C(ATTRIBUTE)
"..."				return ELLIPSIS;
">>="				return RS_ASSIGN;
"<<="				return LS_ASSIGN;
"+="				return ADD_ASSIGN;
"-="				return SUB_ASSIGN;
"*="				return MUL_ASSIGN;
"/="				return DIV_ASSIGN;
"%="				return MOD_ASSIGN;
"&="				return AND_ASSIGN;
"^="				return XOR_ASSIGN;
"|="				return OR_ASSIGN;
"++"				return INC_OP;
"--"				return DEC_OP;
"->"				return PTR_OP;
"void"[ \t]*"*"		return VOID_PTR;
"char"[ \t]*"*"		return CHAR_PTR;

{Id}	 	{
	 dcelval._id = new String(yytext);
     // XXX FIXME:
	 CFEFile * pRoot = pCurFile->GetRoot ();
	 ASSERT (pRoot);
	 if (pRoot->FindUserDefinedType (yytext) != 0)
		return TYPENAME;
	 // typename not found, so it has to be an identifier
     return ID;
	}
{Integer}	{
		dcelval._int = atoi (yytext);
		return LIT_INT;
	}
{Hexadec}	{
	 dcelval._int = HexToInt (yytext + 2);	/* strip off the "0x" */
	 return LIT_INT;
	}
{Octal}		{
	 dcelval._int = OctToInt (yytext);	/* first 0 doesn't matter */
	 return LIT_INT;
	}
{Float}		{
	 dcelval._double = atof (yytext);
	 return LIT_FLOAT;
	}
{Char_lit}	{
	 if (yytext[0] == 'L')
		dcelval._char = yytext[2];
	 else
		dcelval._char = yytext[1];
	 return LIT_CHAR;
	}
{Escape}	{
	 dcelval._char = EscapeToChar (yytext);
	 return LIT_CHAR;
	}
{Oct_char}	{
	 dcelval._char = OctToChar (yytext);
	 return LIT_CHAR;
	}
{Hex_char}	{
	 dcelval._char = HexToChar (yytext);
	 return LIT_CHAR;
	}
{Filename}	{
	 dcelval._str = new String (&yytext[1]);
	 dcelval._str->TrimRight('"');
	 dcelval._str->TrimRight('>');
	 return FILENAME;
	}
{String}	{
	 // check if first is 'L'
	 if (yytext[0] == 'L')
	 {
		dcelval._str = new String (&yytext[2]);
		dcelval._str->TrimRight ('"');
	 }
	 else
	 {
		dcelval._str = new String (&yytext[1]);
		dcelval._str->TrimRight ('"');
	 }
	 return LIT_STR;
	}
{Portspec}	return PORTSPEC;
{Uuid}		{
	 dcelval._str = new String (yytext); 
	 return UUID_STR;
	}

"/*"		{
	 register int c; 
	 for (;;)
	 {
	 	while ((c = yyinput ()) != '*' && c != EOF && c != '\n');	// eat up text of comment
		if (c == '*')
	 	{
	 		while ((c = yyinput ()) == '*');	// eat up trailing *
	 		if (c == '/') break;	// found end
	 	}
	 	if (c == '\n') nLineNbDCE++; 
		if (c == EOF)
	 	{
	 		CCompiler::GccError (pCurFile, nLineNbDCE, "EOF in comment.");
	 		yyterminate (); 
			break;
		}
	 }
	}

"//".*			/* eat C++ style comments */
[ \t]*			/* eat whitespace */
("\r")?"\n"		++nLineNbDCE; 
<<EOF>>         {
					CParser *pCurrentParser = CParser::GetCurrentParser();
					if (pCurrentParser->EndImport())
						yyterminate();
				}
.				{
					CCompiler::GccWarning (pCurFile, nLineNbDCE, "Unknown character \"%s\".", yytext);
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
	 	CCompiler::GccWarning (pCurFile, nLineNbDCE,
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
	 	CCompiler::GccWarning (pCurFile, nLineNbDCE,
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

void* SwitchToFileDce(FILE *fNewFile)
{
	YY_BUFFER_STATE old_buffer = YY_CURRENT_BUFFER;
	YY_BUFFER_STATE buffer = yy_create_buffer(fNewFile, YY_BUF_SIZE);
	yy_switch_to_buffer(buffer);
	nIncludeLevelDCE++;
	return old_buffer;
}

void RestoreBufferDce(void *buffer)
{
	nIncludeLevelDCE--;
	yy_delete_buffer(YY_CURRENT_BUFFER);
	yy_switch_to_buffer((YY_BUFFER_STATE)buffer);
	BEGIN(INITIAL);
}
	
