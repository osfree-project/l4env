/**
 *	\file	dice/src/be/l4/v4/ia32/L4V4IA32CallFunction.h
 *	\brief	contains the declaration of the class CL4V4IA32CallFunction
 *
 *	\date	02/09/2004
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
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

/** preprocessing symbol to check header file */
#ifndef L4V4IA32CALLFUNCTION_H
#define L4V4IA32CALLFUNCTION_H

#include <be/l4/v4/L4V4BECallFunction.h>

/** \class CL4V4IA32CallFunction
 *  \ingroup backend
 *  \brief includes some IA32 specific code for the V4 backend
 */
class CL4V4IA32CallFunction : public CL4V4BECallFunction
{
public:
    /** creates an object of this class */
    CL4V4IA32CallFunction();
    virtual ~CL4V4IA32CallFunction();

protected:
    virtual void WriteMsgTagDeclaration(CBEFile* pFile,  CBEContext* pContext);
};

#endif
