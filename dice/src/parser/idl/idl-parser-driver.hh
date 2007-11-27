/**
 *  \file    dice/src/parser/idl/idl-parser-driver.hh
 *  \brief   contains the declaration of the IDL parser driver
 *
 *  \date    06/14/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
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

/** preprocessing symbol to check header file */
#ifndef __DICE_IDL_PARSER_DRIVER_HH__
#define __DICE_IDL_PARSER_DRIVER_HH__

#include <string>
#include <map>
#include "parser/idl/idl-parser.tab.hh"
#include "parser/parser-driver.hh"

// Announce to Flex the prototype we want for lexing function, ...
#ifndef yylex
# define yylex idllex
#endif
/** \fn yy::idl_parser::token_type idllex (yy::idl_parser::semantic_type* yylval,
 *		yy::idl_parser::location_type* yylloc, idl_parser_driver& driver)
 *  \brief lexer function invoked by parser
 *  \retval yylval the lexical item (token plus value)
 *  \retval yylloc the location of the token
 *  \param driver the parser driver that calls the lexer function
 *  \return the scanned token
 *
 * The parser will call this function to obtain the next token.
 */
/** \def IDL_DECL
 *  \brief redefine \a YY_DECL using \a IDL_DECL so we can have two parsers (C
 *		and IDL)
 */
# define IDL_DECL				\
    yy::idl_parser::token_type                  \
yylex (yy::idl_parser::semantic_type* yylval,   \
    yy::idl_parser::location_type* yylloc,      \
    idl_parser_driver& driver)
// ... and declare it for the parser's sake.
IDL_DECL;
// ... and tell flex to use it
/** \def YY_DECL
 *  \brief redefine the macro used by parser to be the IDL specific one
 */
#undef YY_DECL
#define YY_DECL IDL_DECL

/** \class idl_parser_driver
 *  \ingroup parser
 *  \brief drives the parsing of an IDL file
 */
class idl_parser_driver : public parser_driver
{
public:
    idl_parser_driver ();
    ~idl_parser_driver ();

	/** \brief called when scanning begins
	 *
	 * Even though the parsers are classes and encapsulate the parsing
	 * process, they use a lexing function (\a idllex) that reads input tokens
	 * from a global input buffer. I.e., no new scanner is instantiated with a
	 * new parser. Thus the parser is responsible for managing the input
	 * buffers. For this purpose, whenever a new file is supposed to be
	 * scanned, the parser stores a reference to the currently open input
	 * buffer in a member variable and openes the to-be-parsed file in a new
	 * input buffer. When the parser is finished \a scan_end is called.
	 */
    void scan_begin ();
	/** \brief called when scanning ends
	 *
	 * As mentioned in \a scan_begin the parser keeps reference to a
	 * previously open input buffer of its scanner. When the parsing is
	 * finished this input buffer is restored. So a "outer" parser can
	 * continue its execution.
	 */
    void scan_end ();

    // Handling the parser.
    CFEFile* parse (const std::string& f, bool bPreOnly, bool std_inc);

    // symbol table handling
	/** \typedef yy::idl_parser::token::yytokentype tokentype
	 *  \brief short alias for token type
	 */
    typedef yy::idl_parser::token::yytokentype tokentype;
    tokentype find_attribute(const std::string &s);
	/** \var bool expect_attr
	 *  \brief true if an attribute is expected
	 */
    bool expect_attr;

private:
	/** \var std::map<std::string, tokentype> attr_table
	 *  \brief map containing the known attributes
	 */
    std::map<std::string, tokentype> attr_table;
};

#endif /* __DICE_IDL_PARSER_DRIVER_HH__ */

