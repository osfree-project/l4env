/****************************************************************************************
    Copyright 2004 uja
    Copyright 2004 Trolltech (Qt3)

    This file is part of ujagames_qt which run under GPL
*****************************************************************************************/

// Headerfile fuer gamelib - konstante Teile ujagames_qt: Textanzeigen, Hiscore-Module, maussensitiver CanvasView
// ---------------------------------------------------------------------------------------------------------------

#ifndef GAMELIB_H
#define GAMELIB_H

#include "lang.h"
#include "config.h"
#include <qapplication.h> // OBJECT
#include <qvbox.h>        // Anzeige-Unterteilung
#include <qhbox.h>        // Basis dieses Windows
#include <qgrid.h>        // Unterteilungsgitter
#include <qlabel.h>       // Anzeigen #include <qcanvas.h> 
#include <qpixmap.h>      // Trophaeen
#include <qcanvas.h>

#include <qfile.h>        // Dateikram
#include <qtextstream.h>  // Dateikram via Stream
#include <qlineedit.h>    // Eingaben
#include <qtextedit.h>    // Infofenster
#include <qpushbutton.h>  // Schliessen-Knopf 
#include <qstring.h>      // String-Manipulationen

#include <qinputdialog.h>
#include <qdatetime.h>
#include <qbrush.h>
#include <qimage.h>
#include <qmessagebox.h>
#include <qtimer.h>

// --------------------------------------------------------------

class viewport: public QCanvasView
{
  Q_OBJECT
  
  public:
  viewport(QWidget* parent=0,char* name="viewport");
   
  protected:
  void contentsMousePressEvent(QMouseEvent *e);
  void contentsMouseMoveEvent(QMouseEvent *e);
  void contentsWheelEvent(QWheelEvent *e);
  
  signals:
  void geklickt(int,int);  
  void hover(int,int);  
};


class textviewer: public QVBox
{ 
  Q_OBJECT

  public: 
  textviewer(QWidget* parent=0, char* name="TextViewer");

  private:
  QTextEdit* my_text;
  
  public slots:
  void show_info(int flag=0);  
};


class hiscore;

class hscrec
{ 
  friend class hiscore;
  int punkte;
  QString zeit;
  QString name;
  void setValues(int,QString,QString);
};

class hiscore: public QVBox
{
  Q_OBJECT
  
  public:
  hiscore(QWidget* parent=0, char* name="bestenliste");

  private:
  QTextEdit* my_text;
  QString* hscdatei;
  QString* hscpfad;
  int hscpfad_id;
  
  QString get_feld(QString,bool,bool);
  void dateieinlesen(hscrec[]);
  void dateischreiben(hscrec[]);
  bool get_lock();
  void free_lock();

  public slots:
  void show_hsc();
  void save_hsc(int);    
  void reset_hsc();
  void eintragen(int,QString,QString);
  void set_hscpath(int);
  void save_hscpath();
};

#endif 
