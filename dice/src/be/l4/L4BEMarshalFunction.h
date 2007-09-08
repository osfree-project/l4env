/**
 *  \file    dice/src/be/l4/L4BEMarshalFunction.h
 *  \brief   contains the declaration of the class CL4BEMarshalFunction
 *
 *  \date    10/10/2003
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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
#ifndef L4BEMARSHALFUNCTION_H
#define L4BEMARSHALFUNCTION_H


#include <be/BEMarshalFunction.h>

/** \class CL4BEMarshalFunction
 *  \ingroup backend
 *  \brief the function class for the back-end
 *
 * This class contains resembles a back-end function which belongs to a
 * front-end operation
 */
class CL4BEMarshalFunction : public CBEMarshalFunction
{
// Constructor
public:
    /** \brief constructor
     */
    CL4BEMarshalFunction();
    ~CL4BEMarshalFunction();

    virtual int GetFixedSize(DIRECTION_TYPE nDirection);
    virtual int GetSize(DIRECTION_TYPE nDirection);

protected:
    virtual bool HasVariableSizedParameters(
	DIRECTION_TYPE nDirection);
    virtual void WriteMarshalling(CBEFile& pFile);
};

#endif
