/************************************************************************************************************
    Copyright 2004, 2005 uja 
    Copyright 2004 Trolltech (Qt3)
    This file is part of Laemmerlinge which runs under GPL
    20.08.04: V.1.00 - finished upto gamecraft level, 32 Levels
    25.08.04: V.1.00 - 6 more levels, pause
    26.08.04: V.1.10 - loadgame/savegame (Ostfriesian islands)
    29.08.04: V.1.20 - Ratio ausbrechen/umdrehen angepasst, Kill Schafe eingebaut
 
    27.12.04: V.1.30 - Internationalisierung, Speicher auf 8 hochgesetzt (KDE-Upload 1)
    06.01.05: V.1.40 - 2 Levelfiles einstellbar   
    09.01.05: V.1.50 - Leveleditor für Userlevels, 5 vorgefertigte Levels (KDE-Upload 2)
    
    11.01.05:        - Transporterkapazität auf SMAX erweitert 
    12.01.05: V.1.51 - Nasse Schafe immun gegen Wespenstiche       
    13.01.05: V.1.52 - Cyclamen eingebaut: nach Fressen von Cyclamen sind die Schafe
                       12 Felder lang gegen die Wespe immun
                       Moskito ist über Wasser, Stall und Transporter ungefährlich
    06.02.2005		Anpassung Hinderniserkennung, wenn Schaf zwischendurch eliminiert wird
    
***************************************************************************************************************/
#include <stdlib.h>
#include "wiese.h"

// Listenroutinen-Auszug: (c)uja 1989
int  in_liste(int e,int l[])  { int i,k=-1; if (l[0]>0) for (i=1; i<=l[0]; i++) if (e==l[i]) k=i; return k; }
void add_liste(int e,int l[]) { if (in_liste(e,l)<1) { l[0]++; l[l[0]]=e; } }
// Koordinatenkonvertierungen:
bool in_canvas(int xx,int yy)     { return ((xx>=0) && (yy>=0) && (xx<=XMAX*DX+OFX) && (yy<=(YMAX*DY+OFY))); }
bool in_view(int xx,int yy)       { return ((xx>=OFX)&&(yy>=OFY)&&(xx<=(XMAX-1)*DX+OFX)&&(yy<=((YMAX-1)*DY+OFY))); }
bool in_feldgrenze(int xx,int yy) { return (((xx-OFX)%DX==0) && ((yy-OFY)%DY==0)); }
int  to_feld(int xx,int yy) { if (in_view(xx,yy)) { int i=(xx-OFX)/DX,k=(yy-OFY)/DY; return i+k*XMAX; } else return -1; }
void to_koords(int fnr, int& xx,int& yy) { xx=fnr%XMAX; yy=(fnr-xx)/XMAX; xx=xx*DX+OFX; yy=yy*DY+OFY; }

// ======================================================================================================================  

// --- Schaf: -------------------------------------------------------------------------------------------------------
schaf::schaf(int i,int typ) { id=i; rasse=typ; immun=false; x=0; y=0; vx=0; vy=0; speed=0; }

// -- Schafdarstellung:
void schaf::set_pic()
{ if (ri%2==0) { vx=0; vy=speed; if (ri<1) vy=-vy; } else { vy=0; vx=speed; if (ri>2) vx=-vx; } 
  p_sprite->setFrame(baseimage+ri);
}

void schaf::init_schaf(int fnr,int richtg) 
{ ri=richtg; teleport(fnr); speed=0; 
  itic=-1; immun=false; 
  baseimage=rasse*4;
  p_sprite->setFrame(baseimage+ri);
  p_sprite->show();
}

