/**
 *  \file   dice/src/be/l4/v4/ia32/L4V4IA32IPC.h
 *  \brief  contains the declaration of the class CL4V4IA32IPC
 *
 *  \date   02/08/2004
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
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
#ifndef L4V4IA32IPC_H
#define L4V4IA32IPC_H

#include <be/l4/v4/L4V4BEIPC.h>

/** \class CL4V4IA32IPC
 *  \ingroup backend
 *  \brief contains assembly code for IA32 platforms
 */
class CL4V4IA32IPC : public CL4V4BEIPC
{

public:
    /** creates a new class */
    CL4V4IA32IPC();
    virtual ~CL4V4IA32IPC();

public:
    virtual void WriteCall(CBEFile* pFile,  CBEFunction* pFunction);
};

#endif
