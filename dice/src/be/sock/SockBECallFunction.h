/**
 *  \file   dice/src/be/sock/SockBECallFunction.h
 *  \brief  contains the declaration of the class CBESockCallFunction
 *
 *  \date   11/28/2002
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#ifndef SOCKBECALLFUNCTION_H
#define SOCKBECALLFUNCTION_H

#include "be/BECallFunction.h"

/** \class CSockBECallFunction
 *  \ingroup backend
 *  \brief contains the platform specific code for the call function
 *
 *  This class represents the call function, which is used at the client's side
 *  to send an RPC to the server. It marshals the parameters, sends the message
 *  and unmarshals the response.
 */
class CSockBECallFunction : public CBECallFunction
{
// Constructor
public:
    /** \brief constructor
     */
    CSockBECallFunction();
    virtual ~CSockBECallFunction();

    virtual void CreateBackEnd(CFEOperation *pFEOperation, bool bComponentSide);

protected:
    virtual void WriteInvocation(CBEFile& pFile);
    virtual void WriteVariableInitialization(CBEFile& pFile);
};

#endif