void schaf::set_normal()     { wtic=2; speed=2; if (itic<1) { baseimage=rasse*4; immun=false; } set_pic(); }
void schaf::umdrehen()       { if (rand()%20<1) ausbrechen(); else { ri=(ri+2)%4; set_pic(); } }
void schaf::ausbrechen()     { if (rand()%20<1) umdrehen(); else { ri++; if (rand()%2>0) ri=ri+2; ri=ri%4; set_pic(); } }
void schaf::schaf_weg()      { speed=0; p_sprite->hide(); } 
void schaf::schaf_zuhause()  { speed=0; p_sprite->hide(); } 
void schaf::schaf_tot()      { speed=0; immun=false; baseimage=TOT; set_pic(); } 
void schaf::wschaf()         { if (wtic>0) { baseimage=NASS; speed=1; set_pic(); wtic--; immun=true; } else schaf_tot(); }
void schaf::teleport(int nr) { to_koords(nr,x,y); p_sprite->move(x,y); set_normal(); immun=true; }
void schaf::ischaf()         { if (itic>0) { baseimage=IMMUN; set_pic(); itic--; immun=true; } else set_normal(); }
// ==================================================================================================================

moskito::moskito(int start,int typ)
{ 
  x=start%XMAX; y=(start-x)/XMAX; x=x*DX+OFX; y=y*DY+OFY;
  if (typ==0) { vy=0; if (rand()%2==0) vx=-2; else vx=2;  } 
  else if (typ==1) { vx=0; if (rand()%2==0) vy=-2; else vy=2; }  
  else { if (rand()%2==0) vx=-2; else vx=2; if (rand()%2==0) vy=-2; else vy=2; }  
}

void moskito::init() { p_sprite->move(x,y); p_sprite->show(); }

void moskito::move_moskito()
{ int x1=x+vx,y1=y+vy;
  if ((x1<OFX) || (x1>(XMAX-1)*DX+OFX)) { x1=x; vx=-vx; if (vx>0) p_sprite->setFrame(1); else p_sprite->setFrame(3); }
  if ((y1<OFY) || (y1>(YMAX-1)*DY+OFY)) { y1=y; vy=-vy; }
  x=x1; y=y1; p_sprite->move(x,y);
}

// =============================================================================================================================

cyclam::cyclam() { count0=24; count=-1; phase=-1; }

void cyclam::setze_cyclam(int i) 
{ int x,y,platz=i;
  count=0; phase=0; count0=256; 
  to_koords(platz,x,y); p_sprite->move(x,y);
  p_sprite->setFrame(phase);
  p_sprite->show();
}   

void cyclam::wachsen()
{ count++;
  if (count==2) { phase=1; p_sprite->setFrame(phase); }
  else if (count==4)  { phase=2; p_sprite->setFrame(phase); }
  else if (count==8) { phase=3; p_sprite->setFrame(phase); }
  if (count>=count0) loeschen();
}

void cyclam::loeschen() { p_sprite->hide(); phase=-1; platz=-1; }

// ============================================================================================================0

wiese::wiese(QWidget* parent)
{ int i;
  for (i=0; i<SMAX; i++) f[i]=0; 
  h_liste[0]=0;				
  my_moskito=new moskito(0,0); // Moskito generieren   
  my_cyclam=new cyclam(); // Heilpflanze
  for (i=0; i<MAXSCHAFE; i++)  // Schafherde generieren
  { if (i%6==2) my_schaf[i]=new schaf(i,1); 
    else if (i%6==5) my_schaf[i]=new schaf(i,2); 
    else my_schaf[i]=new schaf(i,0);
  }    
  // Levelkram: ---
  level_id=0;
  my_levelliste=new level(QString(LEVELFILE));  
  my_timer  =new QTimer(this); // Sekundenzaehler
  my_stopper=new QTimer(this); // fuer singleShot
  set_speed(1);                // Schaf- und Moskitobewegungin ms
  
  aktiv=false;
// Sekundenticker und Ani-Ticker: ---  
  connect(my_timer,SIGNAL(timeout()),this,SLOT(ticken())); my_timer->start(1000);
  connect(my_stopper,SIGNAL(timeout()),this,SLOT(move_all())); my_stopper->start(tic);
// Anzeigen window: ---  
  connect(this,SIGNAL(punkte_changed(int)),   parent,SLOT(set_punkte(int)));    // Anzeige Punkte
  connect(this,SIGNAL(zeit_changed(int)),     parent,SLOT(set_zeit(int)));      // Anzeige Restzeit
  connect(this,SIGNAL(level_changed(QString)),parent,SLOT(set_level(QString))); // Levelanzeige
  connect(this,SIGNAL(leben_changed(int)),    parent,SLOT(set_leben(int)));     // Anzeige Restzeit
  connect(this,SIGNAL(pfeile_changed(int)),   parent,SLOT(set_pfeile(int)));    // Pfeilanzeige
  connect(this,SIGNAL(messi(QString)),        parent,SLOT(set_messi(QString)));  // Blabla
}

