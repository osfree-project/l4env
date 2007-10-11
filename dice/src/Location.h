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
	location()
		: begin_line(1), end_line(1), begin_column(0), end_column(0), filename()
	{ }
	location(const yy::location& loc);
	location(const location& src)
		: begin_line(src.begin_line),
		end_line(src.end_line),
		begin_column(src.begin_column),
		end_column(src.end_column),
		filename(src.filename)
	{ }
	~location()
	{ }

	location& operator= (const yy::location& src);
	location& operator += (const yy::location& loc);

	/** used for maximum determination (if different files, do nothing) */
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

	unsigned int getBeginLine()
	{ return begin_line; }
	void setBeginLine(unsigned int line)
	{ begin_line = line; }
	unsigned int getEndLine()
	{ return end_line; }
	void setEndLine(unsigned int line)
	{ end_line = line; }
	std::string getFilename()
	{ return filename; }
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
	unsigned int begin_line;
	unsigned int end_line;
	unsigned int begin_column;
	unsigned int end_column;
	std::string filename;
};

#endif
