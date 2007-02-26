/**
 *    \file    dice/src/be/cdr/CCDRUnmarshalFunction.cpp
 *  \brief   contains the implementation of the class CCDRUnmarshalFunction
 *
 *    \date    10/29/2003
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "be/cdr/CCDRUnmarshalFunction.h"
#include "be/BEContext.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BETrace.h"
#include "Attribute-Type.h"
#include <cassert>

CCDRUnmarshalFunction::CCDRUnmarshalFunction()
 : CBEUnmarshalFunction()
{
}

/** destroys object */
CCDRUnmarshalFunction::~CCDRUnmarshalFunction()
{
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \return true if this function should be written to the given file
 */
bool CCDRUnmarshalFunction::DoWriteFunction(CBEHeaderFile* pFile)
{
    if (!IsTargetFile(pFile))
        return false;
    if (pFile->IsOfFileType(FILETYPE_CLIENT) &&
        (!m_Attributes.Find(ATTR_IN)) &&
        !IsComponentSide())
        return true;
    if (pFile->IsOfFileType(FILETYPE_COMPONENT) &&
        (!m_Attributes.Find(ATTR_OUT)) &&
        IsComponentSide())
        return true;
    return false;
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \return true if this function should be written to the given file
 */
bool CCDRUnmarshalFunction::DoWriteFunction(CBEImplementationFile* pFile)
{
    if (!IsTargetFile(pFile))
        return false;
    if (pFile->IsOfFileType(FILETYPE_CLIENT) &&
        (!m_Attributes.Find(ATTR_IN)) &&
        !IsComponentSide())
        return true;
    if (pFile->IsOfFileType(FILETYPE_COMPONENT) &&
        (!m_Attributes.Find(ATTR_OUT)) &&
        IsComponentSide())
        return true;
    return false;
}

/** \brief writes the unmarshalling instructions
 *  \param pFile the file to write to
 *
 * No opcode unmarshalling.
 */
void CCDRUnmarshalFunction::WriteUnmarshalling(CBEFile* pFile)
{
    assert(m_pTrace);
    bool bLocalTrace = false;
    if (!m_bTraceOn)
    {
	m_pTrace->BeforeUnmarshalling(pFile, this);
	m_bTraceOn = bLocalTrace = true;
    }
    
    if (!IsComponentSide())
    {
        // unmarshal exception
        WriteMarshalException(pFile, false);
        // first unmarshl return value
        WriteMarshalReturn(pFile, false);
    }
    // now unmarshal rest
    CBEOperationFunction::WriteUnmarshalling(pFile);

    if (bLocalTrace)
    {
	m_pTrace->AfterUnmarshalling(pFile, this);
	m_bTraceOn = false;
    }
}
