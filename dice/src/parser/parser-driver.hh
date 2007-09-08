/**
 *  \file    dice/src/parser/parser-driver.hh
 *  \brief   contains the declaration of the parser driver base class
 *
 *  \date    06/20/2007
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
#ifndef __DICE_PARSER_DRIVER_HH__
#define __DICE_PARSER_DRIVER_HH__

#include <string>
#include <map>
#include <stack>

#include "SymbolTable.h"
using dice::parser::CSymbolTable;
#include "parser/idl/location.hh"

class CFEFile;
#include "fe/FEBase.h"

struct yy_buffer_state;

// Conducting the whole scanning and parsing of Calc++.
class parser_driver
{
public:
    parser_driver ();
    virtual ~parser_driver ();

    // Handling file imports
    void import (std::string f, bool std_inc);
    void enter_file (std::string new_file, long new_line, bool std_inc, long on_line,
	bool check_filetype, bool set_path);
    void leave_file (std::string new_file, bool keep_context = false);

    // Error handling.
    void error (const yy::location& l, const std::string& m);
    void error (const std::string& m);
    void warning (const yy::location& l, const std::string& m);

    // symbol table token check
    bool check_token(std::string s, CSymbolTable::SymbolClass type);
    void add_token(std::string name, CSymbolTable::SymbolClass type,
	CFEBase* context, std::string file, unsigned int line, unsigned int column);

    /** \brief accessor function to set next expected token
     *  \var expecting the new token to expect
     */
    void expecting_token(CSymbolTable::SymbolClass expecting)
    { expectingToken = expecting; }
    /** \brief accessor function to get expected token
     *  \return expected token
     */
    CSymbolTable::SymbolClass expecting_token()
    { return expectingToken; }

    /** \brief accessor function for pCurrentFile
     *  \return reference to pCurrentFile
     */
    CFEFile* getCurrentFile()
    { return pCurrentFile; }
    /** \brief accessor function for pSymbolTable
     *  \return reference to pSymbolTable
     */
    static CSymbolTable* getSymbolTable()
    { return pSymbolTable; }
    /** \brief getter function for actuve context
     *  \return reference to currently active context
     */
    CFEBase* getCurrentContext()
    {
	if (trace_scanning)
	    std::cerr << "parser_driver::getCurrentContext called: return " <<
		(contextStack.empty() ? 0 : contextStack.top()) << std::endl;

	if (contextStack.empty())
	    return (CFEBase*)0;
	else
	    return contextStack.top();
    }
    /** \brief setter function for active context
     *  \var pContext the newly active context
     */
    void setCurrentContext(CFEBase* pContext)
    {
	if (trace_scanning)
	    std::cerr << "parser_driver::setCurrentContext(" << pContext << ") called" <<
		std::endl;

	if (!pContext)
	    return;
	if (contextStack.size() > 0)
	    pContext->setParentContext(contextStack.top());
	else
	    pContext->setParentContext(NULL);
	contextStack.push(pContext);
    }
    /** \brief leave currently active context
     */
    void leaveCurrentContext()
    {
	if (trace_scanning)
	    std::cerr << "parser_driver::leaveCurrentContext() called" << std::endl;

	if (contextStack.size() == 0)
	    return;
	contextStack.top()->setParentContext(NULL);
	contextStack.pop();
    }

    /** \var bool trace_scanning
     *  \brief true if scanning should be traced
     */
    bool trace_scanning;
    /** \var string file
     *  \brief the name of the parsed file
     *
     * This is the file name initially used when calling \c parse. The parsed
     * scan buffer may contain other (included) files. These names are only
     * updated in the location information, which reflects in \c current_file.
     */
    std::string file;
    /** \var string current_file
     *  \brief contains the name of the currently parsed
     */
    std::string current_file;

protected:
    int determine_filetype(std::string f);
    void print_filestack();
    void import_symbols(CFEFile *pFEFile);

protected:
    /** \var CFEFile* pCurrentFile
     *  \brief reference to the currently parsed file
     */
    static CFEFile* pCurrentFile;
    /** \var CSymbolTable* pSymbolTable
     *  \brief reference to the one and only symbol table
     */
    static CSymbolTable* pSymbolTable;
    /** \var std::stack<CFEBase*> contextStack
     *  \brief references to the currently active context
     */
    std::stack<CFEBase*> contextStack;
    /** \var CSymbolTable::SymbolClass expectingToken
     *  \brief denotes the next expected token
     */
    CSymbolTable::SymbolClass expectingToken;
    /** \var YY_BUFFER_STATE previousBuffer
     *  \brief reference to the previous buffer if nested parser calls
     */
    struct yy_buffer_state* previousBuffer;
    /** \var std::string lastPath
     *  \brief contains the path last used to open a file
     */
    std::string lastPath;
};

#endif /* __DICE_PARSER_DRIVER_HH__ */

