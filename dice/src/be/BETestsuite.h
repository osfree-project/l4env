/**
 *	\file	dice/src/be/BETestsuite.h
 *	\brief	contains the declaration of the class CBETestsuite
 *
 *	\date	01/11/2002
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

/** preprocessing symbol to check header file */
#ifndef __DICE_BETESTSUITE_H__
#define __DICE_BETESTSUITE_H__

#include "be/BETarget.h"

class CBEContext;
class CFEFile;
class CFEInterface;
class CFEOperation;

/**	\class CBETestsuite
 *	\ingroup backend
 *	\brief the testsuite - a collection of files
 */
class CBETestsuite : public CBETarget  
{
DECLARE_DYNAMIC(CBETestsuite);
// Constructor
public:
	/**	\brief constructor
	 */
	CBETestsuite();
	virtual ~CBETestsuite();

protected:
	virtual bool CreateBackEndImplementation(CFEFile * pFEFile, CBEContext * pContext);
	virtual bool CreateBackEndHeader(CFEFile * pFEFile, CBEContext * pContext);
	virtual void SetFileType(CBEContext *pContext, int nHeaderOrImplementation);

	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CBETestsuite(CBETestsuite &src);


public:
	virtual void Write(CBEContext *pContext);
};

#endif // !__DICE_BETESTSUITE_H__
