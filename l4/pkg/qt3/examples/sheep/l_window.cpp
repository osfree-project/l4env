/****************************************************************************************
    Copyright 2004 uja
    Copyright 2004 Trolltech (Qt3)

    This file is part of Laemmerlinge which runs under GPL.
*****************************************************************************************/

// Hauptfenster schafe Leveleditor - Version 06.01.2005 

#include "l_window.h"
#include <stdio.h>
#include <qapplication.h>

// ================================================================================================

window::window(QWidget *parent,char* name): QHBox(parent,name)
{ int i;
  QString temp;
  // printf("Baue Fenster auf ...\n");
  
  //Geometrie und Aussehen: Elemente bei einer QHBox sind spaltenfoermig angelegt:
  this->setMargin(16);
  this->setSpacing(4);
  this->setPaletteBackgroundColor(BACKGR0);
  this->setPaletteBackgroundPixmap(QPixmap(BACKPIX0));
  this->setCaption(_TITEL);  
//  this->setMaximumSize(WIN_BMAX,WIN_HMAX);
  this->setMinimumSize(WIN_BMIN,WIN_HMIN);

// erste Spalte: Zeile oben: Titelbild, darunter der Canvasview (Spielfeld)
  QVBox* p_spalte0=new QVBox(this);
  p_spalte0->setPaletteForegroundColor(COLOR0);
  p_spalte0->setSpacing(8);

  QLabel* p_ue =new QLabel(p_spalte0);
  temp="<h3>"; temp=temp+_TITEL+"</h3>";
  p_ue->setText(temp);
  p_ue->setMinimumSize(192,32);
     
  my_view=new viewport(p_spalte0,"playfield"); // QCanvasView-Variante
  QLabel* p_spacer01=new QLabel(_NIX,p_spalte0);
  p_spalte0->setStretchFactor(p_ue,1);
  p_spalte0->setStretchFactor(my_view,10);
  p_spalte0->setStretchFactor(p_spacer01,90);
     
// zweite Spalte:  
  QVBox* p_spalte1=new QVBox(this);
  p_spalte1->setPaletteForegroundColor(COLOR0);
  p_spalte1->setSpacing(4);
  p_messi=new QLabel(_NO_LEVEL,p_spalte1);
  p_messi->setPaletteForegroundColor(COLOR0);
  p_messi->setMinimumSize(224,32);
   
  QValidator* intvalidator=new QIntValidator(-1,999,this); // Zahlen 1-999
  QValidator* strvalidator=new QRegExpValidator(QRegExp("[\\s\\w\\-\\.!?,:=+*'/]{1,31}"),0); 
  
 // QValidator* nochnvalidator=new QRegExpValidator(QRegExp("\\w+\\s\\w+"),0); // Testwiese
  
  QGrid* p_rest=new QGrid(2,p_spalte1,"Texte");  // Anzeigetafel
  QLabel* p_llname=new QLabel(p_rest);     p_llname->setText(_LNAME);  p_text[0]=new QLineEdit(p_rest);
  QLabel* p_lzeit=new QLabel(p_rest);      p_lzeit->setText(_LZEIT);    p_text[2]=new QLineEdit(p_rest);
  QLabel* p_lnumpfeile=new QLabel(p_rest); p_lnumpfeile->setText(_LPFEILE);  p_text[1]=new QLineEdit(p_rest);
  p_text[1]->setValidator(intvalidator);
  p_text[2]->setValidator(intvalidator);
  p_text[0]->setValidator(strvalidator);
  
  QButtonGroup* p_vorrat=new QButtonGroup(4,Qt::Horizontal,_ITEMS,p_spalte1);  // Anzeigetafel
  QPushButton* my_button[NUMBG+1];  
  for (i=0; i<NUMBG; i++)
  { temp=temp.setNum(i);
    if (i<10) temp="./images/bg_000"+temp; else temp="./images/bg_00"+temp;
    temp=temp+".png";
    my_button[i]=new QPushButton(p_vorrat);
    my_button[i]->setPixmap(QPixmap(temp));
    my_button[i]->setFixedSize(40,40);
  }  
  
  my_button[NUMBG]=new QPushButton(p_vorrat);
  my_button[NUMBG]->setPixmap(QPixmap("./images/wespe_0000.png"));
  my_button[NUMBG]->setFixedSize(40,40);
  my_button[0]->setPixmap(QPixmap("./images/wiese.png"));
  my_button[8]->setPixmap(QPixmap("./images/schaf_0008.png"));
//  p_vorrat->setExclusive(true);

  schafri=new QButtonGroup(2,Qt::Vertical,_SCHAFRI,p_spalte1);
  for (i=0; i<4; i++)
  { sri[i]=new QRadioButton(schafri); 
    switch (i)
    { case 0: sri[i]->setText(_TOP);    break;
      case 1: sri[i]->setText(_RIGHT);  break;
      case 2: sri[i]->setText(_BOTTOM); break;
      case 3: sri[i]->setText(_LEFT);   break;
    }
  }
  schafri->setExclusive(true);
  schafri->setButton(0);

  moskiri=new QButtonGroup(3,Qt::Horizontal,_MOSKIRI,p_spalte1);
  mri[0]=new QRadioButton("0",moskiri);
  mri[1]=new QRadioButton("30",moskiri);   
  mri[2]=new QRadioButton("45",moskiri); 

  moskiri->setExclusive(true);
  moskiri->setButton(0);
  
  QLabel* p_spacer12=new QLabel(_NIX,p_spalte1);
    p_spalte1->setStretchFactor(p_messi,1); 
  p_spalte1->setStretchFactor(p_vorrat,1); 
  p_spalte1->setStretchFactor(schafri,1); 
  p_spalte1->setStretchFactor(moskiri,1); 
  p_spalte1->setStretchFactor(p_spacer12,99); 
  
  QLabel* p_spalte2=new QLabel(_NIX,this); // nur Spacer
  setStretchFactor(p_spalte0,2);
  setStretchFactor(p_spalte1,1);
  setStretchFactor(p_spalte2,99);
  
  // =================================================================       
  code=".:wB#O*ZA=HT";
  aktlevel=-1;
  my_lliste=new QListBox();
  my_cliste=new QListBox();
  my_sliste=new QListBox();
  my_iliste=new QListBox();
  my_levels=new level(ULEVELFILE); 
  updat_liste();
  aktlevel=my_cliste->count()+1;
  moskitostart=-1;
  schafstart=-1;
  
  // Connections:
  connect(my_view,SIGNAL(geklickt(int,int)),this,SLOT(klick(int,int)));
  connect(my_view,SIGNAL(hover(int,int)),this,SLOT(hovern(int,int)));
  connect(p_vorrat,SIGNAL(clicked(int)),this,SLOT(waehle(int)));
  connect(schafri,SIGNAL(clicked(int)),this,SLOT(updat_schaf(int)));
//  printf(" --- fertig\n");
  
}

