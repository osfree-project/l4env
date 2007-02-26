/****************************************************************************************
    Copyright 2004 uja - Dr. Ulrike Jahnke-Soltau - www.gamecraft.de, www.ujaswelt.de
    Copyright 2004 Trolltech (Qt3)
    This file is part of ujagames_qt which run under GPL.
*****************************************************************************************/
// userspezifische Anpassungen (Netzwerkpfad) in hiscore.ini
// ------------------------------------------------------------------------------------------------------------------
// 02.08.2004 uja: Textviewer für Hilfe, Info, Lizenz - Conntainer fuer Textanzeige aus beliebiger RichText-Datei
// 03.08.2004 uja: Erstellung Hiscore-Module lokal
// 30.08.2004 uja: File-Control-Blocks eingebaut fuer eine Netz-Hiscoreliste
// 30.08.2004 uja: Viewport- und Anzeigenrahmen eingefaerbt
// 05.09.2004 uja: Ueberarbeitung, so dass viewport fuer ujagames_qt einsetzbar sind,
// 07.09.2004 uja: Einfaerbung Schliessen-Button, Umschalter lokaler Hiscore/Netz-Hiscore, 
// 11.09.2004 uja: Erweiterung Viewport, so dass das Mausrad abgefangen wird bei Erhalt der Scrollfaehigkeit  
// 12.09.2004 uja: Zusammenfassung aller konstanten Klassen zur gamelib
// 27.12.2004 uja: Internationalisierung: Trennung von config.h und lang.h, hiscore.ini wird jetzt von hiscore bedient
//                 Straffung zugehoeriger files, Begrenzung lock-Versuche auf 1000 
// ====================================================================================================================
#include "gamelib.h"
#include <fcntl.h>   // getuerktes Lock fuer Hiscore, funktioniert nur unter Unix
#include <stdio.h>   // Display Error messages

// Spielfelddarstellung - klick- und hoversensitiver View
// Da zum einen bei gleicher Canvas-und Viewgroesse stoerende Randpixel auftreten,
// und zum anderen bei arkadeartigen Spielen die Sprites beim Initialisieren im Rand versteckt werden muessen, um 
// ein Flackern zu vermeiden, steht der Canvas an jeder der 4 Seiten etwas ueber.
// Die Scrollbars mussten ausgeblendet werden, zusaetzlich die Reaktion auf das Mausrad. 
// Scrollen selbst sollte erhalten bleiben fuer spaetere Spiele
// ----------------------------------------------------------------------------------------------------------------------------
viewport::viewport(QWidget* parent, char* name):QCanvasView(parent,name)
{ 
  int xmax=XMAX*DX,ymax=YMAX*DY;             // Daten aus config1.h des jeweiligen Programms
  setMinimumSize(xmax+4,ymax+4);             // feste Feldgroesse setzen, Rand mit einrechnen
  setMaximumSize(xmax+4,ymax+4);
  setHScrollBarMode(QScrollView::AlwaysOff); 
  setVScrollBarMode(QScrollView::AlwaysOff);        // Scrollbalken abschalten
  QScrollView::viewport()->setMouseTracking(true);  // Hover-Effekte ermoeglichen

  // schicker Rand:
  // Die Randfarben koennen nicht einzeln veraendert werden, deshalb wurde die aktuelle Palette geklont und in ihr
  // die neuen Randfarben eingestellt. Die neue Palette wurde dem Widget zugewiesen.
  QPalette my_pal=QPalette(this->palette());        
  my_pal.setColor(QColorGroup::Light,RANDHCOLOR);
  my_pal.setColor(QColorGroup::Dark,RANDDCOLOR);
  this->setPalette(my_pal);
  setFrameShape(Box);
  setFrameStyle(Panel);
  setFrameShadow(Sunken);
}
  
void viewport::contentsMousePressEvent(QMouseEvent *e) { int xx=e->x(),yy=e->y(); emit geklickt(xx,yy); }
void viewport::contentsMouseMoveEvent(QMouseEvent* e)  { int xx=e->x(),yy=e->y(); emit hover(xx,yy); }
void viewport::contentsWheelEvent(QWheelEvent* e)      { if (e) setContentsPos(OFX,OFY); } // auf Startwerte setzen

// =================================================================================================================

