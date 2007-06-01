/**
 *  \file    dice/src/be/l4/L4BEMarshalExceptionFunction.cpp
 *  \brief   contains the implementation of the class CL4BEMarshalExceptionFunction
 *
 *  \date    10/10/2003
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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
#include "L4BEMarshalExceptionFunction.h"
#include "be/Trace.h"
#include "be/BEClass.h"
#include "be/BEMsgBuffer.h"
#include "TypeSpec-L4Types.h"
#include <cassert>

CL4BEMarshalExceptionFunction::CL4BEMarshalExceptionFunction()
{ }

CL4BEMarshalExceptionFunction::CL4BEMarshalExceptionFunction(CL4BEMarshalExceptionFunction & src)
: CBEMarshalExceptionFunction(src)
{ }

/** \brief destructor of target class */
CL4BEMarshalExceptionFunction::~CL4BEMarshalExceptionFunction()
{ }

/** \brief write the L4 specific unmarshalling code
 *  \param pFile the file to write to
 *
 * If we send flexpages we have to do special handling:
 * if (exception)
 *   marshal exception
 * else
 *   marshal normal flexpages
 *
 * Without flexpages:
 * marshal exception
 * marshal rest
 */
void
CL4BEMarshalExceptionFunction::WriteMarshalling(CBEFile* pFile)
{
    bool bLocalTrace = false;
    if (!m_bTraceOn && m_pTrace)
    {
	m_pTrace->BeforeMarshalling(pFile, this);
	m_bTraceOn = bLocalTrace = true;
    }

    CBEMarshalExceptionFunction::WriteMarshalling(pFile);

    // set send dope
    CBEMsgBuffer *pMsgBuffer = m_pClass->GetMessageBuffer();
    CMsgStructType nType = GetSendDirection();
    pMsgBuffer->WriteInitialization(pFile, this, TYPE_MSGDOPE_SEND,
	nType);

    if (bLocalTrace)
    {
	m_pTrace->AfterMarshalling(pFile, this);
	m_bTraceOn = false;
    }
}

