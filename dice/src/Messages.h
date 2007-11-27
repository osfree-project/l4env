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

namespace CMessages
{

	/**
	 *  \brief helper function
	 *  \param sMsg the error message to print before exiting
	 *
	 * This method prints an error message and exits the compiler. Any clean up
	 * should be performed  BEFORE this method is called.
	 */
	void Error(const char *sMsg, ...) __attribute__(( format(printf, 1, 2) ));
	void Warning(const char *sMsg, ...) __attribute__(( format(printf, 1, 2) ));
	/** \brief helper function
	 *  \param pFEObject the front-end object where the error occurred
	 *  \param sMsg the error message
	 *
	 * The given front-end object can be used to determine the file this object
	 * belonged to and the line, where this object has been declared. This is
	 * useful if we do not have a current line number available (any time after
	 * the parsing finished).
	 */
	void GccError(CFEBase * pFEObject, const char *sMsg, ...) __attribute__(( format(printf, 2, 3) ));
	void GccErrorVL(CFEBase * pFEObject,	const char *sMsg, va_list vl)
		__attribute__(( format(printf, 2, 0) ));
	void GccWarning(CFEBase * pFEObject, const char *sMsg, ...) __attribute__(( format(printf, 2, 3) ));
	void GccWarningVL(CFEBase * pFEObject, const char *sMsg, va_list vl)
		__attribute__(( format(printf, 2, 0) ));

};

#endif /* __DICE_MESSAGES_H__ */
