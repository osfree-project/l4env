/*
 * \brief	Landscape effect
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 * 
 * based on a source code of Andrea Griffini
 */

/*** GENERAL INCLUDES ***/
#include <math.h>

/*** DOpE SPECIFIC INCLUDES ***/
#include "dope-config.h"
#include <dopelib.h>
#include <vscreen.h>

/*** LOCAL INCLUDES ***/
#include "voxel.h"

#define KEY_UP    103
#define KEY_LEFT  105
#define KEY_RIGHT 106
#define KEY_DOWN  108

static u8 HMap[256*256];   /* Height field */
static u8 CMap[256*256];   /* Color map */
static u8 Video[320*220];  /* Off-screen buffer */

static u16 coltab[64];
static s16 scr_w = 320;
static s16 scr_h = 200;
static u16 *scr_adr = NULL;

static void *voxvscr_id;
extern long app_id;

static int i;
static float ss=50*1024,sa,a=1.4,s=1024*4;
static int xpos0,ypos0;
static int check_keys_flag = 1;


/*** PSEUDO RANDOM VALUE GENERATOR ***/
static unsigned int SEED = 93186752;
static int rand (void)  {
   static unsigned int a = 1588635695, m = 4294967291U, q = 2, r = 1117695901;
   SEED = a*(SEED % q) - r*(SEED / q);
   return (int)(0x0000ffff*(((double)SEED / (double)m)));
}


/* Reduces a value to 0..255 (used in height field computation) */
static int Clamp(int x) {
  return (x<0 ? 0 : (x>255 ? 255 : x));
}


/* Heightfield and colormap computation */
static void ComputeMap(void) {
	int p,i,j,k,k2,p2;

	/* Start from a plasma clouds fractal */
	HMap[0]=128;
	for ( p=256; p>1; p=p2 ) {
		p2=p>>1;
		k=p*8+20; k2=k>>1;
		for ( i=0; i<256; i+=p ) {
			for ( j=0; j<256; j+=p ) {
				int a,b,c,d;
				a=HMap[(i<<8)+j];
				b=HMap[(((i+p)&255)<<8)+j];
				c=HMap[(i<<8)+((j+p)&255)];
				d=HMap[(((i+p)&255)<<8)+((j+p)&255)];
				
				HMap[(i<<8)+((j+p2)&255)] = Clamp(((a+c)>>1)+(rand()%k-k2));
				HMap[(((i+p2)&255)<<8)+((j+p2)&255)] = Clamp(((a+b+c+d)>>2)+(rand()%k-k2));
				HMap[(((i+p2)&255)<<8)+j] = Clamp(((a+b)>>1)+(rand()%k-k2));
			}
		}
	}

	/* Smoothing */
	for ( k=0; k<3; k++ ) 
	for ( i=0; i<256*256; i+=256 ) 
	for ( j=0; j<256; j++ ) {
		HMap[i+j]=(HMap[((i+256)&0xFF00)+j]+HMap[i+((j+1)&0xFF)]+
		           HMap[((i-256)&0xFF00)+j]+HMap[i+((j-1)&0xFF)])>>2;
	}

	/* Color computation (derivative of the height field) */
	for ( i=0; i<256*256; i+=256 )
	for ( j=0; j<256; j++ ) {
		k = 128+(HMap[((i+256)&0xFF00)+((j+1)&255)]-HMap[i+j])*4;
		if ( k<0 ) k=0; if (k>255) k=255;
		CMap[i+j]=k;
	}
}



static int  lasty[320],         /* Last pixel drawn on a given column */
            lastc[320];         /* Color of last pixel on a column */

/*
   Draw a "section" of the landscape; x0,y0 and x1,y1 and the xy coordinates
   on the height field, hy is the viewpoint height, s is the scaling factor
   for the distance. x0,y0,x1,y1 are 16.16 fixed point numbers and the
   scaling factor is a 16.8 fixed point value.
 */
static void Line(int x0,int y0,int x1,int y1,int hy,int s) {
  int i,sx,sy;

  /* Compute xy speed */
  sx=(x1-x0)/320; sy=(y1-y0)/320;
  for ( i=0; i<320; i++ )
  {
    int c,y,h,u0,v0,u1,v1,a,b,h0,h1,h2,h3;

    /* Compute the xy coordinates; a and b will be the position inside the
       single map cell (0..255).
     */
    u0=(x0>>16)&0xFF;    a=(x0>>8)&255;
    v0=((y0>>8)&0xFF00); b=(y0>>8)&255;
    u1=(u0+1)&0xFF;
    v1=(v0+256)&0xFF00;

    /* Fetch the height at the four corners of the square the point is in */
    h0=HMap[u0+v0]; h2=HMap[u0+v1];
    h1=HMap[u1+v0]; h3=HMap[u1+v1];

    /* Compute the height using bilinear interpolation */
    h0=(h0<<8)+a*(h1-h0);
    h2=(h2<<8)+a*(h3-h2);
    h=((h0<<8)+b*(h2-h0))>>16;

    /* Fetch the color at the four corners of the square the point is in */
    h0=CMap[u0+v0]; h2=CMap[u0+v1];
    h1=CMap[u1+v0]; h3=CMap[u1+v1];

    /* Compute the color using bilinear interpolation (in 16.16) */
    h0=(h0<<8)+a*(h1-h0);
    h2=(h2<<8)+a*(h3-h2);
    c=((h0<<8)+b*(h2-h0));

    /* Compute screen height using the scaling factor */
    y=(((h-hy)*s)>>11)+100;

    /* Draw the column */
    if ( y<(a=lasty[i]) )
    {
      unsigned char *b=Video+a*320+i;
      int sc,cc;


      if ( lastc[i]==-1 )
	lastc[i]=c;

      sc=(c-lastc[i])/(a-y);
      cc=lastc[i];

      if ( a>199 ) { b-=(a-199)*320; cc+=(a-199)*sc; a=199; }
      if ( y<0 ) y=0;
      while ( y<a )
      {
	*b=cc>>18; cc+=sc;
	b-=320; a--;
      }
      lasty[i]=y;

    }
    lastc[i]=c;

    /* Advance to next xy position */
    x0+=sx; y0+=sy;
  }
}