// Slots intern und Anzeigen: --------------------------------------------------------------------------------------
void wiese::zeige_feld(int nr) { p_bgsprite[nr]->setFrame(f[nr]); }
void wiese::game_over() { aktiv=false; p_gover->show(); emit punkte_changed(punkte); emit gameover(punkte); } 

void wiese::ticken() 
{ if (aktiv) 
  { zeit--; 
    if (zeit<0) { emit messi(_ZEITUM); punkte=punkte0; game_over(); }
    else emit zeit_changed(zeit);
  }
}

// Auswertung: -----------------------------------------------------------------------------------------------------
void wiese::feld_auswerten(int id,int& fnr) // slot fuer schaf
{ int i; 
  switch (f[fnr])
  { case 0: my_schaf[id]->set_normal(); break;                     // Wiese
    case 1: my_schaf[id]->set_normal(); my_schaf[id]->speed=1; break; // Sand
    case 2: my_schaf[id]->wschaf(); if (my_schaf[id]->speed==0) tot++; break; // Wasser
    case 5: my_schaf[id]->schaf_tot(); tot++; break;               // Loch
    case 6: my_schaf[id]->set_normal();    // Blume
            punkte+=100; 
	    f[fnr]=0;
	    zeige_feld(fnr); 
	    emit punkte_changed(punkte); 
	    break;
    case 7: my_schaf[id]->schaf_zuhause(); // im Stall
            drin++;
	    punkte=punkte+20;
            emit punkte_changed(punkte);
	    emit leben_changed(drin);
	    break;   
    case 8: my_schaf[id]->set_normal(); break;                     // Startfeld
    case 9: if (my_schaf[id]->vy==0) my_schaf[id]->set_normal(); break;    // Bruecke horizontal
    case 10: if (my_schaf[id]->vx==0) my_schaf[id]->set_normal(); break;    // Bruecke vertikal
    case 11: i=in_liste(fnr,h_liste);     // teleport 
	     i++;
             if (i>h_liste[0]) i=1;
	     fnr=h_liste[i];
	     my_schaf[id]->teleport(fnr);	      
	     break; 
    case 12: my_schaf[id]->ri=0; my_schaf[id]->set_normal(); break;   // ab hier Pfeile:
    case 13: my_schaf[id]->ri=1; my_schaf[id]->set_normal(); break;
    case 14: my_schaf[id]->ri=2; my_schaf[id]->set_normal(); break;
    case 15: my_schaf[id]->ri=3; my_schaf[id]->set_normal(); break;
  }
  if (my_cyclam->phase>-1) if (fnr==my_cyclam->platz) my_schaf[id]->itic=12;     
}

// patch 6.2.05: Endlosschleife verhindern, wenn schaf zwischendurch aus dem Verkehr gezogen wird
void wiese::hindernis(int id,int fnr)  // slot fuer schaf
{ if (my_schaf[id]->speed>0) switch (f[fnr])
  { case 3: my_schaf[id]->umdrehen();   break; // Baum
    case 4: my_schaf[id]->ausbrechen(); break; // Zaun
    case 9: if (my_schaf[id]->vy!=0)  my_schaf[id]->ausbrechen(); break; 
    case 10: if (my_schaf[id]->vx!=0) my_schaf[id]->ausbrechen(); break; 
  }
}

