/****************************************************************************************
    Copyright 2004 uja
    Game constants of Lämmerlinge/LittleSheep which runs under GPL
*****************************************************************************************/

// files:
#if !defined(Q_OS_DROPS)
#define PROGICON      "./images/schaf_0001.png"       // icon
#define BACKPIX0      "./images/bgr.png"              // background window
#define BACKPIX1      "./images/wiese.png"            // background canvas
#define BACKPIX2      "./images/birke.png"            // background signpost
#define PAPERBILD     "./images/reispapier_weiss.png" // background textview
#define HSCPOTT       "./images/pott.png"             // Medaillen
#define SCHAFSPRITE   "./images/schaf_%1.png"         // sprites
#define PFEILSPRITE   "./images/pfeil_%1.png"
#define MOSKITOSPRITE "./images/wespe_%1.png"
#define CYCLAMSPRITE  "./images/cyclam_%1.png"
#define BGSPRITE      "./images/bg_%1.png"
#define TRANSSPRITE   "./images/transporter.png"
#define GAMEOVERFILE  "./images/gover.png"
#define PAUSEFILE     "./images/pause.png"
#define LEVELFILE     "./levelfile.csv"               // Levelcode
#define ULEVELFILE    "./userlevels.csv"              // Levelcode Usereingabe
#define HSCFILE       "./hiscore.csv"                 // fallback highscore list (local)
#else
#define PROGICON      "schaf_0001.png"       // icon
#define BACKPIX0      "bgr.png"              // background window
#define BACKPIX1      "wiese.png"            // background canvas
#define BACKPIX2      "birke.png"            // background signpost
#define PAPERBILD     "reispapier_weiss.png" // background textview
#define HSCPOTT       "pott.png"             // Medaillen
#define SCHAFSPRITE   "schaf_%1.png"         // sprites
#define PFEILSPRITE   "pfeil_%1.png"
#define MOSKITOSPRITE "wespe_%1.png"
#define CYCLAMSPRITE  "cyclam_%1.png"
#define BGSPRITE      "bg_%1.png"
#define TRANSSPRITE   "transporter.png"
#define GAMEOVERFILE  "gover.png"
#define PAUSEFILE     "pause.png"
#define LEVELFILE     "/sheep/levelfile.csv"               // Levelcode
#define ULEVELFILE    "/sheep/userlevels.csv"              // Levelcode Usereingabe
#define HSCFILE       "/sheep/hiscore.csv"                 // fallback highscore list (local)
#endif

// colors: 
#define BACKGR0   "#445533"   // Aussenfenster Hintergrund
#define COLOR0    "#cccc99"   // Aussenfenster Textfarbe
#define BACKGR1   "#335500"   // Spielfeld Hintergrund
#define COLOR1    "#cccc99"   // Spielfeld Textfarbe
#define BACKGR2   "#ccbb99"   // Anzeigen Hintergrund
#define COLOR2    "#330000"   // Anzeigen Textfarbe 
#define PAPCOLOR0  "#ddcc99"  // Rahmenfarbe HSCListe
#define HICOLOR    "#990011"  // Hiscore-Top-Farbe
#define LOCOLOR    "#003355"  // Hiscore-Textfarbe
#define RANDHCOLOR "#ccdd99"  // Randfarben Spielfeld
#define RANDDCOLOR "#335544"

// sizes:
#define WIN_BMAX 800 // window sizes, optimized 800x600
#define WIN_HMAX 600
#define WIN_BMIN 720
#define WIN_HMIN 480 // min.width HSCList
#define HSCBREITE 540 
#define HELPBREITE 480
#define INFOBREITE 320

#define MAXSPEICHER  8 // Number games to store
#define MAXEINTRAG  20 // max. entries Hiscore list
#define HSCGRENZE    5 // nr of best players to hilite in Hiscore list 

#define MAXLEVELS   99 // maximum levels to store, not the actual num of levels!

#define NUMSCHAFE 24 // num sprite states
#define NUMWESPEN  4
#define NUMCYCLAM  4
#define NUMPFEILE  4
#define NUMBG     12
#define NUMTRANS   1

#define DX 32     // sprite size, playfield offsets    
#define DY 32
#define OFX 32
#define OFY 32
#define XMAX 16   // field dimensions in sprite units
#define YMAX 12
#define SMAX 192  // max. num fields (for events)  
// ---------------------------------------------------------------------------------