float FOV=3.141592654/4;   /* half of the xy field of view */

/*
// Draw the view from the point x0,y0 (16.16) looking at angle a
*/
static void View(int x0,int y0,float aa) {
  int d;
  int a,b,h,u0,v0,u1,v1,h0,h1,h2,h3;

  /* Clear offscreen buffer */
  memset(Video,0,320*200);

  /* Initialize last-y and last-color arrays */
  for ( d=0; d<320; d++ )
  {
    lasty[d]=200;
    lastc[d]=-1;
  }

  /* Compute viewpoint height value */

  /* Compute the xy coordinates; a and b will be the position inside the
     single map cell (0..255).
   */
  u0=(x0>>16)&0xFF;    a=(x0>>8)&255;
  v0=((y0>>8)&0xFF00); b=(y0>>8)&255;
  u1=(u0+1)&0xFF;
  v1=(v0+256)&0xFF00;

  /* Fetch the height at the four corners of the square the point is in */
  h0=HMap[u0+v0]; h2=HMap[u0+v1];
  h1=HMap[u1+v0]; h3=HMap[u1+v1];

  /* Compute the height using bilinear interpolation */
  h0=(h0<<8)+a*(h1-h0);
  h2=(h2<<8)+a*(h3-h2);
  h=((h0<<8)+b*(h2-h0))>>16;

  /* Draw the landscape from near to far without overdraw */
  for ( d=0; d<100; d+=1+(d>>6) )
  {
    Line(x0+d*65536*cos(aa-FOV),y0+d*65536*sin(aa-FOV),
         x0+d*65536*cos(aa+FOV),y0+d*65536*sin(aa+FOV),
         h-30,100*256/(d+1));
  }

  /* Blit the final image to the screen */
  {
    int row,col;
    u8 *src;
	u16 *dst;

    src = Video;
    dst = scr_adr;
    for ( row=scr_h; row>0; --row )
    {
	
	  for ( col = scr_w; col>0; --col ) {
	  	*(dst++) = coltab[*(src++)];
	  }
    }
  }
}


/*** INITIALISATION OF THE EFFECT - MUST BE CALLED DURING THE START UP ***/
void voxel_init(void) {
	
	/* open window with rt-widget */
	dope_cmd(app_id, "voxwin=new Window()" );
	dope_cmd(app_id, "voxvscr=new VScreen()" );
	dope_cmd(app_id, "voxvscr.setmode(320,200,16)" );
	dope_cmd(app_id, "voxvscr.set(-framerate 25)" );
	dope_cmd(app_id, "voxwin.set(-x 500 -y 150 -w 330 -h 227 -fitx yes -fity yes -content voxvscr -background off)" );

	/* map vscreen buffer to local address space */
	scr_adr = vscr_get_fb( dope_cmd(app_id, "voxvscr.map()") );
	
	/* get identifier of VScreen-server */
	voxvscr_id = vscr_get_server_id(dope_cmd(app_id,"voxvscr.getserver()"));
	
	/* Set up the first 64 colors to a grayscale */
	for ( i=0; i<64; i++ ) coltab[i]= ((i&0xfe) << 10) + (i<<5) + (i>>1);

	/* Compute the height map */
	ComputeMap();
	printf("VScrTest(voxel_init): done\n");
};



/*** MAIN EFFECT ROUTINE - MUST BE CALLED PERIODICALLY ***/
void voxel_exec(exec_flag) {

	vscr_server_waitsync(voxvscr_id);

	if (!exec_flag) return;

	/* update position/angle */
	xpos0+=ss*cos(a); ypos0+=ss*sin(a);
	a+=sa;

	/* slowly reset the angle to 0 */
	if ( sa != 0 ) {
		if ( sa < 0 ) sa += 0.001*4;
		else sa -= 0.001*4;
	}
	
    /* draw the frame */
	View(xpos0,ypos0,a);

	if (check_keys_flag) {
		if (dope_get_keystate(app_id, KEY_UP)) ss+=s;
		if (dope_get_keystate(app_id, KEY_DOWN)) ss-=s;
		if (dope_get_keystate(app_id, KEY_RIGHT)) sa+=0.003*4;
		if (dope_get_keystate(app_id, KEY_LEFT)) sa-=0.003*4;
	
	}
}
