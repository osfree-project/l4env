/**
 *    \file    dice/src/fe/FEEndPointAttribute.h
 *  \brief   contains the declaration of the class CFEEndPointAttribute
 *
 *    \date    01/31/2001
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

/** preprocessing symbol to check header file */
#ifndef __DICE_FE_FEENDPOINTATTRIBUTE_H__
#define __DICE_FE_FEENDPOINTATTRIBUTE_H__

#include "fe/FEAttribute.h"
#include <string>
#include <vector>
using std::vector;

/** \struct PortSpec
 *  \ingroup frontend
 *  \brief a helper class for the end point attribute
 */
struct PortSpec
{
    /** \var std::string sFamily
     *  \brief the family of the port (udp, tcp, ...)
     */
    std::string sFamily;
    /** \var std::string sPort
     *  \brief the port of endpoint
     */
    std::string sPort;
};

/** \class CFEEndPointAttribute
 *    \ingroup frontend
 *  \brief represents the end-point attribute
 *
 * This class represents the attribute end point.
 */
class CFEEndPointAttribute : public CFEAttribute
{

// standard constructor/destructor
public:
    /** \brief constructs an end-point attribute
     *  \param pPortSpecs the ports ant the end-point of the communication
     */
    CFEEndPointAttribute(vector<PortSpec> *pPortSpecs);
    virtual ~CFEEndPointAttribute();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFEEndPointAttribute(CFEEndPointAttribute &src);

// Operations
public:
    virtual CObject* Clone();

// attributes
public:
    /** \var vector<PortSpec> m_PortSpecs
     *  \brief contains all port specifications
     */
    vector<PortSpec> m_PortSpecs;
};

#endif /* __DICE_FE_FEENDPOINTATTRIBUTE_H__ */

