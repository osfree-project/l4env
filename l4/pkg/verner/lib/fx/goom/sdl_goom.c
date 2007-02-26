//#include "config.h"

#include <SDL.h>
#include <SDL_thread.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "goom_tools.h"
#include "goom_core.h"
#include "pixeldoubler.h"
#include "sdl_pixeldoubler.c"
#include "readme.c"

/* cr7 */
#include <l4/thread/thread.h>

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#define gchar unsigned char

#define MAX_FRAMERATE (32)
#define INTERPIX (1000/MAX_FRAMERATE);

/* external interface */
#include "goom.h"

SDL_Surface *surface = NULL;
static int is_fs = 0;

static void thread_func (void);

static void xgoom_init (void);
static void xgoom_cleanup (void);
static void xgoom_render_pcm (gint16 data[2][512]);

SDL_Thread *thread;
SDL_mutex *acces_mutex;

static int resx = INIT_SCREEN_W;
static int resy = INIT_SCREEN_H;

static int doublepix = 0;
static int doubledec = 0;
static Surface *gsurf2 = NULL;
static Surface gsurf;

int fini = 0;

#ifndef STANDALONE
gchar *title = NULL;
int title_changed = 0;
#endif


#ifdef STANDALONE
#include <l4/log/l4log.h>
#include <l4/util/macros.h>
l4_ssize_t l4libc_heapsize = (5 * 1024 * 1024);
char LOG_tag[9] = "goom";
int
main (void)
{
  jeko_init ();
  while (!fini)
    l4thread_sleep (1000);
  jeko_cleanup ();
  return 0;
}
#endif

int disable = 0;

int
jeko_init (void)
{
#ifdef VERBOSE
  printf ("init\n");
#endif
  title = NULL;
  fini = FALSE;
  is_fs = 0;
  resx = INIT_SCREEN_W;
  resy = INIT_SCREEN_H;
  doublepix = 0;
  doubledec = 0;
  disable = FALSE;
  xgoom_init ();
  acces_mutex = SDL_CreateMutex ();
  thread = SDL_CreateThread ((void *) thread_func, NULL);
  return 0;
}

gint16 snd_data[2][512];

// retourne x>>s , en testant le signe de x
#define ShiftRight(_x,_s) ((_x<0) ? -(-_x>>_s) : (_x>>_s))

static void
thread_func ()
{
  Uint32 tnext;
  static gint16 prev0 = 0;
  int i, j;

  tnext = SDL_GetTicks () + INTERPIX;

  while (!fini)
  {
    Uint32 t;
    xgoom_render_pcm (snd_data);
    SDL_mutexP (acces_mutex);
    if (prev0 == snd_data[0][0])
    {
      for (i = 0; i < 2; i++)
	for (j = 0; j < 512; j++)
	  snd_data[i][j] = ShiftRight ((snd_data[i][j] * 31), 5);
    }
    prev0 = snd_data[0][0];
    SDL_mutexV (acces_mutex);
    t = SDL_GetTicks ();
    if (t < tnext)
    {
      //usleep((tnext-t)*1000);
      l4thread_sleep ((tnext - t));
      tnext += INTERPIX;
    }
    else
    {
      tnext = t + INTERPIX;
    }
  }
}

void
jeko_cleanup (void)
{
  SDL_mutexP (acces_mutex);
  fini = TRUE;
  SDL_mutexV (acces_mutex);
  SDL_WaitThread (thread, NULL);

  xgoom_cleanup ();
  if (is_fs)
  {
    SDL_WM_ToggleFullScreen (surface);
  }
  resize_win (resx, resy);

  SDL_Quit ();

  if (title)
    free (title);
  title = NULL;    
}


void
jeko_set_text (char *text)
{
  title = strdup (text);
  title_changed = 1;
}


void
jeko_render_pcm (gint16 data[2][512])
{
  SDL_mutexP (acces_mutex);
  memcpy (snd_data, data, 2048);
  SDL_mutexV (acces_mutex);

//  if (disable)
  // jeko_vp.disable_plugin (&jeko_vp);
}

/*======================*/
/*===============================*/

static void
apply_double (void)
{

  if (gsurf2)
    surface_delete (&gsurf2);
  if (!doublepix)
    return;

  if (surface->format->BytesPerPixel == 4)
    doublepix = 2;
  else
    doublepix = 1;

  if (doublepix == 2)
  {
    resx /= 2;
    resy /= 2;
    doubledec = 0;
  }
  else if (doublepix == 1)
  {
    doubledec = resx % 32;
    resx = resx - doubledec;
    resx /= 2;
    resy /= 2;
    doubledec /= 2;
    gsurf2 = surface_new (resx * 2, resy * 2);
  }

  gsurf.width = resx;
  gsurf.height = resy;
  gsurf.size = resx * resy;
}