// Slots: 
void window::level_laden(int nr)
{ int i,k;
  aktlevel=nr; 
  gewaehlt=0;
  p_cursor->setFrame(gewaehlt);
  QString temp; 
  temp=my_levels->my_level[aktlevel]->titel;                p_text[0]->setText(temp);
  temp=temp.setNum(my_levels->my_level[aktlevel]->zeit0);   p_text[2]->setText(temp);
  temp=temp.setNum(my_levels->my_level[aktlevel]->pfeile0); p_text[1]->setText(temp);
  moskitostart=my_levels->my_level[aktlevel]->moskito;      
  k=my_levels->my_level[aktlevel]->ri0; schafri->setButton(k); updat_schaf(k);
  k=my_levels->my_level[aktlevel]->mostyp; if (k<0) k=0; moskiri->setButton(k);
  temp=my_levels->my_level[aktlevel]->levelcode;
  schafstart=-1;
  for (i=0; i<SMAX; i++) { f[i]=code.find(temp.at(i)); if (f[i]==8) { schafstart=i; f[i]=0; } zeige_feld(i); }
  if (moskitostart<0) wespe_weg(); else set_wespe(moskitostart);
  set_schafstart(schafstart);
  updat_schaf(schafstart);
  temp=temp.setNum(aktlevel);
  temp=_GELADEN+temp;
  p_messi->setText(temp);
}  

void window::leeren()      { for (int i=0; i<SMAX; i++) { f[i]=0; zeige_feld(i); } wespe_weg(); }
void window::fill_oben()   { int i,c=f[0]; for (i=1; i<XMAX; i++) { f[i]=c; zeige_feld(i); } }
void window::fill_rechts() { int i,k=XMAX-1,c=f[k]; for (i=1; i<YMAX; i++) { k=k+XMAX; f[k]=c; zeige_feld(k); } }
void window::fill_unten()  { int i,k=SMAX-XMAX,c=f[k]; for (i=1; i<XMAX; i++) { f[i+k]=c; zeige_feld(i+k); } }
void window::fill_links()  { int i,c=f[0]; for (i=1; i<YMAX; i++) { f[i*XMAX]=c; zeige_feld(i*XMAX); } }

