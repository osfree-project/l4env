/**
 *    \file    dice/src/be/BECPPClass.h
 *    \brief   contains the declaration of the class CBECPPClass
 *
 *    \date    Tue Jul 08 2003
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#ifndef BECPPCLASS_H
#define BECPPCLASS_H


#include <be/BEClass.h>

/** \class CBECPPClass
 *  \ingroup backend
 *  \brief implements C++ specific class
 */
class CBECPPClass : public CBEClass
{
public:
    /** creates an instance of this class */
    CBECPPClass();
    virtual ~CBECPPClass();

    virtual void Write(CBEHeaderFile *pFile, CBEContext *pContext);
    virtual void Write(CBEImplementationFile *pFile, CBEContext *pContext);

protected:
    void WriteClass_var(CBEHeaderFile *pFile, CBEContext *pContext);
    void WriteClass_var(CBEImplementationFile *pFile, CBEContext *pContext);
    void WriteClass_out(CBEHeaderFile *pFile, CBEContext *pContext);
    void WriteClass_out(CBEImplementationFile *pFile, CBEContext *pContext);
};

#endif

