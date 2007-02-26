/***************************************************************************
                          main.cpp  -  description
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

#include <unistd.h>

#include <qapplication.h>
#include <qfont.h>
#include <qstring.h>
#include <qtextcodec.h>
#include <qtranslator.h>

#include "qtinyeditor.h"

int main(int argc, char *argv[])
{
#if defined(Q_WS_QWS) && !defined(Q_OS_DROPS) // Frame Buffer
   QApplication app(argc, argv, QApplication::GuiServer);
#else // X11
   QApplication app(argc, argv);
#endif // Q_WS_QWS

  //app.setFont(QFont("helvetica", 12));
  QTranslator tor( 0 );
  // set the location where your .qm files are in load() below as the last parameter instead of "."
  // for development, use "/" to use the english original as
  // .qm files are stored in the base project directory.
  tor.load( QString("qtinyeditor.") + QTextCodec::locale(), "." );
  app.installTranslator( &tor );

  QTinyEditorApp *qtinyeditor=new QTinyEditorApp();
  app.setMainWidget(qtinyeditor);

  qtinyeditor->show();

  if(argc>1)
    qtinyeditor->openDocumentFile(argv[1]);
	else
	  qtinyeditor->openDocumentFile();
	
  return app.exec();
}