static void
xgoom_init (void)
{
  gint16 data[2][512];
  int i;

  SDL_Init (SDL_INIT_VIDEO);
  surface = SDL_SetVideoMode (resx, resy, 32, SDL_RESIZABLE | SDL_HWSURFACE);
  SDL_WM_SetCaption ("What A Goom!", NULL);
  SDL_ShowCursor (0);
  SDL_EnableKeyRepeat (0, 0);

#ifdef VERBOSE
  printf ("--> INITIALIZING GOOM\n");
#endif
/*
	for (i = 0; i < 0x100; i++) {
		char    str[255];

		if (i < 10)
			sprintf (str, DATADIR "/goom/font/00%d.png", i);
		else if (i < 100)
			sprintf (str, DATADIR "/goom/font/0%d.png", i);
		else
			sprintf (str, DATADIR "/goom/font/%d.png", i);
		
		if (loadpng (str, &pngw[i], &pngh[i], &pngbuf[i])) {
			pngbuf[i] = NULL;
		}
	}
	for (i = 0; i < 0x100; i++) {
		if (pngbuf[i] == NULL) {
			pngbuf[i] = pngbuf[42];
			pngw[i] = pngw[42];
			pngh[i] = pngh[42];
		}
	}
*/
  apply_double ();
  goom_init (resx, resy, 0);
//      goom_set_font ((int***)pngbuf, pngw, pngh);

  for (i = 0; i < 512; i++)
  {
    data[0][i] = 0;
    data[1][i] = 0;
  }

  //framerate_tester_init ();
}

static void
xgoom_cleanup (void)
{
#ifdef VERBOSE
  printf ("--> CLEANUP GOOM\n");
#endif
  goom_close ();
  //framerate_tester_close ();
}

static char *
resize_win (int w, int h)
{
  static SDL_Event e;
  static char s[256];
  e.resize.type = SDL_VIDEORESIZE;
  e.resize.w = w;
  e.resize.h = h;
  SDL_PushEvent (&e);
  sprintf (s, "%dx%d", w, h);
  return s;
}

