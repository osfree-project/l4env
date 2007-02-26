/**
 *    \file    dice/src/be/l4/v4/L4V4BENameFactory.h
 *    \brief    contains the declaration of the class CL4V4BENameFactory
 *
 *    \date    01/08/2004
 *    \author    Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#ifndef L4V4BENAMEFACTORY_H
#define L4V4BENAMEFACTORY_H

#include <be/l4/L4BENameFactory.h>

//@{
#define STR_MSGTAG_VARIABLE     (STR_L4BENF_MAX + 1)    /**< variable name of the MsgTag return variable */
//@}

/** \class CL4V4BENameFactory
 *  \ingroup backend
 *  \brief contains functions to create V4 specific names
 */
class CL4V4BENameFactory : public CL4BENameFactory
{

public:
    /** \brief creates the instance of the name factory
     *    \param bVerbose true if class should print status output
     */
    CL4V4BENameFactory(bool bVerbose = false);
    virtual ~CL4V4BENameFactory();

protected:
    /**    \brief copy constructor
     *    \param src the source to copy from
     */
    CL4V4BENameFactory(CL4V4BENameFactory &src);

public:
    virtual string GetString(int nStringCode, CBEContext *pContext, void *pParam);
    virtual string GetMsgTagVarName(CBEContext *pContext);

protected:
    virtual string GetL4TypeName(int nType, bool bUnsigned, CBEContext *pContext, int nSize);
    virtual string GetTypeName(int nType, bool bUnsigned, CBEContext * pContext, int nSize);

};

#endif
