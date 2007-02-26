/****************************************************************************************
    Copyright 2004 uja
    Copyright 2004 Trolltech (Qt3)

    This file is part of Lämmerlinge/LittleSheep.
    
    Lämmerlinge is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later vezu Fuss an die QT-Libs III: schafe rsion.
	    
    Lämmerlinge is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
	    
    You should have received a copy of the GNU General Public License
    along with Edelsteine; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*****************************************************************************************/

// Hauptprogramm Lämmerlinge
// Module erzeugen, Parameter lesen, Menu erstellen, Canvas und Sprites generieren, mit Appli verbinden
// --------------------------------------------------------------------------------------------------------

#include <qapplication.h>      // Fensterhandling-Event-Routinen
#include <qmenubar.h>          // Menuleiste
#include <qmenudata.h>         // Menu-Items
#include <qaccel.h>            // Shortcuts
#include <qradiobutton.h>      // Auswahl
#include <qbuttongroup.h>      // Auswahl
#include <qfile.h>             // INI-Dateien auswerten
#include <stdio.h>             // debugging
#include <unistd.h>

#include "lang.h"     // laenderspezifische Texte
#include "config.h"   // Paraneter Darstellung, Spielgroesse, Dateien
#include "gamelib.h"  // ujas gamelib: viewport, hiscore, info-textmodule
#include "window.h"   // Hauptfenster Darstellung
#include "wiese.h"    // Gameparameter und -routinen 

// ===============================================================================

// -------------------------------------------------------------------------------------------------------

QCanvasPixmapArray *newQCanvasPixmapArray(QString namePattern, int num = 0)
{
    QValueList<QPixmap> list;
    if (num)
    {
	for (int i = 0; i < num; i++)
        {
	    QString numStr = QString().setNum(i);
	    while (numStr.length() < 4)
		numStr.prepend("0");
	    QString name = namePattern;
	    name.replace("%1", numStr);
	    //qDebug("loading '%s'", name.latin1());
	    list.append(QPixmap::fromMimeSource(name));
	}
    } else {
	list.append(QPixmap::fromMimeSource(namePattern));
    }
    return new QCanvasPixmapArray(list);
}