// Container zu Anzeigen eines RTF-Textes:
textviewer::textviewer(QWidget* parent,char* name): QVBox(parent,name)
{ // Textfeld, Reispapier als Hintergrund:
  my_text=new QTextEdit(this); 
  my_text->setReadOnly(true);       // nur Textanzeige
  my_text->setTextFormat(RichText);
  QBrush* my_brush=new QBrush(COLOR1,QPixmap::fromMimeSource(PAPERBILD));
  my_text->setPaper(*my_brush);
  // mittiger Schliessen-Button:
  QHBox* p_mittig=new QHBox(this);
  QLabel* p_leer1=new QLabel(p_mittig);
  QPushButton* p_wech=new QPushButton(_CLOSE,p_mittig);
  QLabel* p_leer2=new QLabel(p_mittig);
  p_mittig->setStretchFactor(p_leer1,10); 
  p_mittig->setStretchFactor(p_leer2,10); 
  p_mittig->setStretchFactor(p_wech,10);
  connect(p_wech,SIGNAL(clicked()),this,SLOT(hide()));            
}

// Slot: Einlesen und Darstellen eines der 3 Texte Hilfe, Info, Lizenz:
// ---------------------------------------------------------------------
void textviewer::show_info(const int flag)
{ QString buff,infile;
  buff=PROGNAME;
  if (flag==0)      { infile=HELPFILE; resize(HELPBREITE,320); setCaption((buff+" - "+_HELP)); }
  else if (flag==1) { infile=INFOFILE; resize(INFOBREITE,320); setCaption((buff+" - "+_ABOUT)); }
  // else if (flag==2) { infile=GPLFILE;  resize(HELPBREITE,320); setCaption((buff+" - "+_GPL)); }
  else if (flag==3) { infile=LHELPFILE; resize(HELPBREITE,320); setCaption((buff+" - "+_HELP)); }
  else { infile="unknown file"; resize(240,96); }
  QString buffout="Couldn't find "+infile;
  QFile file(infile); // Text als Stream einlesen
  if (file.open(IO_ReadOnly)) { QTextStream stream(&file); my_text->setText(stream.read()); file.close(); }
  polish();
  show();
  raise();
}

// ===============================================================================================================

// Hiscore-Modul: Erzeugen der Hiscore-Datei, erforderlich fuer Netz-Modus 
// Anzeige der Bestenliste, wobei die ersten 5 eine andere Textfarbe erhalten
// und mit einer Trophaehe bedacht werden.
// Punktevergleich und Eingabemodul. Alternativ haette man hier den Login-Namen nehmen koennen und sich das letzte Modul
// sparen koennen, aber der Spass waere nur halb so gross.

// Hiscore-Record belegen:
void hscrec::setValues(int i,QString datum,QString sname) { punkte=i; zeit=datum; name=sname; }  

// 1 Zelle in der Hiscore-Tabelle formatieren, auf Wunsch rechtsbuendig fuer Zahlen oder fett fuer die Top 5:
QString hiscore::get_feld(QString ein,bool rechts=false,bool fett=false)
{ 
  QString fett_on="<font color=\""; fett_on=fett_on+HICOLOR+"\"><b>";
  QString fett_off="</b></font>";
  QString normal_on="<font color=\""; normal_on=normal_on+LOCOLOR+"\">";
  QString normal_off="</font>";
  QString out="<td ";
  if (rechts) out=out+" align=right";
  out=out+"><nobr>";
  if (fett) out=out+fett_on; else out=out+normal_on; 
  out=out+ein;
  if (fett) out=out+fett_off; else out=out+normal_off;
  out=out+"</nobr></td>";
  return out;
}


// Datei-Ein/Ausgabe, Datei ist CSV, Trenner: Semikolon
void hiscore::dateieinlesen(hscrec my_hscliste[])
{ int i,gelesen=0;
  QString buff,buffint;
  QFile file(*hscdatei);
  if (file.open(IO_ReadOnly))
  { for (i=0; i<MAXEINTRAG; i++)
    { my_hscliste[i].setValues(-999,"---","---"); // falls Daten fehlen, selbstaendig erweitern
      gelesen=file.readLine(buff,256);
      if (gelesen>10)
      { buffint=buff.section(';',0,0);
        my_hscliste[i].setValues(buffint.toInt(),buff.section(';',1,1),buff.section(';',2));
      }	 
    }
    file.close();
  }  
}

