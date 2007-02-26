/**
 *  \file     dice/src/be/l4/v4/L4V4BETestMainFunction.h
 *  \brief    contains the declaration of the class CL4V4BETestMainFunction
 *
 *  \date     Sat Jul 3 2004
 *  \author   Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2004 by
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
#ifndef L4V4BETESTMAINFUNCTION_H
#define L4V4BETESTMAINFUNCTION_H

#include <be/l4/L4BETestMainFunction.h>

/** \class CL4V4BETestMainFunction
 *    \ingroup backend
 *    \brief L4 V4 specific implementation of the testsuite's main function
 */
class CL4V4BETestMainFunction : public CL4BETestMainFunction
{
public:
    /** creates a new object of this class */
    CL4V4BETestMainFunction();
    virtual ~CL4V4BETestMainFunction();

public:
    virtual void WriteCleanup(CBEFile * pFile, CBEContext * pContext);
};

#endif
