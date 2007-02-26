/***********************************************
Levels fuer schafe/Lämmerlinge (c) uja
***********************************************/
// Implementierung:

#include "levels.h"

void leveldata::set_level(QString* buff)
{  QString buff1;
   titel=buff->section(';',0,0);
   buff1=buff->section(';',1,1); ri0=buff1.toInt();   
   buff1=buff->section(';',2,2); zeit0=buff1.toInt();
   buff1=buff->section(';',3,3); pfeile0=buff1.toInt();
   buff1=buff->section(';',4,4); moskito=buff1.toInt();
   buff1=buff->section(';',5,5); mostyp=buff1.toInt();
   buff1=buff->section(';',6);   levelcode=buff1.stripWhiteSpace();
}

// Levels reinziehen, so wie sie sind, decodiert wird spaeter:
level::level(QString levelfile)
{ int gelesen,buffsize=2*SMAX;
  anz_levels=0;
  QString buff;
  QFile fh(levelfile);
  if (fh.open(IO_ReadOnly))
  { do
    { gelesen=fh.readLine(buff,buffsize);
      if (gelesen>SMAX)    
      { 
      my_level[anz_levels]=new leveldata();
        my_level[anz_levels]->set_level(&buff);
	//qDebug("%d: '%s'", buff.length(), buff.latin1());
        anz_levels++;
      }
    } while ((gelesen>-1) && (anz_levels<MAXLEVELS));       
    fh.close();    
  }
}
