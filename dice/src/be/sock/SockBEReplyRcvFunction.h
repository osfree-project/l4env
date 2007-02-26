/**
 *	\file	dice/src/be/sock/SockBEReplyRcvFunction.h
 *	\brief	contains the declaration of the class CSockBEReplyRcvFunction
 *
 *	\date	11/29/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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

#ifndef SOCKBEREPLYRCVFUNCTION_H
#define SOCKBEREPLYRCVFUNCTION_H

#include "be/BEReplyRcvFunction.h"

/** \class CSockBEReplyRcvFunction
 *  \ingroup backend
 *  \brief contains the platform specific code for the reply-and-receive function
 *
 *  This class represents the reply-and-receive function, which sends a specific reply
 *  to a specific client and wait for the next message from the same client. This function
 *  can be used to send the reply to a request from a client and wait for further request
 *  which follow the former request. A protocol, which specifies a order of messages can
 *  be implemented using this function.
 */
class CSockBEReplyRcvFunction : public CBEReplyRcvFunction
{
DECLARE_DYNAMIC(CSockBEReplyRcvFunction);
// Constructor
public:
	/**	\brief constructor
	 */
	CSockBEReplyRcvFunction();
	virtual ~CSockBEReplyRcvFunction();

protected:
	/**	\brief copy constructor */
	CSockBEReplyRcvFunction(CSockBEReplyRcvFunction &src);

    virtual void WriteInvocation(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext);
    virtual bool AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext);
};

#endif
