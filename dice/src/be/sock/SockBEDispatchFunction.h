/**
 *    \file    dice/src/be/sock/SockBEDispatchFunction.h
 *    \brief    contains the declaration of the class CSockBEDispatchFunction
 *
 *    \date    06/01/2004
 *    \author    Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2004
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

#ifndef SOCKBEDISPATCHFUNCTION_H
#define SOCKBEDISPATCHFUNCTION_H

#include <be/BEDispatchFunction.h>

/** \class CSockBEDispatchFunction
 *    \ingroup backend
 *    \brief specializes the CBEDispatchFunction class for the socket backend
 */
class CSockBEDispatchFunction : public CBEDispatchFunction
{

public:
    /**    \brief constructor
     */
    CSockBEDispatchFunction();
    virtual ~CSockBEDispatchFunction();

protected:
    /** copy constructor
     *    \param src the source to copy from
     */
    CSockBEDispatchFunction(CSockBEDispatchFunction &src);

    virtual bool AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext);
    virtual void WriteCallAfterParameters(CBEFile* pFile,  CBEContext* pContext,  bool bComma);
};

#endif
