/***********************************************
Levels fuer schafe/Lämmerlinge (c) uja
***********************************************/

#include "config.h"
#include <qfile.h>
#include <qstring.h>

#ifndef LEVELS
#define LEVELS

// Einzellevel:
class leveldata
{ 
  public:
  QString titel;
  QString levelcode;
  int ri0,zeit0,pfeile0,moskito,mostyp;
  int f0[SMAX];
  void set_level(QString*);
  
};

// Gesamtheit der Levels:
class level
{ 
  public:
  level::level(QString levelfile);
  leveldata* my_level[MAXLEVELS];
  int anz_levels;

};

#endif
