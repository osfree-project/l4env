/**
 *	\file	dice/src/be/l4/x0/L4X0aBEClassFactory.h
 *	\brief	contains the declaration of the class CL4X0aBEClassFactory
 *
 *	\date	12/01/2002
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

/** preprocessing symbol to check header file */
#ifndef L4X0aBECLASSFACTORY_H
#define L4X0aBECLASSFACTORY_H

#include "be/l4/L4BEClassFactory.h"

/** \class CL4X0aBEClassFactory
 *  \ingroup backend
 *  \brief contains platform specific class factory
 *
 * This is the main starting point for a new back-end, because this class
 * is used by the compiler to generate all the other classes. This class
 * is also the only one known to the compiler from a certain back-end.
 */
class CL4X0aBEClassFactory : public CL4BEClassFactory
{
DECLARE_DYNAMIC(CL4X0aBEClassFactory);
// Constructor
public:
	/**	\brief constructor
	 *	\param bVerbose true if class should print status output
	 */
	CL4X0aBEClassFactory(bool bVerbose = false);
	virtual ~CL4X0aBEClassFactory();

protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CL4X0aBEClassFactory(CL4X0aBEClassFactory &src);

public: // Public methods
    virtual CBECallFunction* GetNewCallFunction();
    virtual CBESizes * GetNewSizes();
    virtual CBEWaitFunction * GetNewWaitFunction();
    virtual CBEWaitAnyFunction * GetNewWaitAnyFunction();
    virtual CBESndFunction * GetNewSndFunction();
    virtual CBEReplyAnyWaitAnyFunction * GetNewReplyAnyWaitAnyFunction();
    virtual CBERcvAnyFunction * GetNewRcvAnyFunction();
	virtual CBECommunication* GetNewCommunication();
	virtual CBEReplyFunction* GetNewReplyFunction();
};

#endif
