/**
 *  \file    dice/src/Location.h
 *  \brief   contains the declaration of the class location
 *
 *  \date    08/11/2007
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
#ifndef __DICE_LOCATION_H__
#define __DICE_LOCATION_H__

#include <string>

namespace yy { class location; }

/** \class location
 *  \ingroup backend
 *  \brief class for location tracking of objects
 */
class location
{
public:
	/** \brief basic constructor
	 */
	location()
		: begin_line(1), end_line(1), begin_column(0), end_column(0), filename()
	{ }
	/** \brief copy constructor for parser locations
	 *  \param loc the source of the copy operation
	 */
	location(const yy::location& loc);
	/** \brief copy constructor
	 *  \param src the source of the copy operation
	 */
	location(const location& src)
		: begin_line(src.begin_line),
		end_line(src.end_line),
		begin_column(src.begin_column),
		end_column(src.end_column),
		filename(src.filename)
	{ }
	/** \brief destructor
	 */
	~location()
	{ }

	location& operator= (const yy::location& src);
	location& operator += (const yy::location& loc);

	/** \brief used for maximum determination (if different files, do nothing)
	 *  \param loc the location to add to the current location
	 *  \return this object
	 *
	 * The returned object is the transitive hull of the current object and \a
	 * loc. If the file names are different the current object is returned.
	 */
	location& operator+= (const location& loc)
	{
		if (filename != loc.filename)
			return *this;

		if (loc.begin_line < begin_line)
		{
			begin_line = loc.begin_line;
			begin_column = loc.begin_column;
		}
		else if (loc.begin_column < begin_column)
			begin_column = loc.begin_column;

		if (loc.end_line > end_line)
		{
			end_line = loc.end_line;
			end_column = loc.end_column;
		}
		else if (loc.end_column > end_column)
			end_column = loc.end_column;

		return *this;
	}

	/** \brief comparison function for locations
	 *  \param loc the location to compare to
	 *  \return true if this location is "bigger" than \a loc
	 *
	 * First, an order is determined based on the begin line numbers. If the
	 * begin line numbers are different, then the value of the expression
	 * begin_line > loc.begin_line is the return value. If the begin lines are
	 * the same, the order of the begin columns determines the return value.
	 */
	bool operator> (const location& loc)
	{
		if (begin_line > loc.begin_line)
			return true;
		if (begin_line < loc.begin_line)
			return false;
		/* begin_line == loc.begin_line */
		return begin_column > loc.begin_column;
	}

	friend std::ostream& operator<< (std::ostream&, const location&);

	/** \brief getter function for begin line
	 *  \return value of begin_line
	 */
	unsigned int getBeginLine()
	{ return begin_line; }
	/** \brief setter function for begin line
	 *  \param line the new line number
	 */
	void setBeginLine(unsigned int line)
	{ begin_line = line; }
	/** \brief getter function for end line
	 *  \return value of end line
	 */
	unsigned int getEndLine()
	{ return end_line; }
	/** \brief setter function for end line
	 *  \param line the new line number
	 */
	void setEndLine(unsigned int line)
	{ end_line = line; }
	/** \brief getter function for file name
	 *  \return the file name
	 */
	std::string getFilename()
	{ return filename; }
	/** \brief setter function for whole location
	 *  \param name the new file name
	 *  \param bLine the new begin line
	 *  \param bColumn the new begin column
	 *  \param eLine the new end line
	 *  \param eColumn the new end column
	 */
	void setLocation(std::string name, unsigned int bLine, unsigned int bColumn,
		unsigned int eLine, unsigned int eColumn)
	{
		filename = name;
		begin_line = bLine;
		begin_column = bColumn;
		end_line = eLine;
		end_column = eColumn;
	}

protected:
	/** \var unsigned int begin_line
	 *  \brief begin line of location
	 */
	unsigned int begin_line;
	/** \var unsigned int end_line
	 *  \brief end line of location
	 */
	unsigned int end_line;
	/** \var unsigned int begin_column
	 *  \brief egin column of location
	 */
	unsigned int begin_column;
	/** \var unsigned int end_column
	 *  \brief end column of location
	 */
	unsigned int end_column;
	/** \var std::string filename
	 *  \brief the filename of the location
	 */
	std::string filename;
};

#endif
