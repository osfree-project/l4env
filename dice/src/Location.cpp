/**
 *  \file    dice/src/Location.cpp
 *  \brief   contains the implementation of the class location
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

#include "Location.h"
/* we want to use the bison generated position classes, therefore, we select
 * one of the generated files (from the IDL parser) to include.
 */
#include "parser/idl/location.hh"

location::location(const yy::location& loc)
: begin_line(loc.begin.line),
    end_line(loc.end.line),
    begin_column(loc.begin.column),
    end_column(loc.end.column),
    filename(*loc.begin.filename)
{ }

location& location::operator= (const yy::location& src)
{
    begin_line = src.begin.line;
    begin_column = src.begin.column;
    end_line = src.end.line;
    end_column = src.end.column;
    filename = *src.begin.filename;
    return *this;
}

location& location::operator += (const yy::location& loc)
{
    location t(loc);
    return operator+= (t);
}

std::ostream& operator<< (std::ostream& os, const location& loc)
{
    os << loc.filename << ":" << loc.begin_line << "." << loc.begin_column;
    if (loc.end_line != loc.begin_line)
	os << "-" << loc.end_line << "." << loc.end_column;
    else if (loc.end_column != loc.begin_column)
	os << "-" << loc.end_column;
    return os;
}
