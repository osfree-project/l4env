/**
 *  \file    dice/src/FactoryFactory.h
 *  \brief   contains the declaration of the class CFactoryFactory
 *
 *  \date    09/04/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
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
#ifndef __DICE_FACTORYFACTORY_H__
#define __DICE_FACTORYFACTORY_H__

class CBEClassFactory;
class CBENameFactory;

/** \class CFactoryFactory
 *  \brief the factory to create class and name factory
 */
class CFactoryFactory
{
// Constructor
public:
	CBEClassFactory* GetNewClassFactory();
	CBENameFactory* GetNewNameFactory();
};

#endif // !__DICE_FACTORYFACTORY_H__
