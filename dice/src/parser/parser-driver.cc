/**
 *  \file    dice/src/parser/parser-driver.cc
 *  \brief   contains the definition of the parser driver base class
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

#include "parser-driver.hh"
#include "parser/idl/idl-parser-driver.hh"
#undef PARSER_HEADER_H
#include "parser/c-c++/c-parser-driver.hh"
#include "Preprocessor.h"
using dice::parser::CPreprocessor;
#include "fe/FEFile.h"
#include "Compiler.h"
#include "Error.h"
#include <stack>
#include <sstream>

CFEFile* parser_driver::pCurrentFile = 0;
CSymbolTable* parser_driver::pSymbolTable = 0;

parser_driver::parser_driver()
    : trace_scanning(false),
      expectingToken(CSymbolTable::NONE),
      previousBuffer(0)
{ }

parser_driver::~parser_driver()
{ }

/** \brief try to determine the file type to use
 *  \param f the file name
 *
 * Currently, this function just checks for "idl" and returns USE_FILE_IDL if
 * so. In all other cases it returns USE_FILE_C.
 */
int parser_driver::determine_filetype(std::string f)
{
	string ext = f.substr(f.rfind(".")+1);
	std::transform(ext.begin(), ext.end(), ext.begin(), _tolower);

	if (ext == "idl")
		return USE_FILE_IDL;
	return USE_FILE_C;
}

/** \brief import a file given in an import statement
 *  \param f the filename to import
 *  \param std_inc true if standard include "<>"
 *
 * This function checks the file extension, and depending on that creates a
 * new parser object and calls its parse function. This function does nothing
 * if there is an error.
 */
void parser_driver::import (std::string f, bool std_inc)
{
	if (f.empty())
		return;

	CFEFile *pFEFile = 0;
	switch (determine_filetype(f))
	{
	case USE_FILE_IDL:
		{
			idl_parser_driver p;
			pFEFile = p.parse(f, false, std_inc);
		}
		break;
	case USE_FILE_C:
		{
			c_parser_driver p;
			pFEFile = p.parse(f, false, std_inc);
		}
		break;
	default:
		break;
	}
	if (!pFEFile)
		return; // only preprocessing

	import_symbols(pFEFile);
}

/** \brief import the symbols of an imported file into the current scope
 *  \param pFEFile the imported file
 */
void parser_driver::import_symbols (CFEFile *pFEFile)
{
	// XXX fast hack:
	// replace occurrences of pFEFile in symbol table with current context
	// -> import parsed symbols into local context scheme
	if (getSymbolTable() && pCurrentFile)
		getSymbolTable()->change_context(pFEFile, pCurrentFile);

	// a clean solution is to iterate the symbols in the file and add them to
	// the current context in the current file.
}

/** \brief enter a new file
 *  \param new_file the filename of the new file
 *  \param new_line the linenumber where we start in the file
 *  \param std_inc true if standard include
 *  \param on_line is the line number in the original file
 *  \param check_filetype false if check of file type should be omitted
 *  \param set_path true if the path of the file object should be set
 *
 * We simply create a new AST file object, add it to the current file and set
 * the new file object as current file. To ensure that we do not switch the
 * parser in between, we check the file extension of the current file and the
 * file extension of the new file.
 */
void
parser_driver::enter_file (std::string new_file,
    long new_line,
    bool std_inc,
    long on_line,
    bool check_filetype,
    bool set_path)
{
	if (trace_scanning)
		std::cerr << "parser_driver::enter_file (" << new_file << ", " << new_line <<
			", " << (std_inc ? "true" : "false") << ", " << on_line << ", " <<
			(check_filetype ? "true" : "false") << ", " <<
			(set_path ? "true" : "false") << ") called" << std::endl;
	if (pCurrentFile && check_filetype &&
		(determine_filetype(new_file) != determine_filetype(pCurrentFile->GetFileName())) )
	{
		std::ostringstream os;
		os << pCurrentFile->GetFileName() << ":" << on_line
			<< ": error: Cannot include file \"" << new_file << "\" with different file type."
			<< std::endl;
		os << "Use \'import \"" << new_file << "\"\' instead.";
		error(os.str());
	}

	std::string path;
	if (set_path)
	{
		/* first, try to find the path in the name of the file (doing it the
		 * other way around would allow to set two paths, or the same path
		 * twice) */
		path = CPreprocessor::GetPreprocessor()->FindIncludePathInName(new_file);
		/* if that didn't work, reuse last path used to include the file */
		if (path.empty())
			path = lastPath;
		if (!path.empty() && new_file.find(path) == 0)
			new_file = new_file.substr(path.length());
	}
	if (trace_scanning)
		std::cerr << "parser_driver::enter_file: create CFEFile(" << new_file << ", " <<
			path << ", " << new_line << ", " << (std_inc ? "true" : "false") << ")" << std::endl;
	CFEFile *pNewFile = new CFEFile(new_file, path, new_line, std_inc);
	pNewFile->m_sourceLoc.setLocation(pCurrentFile ? pCurrentFile->GetFileName() : "(null)",
		on_line, 0, on_line, 0);
	if (pCurrentFile)
		pCurrentFile->m_ChildFiles.Add(pNewFile);
	pCurrentFile = pNewFile;

	// current file is current context now
	setCurrentContext(pCurrentFile);

	if (trace_scanning)
		std::cerr << "parser_driver::enter_file: " << pCurrentFile->GetFileName() <<
			" is current file" << std::endl;
}

