/**
 *  \file   dice/src/be/l4/v2/L4V2BEDispatchFunction.h
 *  \brief  contains the declaration of the class CL4V2BEDispatchFunction
 *
 *  \date   08/03/2006
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006
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
#ifndef L4V2BEDISPATCHFUNCTION_H
#define L4V2BEDISPATCHFUNCTION_H

#include <be/l4/L4BEDispatchFunction.h>

/** \class CL4V2BEDispatchFunction
 *  \ingroup backend
 *  \brief the function class for the back-end
 *
 * This class contains resembles a back-end function which belongs to a
 * front-end operation
 */
class CL4V2BEDispatchFunction : public CL4BEDispatchFunction
{
// Constructor
public:
    /** \brief constructor
     */
    CL4V2BEDispatchFunction();
    ~CL4V2BEDispatchFunction();

protected:
    /** \brief copy constructor */
    CL4V2BEDispatchFunction(CL4V2BEDispatchFunction &src);

protected:
    virtual void WriteSetWrongOpcodeException(CBEFile* pFile);
};

#endif
