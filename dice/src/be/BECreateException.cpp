/**
 *    \file    dice/src/be/BECreateException.cpp
 *  \brief   contains the implementation of the class CBECreateException
 *
 *    \date    06/30/2005
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2005
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

#include "BECreateException.h"
#include <iostream>

CBECreateException::CBECreateException()
{
}

CBECreateException::CBECreateException(string sReason)
: CException(sReason)
{
}

CBECreateException::CBECreateException(CBECreateException & src)
    : CException(src)
{
}

/** cleans up the object */
CBECreateException::~CBECreateException()
{
}

/** \brief print the reason of the create exception
 */
void
CBECreateException::Print()
{
    std::cerr << "Creation failed, because " << m_sReason << std::endl;
}

