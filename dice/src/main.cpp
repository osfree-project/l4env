/**
 *	\file	dice/src/main.cpp
 *	\brief	is the starting point of the compiler
 *
 *	\date	01/31/2001
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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

#include "Compiler.h"

/** \brief the main function
 *  \param argc the number of arguments
 *  \param argv the array of arguments
 */
int main(int argc, char *argv[])
{
    CCompiler *pCompiler = new CCompiler();

    if (argc > 1)
    {
        pCompiler->ParseArguments(argc, argv);
        pCompiler->Parse();
        pCompiler->PrepareWrite();
        pCompiler->Write();
    }
    else
    {
        pCompiler->ShowHelp();
    }

    delete pCompiler;

    return 0;
}