// Die Auswertung ueber den Slot-Mechanismus braucht zu viel Zeit, so dass es merkwuerdige Ergebnisse gibt:
// Schafe rennen ueber Buesche und reagieren erratisch auf Richtungswechsel.
// Gleiches Problem mit den Qt-Spritelibraries!
// Eine RealTime-Routine musste her. 
// Dieser Compiler straeubt sich gegen forward declaration, daher wurde der Schafmove auf die Wiese verlegt.
void wiese::move_schaf(int si)
{
  int nr,n1=-1,xx=my_schaf[si]->x,yy=my_schaf[si]->y,oldri;
  if (my_schaf[si]->speed>0) 
  { xx=xx+my_schaf[si]->vx; 
    yy=yy+my_schaf[si]->vy; 
    if (!in_canvas(xx,yy)) { my_schaf[si]->schaf_weg(); weg++; }
   }
  if (my_schaf[si]->speed>0) if (in_view(xx,yy)) if (in_feldgrenze(xx,yy)) // Schaf komplett im Quadrant
  { nr=to_feld(xx,yy);
    n1=nr;
    feld_auswerten(si,nr);
    if (my_schaf[si]->speed>0) if (my_schaf[si]->itic>0) my_schaf[si]->ischaf();
    if (n1!=nr) to_koords(nr,xx,yy); // Transporter
    do // auf Hindernisse vor dem Schaf reagieren:
    { oldri=my_schaf[si]->ri;
      n1=-1;
      switch (oldri)
      { case 0: if (nr>=XMAX)         n1=nr-XMAX; break;
        case 1: if (nr%XMAX<(XMAX-1)) n1=nr+1;    break; 
        case 2: if (nr<(SMAX-XMAX))   n1=nr+XMAX; break; 
        case 3: if (nr%XMAX>0)        n1=nr-1;    break;
      }
      if (n1>=0) hindernis(si,n1);
    }       
    while (oldri!=my_schaf[si]->ri);  
  }
  my_schaf[si]->x=xx;     
  my_schaf[si]->y=yy;     
  my_schaf[si]->p_sprite->move(my_schaf[si]->x,my_schaf[si]->y);
}

// Loopen: --------------------------------------------
void wiese::move_all() 
{ int i=-1;
  if (aktiv)  
  { if (my_moskito->x>0) 
    { my_moskito->move_moskito();
      if (my_moskito->vy!=0) 
      { if (my_cyclam->phase>=0) my_cyclam->wachsen(); 
        else if (rand()%100==0) while (i<0) { i=rand()%SMAX; if (f[i]<2) my_cyclam->setze_cyclam(i); else i=-1; } 
      }
    }
    for (i=0; i<MAXSCHAFE; i++) if (my_schaf[i]->speed>0)
    { move_schaf(i);
      if (my_cyclam->p_sprite->collidesWith(my_schaf[i]->p_sprite)) { my_schaf[i]->itic=12; my_schaf[i]->ischaf(); }
      if ((my_schaf[i]->speed>0) && (my_moskito->x>0))
      if (my_moskito->p_sprite->collidesWith(my_schaf[i]->p_sprite)) // crash
      if (!my_schaf[i]->immun) // Wasser, Stall und Transporter - Schwerkunkt ausnehmen:
      { int iz=to_feld(my_schaf[i]->x+DX/2,my_schaf[i]->y+DY/2);
        if (f[iz]!=2) if (f[iz]!=7) if (f[iz]!=11) { my_schaf[i]->schaf_tot(); tot++; }
      }
    }
    if (draussen>0) if (drin+tot+weg==MAXSCHAFE) { aktiv=false; QTimer::singleShot(500,this,SLOT(levelende())); }    
  }
}

// Startphase: -----------------------------------------------------------------------------------------------------
void wiese::schafe_raus()
{ if (draussen<MAXSCHAFE) 
  { my_schaf[draussen]->speed=2;
    draussen++; 
    QTimer::singleShot(600,this,SLOT(schafe_raus()));
  }
  else { emit messi(_NIX); }
}

