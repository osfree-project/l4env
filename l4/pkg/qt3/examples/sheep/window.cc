/****************************************************************************************
    Copyright 2004 uja
    Copyright 2004 Trolltech (Qt3)

    This file is part of Laemmerlinge which runs under GPL.
*****************************************************************************************/

// Hauptfenster schafe - Version 29.12.2004 14:00: mittiges Spielfeld

#include "window.h"
// ================================================================================================

window::window(QWidget *parent,char* name): QHBox(parent,name)
{ 
  //Geometrie und Aussehen: Elemente bei einer QHBox sind spaltenfoermig angelegt:
  this->setMargin(16);
  this->setSpacing(4);
  this->setPaletteBackgroundColor(BACKGR0);
  this->setPaletteBackgroundPixmap(QPixmap::fromMimeSource(BACKPIX0));
  this->setCaption(_TITEL);  
//  this->setMaximumSize(WIN_BMAX,WIN_HMAX);
  this->setMinimumSize(WIN_BMIN,WIN_HMIN);

  QLabel* p_spalte=new QLabel(_NIX,this); // nur Spacer

// erste Spalte: Zeile oben: Titelbild, darunter der Canvasview (Spielfeld)
  QVBox* p_spalte0=new QVBox(this);
  p_spalte0->setSpacing(8);
  QLabel* p_ue =new QLabel(p_spalte0);
  p_ue->setPixmap(QPixmap::fromMimeSource(TITELBILD)); 
  my_view=new viewport(p_spalte0,"playfield"); // QCanvasView-Variante
  QLabel* p_spacer01=new QLabel(_NIX,p_spalte0);
  p_spalte0->setStretchFactor(p_ue,1);
  p_spalte0->setStretchFactor(my_view,10);
  p_spalte0->setStretchFactor(p_spacer01,90);
    
// zweite Spalte: Anzeigentafel als 2-spaltiges Gitter, mittig angeordnet:  
  QVBox* p_spalte1=new QVBox(this);
  p_spalte1->setSpacing(8);
  QLabel* p_spacer11=new QLabel(_NIX,p_spalte1);
  p_messi=new QLabel(_NIX,p_spalte1);
  p_messi->setMinimumSize(160,128);
  p_messi->setPaletteForegroundColor(COLOR0);
  
  QGrid* p_anzeigen=new QGrid(2,p_spalte1,"anzeigen");  // Anzeigetafel
  p_anzeigen->setPaletteBackgroundPixmap(QPixmap::fromMimeSource(BACKPIX2));
  p_anzeigen->setPaletteForegroundColor(COLOR2);
  p_anzeigen->setMinimumSize(192,96);
  p_anzeigen->setFrameShadow(Sunken);
  p_anzeigen->setFrameShape(Box);
  p_anzeigen->setMargin(4);
  
  // Daten einfuellen, ist der Labeltext leer, wird die Anzeige unterdrueckt - nicht jedes Spiel braucht alle Anzeigen  
  if (_ANZEIGE0!="") { /*QLabel* p_llevel =*/new QLabel(_ANZEIGE0,p_anzeigen); p_level  =new QLabel(p_anzeigen); }
  if (_ANZEIGE4!="") { /*QLabel* p_lpfeile=*/new QLabel(_ANZEIGE4,p_anzeigen); p_pfeile =new QLabel(p_anzeigen); }
  if (_ANZEIGE3!="") { /*QLabel* p_lleben =*/new QLabel(_ANZEIGE3,p_anzeigen); p_leben  =new QLabel(p_anzeigen); }
  if (_ANZEIGE2!="") { /*QLabel* p_lzeit  =*/new QLabel(_ANZEIGE2,p_anzeigen); p_zeit   =new QLabel(p_anzeigen); }
  if (_ANZEIGE1!="") { /*QLabel* p_lpunkte=*/new QLabel(_ANZEIGE1,p_anzeigen); p_punkte =new QLabel(p_anzeigen); } 
  
  QLabel* p_spacer12=new QLabel(_NIX,p_spalte1);
  
  p_spalte1->setStretchFactor(p_spacer11,2); 
  p_spalte1->setStretchFactor(p_messi,1); 
  p_spalte1->setStretchFactor(p_anzeigen,2); 
  p_spalte1->setStretchFactor(p_spacer12,99); 

  QLabel* p_spalte2=new QLabel(_NIX,this); // nur Spacer
  setStretchFactor(p_spalte,99);  
  setStretchFactor(p_spalte0,3);
  setStretchFactor(p_spalte1,1);
  setStretchFactor(p_spalte2,99);
  
}

// Slots: - nicht benoetigte Slots stoeren erstmal nicht
void window::set_punkte(int p)    { p_punkte->setNum(p); }
void window::set_zeit(int p)      { p_zeit->setNum(p);   }
void window::set_level(QString p) { p_level->setText(p); }
void window::set_leben(int p)     { p_leben->setNum(p);  }
void window::set_pfeile(int p)    { p_pfeile->setNum(p); }
void window::set_messi(QString p) { p_messi->setText(p); }

// ==================================================================================================================
