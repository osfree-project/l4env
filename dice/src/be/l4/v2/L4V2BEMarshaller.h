/**
 *    \file    dice/src/be/l4/v2/L4V2BEMarshaller.h
 *    \brief    contains the declaration of the class CL4V2BEMarshaller
 *
 *    \date    05/20/2003
 *    \author    Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#ifndef L4V2BEO1MARSHALLER_H
#define L4V2BEO1MARSHALLER_H


#include <be/l4/L4BEMarshaller.h>

/** \class CL4V2BEMarshaller
 *  \brief implements L4V2 specific marshalling primitives
 */
class CL4V2BEMarshaller : public CL4BEMarshaller
{
public:
    /** \brief constructor of marshaller */
    CL4V2BEMarshaller();
    virtual ~CL4V2BEMarshaller();

public:
    virtual int Marshal(CBEFile* pFile,  CBETypedDeclarator* pParameter,  int nStartOffset,  bool& bUseConstOffset,  bool bLastParameter,  CBEContext* pContext);
};

#endif