void hiscore::dateischreiben(hscrec my_hscliste[])
{ int i,geschrieben=0;
  QString buff;
  QFile file(*hscdatei); 
  if (file.open(IO_WriteOnly))
  { for (i=0; i<MAXEINTRAG; i++)
    {  buff=buff.setNum(my_hscliste[i].punkte)+';'+my_hscliste[i].zeit+';'+my_hscliste[i].name;
       geschrieben=file.writeBlock(buff,buff.length());
    }
    file.close();
  }  
}

// --------------------------------------------------------------------------------------------------

// Slot, Darstellung der Bestenliste als RTF mit Hilfe einer Tabelle:
void hiscore::show_hsc()
{ 
  int i,platz=0;
  bool spitze;                           // Zugehoerigkeit zur Spitzengruppe
  QString buffint,buffout="<br><table>";
  hscrec my_hscliste[MAXEINTRAG];
  QMimeSourceFactory::defaultFactory()->setImage("myimage",QImage(HSCPOTT)); 
  // RichText-Bild muss kreiert werden in MimeFactory
  
  dateieinlesen(my_hscliste);
  for (i=0; i<MAXEINTRAG; i++)  
  { platz++;
    spitze=(platz<=HSCGRENZE);     // Zugehoerigkeit bestimmen
    buffout=buffout+"<tr>";
    buffint=buffint.setNum(platz)+'.';
    if (spitze) buffint="<img src=\"myimage\" align=left> "+buffint;
    buffout=buffout+get_feld(buffint,true,spitze)+get_feld(my_hscliste[i].name,false,spitze);
    buffint=buffint.setNum(my_hscliste[i].punkte,10);
    buffout=buffout+get_feld(buffint,true,spitze)+get_feld(my_hscliste[i].zeit,false,spitze)+"</tr>";
  }  
  buffout=buffout+"</table>"; 

  my_text->setText(buffout);
  my_text->setContentsPos(0,0);
  polish();
  show();
  raise();
}

// HSC resetten - Sicherheitsabfrage ueber QMessageBox 
void hiscore::reset_hsc()
{ 
  int i,k;
  hscrec my_hscliste[MAXEINTRAG];
  k=QMessageBox::question(this,tr(PROGNAME),tr(_CONFRESET),tr(_YES),tr(_NO),QString::null,1,1);
  if (k==0)
  { for (i=0; i<MAXEINTRAG; i++) my_hscliste[i].setValues((500-10*i),"--:--","---\n");
    dateischreiben(my_hscliste);        
  }
}

// ===== Block HSCSchreiben, Einstieg: save_hsc ================================================
// --- fcntl.h - Bibliothek fuer getuerktes lock: ----------------------------------------------
// Wer zuerst kommt und diese Datei schreibt, bekommt hierauf das alleinige Recht und darf sich eintragen. 
// Kommt waehrenddessen ein weiterer und versucht, diese Datei zu schreiben, schlaegt fuer ihn die Sache fehl, und er wird abgewiesen.
// Nach Eintrag wird das Lockfile wieder geloescht und damit die Hiscore-Liste wieder freigegeben.

bool hiscore::get_lock()  
{ QString lockfile=*hscpfad+"gotcha.lck"; 
  int gotcha=creat(lockfile,S_IRWXU); 
  return (gotcha>-1);
}

void hiscore::free_lock() { QString lockfile=*hscpfad+"gotcha.lck"; remove(lockfile); }
// ---------------------------------------------------------------------------------------------

void hiscore::eintragen(int punkte,QString datum,QString sname)
{ 
  int i,versuch=0,platz=MAXEINTRAG;
  hscrec my_hscliste[MAXEINTRAG];
  
  while (versuch<1000) // possible source of gentoo - problems
  { versuch++;
    if (get_lock()) 
    {  
      dateieinlesen(my_hscliste);
      for (i=MAXEINTRAG-1; i>=0; i--) if (punkte>=my_hscliste[i].punkte) platz=i;
      if (platz<MAXEINTRAG)
      { for (i=MAXEINTRAG-1; i>platz; i--) my_hscliste[i]=my_hscliste[i-1];
        my_hscliste[platz].setValues(punkte,datum,sname);
        dateischreiben(my_hscliste);        
      }
      free_lock();
      show_hsc(); 
      versuch=2000; 
    }
  }
  if (versuch<2000) printf("Error Hiscore: Check path and permissions!\n"); 
}
// ----------------------------------------------------------------------------------------------
void hiscore::save_hsc(int punkte)
{ 
  // Punktegrenze bestimmen
  hscrec my_hscliste[MAXEINTRAG];
  dateieinlesen(my_hscliste); 
  if (punkte>=(my_hscliste[MAXEINTRAG-1].punkte)) // Namen holen
  {
    QDateTime timestamp=QDateTime::currentDateTime();
    QString datum=timestamp.toString(TIMEFORM);
    QString blabla,buffint,zeile;
    bool ok;
    buffint=buffint.setNum(punkte,10);
    blabla=_HSCMESS0+buffint+_HSCMESS1+_HSCMESS2;
    QString my_name=QInputDialog::getText(PROGNAME,blabla,QLineEdit::Normal,QString::null,&ok,0);
    if (ok && !my_name.isEmpty())
    { my_name=my_name.left(32)+"\n"; // Record-Ende einfügen
      eintragen(punkte,datum,my_name);
    }
  }    
}

