/****************************************************************************************
    Copyright 2004 uja
    Copyright 2004 Trolltech (Qt3)

    This file is part of Fleur which runs under GPL
*****************************************************************************************/

// Headerfile fuer Objekt window: window.h
// erstellt ein zweigeteiltes Fenster, links Grafikausgabe, rechts Eingabeparameter und Anzeigen
// Klamotten wie Help, Neu  Load und Save werden von QApplication in main.cpp erledigt
// ----------------------------------------------------------------------------------------------

#ifndef WINDOW_H
#define WINDOW_H
#include "llang.h"
#include "config.h"
#include "gamelib.h"
#include "levels.h"
#include <qvbox.h>        // Anzeige-Unterteilung
#include <qhbox.h>        // Basis dieses Windows
#include <qgrid.h>        // Unterteilungsgitter
#include <qlabel.h>       // Anzeigen 
#include <qcanvas.h>      // Spielwiese
#include <qradiobutton.h>
#include <qlineedit.h>
#include <qvalidator.h>
#include <qbuttongroup.h>
#include <qlistbox.h>
#include <qfile.h>

class window:public QHBox
{
  Q_OBJECT

  private:
  int gewaehlt,moskitostart,schafstart,npfeile,nzeit,aktlevel;
  int f[SMAX];
  QString code,spfeile,szeit,nlname;
  
  QString mach_record();
  void zeige_feld(int);
  void set_wespe(int);
  void set_bgr(int);
  void wespe_weg();
  void set_schafstart(int);
  bool backup();
  
  private slots:
  void klick(int xx,int yy);
  void hovern(int xx,int yy);
  void waehle(int);
  void updat_schaf(int);
 
  public:
  window::window(QWidget* parent=0, char* name=0);
  viewport* my_view;
  level* my_levels;
  QListBox* my_lliste;
  QListBox* my_sliste;
  QListBox* my_cliste;
  QListBox* my_iliste;
  
  QLabel* p_messi;
  QLineEdit* p_text[3];
  QButtonGroup* schafri;
  QButtonGroup* moskiri;
  QRadioButton* sri[4];
  QRadioButton* mri[3];
  
  QCanvasSprite* p_bgsprite[SMAX];
  
  QCanvasSprite* p_cursor;
  QCanvasSprite* p_wespe;  
  QCanvasSprite* p_schaf;
  QCanvasPixmapArray* my_bgarray;        // Gelaende
  QCanvasPixmapArray* my_schafarray;     // Schafstart
  
  
  public slots:
  void leeren();
  void fill_oben();
  void fill_unten();
  void fill_rechts();
  void fill_links();
  void nach_oben();
  void nach_unten();
  void nach_rechts();
  void nach_links();
  
  void level_laden(int);
  void level_speichern(int);
  void level_einschieben_vor(int);
  void level_anhaengen();
  void level_loeschen(int);
  void set_messi(QString);

  void updat_liste();
  
  signals:
  void loesche_levellisten();
  
};

#endif 
