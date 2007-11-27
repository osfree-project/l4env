/**
 *  \file    dice/src/be/l4/fiasco/L4FiascoBEUnmarshalFunction.h
 *  \brief   contains the declaration of the class CL4FiascoBEUnmarshalFunction
 *
 *  \date    11/02/2007
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
#ifndef __DICE_L4FIASCOBEUNMARSHALFUNCTION_H__
#define __DICE_L4FIASCOBEUNMARSHALFUNCTION_H__

#include "be/l4/L4BEUnmarshalFunction.h"

/** \class CL4FiascoBEUnmarshalFunction
 *  \ingroup backend
 *  \brief the function class for the back-end
 *
 * This class contains resembles a back-end function which belongs to a front-end operation
 */
class CL4FiascoBEUnmarshalFunction : public CL4BEUnmarshalFunction
{
// Constructor
public:
    /** \brief constructor
     */
    CL4FiascoBEUnmarshalFunction();
    virtual ~CL4FiascoBEUnmarshalFunction();

	virtual void CreateBackEnd(CFEOperation * pFEOperation, bool bComponentSide);
};

#endif // !__DICE_L4BEUNMARSHALFUNCTION_H__
