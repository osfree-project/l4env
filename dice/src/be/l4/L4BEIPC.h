/**
 *    \file    dice/src/be/l4/L4BEIPC.h
 *    \brief   contains the declaration of the class CL4BEIPC
 *
 *    \date    04/18/2006
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006-2007
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
#ifndef L4BEIPC_H
#define L4BEIPC_H

#include "be/BECommunication.h"
#include "be/BEFunction.h" // DIRECTION_TYPE

/** \class CL4BEIPC
 *  \ingroup backend
 *  \brief combines all the IPC code writing
 *
 * This class contains the writing of IPC code -> it provide a central place
 * to change IPC bindings.
 */
class CL4BEIPC : public CBECommunication
{
public:
    /** create a new IPC object */
    CL4BEIPC();
    virtual ~CL4BEIPC();

    /** \brief write the reply-and-wait invocation */
    virtual void WriteReplyAndWait(CBEFile& , CBEFunction*, bool, bool) = 0;
    virtual void WriteReplyAndWait(CBEFile& , CBEFunction*);

protected:
    virtual bool IsShortIPC(CBEFunction *pFunction,
	DIRECTION_TYPE nDirection);
    virtual bool UseAssembler(CBEFunction *pFunction);
};

#endif
