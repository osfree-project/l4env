/**
 *  \file    dice/src/parser/newc/c-parser-driver.hh
 *  \brief   contains the declaration of the C/C++ parser driver
 *
 *  \date    06/21/2007
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
#ifndef __DICE_C_PARSER_DRIVER_HH__
#define __DICE_C_PARSER_DRIVER_HH__

#include <string>
#include <map>
#include "parser/c-c++/c-parser.tab.hh"
#include "parser/parser-driver.hh"

// Announce to Flex the prototype we want for lexing function, ...
#ifndef yylex
# define yylex clex
#endif
# define C_DECL				\
    yy::c_parser::token_type                  \
yylex (yy::c_parser::semantic_type* yylval,   \
    yy::c_parser::location_type* yylloc,      \
    c_parser_driver& driver)
// ... and declare it for the parser's sake.
C_DECL;
// .. and tell flex to use it
#undef YY_DECL
#define YY_DECL C_DECL

// Conducting the whole scanning and parsing of Calc++.
class c_parser_driver : public parser_driver
{
public:
    c_parser_driver ();
    ~c_parser_driver ();

    // Handling the scanner.
    void scan_begin ();
    void scan_end ();

    // Handling the parser.
    CFEFile* parse (const std::string& f, bool bPreOnly, bool std_inc);

protected:
    bool is_header(const std::string& f);
    std::string include_next_fix_begin(const std::string& f);
    void include_next_fix_end(const std::string& f);
    void include_next_fix_after_parse(const std::string& f, CFEFile* pWrapper);
};

#endif /* __DICE_C_PARSER_DRIVER_HH__ */