int main(int argc, char **argv)
{ 
  QApplication my_app(argc,argv);                  // die Fenster/Appli-Steuerung  
  window*      my_window    =new window();         // eine Instanz des zu zeigenden Spiel-Hauptfensters
  hiscore*     my_hiscore   =new hiscore();        // Bestenlisten-Hauptfenster
  textviewer*  my_textviewer=new textviewer();     // Fenster fuer Hilfstexte
  wiese*       my_wiese     =new wiese(my_window); // spezifisch fuer Spiel: Wiese und Bewohner
  viewport*    my_view      =my_window->my_view;   // Darstellungsbereich des Spiels  
  
  int i,j,k;
  
// Speichernamen und Nr.Bestenlisteneintragspfad:
  int hilist=0;
  QString buff;
  QString speicher[MAXSPEICHER];
  QFile fh(QString("/sheep/speicher.ini"));
  if (fh.open(IO_ReadOnly)) { fh.readLine(buff,254); fh.close(); } else qDebug("I/O error"); 
  for (i=0; i<MAXSPEICHER; i++) { speicher[i]=buff.section(';',i,i); }
  QFile fh1(QString("/sheep/hiscore.ini"));
  if (fh1.open(IO_ReadOnly)) { fh1.readLine(buff,254); fh1.close(); } else qDebug("I/O error");
  hilist=buff.toInt();
  if (hilist>1) hilist=1; else if (hilist<0) hilist=0;  
  
    
// --- allgemeines Spielemenu: -----------------------------------------------------------------------------------------      
  QMenuBar*   my_menubar=new QMenuBar(my_window);  // Menubar mit Window verbinden    

// Menu game: neu, restart level, pause/weiter, Bestenliste/Reset, Quit  
  QPopupMenu* my_menu1=new QPopupMenu;
  my_menubar->insertItem(_GAME,my_menu1);     
  my_menu1->insertItem(_NEU,    my_wiese,SLOT(neues_spiel()),QKeySequence("CTRL+N"));   // _NEW kills Qt-library!      
  my_menu1->insertItem(_RESTART,my_wiese,SLOT(restart()),    QKeySequence("CTRL+L"));         
  my_menu1->insertItem(_PAUSE,  my_wiese,SLOT(pause()),      QKeySequence("CTRL+P"));
  my_menu1->insertItem(_KILL,   my_wiese,SLOT(aufgeben()),   QKeySequence("CTRL+K"));
  my_menu1->insertSeparator();
  my_menu1->insertItem(_QUIT,&my_app,SLOT(quit()),QKeySequence("CTRL+Q"));  

// Levelfile und Geschwindigkeit einstellen:
  QPopupMenu* my_menu1a=new QPopupMenu;
  my_menubar->insertItem(_OPTIONS,my_menu1a);     
  QButtonGroup* my_wig11=new QButtonGroup(1,Qt::Horizontal,_LEVELFILE,0);
  QRadioButton* wig110=new QRadioButton(_DEFAULT,my_wig11);
  QRadioButton* wig111=new QRadioButton(_USERLEVEL,my_wig11);
  bool cpppatsch;  // irgendein Quatsch, damit der Compiler die Schnauze haelt
  cpppatsch=((wig110) && (wig111));      
  my_wig11->setExclusive(true);
  my_wig11->setButton(0);
  my_menu1a->insertItem(my_wig11,11); 

  QButtonGroup* my_wig12=new QButtonGroup(1,Qt::Horizontal,_SPEED,0);
  QRadioButton* wig120=new QRadioButton(_SLOW,my_wig12);
  QRadioButton* wig121=new QRadioButton(_MEDIUM,my_wig12);
  QRadioButton* wig122=new QRadioButton(_FAST,my_wig12);
  cpppatsch=((wig120) && (wig121) && (wig122));      
  my_wig12->setExclusive(true);
  my_wig12->setButton(1);
  my_menu1a->insertItem(my_wig12,12); 

      
// Menu Hiscore: show, reset, use local, use net, save settings 
  QPopupMenu* my_menu2=new QPopupMenu;
  my_menubar->insertItem(_HISCORE,my_menu2);
  my_menu2->insertItem(_SHOWHSC, my_hiscore,SLOT(show_hsc()),QKeySequence("CTRL+B"));
  my_menu2->insertItem(_HSCRESET,my_hiscore,SLOT(reset_hsc())); 
  my_menu2->insertSeparator();
  QButtonGroup* my_wig23=new QButtonGroup(1,Qt::Horizontal,"",0);
  QRadioButton* wig230=new QRadioButton(_LOCALHI,my_wig23);
  QRadioButton* wig231=new QRadioButton(_NETHI,my_wig23);

  cpppatsch=((wig230) && (wig231));      
  my_wig23->setExclusive(true);
  my_wig23->setButton(hilist);
  my_menu2->insertItem(my_wig23,23); 
  my_app.connect(my_wig23,SIGNAL(clicked(int)),my_hiscore,SLOT(set_hscpath(int)));
  my_menu2->insertSeparator();
  // DROPS: crashes:
  //my_menu2->insertItem(_SAVEPARM,my_hiscore,SLOT(save_hscpath())); 
 
// laden und speichern vom Spielstandspeicher:
  QPopupMenu* my_menu3=new QPopupMenu;
  my_menubar->insertItem(_LOAD,my_menu3);
  for (i=0; i<MAXSPEICHER; i++) 
  { k=i+30;
    my_menu3->insertItem(speicher[i],my_wiese,SLOT(lade_spielstand(int)),0,k);
    my_menubar->setItemParameter(k,i);
  }
  
  QPopupMenu* my_menu4=new QPopupMenu;
  my_menubar->insertItem(_SAVE,my_menu4);
  for (i=0; i<MAXSPEICHER; i++) 
  { k=i+40;
    my_menu4->insertItem(speicher[i],my_wiese,SLOT(speichere_spielstand(int)),0,k);
    my_menubar->setItemParameter(k,i);
  }

// Info-Menuitem:I
  QPopupMenu* my_menu5=new QPopupMenu;
  my_menubar->insertItem(_INFO,my_menu5);
  my_menu5->insertItem(_HELP,my_textviewer,SLOT(show_info(int)),QKeySequence("CTRL+H"),51);
  my_menu5->insertItem(_ABOUT,my_textviewer,SLOT(show_info(int)),QKeySequence("CTRL+A"),52); 
  my_menubar->setItemParameter(51,0);
  my_menubar->setItemParameter(52,1);
 
// --------------------------------------------------------------------------------------------------------------------
// Canvas fuer Sprites e#define INIFILE   "./gameparm.ini"rzeugen, mit Viewport verbinden,
// Wiesen-Sprites belegen und mit Canvas verbinden:

  QCanvas* my_canvas=new QCanvas(DX*(XMAX+2),DY*(YMAX+2));
  my_view->setCanvas(my_canvas);    
  my_canvas->setBackgroundColor(BACKGR1);
  my_canvas->setBackgroundPixmap(QPixmap::fromMimeSource(BACKPIX1));
  my_canvas->setDoubleBuffering(true);
  my_canvas->setAdvancePeriod(10);

  my_wiese->my_bgarray    =newQCanvasPixmapArray(BGSPRITE,NUMBG);
  my_wiese->my_pfeilarray =newQCanvasPixmapArray(PFEILSPRITE,NUMPFEILE);
  my_wiese->my_schafarray =newQCanvasPixmapArray(SCHAFSPRITE,NUMSCHAFE);
  my_wiese->my_wespenarray=newQCanvasPixmapArray(MOSKITOSPRITE,NUMWESPEN);
  my_wiese->my_transparray=newQCanvasPixmapArray(TRANSSPRITE);
  my_wiese->my_goverarray =newQCanvasPixmapArray(GAMEOVERFILE);
  my_wiese->my_pausearray =newQCanvasPixmapArray(PAUSEFILE);

  my_wiese->my_cyclamarray=newQCanvasPixmapArray(CYCLAMSPRITE,NUMCYCLAM);

  
  // Check auf Vollstaendigkeit der Bilder:
  QString err="";
  if (!my_wiese->my_bgarray->isValid())     err=err+"Some background images are missing!\n";
  if (!my_wiese->my_pfeilarray->isValid())  err=err+"Some arrow images are missing!\n";
  if (!my_wiese->my_schafarray->isValid())  err=err+"Some sheep images are missing!\n";
  if (!my_wiese->my_wespenarray->isValid()) err=err+"Some moskito images are missing!\n";
  if (!my_wiese->my_transparray->isValid()) err=err+"Part of the transporter is missing!\n";
  if (!my_wiese->my_goverarray->isValid())  err=err+"Gameover screen is missing!\n";
  if (!my_wiese->my_pausearray->isValid())  err=err+"Pause screen is missing!\n";

  
  if (err=="")  // Spritebelegung und Initialisierung:
  {  
    int x,y;
    for (i=0; i<NUMPFEILE; i++) my_wiese->my_bgarray->setImage(i+NUMBG,my_wiese->my_pfeilarray->image(i));
    my_wiese->p_gover=new QCanvasSprite(my_wiese->my_goverarray,my_canvas);
    my_wiese->p_pause=new QCanvasSprite(my_wiese->my_pausearray,my_canvas);
    my_wiese->p_pause->setZ(SMAX+1); my_wiese->p_pause->move(DX+OFX,DY+OFY); // pause zweitoberste Prioritaet
    my_wiese->p_gover->setZ(SMAX+2); my_wiese->p_gover->move(5*DX,2*DY);     // gameover oberste Prioritaet
    // Background-Sprites: Prioritaet 1, Ausnahme: Pfeile, dies wird aber dynamisch erledigt
    k=0;
    for (j=0; j<YMAX; j++) for (i=0; i<XMAX; i++)
    { x=i*DX+OFX; y=j*DY+OFY; 
      my_wiese->p_bgsprite[k]=new QCanvasSprite(my_wiese->my_bgarray,my_canvas);
      my_wiese->p_bgsprite[k]->move(x,y,k%(NUMBG));
      my_wiese->p_bgsprite[k]->setZ(1);
      my_wiese->p_bgsprite[k]->setVisible(true);
      my_wiese->p_transsprite[k]=new QCanvasSprite(my_wiese->my_transparray,my_canvas);
      my_wiese->p_transsprite[k]->setZ(62);
      k++;
    }
    // Schafe, Moskito und Transporterdeckel:
    for (i=0; i<MAXSCHAFE; i++)
    {
      my_wiese->my_schaf[i]->p_sprite=new QCanvasSprite(my_wiese->my_schafarray,my_canvas);
      my_wiese->my_schaf[i]->p_sprite->setZ(20-i);
    }
    my_wiese->my_moskito->p_sprite=new QCanvasSprite(my_wiese->my_wespenarray,my_canvas);
    my_wiese->my_moskito->p_sprite->setZ(63);
    my_wiese->my_cyclam->p_sprite=new QCanvasSprite(my_wiese->my_cyclamarray,my_canvas);
    my_wiese->my_cyclam->p_sprite->setZ(3);
   // --- Ende Spritegenerierung -------------------------------------------

    // Game starten:
    my_hiscore->set_hscpath(hilist);
    my_wiese->neues_spiel();
    my_view->setContentsPos(OFX,OFY);
    my_view->polish();
    my_view->show();
  }	    
  else qDebug(err); // Bilder fehlen

// ---------------------------------------------------------------------------------------------------------------------
  my_app.setMainWidget(my_window);                                                // my_window als Hauptfenster  
  my_app.connect(my_view,SIGNAL(geklickt(int,int)),my_wiese,SLOT(klick(int,int)));        // Klick ins Spielfeld,  Parameter: Quadrant 
  my_app.connect(my_wiese,SIGNAL(gameover(int)),my_hiscore,SLOT(save_hsc(int)));  // Auswertung Spielende. Parameter: Punkte
  my_app.connect(my_wig11,SIGNAL(clicked(int)),my_wiese,SLOT(set_levelliste(int)));
  my_app.connect(my_wiese,SIGNAL(levelfile0_changed(bool)),wig110,SLOT(setChecked(bool)));
  my_app.connect(my_wiese,SIGNAL(levelfile1_changed(bool)),wig111,SLOT(setChecked(bool)));
  my_app.connect(my_wig12,SIGNAL(clicked(int)),my_wiese,SLOT(set_speed(int)));
 
  QPixmap my_pixmap=QPixmap::fromMimeSource(PROGICON);                                            // Programmicon setzen  
  my_window->setIcon(my_pixmap);
  my_window->show();                      // anzeigen
  
  // printf("Breite x Hoehe %i x %i\n",my_window->width(),my_window->height());    // Debug-Info
  return my_app.exec();                   // Exec-Schleife der my_app
}