static void
xgoom_render_pcm (gint16 data[2][512])
{
  static char *msg_tab[] = {
    "What a GOOM! version " VERSION
      "\n\n\n\n\n\n\n\n"
      "an iOS sotfware production.\n" "\n\n\n" "http://ios.free.fr",
    goom_readme,
    goom_big_readme,
    "copyright (c)2000-2002, by jeko"
  };
  static int msg_pos = 0;
#define ENCORE_NUL_LOCK (32*60)
  static int encore_nul = 0;

  guint32 *buf;
  SDL_Surface *tmpsurf = NULL;

  static int display_fps = 0;

  gchar *message = NULL;

  int forceMode = 0;

#define NBresoli 11
  static int resoli = 7;
  static int resolx[] =
    { 320, 320, 400, 400, 512, 512, 640, 640, 640, 800, 800 };
  static int resoly[] =
    { 180, 240, 200, 300, 280, 384, 320, 400, 480, 400, 600 };

  int i;
  SDL_Event event;

  /* Check for events */
  while (SDL_PollEvent (&event))
  {				/* Loop until there are no events left on 
				 * the queue */
    switch (event.type)
    {				/* Process the appropiate event type */
    case SDL_QUIT:
      {
#ifdef STANDALONE
	fini = 1;
#else
	static gchar *tabmsg[] = {
	  "You can't disable goom from here.",
	  "Use the config dialog please...",
	  "CTRL-V makes it appear easily.",
	  "Hey ?? Are you listening to me ??",
	  "Why don't you like me ?",
	  "Grrrrrrrrrrr !!!",
	  "GRRRRRRRRRRRRRRRRR !!!!",
	  "Stupid user.",
	  "Ok... the next time I'll leave XMMS for you",
	  "Be careful, I'll really do it...",
	  "Bye! "
	};
	static int tmp = 0;

	disable = TRUE;
	if (tmp >= 10)
	  title = strdup (tabmsg[10]);
	else
	  title = strdup (tabmsg[tmp++]);
#endif
      }
      break;

    case SDL_KEYDOWN:		/* Handle a KEYDOWN event */
      if ((event.key.keysym.sym == SDLK_TAB)||(event.key.keysym.sym == SDLK_f))
      {
	SDL_WM_ToggleFullScreen (surface);
	is_fs = !is_fs;
      }

      /* f is for fullscreen - and s is now for show framerate */
      if (event.key.keysym.sym == SDLK_s)
      {
	display_fps = !display_fps;
      }
      
      if ((event.key.keysym.sym == SDLK_KP_PLUS) && (resoli + 1 < NBresoli))
      {
	resoli = resoli + 1;
	resize_win (resolx[resoli], resoly[resoli]);
      }
      if ((event.key.keysym.sym == SDLK_KP_MINUS) && (resoli > 0))
      {
	resoli = resoli - 1;
	resize_win (resolx[resoli], resoly[resoli]);
      }
      /* also d like double is possible */
      if ((event.key.keysym.sym == SDLK_KP_MULTIPLY)||(event.key.keysym.sym == SDLK_d))
      {
	doublepix = !doublepix;
	if (doublepix)
	  resize_win (resx, resy);
	else
	  resize_win (resx * 2, resy * 2);
      }
      if (event.key.keysym.sym == SDLK_ESCAPE)
      {
	message = "";
      }
      if (event.key.keysym.sym == SDLK_SPACE)
      {
	encore_nul = ENCORE_NUL_LOCK;
      }

      if (event.key.keysym.sym == SDLK_F1)
	forceMode = 1;
      if (event.key.keysym.sym == SDLK_F2)
	forceMode = 2;
      if (event.key.keysym.sym == SDLK_F3)
	forceMode = 3;
      if (event.key.keysym.sym == SDLK_F4)
	forceMode = 4;
      if (event.key.keysym.sym == SDLK_F5)
	forceMode = 5;
      if (event.key.keysym.sym == SDLK_F6)
	forceMode = 6;
      if (event.key.keysym.sym == SDLK_F7)
	forceMode = 7;
      if (event.key.keysym.sym == SDLK_F8)
	forceMode = 8;
      if (event.key.keysym.sym == SDLK_F9)
	forceMode = 9;
      if (event.key.keysym.sym == SDLK_F10)
	forceMode = 10;

      break;
    case SDL_VIDEORESIZE:
      resx = event.resize.w;
      resy = event.resize.h;
      // printf ("resize : %d x %d\n",resx,resy);
      surface =
	SDL_SetVideoMode (resx, resy, 32, SDL_RESIZABLE | SDL_HWSURFACE);
      apply_double ();
      goom_set_resolution (resx, resy, 0);
      if (is_fs)
	SDL_WM_ToggleFullScreen (surface);
      break;
//              default:                                                                                /* Report an unhandled event */
      // printf("I don't know what this event is!\n");
    }
  }

  for (i = 0; i < 512; i++)
    if (data[0][i] > 2)
    {
      if (encore_nul > ENCORE_NUL_LOCK)
	encore_nul = 0;
      break;
    }

  if ((i == 512) && (!encore_nul))
    encore_nul = ENCORE_NUL_LOCK + 100;

  if (encore_nul == ENCORE_NUL_LOCK)
  {
    message = msg_tab[msg_pos];
    msg_pos++;
    msg_pos %= 4;
  }

  if (encore_nul)
    encore_nul--;

  
  buf = goom_update (data, forceMode, -1, title, message);

  /* ensure only showing a few secs */
  if(title_changed)
  {
    title_changed = 0;
    if(title) free(title);
    title = NULL;
  }

  if (doublepix == 2)
  {
    gsurf.buf = buf;
    sdl_pixel_doubler (&gsurf, surface);
  }
  else if (doublepix == 1)
  {
    SDL_Rect rect;
    gsurf.buf = buf;
    pixel_doubler (&gsurf, gsurf2);
    tmpsurf =
      SDL_CreateRGBSurfaceFrom (gsurf2->buf, resx * 2, resy * 2,
				32, resx * 8,
				0x00ff0000, 0x0000ff00, 0x000000ff,
				0x00000000);
    rect.x = doubledec;
    rect.y = 0;
    rect.w = resx * 2;
    rect.h = resy * 2;
    SDL_BlitSurface (tmpsurf, NULL, surface, &rect);
    SDL_FreeSurface (tmpsurf);
  }
  else
  {
    tmpsurf =
      SDL_CreateRGBSurfaceFrom (buf, resx, resy, 32, resx * 4,
				0x00ff0000, 0x0000ff00, 0x000000ff,
				0x00000000);
    SDL_BlitSurface (tmpsurf, NULL, surface, NULL);
    SDL_FreeSurface (tmpsurf);
  }
  SDL_Flip (surface);

/*
	printf ("%3.2f fps...\n",framerate_tester_getvalue());
	fflush(stdout);
*/

  //framerate_tester_newframe ();
}
