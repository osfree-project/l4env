/****************************************************************************************
    Copyright 2004 uja
    Copyright 2004 Trolltech (Qt3)
    This file is ujas common textviewer header file which runs under GPL
*****************************************************************************************/

// Headerfile fuer Objekt textviewer: textviewer.h
// erstellt ein zweigeteiltes Fenster, links Grafikausgabe, rechts Eingabeparameter und Anzeigen
// Klamotten wie Help, Neu  Load und Save werden von QApplication in main.cpp erledigt
// ----------------------------------------------------------------------------------------------

#ifndef TEXTVIEWER_H
#define TEXTVIEWER_H

#include "config.h"

#include <qfile.h>
#include <qtextedit.h>    // Infofenster
#include <qvbox.h>
#include <qgrid.h>
#include <qpushbutton.h>  // Schliessen-Knopf 
#include <qlabel.h>
#include <qstring.h>
#include <qpixmap.h>

class textviewer: public QVBox
{ 
  Q_OBJECT

  public: 
  textviewer(QWidget* parent=0, char* name="TextViewer");
  QTextEdit* my_text;
  
  public slots:
  void show_info(int flag=0);
  
  private slots:
  void hide_info();
};

#endif 
