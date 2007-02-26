/****************************************************************************
** $Id$
**
** Implementation of the QTextView class
**
** Created : 990101
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the widgets module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "qtextview.h"

#ifndef QT_NO_TEXTVIEW

/*! \class QTextView
    \brief The QTextView class provides a rich-text viewer.

  \obsolete

  This class wraps a read-only \l QTextEdit.
  Use a \l QTextEdit instead, and call setReadOnly(TRUE)
  to disable editing.
*/

/*! \reimp */

QTextView::QTextView( const QString& text, const QString& context,
		      QWidget *parent, const char *name )
    : QTextEdit( text, context, parent, name )
{
    setReadOnly( TRUE );
}

/*! \reimp */

QTextView::QTextView( QWidget *parent, const char *name )
    : QTextEdit( parent, name )
{
    setReadOnly( TRUE );
}

/*! \reimp */

QTextView::~QTextView()
{
}

/*!
    \property QTextView::undoDepth
    \brief the number of undoable steps
*/

/*!
    \property QTextView::overwriteMode
    \brief whether new text overwrites or pushes aside existing text
*/

/*!
    \property QTextView::modified
    \brief Whether the text view's contents have been modified.
*/

/*!
    \property QTextView::readOnly
    \brief Whether the text view's contents are read only.
*/

/*!
    \property QTextView::undoRedoEnabled
    \brief Whether undo and redo are enabled.
*/

#endif
