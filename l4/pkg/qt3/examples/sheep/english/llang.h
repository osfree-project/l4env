/****************************************************************************************
    Language dependent part of Lämmerlinge/LittleSheep which runs under GPL
    Copyright 2004 uja
*****************************************************************************************/
#ifndef PROGNAME

#define PROGNAME  "Leveleditor LittleSheep"
// editor.cpp:
#define _LEVEL      "Level        "
#define _LLADEN     "Load level"
#define _LSPEICHERN "Save level"
#define _LINSERT    "Insert level before"
#define _LAPPEND    "Append level"
#define _LLOESCHEN  "Delete level"
#define _QUIT       "&Quit"

#define _TOOLS     "Tools        "
#define _LCLEAN    "Clear meadow"
#define _FILLTOP   "Fill top row with leftmost field"
#define _FILLRIGHT "Fill right column with top field"
#define _FILLBOTTOM "Fill bottom row with leftmost field"
#define _FILLLEFT  "Fill left column with top field"
#define _FTOTOP    "Move fields to the top"
#define _FTORIGHT  "Move fields to the right"
#define _FTOBOTTOM "Move fields to the bottom"
#define _FTOLEFT   "Move fields to the left"

#define _INFO     "Infos"
#define _ABOUT    "About"
#define _HELP     "Help"

// lwindow.cpp: -----------------------------------------------
#define _TITEL     "Leveleditor LittleSheep" // Programmtitel
#define _NIX       " "
#define _LNAME     "Levelname:"
#define _LPFEILE   "Num.Signs:"
#define _LZEIT     "Time (s):"
#define _ITEMS     "Field types"
#define _SCHAFRI   "Start direction sheep"
#define _TOP       "top"
#define _BOTTOM    "bottom"
#define _LEFT      "left"
#define _RIGHT     "right"
#define _MOSKIRI   "Moskito fly in deg from horizontal"

#define _NO_LEVEL  "No level loaded!"
#define _GELADEN   "loaded: level "
#define _LLEVEL    "level "
#define _NLEVEL    "--- new level ---"
#define _APPEND_OK "level appended"
#define _SAVE_OK   "saved: level "
#define _INSERT_OK "level inserted before "
#define _LOESCH_OK "deleted: level "

#define _ERRLNAME  "Error: no levelname given!"
#define _ERRZEIT   "Error: invalid time (should be 10 - 999)!"
#define _ERRSRI    "Error: no start direction for sheep!"
#define _ERRPFEILE "Error: no signs!"

// gamelib-Module: gleich für lang und llang: ---------------------------
// module hiscore:
#define TIMEFORM   "MM.dd.yyyy hh:mm:ss"            // time format
#define _HSCMESS0   "Congrats, you got a score of " // Eintragstexte
#define _HSCMESS1   "!\n "
#define _HSCMESS2   "Please enter your name:"
#define _CONFRESET  "Are you sure you want to reset the highscore list?"
#define _YES       "yes"
#define _NO        "no"
#define _CLOSE     "close"

// files
#define LHELPFILE  "/sheep/lhelp.rtf"
#define HELPFILE   "/sheep/help.rtf"
#define INFOFILE   "/sheep/about.rtf"  

#endif
