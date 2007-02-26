/**
 *    \file    dice/src/be/l4/v2/ia32/V2IA32ClassFactory.h
 *    \brief   contains the declaration of the class CL4V2IA32BEClassFactory
 *
 *    \date    04/18/2006
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#ifndef __DICE_V2IA32CLASSFACTORY_H__
#define __DICE_V2AIA32LASSFACTORY_H__

#include "be/l4/v2/L4V2BEClassFactory.h"

/** \class CL4V2IA32BEClassFactory
 *  \ingroup backend
 *  \brief the class factory for the back-end classes
 *
 * We use seperate functions for each class, because the alternative is to use
 * some sort of identifier to find out which class to generate. This involves
 * writing a big switch statement.
 */
class CL4V2IA32BEClassFactory : public CL4V2BEClassFactory
{
// Constructor
public:
    /** \brief constructor
     */
    CL4V2IA32BEClassFactory();
    virtual ~CL4V2IA32BEClassFactory();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CL4V2IA32BEClassFactory(CL4V2IA32BEClassFactory &src);

public:
    virtual CBESizes * GetNewSizes();
    virtual CBECommunication* GetNewCommunication();
};

#endif // !__DICE_V2IA32CLASSFACTORY_H__
