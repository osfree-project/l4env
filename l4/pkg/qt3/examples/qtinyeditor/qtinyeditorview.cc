/***************************************************************************
                          qtinyeditorview.cpp  -  description
                             -------------------
    begin                : Tue Aug 19 20:33:02 Local time zone must be set--see zic manual page 2003
    copyright            : (C) 2003 by François Dupoux
    email                : fdupoux@free.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// include files for Qt
#include <qpainter.h>

// application specific includes
#include "qtinyeditorview.h"
#include "qtinyeditordoc.h"


QTinyEditorView::QTinyEditorView(QTinyEditorDoc* pDoc, QWidget *parent, const char* name, int /*wflags*/)
 : QTextEdit/*QWidget*/(parent, name) //, wflags)
{
    doc=pDoc;
    setTextFormat( Qt::PlainText );
    connect(this, SIGNAL(textChanged ()), this, SLOT(slotTextChanged ()));
}

QTinyEditorView::~QTinyEditorView()
{
}

QTinyEditorDoc *QTinyEditorView::getDocument() const
{
	return doc;
}

void QTinyEditorView::update(QTinyEditorView* pSender)
{
	if(pSender != this)
		repaint();
}

/*void QTinyEditorView::print(QPrinter *pPrinter)
{
  if (pPrinter->setup(this))
  {
		QPainter p;
		p.begin(pPrinter);
		
		///////////////////////////////
		// TODO: add your printing code here
		///////////////////////////////
		
		p.end();
  }
}*/

void QTinyEditorView::closeEvent(QCloseEvent*)
{
  // LEAVE THIS EMPTY: THE EVENT FILTER IN THE QTinyEditorApp CLASS TAKES CARE FOR CLOSING
  // QWidget closeEvent must be prevented.
}

void QTinyEditorView::slotTextChanged()
{
  doc->setModified(true);
}
