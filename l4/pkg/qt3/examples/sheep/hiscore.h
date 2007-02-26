/****************************************************************************************
    Copyright 2004 uja
    Copyright 2004 Trolltech (Qt3)
    This file is ujas common highscore modul ini file which runs under GPL
*****************************************************************************************/

// Headerfile fuer Objekt hiscore: hiscore.h
// erstellt ein Grid-Fenster fuer die Bestenliste
// ----------------------------------------------------------------------------------------------

#ifndef HISCORE_H
#define HISCORE_H

#include "config.h"

#include <qvbox.h>
#include <qlabel.h> 
#include <qfile.h>
#include <qstring.h>
#include <qpushbutton.h>
#include <qinputdialog.h>
#include <qpixmap.h>
#include <qtextedit.h>
#include <qdatetime.h>
#include <qbrush.h>
#include <qimage.h>
#include <qinputdialog.h>
#include <qmessagebox.h>
#include <qimage.h>
#include <qtimer.h>
// ein bisschen globales Zeug:

class hscrec
{ 
  public:
  int punkte;
  QString zeit;
  QString name;
  void setValues(int,QString,QString);
} ;


class hiscore: public QVBox
{
  Q_OBJECT
  
  public:
  hiscore(QWidget* parent=0, char* name="bestenliste");

  private:
  QTextEdit* my_text;
  QString get_feld(QString,bool,bool);
  void dateieinlesen(hscrec[]);
  void datenschreiben(hscrec[]);
  void eintragen(int,QString,QString);
  bool get_lock();
  void free_lock();

  public slots:
  void show_hsc();
  void save_hsc(int);    
  void reset_hsc();
};

#endif 
