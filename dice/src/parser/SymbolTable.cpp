/**
 *  \file    dice/src/parser/SymbolTable.cpp
 *  \brief   contains the definition of symbol table
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

#include "SymbolTable.h"
#include "fe/FEBase.h"
#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include <iostream>

using namespace dice::parser;

CSymbolTable::CSymTabEntry::CSymTabEntry()
{
	type = NONE;
	context = 0;
}

CSymbolTable::CSymTabEntry::CSymTabEntry(const CSymTabEntry& src)
: name(src.name), type(src.type), file(src.file), line(src.line), column(src.column)
{
	context = src.context;
}

bool CSymbolTable::CSymTabEntry::operator==(const CSymTabEntry& src)
{
	return name == src.name &&
		type == src.type &&
		file == src.file &&
		line == src.line &&
		column == src.column;
}

CSymbolTable::CSymbolTable()
{ }

CSymbolTable::~CSymbolTable()
{ }

#if 0
static std::string context_to_str(CFEBase *context)
{
	std::string ret("other");
	if (dynamic_cast<CFEFile*>(context))
		ret = " file " + static_cast<CFEFile*>(context)->GetFileName();
	else if (dynamic_cast<CFELibrary*>(context))
		ret = " lib " + static_cast<CFELibrary*>(context)->GetName();
	else if (dynamic_cast<CFEInterface*>(context))
		ret = " interface " + static_cast<CFEInterface*>(context)->GetName();
	return ret;
}

static std::string class_to_str(CSymbolTable::SymbolClass sym)
{
	switch (sym)
	{
	case CSymbolTable::NONE:
		return std::string("none");
	case CSymbolTable::LIBRARY:
		return std::string("library");
	case CSymbolTable::INTERFACE:
		return std::string("interface");
	case CSymbolTable::FUNCTION:
		return std::string("function");
	case CSymbolTable::VARIABLE:
		return std::string("variable");
	case CSymbolTable::TYPENAME:
		return std::string("type");
	case CSymbolTable::ENUM:
		return std::string("enum");
	case CSymbolTable::TEMPLATE:
		return std::string("template");
	default:
		break;
	}
	return std::string("other");
}
#endif

/** \brief adds a new symbol to the symbol table
 *  \param name the name of the symbol
 *  \param type the type of the symbol
 *  \param context the context of the symbol
 *  \param file the file where the symbol was declared
 *  \param line the line where the symbol was declared
 *  \param column the column where the symbol was declared
 */
void CSymbolTable::add(std::string name, SymbolClass type, CFEBase* context, std::string file,
	unsigned int line, unsigned int column)
{
	CSymTabEntry t;
	t.name = name;
	t.type = type;
	t.context = context;
	t.file = file;
	t.line = line;
	t.column = column;

	symtab.insert(std::pair<std::string, CSymTabEntry>(name, t));
}

/** \brief check if a symbol in a range has the given context and type
 *  \param range a range of symbol previously obtained using the name of the
 *		symbol
 *  \param context the context of the symbol
 *  \param type the type of the symbol
 *  \return true if one of the symbols in \a range matches \a context and \a
 *		type
 */
bool CSymbolTable::check_context(symrange_t range, CFEBase* context, SymbolClass type)
{
	symtab_t::iterator it;
	for (it = range.first; it != range.second; it++)
	{
		if ( ((*it).second.context == context) &&
			((*it).second.type == type) )
			return true;
	}
	return false;
}

/** \brief check if symbol in a range matches file and type
 *  \param range a range of symbols previously obtained using the name of the
 *		symbol
 *  \param file the file to use as context
 *  \param type the type of the symbol
 *  \return true if one of the symbols in \a range matches
 *
 * This fuctions checks if one of the included files in \a file is the context
 * of a symbol in \a range using \a check_context .
 */
bool CSymbolTable::check_context_in_file(symrange_t range, CFEFile *file, SymbolClass type)
{
	if (!file)
		return false;

	std::vector<CFEFile*>::iterator i;
	for (i = file->m_ChildFiles.begin(); i != file->m_ChildFiles.end(); i++)
	{
		if (check_context(range, *i, type))
			return true;

		if (check_context_in_file(range, *i, type))
			return true;
	}

	return false;
}

/** \brief check if the given name and type matches a symbol in the table
 *  \param pCurrentContext the current active context
 *  \param name the name of the symbol to find
 *  \param type the type of the symbol to find
 *	\return true if symbol found
 */
bool CSymbolTable::check(CFEBase* pCurrentContext, std::string name, SymbolClass type)
{
	if (symtab.find(name) == symtab.end())
		return false;

	// check whether context matches
	symrange_t range = symtab.equal_range(name);
	CFEBase *context = pCurrentContext;
	// test at the end, so a NULL context can be checked as well
	do
	{
		if (check_context(range, context, type))
			return true;
		/** we couldn't find the symbol in the current context or in one of the
		 * parent contextes. If the current context is a file then check if it
		 * has child (included) files we can search in. Only check inside the
		 * current context stack, because otherwise we could intermix multiple
		 * include paths.
		 * A.idl -> import b.h -> include c.h -> defines type a;
		 * A.idl -> import d.h -> include c.h -> defines type a;
		 *                     -> use type a in a function (check included files
		 *                     of d.h)
		 */
		// check for file is done inside check_context_in_file
		if (check_context_in_file(range, dynamic_cast<CFEFile*>(context), type))
			return true;

		if (context)
			context = context->getParentContext();
	} while (context);

	return false;
}

/** \brief replace a given original context with a new context
 *  \param pOriginal the context to replace
 *  \param pNew the context to replace with
 */
void CSymbolTable::change_context(CFEBase *pOriginal, CFEBase* pNew)
{
	symtab_t::iterator it;
	for (it = symtab.begin(); it != symtab.end(); it++)
	{
		if ((*it).second.context == pOriginal)
			(*it).second.context = pNew;
	}
}

