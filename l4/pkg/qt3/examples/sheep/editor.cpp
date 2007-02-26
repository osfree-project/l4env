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

// Leveleditor Lämmerlinge
// --------------------------------------------------------------------------------------------------------

#include <qapplication.h>      // Fensterhandling-Event-Routinen
#include <qmenubar.h>          // Menuleiste
#include <qmenudata.h>         // Menu-Items
#include <qaccel.h>            // Shortcuts
#include <qradiobutton.h>      // Auswahl
#include <qbuttongroup.h>      // Auswahl
#include <qfile.h>             // INI-Dateien auswerten
#include <stdio.h>             // debugging
#include <qlistbox.h>

#include "llang.h"    // laenderspezifische Texte
#include "config.h"   // Paraneter Darstellung, Spielgroesse, Dateien
#include "gamelib.h"  // ujas gamelib: viewport, hiscore, info-textmodule
#include "l_window.h" // Fenster
#include "levels.h"    // Levels einlesen, konvertieren
// ===============================================================================

// -------------------------------------------------------------------------------------------------------

int main(int argc, char **argv)
{ 
  QApplication my_app(argc,argv);                  // die Fenster/Appli-Steuerung  
  textviewer*  my_textviewer=new textviewer();     // Fenster fuer Hilfstexte
  window*      my_window    =new window(0);  
  viewport*    my_view      =my_window->my_view;     // Darstellungsbereich Level 
  QListBox*    my_lliste    =my_window->my_lliste;     
  QListBox*    my_sliste    =my_window->my_sliste;     
  QListBox*    my_cliste    =my_window->my_cliste;     
  QListBox*    my_iliste    =my_window->my_iliste;     
  int i,j,k;
  QString buff,temp;
 
// --- Menus: Levels, Info -----------------------------------------------------------------      
  QMenuBar*   my_menubar=new QMenuBar(my_window);  
  
// --- Menu: Levelliste laden,speichern,quit ---  
  my_lliste->setMinimumSize(224,96); my_lliste->setSelectionMode(QListBox::Single); 
  my_lliste->setVariableWidth(true); my_iliste->setAutoScrollBar(true);  
  my_sliste->setMinimumSize(224,96); my_sliste->setSelectionMode(QListBox::Single); 
  my_sliste->setVariableWidth(true); my_sliste->setAutoScrollBar(true);  
  my_cliste->setMinimumSize(224,96); my_cliste->setSelectionMode(QListBox::Single); 
  my_cliste->setVariableWidth(true); my_cliste->setAutoScrollBar(true);  
  my_iliste->setMinimumSize(224,96); my_iliste->setSelectionMode(QListBox::Single); 
  my_iliste->setVariableWidth(true); my_iliste->setAutoScrollBar(true);  
  
  QPopupMenu* my_ladeliste=new QPopupMenu();   my_ladeliste->insertItem(my_lliste);
  QPopupMenu* my_saveliste=new QPopupMenu();   my_saveliste->insertItem(my_sliste);
  QPopupMenu* my_insertliste=new QPopupMenu(); my_insertliste->insertItem(my_iliste);
  QPopupMenu* my_loeliste=new QPopupMenu();    my_loeliste->insertItem(my_cliste);
 
  QPopupMenu* my_menu2=new QPopupMenu();  
  my_menu2->insertItem(_LLADEN,my_ladeliste);
  my_menu2->insertItem(_LSPEICHERN,my_saveliste);
  my_menu2->insertItem(_LINSERT,my_insertliste);
  my_menu2->insertItem(_LAPPEND,my_window,SLOT(level_anhaengen()));
  my_menu2->insertSeparator();
  my_menu2->insertItem(_LLOESCHEN,my_loeliste);
  my_menu2->insertSeparator();
  my_menu2->insertItem(_QUIT,&my_app,SLOT(quit()),QKeySequence("CTRL+Q"));  
  my_menubar->insertItem(_LEVEL,my_menu2);
  my_app.connect(my_lliste,SIGNAL(selected(int)),my_window,SLOT(level_laden(int)));
  my_app.connect(my_sliste,SIGNAL(selected(int)),my_window,SLOT(level_speichern(int)));
  my_app.connect(my_iliste,SIGNAL(selected(int)),my_window,SLOT(level_einschieben_vor(int)));
  my_app.connect(my_cliste,SIGNAL(selected(int)),my_window,SLOT(level_loeschen(int)));

  
// Menu 1: Feld leeren, Randautomatik, Feld verschieben
  QPopupMenu* my_menu1=new QPopupMenu();
  my_menu1->insertItem(_LCLEAN,my_window,SLOT(leeren())); 
  my_menu1->insertSeparator();
  my_menu1->insertItem(_FILLTOP,  my_window,SLOT(fill_oben()));
  my_menu1->insertItem(_FILLRIGHT,my_window,SLOT(fill_rechts()));  
  my_menu1->insertItem(_FILLBOTTOM, my_window,SLOT(fill_unten()));  
  my_menu1->insertItem(_FILLLEFT, my_window,SLOT(fill_links()));  
  my_menu1->insertSeparator();
  my_menu1->insertItem(_FTOTOP,  my_window,SLOT(nach_oben()));
  my_menu1->insertItem(_FTORIGHT,my_window,SLOT(nach_rechts()));  
  my_menu1->insertItem(_FTOBOTTOM, my_window,SLOT(nach_unten()));  
  my_menu1->insertItem(_FTOLEFT, my_window,SLOT(nach_links()));  
  my_menubar->insertItem(_TOOLS,my_menu1);
  
// Info-Menuitems: 
  QPopupMenu* my_menu5=new QPopupMenu(); 
  my_menu5->insertItem(_HELP,my_textviewer,SLOT(show_info(int)),QKeySequence("CTRL+H"),51);
  my_menu5->insertItem(_ABOUT,my_textviewer,SLOT(show_info(int)),QKeySequence("CTRL+A"),52); 
  my_menubar->insertItem(_INFO,my_menu5);
  my_menubar->setItemParameter(51,3);
  my_menubar->setItemParameter(52,1);
  
// --------------------------------------------------------------------------------------------------------------------
  QCanvas* my_canvas=new QCanvas(DX*(XMAX+2),DY*(YMAX+2));
  my_canvas->setAdvancePeriod(50);
  my_view->setCanvas(my_canvas);    
  my_canvas->setBackgroundColor(BACKGR1);
  my_canvas->setBackgroundPixmap(QPixmap(BACKPIX1));
  my_window->my_bgarray    =new QCanvasPixmapArray(BGSPRITE,NUMBG);
  my_window->my_schafarray =new QCanvasPixmapArray(SCHAFSPRITE,4);
  QCanvasPixmap* my_schaf  =new QCanvasPixmap("./images/schaf_0012.png");
  my_window->my_bgarray->setImage(8,my_schaf);
  QCanvasPixmap* my_wespe  =new QCanvasPixmap("./images/wespe_0001.png"); 
  my_window->my_bgarray->setImage(NUMBG,my_wespe);
 
  // Check auf Vollstaendigkeit der Bilder:
  QString err="";
  if (!my_window->my_bgarray->isValid())     err=err+"Some background images are missing!\n";
  if (!my_window->my_schafarray->isValid())  err=err+"Some sheep images are missing!\n";

  if (err=="")  // Spritebelegung und Initialisierung:
  {  
    int x,y;
    k=0;
    for (j=0; j<YMAX; j++) for (i=0; i<XMAX; i++)
    { x=i*DX+OFX; y=j*DY+OFY; 
      my_window->p_bgsprite[k]=new QCanvasSprite(my_window->my_bgarray,my_canvas);
      my_window->p_bgsprite[k]->move(x,y,k%((NUMBG+1)));
      my_window->p_bgsprite[k]->setZ(1);
      my_window->p_bgsprite[k]->setVisible(true);
      k++;
    }
    // Cursor:
    my_window->p_cursor=new QCanvasSprite(my_window->my_bgarray,my_canvas);
    my_window->p_cursor->setZ(63);
    my_window->p_cursor->show();
    // Wespe:
    my_window->p_wespe=new QCanvasSprite(my_window->my_bgarray,my_canvas);
    my_window->p_wespe->move(0,0,NUMBG);
    my_window->p_wespe->setZ(31);
    my_window->p_wespe->hide();
    // Schaf:
    my_window->p_schaf=new QCanvasSprite(my_window->my_schafarray,my_canvas);
    my_window->p_schaf->move(0,0,0);
    my_window->p_schaf->setZ(23);
    my_window->p_wespe->hide();

   // --- Ende Spritegenerierung -------------------------------------------
    my_view->setContentsPos(OFX,OFY);
    my_view->polish();
    my_view->show();
  }	    
  else printf(err); // Bilder fehlen
  
// ---------------------------------------------------------------------------------------------------------------------
  my_app.setMainWidget(my_window);                                                // my_window als Hauptfenster  
  
  QPixmap my_pixmap=QPixmap(PROGICON);                                            // Programmicon setzen  
  my_window->setIcon(my_pixmap);
  my_window->show();                      // anzeigen
  my_window->leeren();
//  printf("Breite x Hoehe %i x %i\n",my_window->width(),my_window->height());    // Debug-Info
  return my_app.exec();                   // Exec-Schleife der my_app
}