void window::nach_oben()
{ int i,j,k;
  for (i=0; i<XMAX; i++) 
  { for (j=0; j<YMAX-1; j++) { k=i+j*XMAX; f[k]=f[k+XMAX]; zeige_feld(k); }
    k=(YMAX-1)*XMAX+i; f[k]=0; zeige_feld(k);
  }
}

void window::nach_unten()
{ int i,j,k;
  for (i=0; i<XMAX; i++) 
  { for (j=YMAX-1; j>0; j--) { k=i+j*XMAX; f[k]=f[k-XMAX]; zeige_feld(k); }
    f[i]=0; zeige_feld(i);
  }
}

void window::nach_rechts()
{ int i,j,k;
  for (j=0; j<YMAX; j++) 
  { for (i=XMAX-1; i>0; i--) { k=i+j*XMAX; f[k]=f[k-1]; zeige_feld(k); }
    f[j*XMAX]=0; zeige_feld(j*XMAX);
  }
}

void window::nach_links()
{ int i,j,k;
  for (j=0; j<YMAX; j++) 
  { for (i=0; i<XMAX-1; i++) { k=i+j*XMAX; f[k]=f[k+1]; zeige_feld(k); }
    f[XMAX*(j+1)-1]=0; zeige_feld(XMAX*(j+1)-1);
  }
}

void window::zeige_feld(int nr) { p_bgsprite[nr]->setFrame(f[nr]); }
void window::set_messi(QString p)    { p_messi->setText(p); }

void window::set_wespe(int nr) 
{ int x=nr%XMAX,y=(nr-x)/XMAX; 
  x=x*DX+OFX; y=y*DY+OFY; 
  p_wespe->move(x,y,31);
  p_wespe->show();
}

void window::set_schafstart(int nr) 
{ int x=nr%XMAX,y=(nr-x)/XMAX; 
  x=x*DX+OFX; y=y*DY+OFY; 
  p_schaf->move(x,y,23);
  p_schaf->show();
  schafstart=nr;
}


void window::set_bgr(int nr)
{ int i=p_cursor->frame();
  if (i==NUMBG) // nur 1 Wespe!
  { if (nr==moskitostart) wespe_weg(); else
    { moskitostart=nr; 
      set_wespe(nr);
    }   
  }
  else if (i==8) { set_schafstart(nr); f[nr]=0; }
  else { f[nr]=i; zeige_feld(nr); } // Vorereitung fuer mehrere Schafstarts
}

void window::wespe_weg() { moskitostart=-1; p_wespe->hide(); }

// Erguesse von QPushbutton: -------------------------------------------------------------------------------
void window::waehle(int nr) { p_cursor->setFrame(nr); }
void window::updat_schaf(int nr) { p_schaf->setFrame(nr); }

// Erguesse von Viewport: ---------------------------------------------------------------------------------
void window::klick(int xx,int yy) { int x=(xx-OFX)/DX,y=(yy-OFY)/DY,z=x+XMAX*y; set_bgr(z); }
void window::hovern(int xx,int yy) { int mx=xx-DX/2,my=yy-DY/2; p_cursor->move(mx,my); }


// levelfile-Handling ========================================================================================
void window::updat_liste() // nach Speichern listen aktualisieren
{ int i,k;
  QString temp;
  my_levels=new level(ULEVELFILE); 
  k=my_levels->anz_levels;
  my_lliste->clear();
  my_sliste->clear();
  my_cliste->clear();
  my_iliste->clear();
  for (i=0; i<k; i++) 
  { temp=temp.setNum(i); 
    temp=_LLEVEL+temp+": "+my_levels->my_level[i]->titel;
    my_lliste->insertItem(temp,-1); 
    my_sliste->insertItem(temp,-1); 
    my_cliste->insertItem(temp,-1); 
    my_iliste->insertItem(temp,-1); 
  }
  temp=temp.setNum(k);
  temp=_LLEVEL+temp+": "+_NLEVEL;
  my_iliste->insertItem(temp,-1);
}

QString window::mach_record()
{ int i,i1,i2,k;
  bool ok=true;
  QString t0,t1,t2,t3,temp;
  if (p_text[0]->text()=="") {  emit set_messi(_ERRLNAME); ok=false; }
  k=schafri->selectedId();
  if (k<0) { emit set_messi(_ERRSRI); ok=false; } else t0=t0.setNum(k);
  t1=p_text[2]->text(); i1=t1.toInt();
  if (i1<10) { emit set_messi(_ERRZEIT); ok=false; } else t1=t1.setNum(i1);
  t2=p_text[1]->text(); i2=t2.toInt();
  if (i2<1) { emit set_messi(_ERRPFEILE); ok=false; } else t2=t2.setNum(i2);
  if (ok)
  { temp=p_text[0]->text()+';'+t0+';'+t1+';'+t2+';';
    t0=t0.setNum(moskitostart);
    k=moskiri->selectedId(); if (k<0) k=0; t1=t1.setNum(k);
    temp=temp+t0+';'+t1+';';
    for (i=0; i<SMAX; i++) if (i==schafstart) temp=temp+code.at(8); else temp=temp+code.at(f[i]);    
  }
  else temp="";
  return temp;
}