/** \brief leave current file
 *  \param new_file the name of the file we switch to
 *  \param keep_context true if context should not be left (defaults to false)
 *
 * We simple hop up the file tree until we found the file that matches the
 * given name and set that file as current file. If no file name is given,
 * simply select next file.
 */
void
parser_driver::leave_file (std::string new_file, bool keep_context)
{
    CFEFile *pFile = pCurrentFile;
    if (trace_scanning)
	std::cerr << "parser_driver::leave_file (" << new_file << ") called, current is " <<
	    (pFile ? pFile->GetFileName() : "(null)") << std::endl;

    // leave current context (current file)
    if (!keep_context)
	leaveCurrentContext();

    while (pFile)
    {
	pFile = pFile->GetSpecificParent<CFEFile>();
	if (trace_scanning)
	    std::cerr << "parser_driver::leave_file: checking file " <<
		(pFile ? pFile->GetFileName() : "(null)") << std::endl;

	if (pFile && (pFile->GetFileName() == new_file ||
		pFile->GetFullFileName() == new_file ||
		new_file.empty()))
	{
	    pCurrentFile = pFile;
	    if (trace_scanning)
		std::cerr << "parser_driver::leave_file: current file is " << pFile->GetFileName() <<
		    std::endl;
	    return;
	}
    }

    if (trace_scanning)
	std::cerr << "parser_driver::leave_file: file " << new_file << " not found" << std::endl;
}

/** \brief print the stack of file up to the current file
 *
 * We simply step the list of files up to the top file. The we go down the
 * stack again and print the name of the current file and the name of the line
 * number of the next file (which indicates the line of the include
 * statement).
 *
 * When building the stack, we skip the current file, because its name will be
 * printed by the following error or warning message.
 */
void
parser_driver::print_filestack()
{
    std::stack<CFEFile*> stack;
    CFEFile *pFile = pCurrentFile;
    if (!pCurrentFile)
	return;
    while ((pFile = pFile->GetSpecificParent<CFEFile>()))
	stack.push(pFile);

    std::cerr << "In file included ";
    while (!stack.empty())
    {
	pFile = stack.top();
	stack.pop();
	int line = 1;
	if (stack.empty())
	    line = pCurrentFile->m_sourceLoc.getBeginLine();
	else
	    line = stack.top()->m_sourceLoc.getBeginLine();
	std::cerr << "from " << pFile->GetFullFileName() << ":" << line;
	if (stack.empty())
	    std::cerr << ":" << std::endl;
	else
	    std::cerr << "," << std::endl << "                 ";
    }
}

/** \brief print error message with exact location
 *  \param l the location of the parser
 *  \param m the message to print
 */
void
parser_driver::error (const yy::location& l, const std::string& m)
{
    print_filestack();
    std::cerr << l << ": error: " << m << std::endl;
    throw error::parse_error();
}

/** \brief print error message with exact location
 *  \param m the message to print
 */
void
parser_driver::error (const std::string& m)
{
    print_filestack();
    std::cerr << m << std::endl;
    throw error::parse_error();
}

/** \brief print warning message with exact location
 *  \param l the location of the parser
 *  \param m the message to print
 */
void
parser_driver::warning (const yy::location& l, const std::string& m)
{
    print_filestack();
    std::cerr << l << ": warning: " << m << std::endl;
}

/** \brief check if the string appears somewhere in the symbol table.
 *  \param s the string to check
 *  \param type the expected type (or NONE)
 *  \return true if found in that constellation
 *
 * If no symbol table exists so far or the token could not be found in the
 * symbol table, return CSymbolTable::NONE.
 */
bool
parser_driver::check_token(std::string s,
    CSymbolTable::SymbolClass type)
{
    if (trace_scanning)
	std::cerr << "parser_driver::check_token (" << s << ") called" << std::endl;

    if (!pSymbolTable)
	return false;
    return pSymbolTable->check(getCurrentContext(), s, type);
}

/** \brief add a new entry to the symbol table
 *  \param name the original string
 *  \param type the class or type of the entry
 *  \param context the context of the definition of this entry
 *  \param f the file of the definition
 *  \param line the line number in the file of the definition
 *  \param column the column number in the file of the definition
 *
 * If no symbol table exists, one will be created and the new entry is added.
 */
void parser_driver::add_token(std::string name, CSymbolTable::SymbolClass type,
    CFEBase* context, std::string f, unsigned int line, unsigned int column)
{
    if (trace_scanning)
	std::cerr << "parser_driver::add_token(" << name << ", " << (int)type <<
	    ", " << context << ", " << f << ", " << line << ", " << column <<
	    ") called" << std::endl;

    if (!pSymbolTable)
	pSymbolTable = new CSymbolTable();
    if (!context)
	context = getCurrentContext();
    pSymbolTable->add(name, type, context, f, line, column);
}

