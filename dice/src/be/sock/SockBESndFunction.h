/**
 *  \file   dice/src/be/sock/SockBESndFunction.h
 *  \brief  contains the declaration of the class CBESockSndFunction
 *
 *  \date   23/8/2006
 *  \author Stephen Kell  <Stephen.Kell@cl.cam.ac.uk>
 *
 *  Based on SockBECallFunction.h, for which the original copyright notice
 *  follows.
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

#ifndef SOCKBESNDFUNCTION_H
#define SOCKBESNDFUNCTION_H

#include "be/BESndFunction.h"

/** \class CSockBESndFunction
 *  \ingroup backend
 *  \brief contains the platform specific code for the send function
 *
 *  This class represents the send function, which is used at the client's side
 *  to send an RPC to the server when there is no return value.. 
 *  It marshals the parameters and sends the message.
 */
class CSockBESndFunction : public CBESndFunction
{
// Constructor
public:
    /** \brief constructor
     */
    CSockBESndFunction();
    virtual ~CSockBESndFunction();

    virtual void CreateBackEnd(CFEOperation *pFEOperation);

protected:
    /** \brief copy constructor */
    CSockBESndFunction(CSockBESndFunction &src);

    virtual void WriteInvocation(CBEFile * pFile);
    virtual void WriteVariableInitialization(CBEFile * pFile);

public:
};

#endif
