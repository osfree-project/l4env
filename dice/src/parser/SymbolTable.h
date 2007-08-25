/**
 *  \file    dice/src/parser/SymbolTable
 *  \brief   contains the declaration of the symbol table
 *
 *  \date    06/23/2007
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
#ifndef __DICE_PARSER_SYMBOLTABLE_H__
#define __DICE_PARSER_SYMBOLTABLE_H__

#include <string>
#include <map>

class CFEBase;
class CFEFile;

namespace dice {
    namespace parser {

	class CSymbolTable
	{
	public:
	    CSymbolTable();
	    ~CSymbolTable();

	    enum SymbolClass
	    {
		NONE,
		LIBRARY,
		NAMESPACE = LIBRARY,
		INTERFACE,
		CLASS = INTERFACE,
		FUNCTION,
		VARIABLE,
		TYPENAME,
		ENUM,
		TEMPLATE
	    };

	    struct CSymTabEntry
	    {
		std::string name;
		SymbolClass type;
		CFEBase* context;
		/* location */
		std::string file;
		unsigned int line;
		unsigned int column;

		CSymTabEntry();
		CSymTabEntry(const CSymTabEntry&);

		bool operator==(const CSymTabEntry&);
	    };

	    void add(std::string name, SymbolClass type, CFEBase* context, std::string file,
		unsigned int line, unsigned int column);
	    bool check(CFEBase* pCurrentContext, std::string name, SymbolClass type);
	    void change_context(CFEBase* pOriginal, CFEBase* pNew);

	    const CSymTabEntry InvalidEntry;

	protected:
	    typedef std::multimap<std::string, CSymTabEntry> symtab_t;
	    typedef std::pair<symtab_t::iterator, symtab_t::iterator> symrange_t;

	    symtab_t symtab;

	    CFEBase* symbol_context(CFEBase *context);
	    bool check_context(symrange_t range, CFEBase *context, SymbolClass type);
	    bool check_context_in_file(symrange_t range, CFEFile *file, SymbolClass type);
	};

    };
};

#endif /* __DICE_PARSER_SYMBOLTABLE_H__ */
