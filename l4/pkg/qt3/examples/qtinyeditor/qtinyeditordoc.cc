/***************************************************************************
                          qtinyeditordoc.cpp  -  description
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
#include <qdir.h>
#include <qfileinfo.h>
#include <qwidget.h>
#include <qmsgbox.h>
#include <qfiledialog.h>


// application specific includes
#include "qtinyeditordoc.h"
#include "qtinyeditor.h"
#include "qtinyeditorview.h"


QTinyEditorDoc::QTinyEditorDoc()
{
  pViewList = new QList<QTinyEditorView>;
  pViewList->setAutoDelete(false);
}

void QTinyEditorDoc::setModified(bool _m/*=true*/)
{
  QTinyEditorView *w;
	QString strTitle;
	QFileInfo f(m_filename);

  modified=_m;

	strTitle = f.fileName();
	if (modified)
		strTitle += " [*]";

	setTitle(strTitle);
  
  w=pViewList->first();
  if (w)
    w->setCaption(title());
}

QTinyEditorDoc::~QTinyEditorDoc()
{
  delete pViewList;
}

void QTinyEditorDoc::addView(QTinyEditorView *view)
{
  pViewList->append(view);
	changedViewList();
}

void QTinyEditorDoc::removeView(QTinyEditorView *view)
{
	  pViewList->remove(view);
	  if(!pViewList->isEmpty())
			changedViewList();
		else
			deleteContents();
}

void QTinyEditorDoc::changedViewList(){	
	
	QTinyEditorView *w;
	if((int)pViewList->count() == 1){
  	w=pViewList->first();
  	w->setCaption(m_title);
	}
	else{
		int i;
    for( i=1,w=pViewList->first(); w!=0; i++, w=pViewList->next())
  		w->setCaption(QString(m_title+":%1").arg(i));	
	}
}

bool QTinyEditorDoc::isLastView() {
  return ((int) pViewList->count() == 1);
}


void QTinyEditorDoc::updateAllViews(QTinyEditorView *sender)
{
  QTinyEditorView *w;
  for(w=pViewList->first(); w!=0; w=pViewList->next())
  {
     w->update(sender);
  }
}

void QTinyEditorDoc::setPathName(const QString &name)
{
  m_filename=name;
	m_title=QFileInfo(name).fileName();
}

const QString& QTinyEditorDoc::pathName() const
{
  return m_filename;
}

void QTinyEditorDoc::setTitle(const QString &title)
{
  m_title=title;
}

const QString &QTinyEditorDoc::title() const
{
  return m_title;
}


void QTinyEditorDoc::closeDocument()
{
  QTinyEditorView *w;
  if(!isLastView())
  {
    for(w=pViewList->first(); w!=0; w=pViewList->next())
    {
   	 	if(!w->close())
 				break;
    }
	}
  if(isLastView())
  {
  	w=pViewList->first();
  	w->close();
  }
}

bool QTinyEditorDoc::newDocument()
{
  /////////////////////////////////////////////////
  // TODO: Add your document initialization code here
  /////////////////////////////////////////////////
  modified=false;
  return true;
}

bool QTinyEditorDoc::openDocument(const QString &filename, const char */*format*/ /*=0*/)
{
    QFile fileObj(filename);
    if ( !fileObj.open( IO_ReadOnly ) )
      return false;

    QTextStream ts( &fileObj );
    m_strText = ts.read();
    //edit->setText(  );
    //doc->firstView()->setText(ts.read());
    /*tabWidget->showPage( edit );
    edit->viewport()->setFocus();
    filenames.replace( edit, f );*/

   fileObj.close();
	
  modified=false;
  m_filename=filename;
	setModified(false);
	updateAllViews(NULL);
  return true;
}

bool QTinyEditorDoc::saveDocument(const QString &filename, const char */*format*/ /*=0*/)
{
	  // get the text from the view
    m_strText = firstView()->text();

    QFile fileObj(filename);
    if ( !fileObj.open( IO_WriteOnly ) )
      return false;

    QTextStream ts( &fileObj );
    ts << getText();

    fileObj.close();

  setModified(false);
  m_filename=filename;
	m_title=QFileInfo(fileObj).fileName();
  return true;
}

void QTinyEditorDoc::deleteContents()
{
  /////////////////////////////////////////////////
  // TODO: Add implementation to delete the document contents
  /////////////////////////////////////////////////

}

bool QTinyEditorDoc::canCloseFrame(QTinyEditorView* pFrame)
{
	int nRes;

	if(!isLastView())
		return true;
		
	bool ret=false;
  if(isModified())
  {
		QString saveName;
		nRes = QMessageBox::information(pFrame, title(), tr("The current file has been modified.\n"
           "Do you want to save it?"), QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);

    switch(nRes)
    {
			case QMessageBox::Yes:
				if(title().contains(tr("Untitled")))
				{
					saveName=QFileDialog::getSaveFileName(0, 0, pFrame);
          if(saveName.isEmpty())
             return false;
				}
				else
					saveName=pathName();

				if(!saveDocument(saveName))
				{
 					switch(QMessageBox::critical(pFrame, tr("I/O Error !"), tr("Could not save the current document !\n"
								"Close anyway ?"),QMessageBox::Yes ,QMessageBox::No))

 					{
 						case QMessageBox::Yes:
 							ret=true;
 						case QMessageBox::No:
 							ret=false;
 					}
				}
				else
					ret=true;
				break;
			case QMessageBox::No:
				ret=true;
				break;
			case QMessageBox::Cancel:
			default:
				ret=false;
				break;
		}
	}
	else
		ret=true;

	return ret;
}
