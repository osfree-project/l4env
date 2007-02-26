/****************************************************************************************
    Copyright 2004 uja
    Copyright 2004 Trolltech (Qt3)
    This file is part of laemmerlinge which runs under GPL
*****************************************************************************************/
// Definition Schafe, Herde, Moskito, Wiese:

#ifndef WIESE_H
#define WIESE_H

#define MAXSCHAFE 12 // Herdengroesse
#define NASS 12      // Bildbase nasses Schaf
#define TOT  16      // Bildbase totes Schaf
#define IMMUN 20     // Bildbase Schaf immun durch Cyclamen

#include "lang.h" 
#include "config.h"
#include "levels.h"
#include <qapplication.h>
#include <qcanvas.h>
#include <qtimer.h>
#include <qfile.h>
#include <stdio.h> // debugging

class schaf:public QCanvas
{
  Q_OBJECT
  
  public:
  schaf(int,int rasse=0);                     // Uebergabe: 0-2: weiss braun oder schwarz
  QCanvasSprite* p_sprite;
  int id,x,y,vx,vy,speed,ri,rasse,zustand,wtic,itic;  // Koordinaten,Bewegungsparameter,Aussehen,Zaehler, wie lange im Wasser
  bool immun;
  int baseimage;

  void set_pic();       // Darstellung
  void set_normal();    // Schaf resetten nach neu()
  void set_immun();    // Schaf resetten nach neu()
  void umdrehen();      // Reaktion auf baum 
  void ausbrechen();    // Reaktion auf Zaun
  void schaf_tot();     // ins Loch gefallen, vom Moskito gestochen oder zu lange im Wasser
  void schaf_weg();     // aus dem Bild gelaufen 
  void schaf_zuhause(); // Schaf gerettet
  void wschaf();        // Schaf ins/im Wasser
  void ischaf();        // Schaf immun
  void teleport(int);
  void init_schaf(int,int);  
};

// -----------------------------------------------------------------------------------------------------------------------

class moskito
{
  public:
  moskito::moskito(int,int); // Startfeld, Bewegungsmuster
  QCanvasSprite* p_sprite;    
  int x,y,vx,vy,typ;
  void init();
  void move_moskito();
};

// ----------------------------------------------------------------------------------------------------------------------

// -----------------------------------------------------------------------------------------------------------------------

class cyclam
{ public:
  cyclam::cyclam(); // Startfeld, Bewegungsmuster
  QCanvasSprite* p_sprite;    
  int platz,phase,count0,count;
  
  void setze_cyclam(int);
  void wachsen();
  void loeschen();
};



class wiese:public QCanvas
{
  Q_OBJECT
  
  public:
  wiese(QWidget* parent=0);
  // alle Bilder:
  QCanvasPixmapArray* my_bgarray;     // Gelaende
  QCanvasPixmapArray* my_pfeilarray;  // Richtungspfeile
  QCanvasPixmapArray* my_transparray; // Transporterdeckel
  QCanvasPixmapArray* my_wespenarray; // Moskito
  QCanvasPixmapArray* my_schafarray;  // Schafbilder
  QCanvasPixmapArray* my_goverarray;  // gameover
  QCanvasPixmapArray* my_pausearray;  // pause
  QCanvasPixmapArray* my_cyclamarray;  // pause
  
  QCanvasSprite* p_bgsprite[SMAX];   // Gelaendesprites
  QCanvasSprite* p_transsprite[SMAX]; // Transporterdeckel-Sprite
  QCanvasSprite* p_pause;
  QCanvasSprite* p_gover;
  
  level*      my_levelliste;         // Gelaendetypenvorrat
  schaf*      my_schaf[MAXSCHAFE];   // Bewohner 1
  moskito*    my_moskito;            // Bewohner 2
  cyclam*     my_cyclam;             // Blümchen
  
  bool aktiv,pausiert;               // Flags
  int pfeile0,pfeile,punkte,punkte0,zeit,zeit0,aktlevel,level_id,drin,tot,weg,tic; // Spielparameter
  int f[SMAX],h_liste[SMAX];         // Feld, Transporter
  QString levelname; 	      
  QTimer* my_timer;
  QTimer* my_stopper;
  
  private:
  int draussen;
  bool alle_weg();
  void game_over();
  void zeige_feld(int);
  void feld_auswerten(int,int&);
  void hindernis(int,int);
  void move_schaf(int); // Schafbewegung, wg.Realtime Processing gesteuert von wiese
   
  private slots:
  void ticken();
  void move_all();
  void schafe_raus();
  void levelende();
 
  public slots:
  void neues_spiel();               // Start von Level0 aus
  void starte_level(int,bool);      // Start eines beliebigen Levels
  void pause(); 
  void weiter();                    
  void lade_spielstand(int);
  void speichere_spielstand(int);
  void klick(int,int);              // Klick ins Feld, Pfeil setzen oder drehen
  void aufgeben();
  void restart();
  void set_levelliste(int);
  void set_speed(int);
  
  signals:
  void punkte_changed(int);         // an window, Punkte anzeigen
  void levelfile0_changed(bool);      // an wig110, Menu 1a updaten
  void levelfile1_changed(bool);      // an wig111, Menu 1a updaten
  void zeit_changed(int);           // an window, Restzeit anzeigen
  void pfeile_changed(int);         // an window, Restzeit anzeigen   
  void leben_changed(int);          // an window, Schafe im Stall anzeigen
  void level_changed(QString);      // an window, Levelnamen anzeigen
  void messi(QString);              // allgem. Messages
  void gameover(int);  // Gameover-Bild, Punkte weiterreichen und bei Bedarf HSC-Eintrag
};

#endif
