/**
 *	\file	dice/src/be/l4/L4BETestsuite.h
 *	\brief	contains the declaration of the class CL4BETestsuite
 *
 *	\date	Tue May 28 2002
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

#ifndef L4BETESTSUITE_H
#define L4BETESTSUITE_H

#include <be/BETestsuite.h>

/** \class CL4BETestsuite
 *  \ingroup backend
 *  \brief implements the L4 specific testsuite
 */
class CL4BETestsuite : public CBETestsuite
{
DECLARE_DYNAMIC(CL4BETestsuite);
public:
    /** class constructor */
	CL4BETestsuite();
	~CL4BETestsuite();

public: // public methods
  virtual bool CreateBackEnd(CFEFile * pFEFile, CBEContext * pContext);
};

#endif
