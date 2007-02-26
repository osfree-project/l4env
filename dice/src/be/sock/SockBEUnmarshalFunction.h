/**
 *	\file	dice/src/be/sock/SockBEUnmarshalFunction.h
 *	\brief	contains the declaration of the class CSockBEUnmarshalFunction
 *
 *	\date	11/30/2002
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

#ifndef SOCKBEUNMARSHALFUNCTION_H
#define SOCKBEUNMARSHALFUNCTION_H

#include "be/BEUnmarshalFunction.h"

/** \class CSockBEUnmarshalFunction
 *  \ingroup backend
 *  \brief contains target platfrom specific code
 *
 *  This class represents a unmarshalling function, which is used at
 *  the receiver's side to extract the parameters from the message buffer.
 */
class CSockBEUnmarshalFunction : public CBEUnmarshalFunction
{
DECLARE_DYNAMIC(CSockBEUnmarshalFunction);
// Constructor
public:
	/**	\brief constructor
	 */
	CSockBEUnmarshalFunction();
	virtual ~CSockBEUnmarshalFunction();

protected:
	/**	\brief copy constructor */
	CSockBEUnmarshalFunction(CSockBEUnmarshalFunction &src);

    virtual bool AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext);
};

#endif
