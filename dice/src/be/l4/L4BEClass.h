/**
 *    \file    dice/src/be/l4/L4BEClass.h
 *    \brief   contains the declaration of the class CL4BEClass
 *
 *    \date    01/29/2003
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2004
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
#ifndef __DICE_L4BECLASS_H__
#define __DICE_L4BECLASS_H__

#include <be/BEClass.h>

/**
 * \class CL4BEClass
 * \brief a L4 specific implementation of the back-end class class
 * \ingroup backend
 *
 * This class contains L4 specific implementations of some of the
 * base class' functions. It especially overloads the WriteHelperFunctions
 * method to write the declarations of L4 specific helper functions
 */
class CL4BEClass : public CBEClass
{
public:
    /** \brief constructor */
    CL4BEClass();
    virtual ~CL4BEClass();

protected: // Protected methods
    virtual void WriteHelperFunctions(CBEHeaderFile * pFile, CBEContext * pContext);
};

#endif
