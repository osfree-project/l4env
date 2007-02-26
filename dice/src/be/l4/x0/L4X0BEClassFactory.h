/**
 *    \file    dice/src/be/l4/x0/L4X0BEClassFactory.h
 *  \brief   contains the declaration of the class CL4X0BEClassFactory
 *
 *    \date    12/01/2002
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
#ifndef L4X0BECLASSFACTORY_H
#define L4X0BECLASSFACTORY_H

#include "be/l4/L4BEClassFactory.h"

/** \class CL4X0BEClassFactory
 *  \ingroup backend
 *  \brief contains platform specific class factory
 *
 * This is the main starting point for a new back-end, because this class
 * is used by the compiler to generate all the other classes. This class
 * is also the only one known to the compiler from a certain back-end.
 */
class CL4X0BEClassFactory : public CL4BEClassFactory
{
// Constructor
public:
    /** \brief constructor
     */
    CL4X0BEClassFactory();
    virtual ~CL4X0BEClassFactory();

public: // Public methods
    virtual CBESizes * GetNewSizes();
    virtual CBETrace* GetNewTrace();
    virtual CBECommunication* GetNewCommunication();
    virtual CBEMsgBuffer* GetNewMessageBuffer();
    virtual CBEDispatchFunction* GetNewDispatchFunction();
};

#endif
