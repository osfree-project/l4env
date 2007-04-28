/**
 *  \file    dice/src/Messages.h
 *  \brief   contains the declaration of the class CMessages
 *
 *  \date    04/10/2007
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
#ifndef __DICE_MESSAGES_H__
#define __DICE_MESSAGES_H__

#include <stdarg.h>

class CFEBase;

class CMessages
{
public:
    static void Error(const char *sMsg, ...) 
	__attribute__(( format(printf, 1, 2) ));
    static void Warning(const char *sMsg, ...)
	__attribute__(( format(printf, 1, 2) ));
    static void GccError(CFEBase * pFEObject, int nLinenb,
	const char *sMsg, ...) __attribute__(( format(printf, 3, 4) ));
    static void GccErrorVL(CFEBase * pFEObject, int nLinenb,
	const char *sMsg, va_list vl);
    static void GccWarning(CFEBase * pFEObject, int nLinenb,
	const char *sMsg, ...) __attribute__(( format(printf, 3, 4) ));
    static void GccWarningVL(CFEBase * pFEObject, int nLinenb,
	const char *sMsg, va_list vl);
};

#endif /* __DICE_MESSAGES_H__ */