bool window::backup()
{ bool ok=false;
  int i,j,k=my_levels->anz_levels,buffsize=2*SMAX;
  QString raus_damit,buff,temp=ULEVELFILE; temp=temp+"_backup";
  set_messi(temp);
  QFile fb(temp);
  QFile fh(ULEVELFILE);
  { if (fh.open(IO_ReadOnly))
    { raus_damit="";
      for (i=0; i<k; i++)
      { j=fh.readLine(buff,buffsize);
        raus_damit=raus_damit+buff;
      }
      fh.close();
      if (fb.open(IO_WriteOnly)) { j=fb.writeBlock(raus_damit,raus_damit.length()); fb.close(); ok=(j>0); }
    } 
  }
  return ok;
}

void window::level_anhaengen() 
{ int k=-1;
  bool ok=backup();
  QFile fh(ULEVELFILE);
  QString rec=mach_record()+"\n";
  if (ok) if (rec!="\n")
  { ok=false;
    if  (fh.open(IO_WriteOnly|IO_Append)) 
    { k=fh.writeBlock(rec,rec.length()); 
      ok=(k>0);
      fh.close();
    } 
  }
  if (k>0) set_messi(_APPEND_OK);
  updat_liste();
}

void window::level_speichern(int nr) 
{ int i,j,k=my_levels->anz_levels,buffsize=2*SMAX;
  bool ok=backup();
  QFile fh(ULEVELFILE);
  QString rec=mach_record()+"\n";
  QString raus_damit;
  QString buff;
  if (rec=="\n") ok=false;
  if (ok)
  { ok=false;
    if (fh.open(IO_ReadOnly))
    { raus_damit="";
      for (i=0; i<k; i++)
      { j=fh.readLine(buff,buffsize);
        if (i==nr) raus_damit=raus_damit+rec; else raus_damit=raus_damit+buff;
      }
      fh.close();
      if (fh.open(IO_WriteOnly)) { j=fh.writeBlock(raus_damit,raus_damit.length()); fh.close(); ok=(j>0); }
    } 
    updat_liste(); 
  }
  buff=buff.setNum(nr);
  buff=_SAVE_OK+buff;
  if (ok) set_messi(buff);
}

void window::level_einschieben_vor(int nr) 
{ int i,j,k=my_levels->anz_levels,buffsize=2*SMAX;
  bool ok=backup(); 
  QFile fh(ULEVELFILE);
  QString rec=mach_record()+"\n";
  QString raus_damit;
  QString buff;
  if (rec=="\n") ok=false;
  if (ok)
  { ok=false;
    if (fh.open(IO_ReadOnly))
    { raus_damit="";
      for (i=0; i<k; i++)
      { if (i==nr) raus_damit=raus_damit+rec;
        j=fh.readLine(buff,buffsize);
	raus_damit=raus_damit+buff;
      }
      fh.close();
      if (nr>=k) raus_damit=raus_damit+rec;
      if (fh.open(IO_WriteOnly)) { j=fh.writeBlock(raus_damit,raus_damit.length()); fh.close(); ok=(j>0); }
    }      
    updat_liste(); 
  }
  buff=buff.setNum(nr);
  buff=_INSERT_OK+buff;
  if (ok) set_messi(buff);
}


void window::level_loeschen(int nr)  
{ int i,j=-1,k=my_levels->anz_levels,buffsize=2*SMAX;
  bool ok=backup();
  QFile fh(ULEVELFILE);
  QString raus_damit;
  QString buff;
  if (ok) if (fh.open(IO_ReadOnly))
  { ok=false;
    raus_damit="";
    for (i=0; i<k; i++)
    { j=fh.readLine(buff,buffsize);
      if (i!=nr) raus_damit=raus_damit+buff;
    }
   fh.close();
   if (fh.open(IO_WriteOnly)) { j=fh.writeBlock(raus_damit,raus_damit.length()); fh.close(); ok=(j>0); }
    updat_liste(); 
  }
  buff=buff.setNum(nr);
  buff=_LOESCH_OK+buff; 
  if (ok) set_messi(buff);
}

// ==================================================================================================================
