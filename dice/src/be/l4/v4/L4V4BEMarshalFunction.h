/**
 *  \file     dice/src/be/l4/v4/L4V4BEMarshalFunction.h
 *  \brief    contains the declaration of the class CL4V4BEMarshalFunction
 *
 *  \date     Mon Jul 5 2004
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
#ifndef L4V4BEMARSHALFUNCTION_H
#define L4V4BEMARSHALFUNCTION_H

#include <be/l4/L4BEMarshalFunction.h>

/** \class CL4V4BEMarshalFunction
 *    \ingroup backend
 *  \brief specializes some members for V4 backend
 */
class CL4V4BEMarshalFunction : public CL4BEMarshalFunction
{
public:
    /** creates a new instance of this class */
    CL4V4BEMarshalFunction();
    virtual ~CL4V4BEMarshalFunction();

protected:
    virtual void WriteMarshalling(CBEFile& pFile);
};

#endif
