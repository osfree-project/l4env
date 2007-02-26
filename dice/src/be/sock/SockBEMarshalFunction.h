/**
 *	\file	dice/src/be/sock/SockBEMarshalFunction.h
 *	\brief	contains the declaration of the class CSockBEMarshalFunction
 *
 *	\date	10/14/2003
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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

#ifndef SOCKBEMARSHALFUNCTION_H
#define SOCKBEMARSHALFUNCTION_H


#include <be/BEMarshalFunction.h>


/** \class CSockBEMarshalFunction
 *  \ingroup backend
 *  \brief contains target platfrom specific code
 *
 *  This class represents a marshalling function, which is used at
 *  the sender's side to marshal the parameters into the message buffer.
 */
class CSockBEMarshalFunction : public CBEMarshalFunction
{
DECLARE_DYNAMIC(CSockBEMarshalFunction);
// Constructor
public:
	/**	\brief constructor
	 */
	CSockBEMarshalFunction();
	virtual ~CSockBEMarshalFunction();

protected:
	/**	\brief copy constructor */
	CSockBEMarshalFunction(CSockBEMarshalFunction &src);

    virtual bool AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext);
	virtual void WriteCallAfterParameters(CBEFile* pFile,  CBEContext* pContext,  bool bComma);
};

#endif