void wiese::starte_level(int nr,bool pktreset)
{ int i,k,x1,y1,start=-1;
  h_liste[0]=0;for (i=0; i<SMAX; i++) { f[i]=0; p_transsprite[i]->hide(); }
  my_moskito->x=0; my_moskito->y=0; my_moskito->p_sprite->hide();
  for (i=0; i<MAXSCHAFE; i++) my_schaf[i]->p_sprite->hide();
  if (pktreset) punkte=0;
  aktlevel=nr; draussen=0; drin=0; tot=0; weg=0; punkte0=punkte;  // Punktebackup fuer Unterbrechung oder gameover

  QString code=".:wB#O*ZA=HT";  
  QString my_code=my_levelliste->my_level[aktlevel]->levelcode;
  for (int i=0; i<SMAX; i++) { f[i]=code.find(my_code.at(i)); if (f[i]==8) start=i; }    
  for (i=0; i<SMAX; i++) if (f[i]==11) add_liste(i,h_liste); // Transporter
  if (h_liste[0]>0) for (i=1; i<=h_liste[0]; i++)
  { x1=h_liste[i]%XMAX; y1=(h_liste[i]-x1)/XMAX; x1=x1*DX+OFX; y1=y1*DY+OFY;
    p_transsprite[i-1]->move(x1,y1,-1); p_transsprite[i-1]->show();
  }
  k=my_levelliste->my_level[aktlevel]->moskito;
  if (k>=0) 
  { x1=k%XMAX; y1=(k-x1)/XMAX; my_moskito->x=x1*DX+OFX;my_moskito->y=y1*DY+OFY;
    my_moskito->vx=4; my_moskito->vy=my_levelliste->my_level[aktlevel]->mostyp;
    my_moskito->p_sprite->move(my_moskito->x,my_moskito->y,1);
    my_moskito->p_sprite->show();
  }        
  pfeile=my_levelliste->my_level[aktlevel]->pfeile0;
  zeit  =my_levelliste->my_level[aktlevel]->zeit0;
  for (i=0; i<SMAX; i++) { p_bgsprite[i]->setZ(1); zeige_feld(i); }
  for (i=0; i<MAXSCHAFE; i++)  my_schaf[i]->init_schaf(start,my_levelliste->my_level[aktlevel]->ri0);
  emit punkte_changed(punkte); emit zeit_changed(zeit); emit pfeile_changed(pfeile); emit leben_changed(drin);  
  emit level_changed(my_levelliste->my_level[aktlevel]->titel);
  p_gover->hide();
  my_cyclam->loeschen();
  weiter();
  QTimer::singleShot(1500,this,SLOT(schafe_raus()));
}

// Endphase: --------------------------------------------------------------------------------------------------------
void wiese::levelende()
{ if (drin<MAXSCHAFE/2) { punkte=punkte0; emit messi(_KEINESCHAFE); game_over(); } 
  else 
  { aktlevel++; 
    punkte=punkte+zeit;
    if (aktlevel>=my_levelliste->anz_levels) { punkte=punkte+1000; emit messi(_KEINELEVELS);  game_over(); }
    else starte_level(aktlevel,false);
  }
}

// slots User Interface: ---------------------------------------------------------------------------------------------
void wiese::pause()    { if (aktiv) { aktiv=false; pausiert=true;  p_pause->show(); emit messi(_PAUSED); } }
void wiese::weiter()   { aktiv=true; pausiert=false; p_pause->hide(); }
void wiese::neues_spiel() { starte_level(0,true); }

// alle Schafe, die draussen sind, killen:
void wiese::aufgeben() { for (int i=0; i<MAXSCHAFE; i++) if (my_schaf[i]->speed>0) { my_schaf[i]->schaf_tot(); tot++; }  }

