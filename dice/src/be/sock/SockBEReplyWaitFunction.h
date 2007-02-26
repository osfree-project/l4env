/**
 *	\file	dice/src/be/sock/SockBEReplyWaitFunction.h
 *	\brief	contains the declaration of the class CSockBEReplyWaitFunction
 *
 *	\date	11/28/2002
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

#ifndef SOCKREPLYWAITFUNCTION_H
#define SOCKREPLYWAITFUNCTION_H

#include "be/BEReplyWaitFunction.h"

/** \class CSockBEReplyWaitFunction
 *  \ingroup backend
 *  \brief contains the platform specific code for the reply-and wait function
 *
 *  This class represents the reply-and-wait function. The reply-and-wait function
 *  responds to a specific request with a message and waits for any message from
 *  any sender. It is usually used by the server-loop to send the response to the
 *  client and wait for the next request.
 */
class CSockBEReplyWaitFunction : public CBEReplyWaitFunction
{
DECLARE_DYNAMIC(CSockBEReplyWaitFunction);
// Constructor
public:
	/**	\brief constructor
	 */
	CSockBEReplyWaitFunction();
	virtual ~CSockBEReplyWaitFunction();

protected:
	/**	\brief copy constructor */
	CSockBEReplyWaitFunction(CSockBEReplyWaitFunction &src);
    
    virtual void WriteInvocation(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext);
    virtual bool AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext);
};

#endif
