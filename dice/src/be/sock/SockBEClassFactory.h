/**
 *    \file    dice/src/be/sock/SockBEClassFactory.h
 *    \brief   contains the declaration of the class CBEClassFactory
 *
 *    \date    01/10/2002
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
#ifndef SOCKBECLASSFACTORY_H
#define SOCKBECLASSFACTORY_H

#include "be/BEClassFactory.h"

class CSockBEClassFactory : public CBEClassFactory
{
// Constructor
public:
    /**    \brief constructor
     *    \param bVerbose true if class should print status output
     */
    CSockBEClassFactory(bool bVerbose = false);
    virtual ~CSockBEClassFactory();

    virtual CBECallFunction * GetNewCallFunction();
    virtual CBESizes * GetNewSizes();
    virtual CBEWaitAnyFunction * GetNewWaitAnyFunction();
    virtual CBESrvLoopFunction * GetNewSrvLoopFunction();
    virtual CBEUnmarshalFunction * GetNewUnmarshalFunction();
    virtual CBEMarshalFunction* GetNewMarshalFunction();
    virtual CBECommunication * GetNewCommunication();
    virtual CBEWaitAnyFunction* GetNewReplyAnyWaitAnyFunction();
    virtual CBEDispatchFunction* GetNewDispatchFunction();

protected:
    /**    \brief copy constructor
     *    \param src the source to copy from
     */
    CSockBEClassFactory(CSockBEClassFactory &src);
};

#endif