// ===============================================================================================
hiscore::hiscore(QWidget* parent,char* name): QVBox(parent,name)
{ 
  // Initialisierung HSCFile 
  hscdatei=new QString(HSCFILE);
  QString buff=PROGNAME;
  // Anzeige aufbauen aus config.h:
  setPaletteBackgroundColor(BACKGR0);
  setPaletteForegroundColor(COLOR0);
  setPaletteBackgroundPixmap(QPixmap::fromMimeSource(BACKPIX0));
  setMargin(16);
  setSpacing(8);
  setCaption((buff+" - "+_HISCORE));

  QLabel* p_ue=new QLabel(this);
  p_ue->setText(("<h3>"+buff+" - "+_HISCORE+"</h3>"));
  
  // Textfeld:
  my_text=new QTextEdit(this);
  my_text->setReadOnly(true);
  my_text->setTextFormat(RichText);
  my_text->setPaletteBackgroundColor(BACKGR2);
  my_text->setPaletteBackgroundPixmap(QPixmap::fromMimeSource(PAPERBILD));
  my_text->setBackgroundPixmap(QPixmap::fromMimeSource(BACKPIX2));
  my_text->setFrameShadow(Sunken);
  my_text->setFrameShape(Box);
  my_text->setMargin(8); 
  my_text->setMinimumWidth(HSCBREITE);      
  my_text->setMinimumHeight(160);      

  QBrush* my_brush=new QBrush(QColor(BACKGR2),QPixmap::fromMimeSource(PAPERBILD));
  my_brush->setPixmap(QPixmap::fromMimeSource(BACKPIX2));
  my_text->setPaper(*my_brush);

  // mittiger Button:
  QHBox* p_mittig=new QHBox(this);
  QLabel* p_leer1=new QLabel(p_mittig);
  QPushButton* p_wech=new QPushButton(_CLOSE,p_mittig);
  p_wech->setPaletteBackgroundColor(BACKGR2);
  QLabel* p_leer2=new QLabel(p_mittig);
  p_mittig->setStretchFactor(p_leer1,10); 
  p_mittig->setStretchFactor(p_leer2,10); 
  p_mittig->setStretchFactor(p_wech,10);
  connect(p_wech,SIGNAL(clicked()),this,SLOT(hide()));         
}

void hiscore::set_hscpath(int nr)
{ // Hiscoredatei bestimmen aus hiscore.ini, falls nicht vorhanden, Voreinstellung aus config.h:
  hscpfad_id=nr;
  hscpfad=new QString("/sheep");
  hscdatei=new QString;  
  QString buff="";
  QFile file("/sheep/hiscore.ini");
  if (file.open(IO_ReadOnly)) { for (int i=0; i<=(nr+1); i++) file.readLine(buff,96); file.close(); }
  if (buff.length()>3) *hscpfad=buff.section(';',0,0); 
  *hscdatei=*hscpfad+"hiscore.csv";
}

void hiscore::save_hscpath()
{ int i,num=3;
  QString zeile[num];
  QFile fh("/sheep/hiscore.ini"); 
  if (fh.open(IO_ReadOnly)) { for (i=0; i<=num; i++) fh.readLine(zeile[i],254); fh.close(); }  
  zeile[0]=zeile[0].setNum(hscpfad_id)+"\n";
  if (fh.open(IO_WriteOnly)) { for (i=0; i<num; i++) fh.writeBlock(zeile[i],zeile[i].length()); fh.close(); }
}

// =========================================================================================================
