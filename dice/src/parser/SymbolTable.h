/**
 *  \file    dice/src/parser/SymbolTable.h
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

		/** \class CSymbolTable
		 *  \ingroup parser
		 *  \brief contains the map of names to type of symbol
		 *
		 * This class is used to lookup whether an identifier in the IDL is a
		 * type, an interface, or something else.
		 */
		class CSymbolTable
		{
		public:
			/** constructor */
			CSymbolTable();
			/** destructor */
			~CSymbolTable();

			/** \enum SymbolClass
			 *  \brief the type of symbol
			 */
			enum SymbolClass
			{
				NONE,					/**< nothing known */
				LIBRARY,				/**< a library name */
				NAMESPACE = LIBRARY,	/**< a library name */
				INTERFACE,				/**< an interface name */
				CLASS = INTERFACE,		/**< an interface name */
				FUNCTION,				/**< a function name */
				VARIABLE,				/**< a variable name */
				TYPENAME,				/**< a type name */
				ENUM,					/**< an enumeration name */
				TEMPLATE				/**< a template name */
			};

			/** \struct CSymTabEntry
			 *  \brief type of an entry
			 */
			struct CSymTabEntry
			{
				/** \var std::string name
				 *  \brief the name of the token
				 */
				std::string name;
				/** \var SymbolClass type
				 *  \brief the type of the token
				 */
				SymbolClass type;
				/** \var CEBase* context
				 *  \brief the context this toke was declared in
				 */
				CFEBase* context;
				/** \var std::string file
				 *  \brief the file where the token was declared
				 */
				std::string file;
				/** \var unsigned int line
				 *  \brief the line of the token declaration
				 */
				unsigned int line;
				/** \var unsigned int column
				 *  \brief the colum of the token declaration
				 */
				unsigned int column;

				/** constructor */
				CSymTabEntry();
				/** copy constructor
				 *  \param src the source to copy from
				 */
				CSymTabEntry(const CSymTabEntry&);

				/** \brief comparison operator
				 *  \param src the other entry to compare to
				 */
				bool operator==(const CSymTabEntry&);
			};

			void add(std::string name, SymbolClass type, CFEBase* context, std::string file,
				unsigned int line, unsigned int column);
			bool check(CFEBase* pCurrentContext, std::string name, SymbolClass type);
			void change_context(CFEBase* pOriginal, CFEBase* pNew);

			/** \var CSymTabEntry InvalidEntry
			 *  \brief an invalid entry to compare to
			 */
			const CSymTabEntry InvalidEntry;

		protected:
			/** \typedef std::multimap<std::string, CSymTabEntry> symtab_t
			 *  \brief alias for map from name to symbol table entry
			 */
			typedef std::multimap<std::string, CSymTabEntry> symtab_t;
			/** \typedef std::pair<symtab_t::iterator, symtab_t::iterator> symrange_t
			 *  \brief alias for rane of iterators indicating found symbol
			 *		table entries for one name
			 */
			typedef std::pair<symtab_t::iterator, symtab_t::iterator> symrange_t;

			/** \var symtab_t symtab
			 *  \brief the symbol table
			 */
			symtab_t symtab;

			bool check_context(symrange_t range, CFEBase *context, SymbolClass type);
			bool check_context_in_file(symrange_t range, CFEFile *file, SymbolClass type);
		};

	};
};

#endif /* __DICE_PARSER_SYMBOLTABLE_H__ */
