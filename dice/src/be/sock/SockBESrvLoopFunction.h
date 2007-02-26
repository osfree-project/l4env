/**
 *	\file	dice/src/be/sock/SockBESrvLoopFunction.h
 *	\brief	contains the declaration of the class CSockBESrvLoopFunction
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

#ifndef SOCKBESRVLOOPFUNCTION_H
#define SOCKBESRVLOOPFUNCTION_H

#include "be/BESrvLoopFunction.h"

/** \class CSockBESrvLoopFunction
 *  \ingroup backend
 *  \brief contains platform specific code
 *
 *  Thsi class represents a server loop function. This function receives
 *  messages, extracts the opcode and decides which function has to be called.
 *  For each function there exists a switch-case in the server loop's switch.
 *  Inside the switch-case the unmarshal function is called, then the component
 *  function and eventually the reply-and-wait function.
 */
class CSockBESrvLoopFunction : public CBESrvLoopFunction
{
DECLARE_DYNAMIC(CSockBESrvLoopFunction);
// Constructor
public:
	/**	\brief constructor
	 */
	CSockBESrvLoopFunction();
	virtual ~CSockBESrvLoopFunction();

protected:
	/**	\brief copy constructor */
	CSockBESrvLoopFunction(CSockBESrvLoopFunction &src);

    virtual void WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteCleanup(CBEFile * pFile, CBEContext * pContext);

public:
};

#endif