void wiese::lade_spielstand(int nr)      
{  int bufsize=254,i,k=nr+1;
   aktiv=false;
   QString buff,buff1,bname[MAXSPEICHER];
   QFile fh("/sheep/speicher.ini");
   if (fh.open(IO_ReadOnly))
   { fh.readLine(buff,bufsize);
     for (i=0; i<MAXSPEICHER; i++) { bname[i]=buff.section(';',i,i); }
     for (i=1; i<=k; i++) { fh.readLine(buff,bufsize); }
     buff1=buff.section(';',0,0); level_id=buff1.toInt();
     buff1=buff.section(';',1,1); aktlevel=buff1.toInt();  
     buff1=buff.section(';',2,2); punkte0=buff1.toInt();
     fh.close();
   }
   switch (level_id)
   { case 0: my_levelliste=new level(QString(LEVELFILE)); 
             emit levelfile0_changed(true); levelfile1_changed(false);
	     break; 
     case 1: my_levelliste=new level(QString(ULEVELFILE)); 
             emit levelfile0_changed(false); levelfile1_changed(true);
             break;
   }
   if (aktlevel>=my_levelliste->anz_levels) aktlevel=my_levelliste->anz_levels-1;
   if (aktlevel<0) // falls  userlevel.csv geplaettet wurde
   { aktlevel=0; my_levelliste=new level(QString(LEVELFILE)); punkte0=0; 
     emit levelfile0_changed(true); levelfile1_changed(false);
   }
   punkte=punkte0; 
   QString raus;
   raus=_GAMELOADED+bname[nr];
   emit messi(raus); 
   starte_level(aktlevel,false);
}  

void wiese::speichere_spielstand(int nr) 
{ int bufsize=254,i,j=aktlevel,k=nr+1,nrecs=MAXSPEICHER;
  if (j<0) j=0;
  QString buff[nrecs+1];
  QString bu0="",bu1="",bu2="";
  QString bname[MAXSPEICHER];
  QFile fh("/sheep/speicher.ini"); 
  if (fh.open(IO_ReadOnly)) 
  { fh.readLine(buff[0],bufsize);
    for (i=0; i<MAXSPEICHER; i++) { bname[i]=buff[0].section(';',i,i); }
    for (i=1; i<=nrecs; i++) fh.readLine(buff[i],bufsize); fh.close(); }  
  buff[k]=bu0.setNum(level_id)+';'+bu1.setNum((j))+';'+bu2.setNum(punkte0)+";\n";
  if (fh.open(IO_WriteOnly)) { for (i=0; i<=nrecs; i++) { fh.writeBlock(buff[i],buff[i].length()); } fh.close(); }
   QString raus;
   raus=_GAMESAVED+bname[nr];
   emit messi(raus); 
}

void wiese::restart()
{ aktiv=false;
  punkte=punkte0;
  starte_level(aktlevel,false);
}

void wiese::klick(int xx, int yy)
{ int x=(xx-OFX)/DX,y=(yy-OFY)/DY,nr=x+y*XMAX; 
  if (aktiv) 
  { 
    if ((f[nr]==0) && (pfeile>0)) // Pfeil setzen 
    { f[nr]=NUMBG; 
      p_bgsprite[nr]->setZ(61);  // damit er auch bei gemordeten Schafen sichtbar bleibt
      zeige_feld(nr); 
      pfeile--; 
      emit pfeile_changed(pfeile);
    } 
    else if (f[nr]>=NUMBG) { f[nr]++; if (f[nr]>=NUMBG+4) f[nr]=NUMBG; zeige_feld(nr); }  // drehe Pfeil
  }    
  else if (pausiert) { emit messi(_NIX); weiter(); }
}

void wiese::set_levelliste(int nr)
{ level_id=nr;
  switch (level_id)
  { case 0: my_levelliste=new level(QString(LEVELFILE)); break; 
    case 1: my_levelliste=new level(QString(ULEVELFILE)); break;
  }
  starte_level(0,true);
}

void wiese::set_speed(int nr) 
{ switch(nr)
  { case 0: tic=45; break;
    case 1: tic=35; break;
    case 2: tic=25; break;
  }
  my_stopper->changeInterval(tic);   
}

// ===============================================================================================================
