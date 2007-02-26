#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

#include "SDL.h"


#define PROGNAME "ux_con"

#define DFL_MOUSE_VISIBLE 1
#define DFL_REFRESH_INTERVAL 40

#define WM_WINDOW_TITLE "Fiasco/UX graphical console"
#define WM_ICON_TITLE   "F/UX con"

/* this is from libinput.h  --  size == 16 byte */
typedef struct
{
  long long time;		/* used as a marker here */
  unsigned short type;
  unsigned short code;
  unsigned int value;
} l4input_t;

enum {
  SUPERPAGESIZE = 1 << 22,
};

/* needs to be the same as in libinput-ux! */
enum {
  INPUTMEM_SIZE = 1 << 12,
  NR_INPUT_OBJS = INPUTMEM_SIZE / sizeof(l4input_t),
};

#define EV_KEY		0x01
#define EV_REL		0x02

#define BTN_LEFT	0x110
#define BTN_RIGHT	0x111

int physmem_fd;
unsigned long physmem_fb_start;
int width, height, depth;
size_t physmem_size;
size_t fb_size;
int depth_bytes;
int refresh_rate;
int mouse_visible;
l4input_t *input_mem;
int input_queue_pos;

struct key_mapping {
  unsigned long sdlkey;
  unsigned long l4ev;
};

/*
 * This table is a bit confusing and not all keys a here, so add keys as you
 * need them.
 */
static struct key_mapping key_map[] = {
  { SDLK_BACKSPACE,    14 },
  { SDLK_TAB,          15 },
  { SDLK_RETURN,       28 },
  { SDLK_PAUSE,        119 },
  { SDLK_ESCAPE,       1 },
  { SDLK_SPACE,        57 },
  { SDLK_COMMA,        51 },
  { SDLK_MINUS,        53 },
  { SDLK_PERIOD,       52 },
  { SDLK_SLASH,        53 },
  { SDLK_EQUALS,       13 },
  { SDLK_CARET,        41 },
  { SDLK_1,            2 },
  { SDLK_2,            3 },
  { SDLK_3,            4 },
  { SDLK_4,            5 },
  { SDLK_5,            6 },
  { SDLK_6,            7 },
  { SDLK_7,            8 },
  { SDLK_8,            9 },
  { SDLK_9,            10 },
  { SDLK_0,            11 },
  { SDLK_SEMICOLON,    39 },
  { SDLK_QUESTION,     214 },
  { SDLK_BACKSLASH,    43 },
  { SDLK_a,            30 },
  { SDLK_b,            48 },
  { SDLK_c,            46 },
  { SDLK_d,            32 },
  { SDLK_e,            18 },
  { SDLK_f,            33 },
  { SDLK_g,            34 },
  { SDLK_h,            35 },
  { SDLK_i,            23 },
  { SDLK_j,            36 },
  { SDLK_k,            37 },
  { SDLK_l,            38 },
  { SDLK_m,            50 },
  { SDLK_n,            49 },
  { SDLK_o,            24 },
  { SDLK_p,            25 },
  { SDLK_q,            16 },
  { SDLK_r,            19 },
  { SDLK_s,            31 },
  { SDLK_t,            20 },
  { SDLK_u,            22 },
  { SDLK_v,            47 },
  { SDLK_w,            17 },
  { SDLK_x,            45 },
  { SDLK_y,            44 },
  { SDLK_z,            21 },
  { SDLK_DELETE,       111 },

  /* Arrows + Home/End pad */
  { SDLK_UP,           103 },
  { SDLK_DOWN,         108 },
  { SDLK_RIGHT,        106 },
  { SDLK_LEFT,         105 },
  { SDLK_INSERT,       110 },
  { SDLK_HOME,         102 },
  { SDLK_END,          107 },
  { SDLK_PAGEUP,       104 },
  { SDLK_PAGEDOWN,     109 },

  /* Function keys */
  { SDLK_F1,           59 },
  { SDLK_F2,           60 },
  { SDLK_F3,           61 },
  { SDLK_F4,           62 },
  { SDLK_F5,           63 },
  { SDLK_F6,           64 },
  { SDLK_F7,           65 },
  { SDLK_F8,           66 },
  { SDLK_F9,           67 },
  { SDLK_F10,          68 },
  { SDLK_F11,          87 },
  { SDLK_F12,          88 },
  { SDLK_F13,          85 },
  { SDLK_F14,          89 },
  { SDLK_F15,          90 },

  /* Modifiers */
  { SDLK_RSHIFT,       54 },
  { SDLK_LSHIFT,       42 },
  { SDLK_RCTRL,        97 },
  { SDLK_LCTRL,        29 },
  { SDLK_RALT,         100 },
  { SDLK_LALT,         56 },
  { SDLK_RMETA,        100 },
  { SDLK_LMETA,        56 },
  { SDLK_MODE,         100 }, /* Alt Gr */

  { 0, 0},
};

static unsigned long map_keycode(SDLKey sk)
{
  unsigned int i;

  for (i = 0; key_map[i].sdlkey; i++)
    if (key_map[i].sdlkey == sk)
      return key_map[i].l4ev;

  printf("%s: Unknown key pressed/released: #%d (%s)\n",
         PROGNAME, sk, SDL_GetKeyName(sk));
  return 0;
}

void usage(char *prog)
{
  printf("%s options\n\n"
         "  -f fd        File descriptor to mmap\n\n"
         "  -s 0x...     Start of FB in physmem\n"
         "  -x val       Width of screen in pixels\n"
         "  -y val       Height of screen in pixels\n"
         "  -d val       Color depth in bits\n"
         "  -r val       Refresh rate in ms\n"
	 "  -m           Flip mouse visibility\n"
	 "  -F           Switch to fullscreen mode\n"
         ,prog);
}


Uint32 timer_call_back(Uint32 interval, void *param)
{
  static SDL_Event se = { .type = SDL_USEREVENT };

  (void)param;

  SDL_PushEvent(&se);

  return interval;
}

static inline void generate_irq(void)
{
  if (write(0, "I", 1) == -1) {
    printf("%s: Communication problems with Fiasco/UX, dying...!\n",
	   PROGNAME);
    exit(0);
  }
} 

static inline int enqueue_event(l4input_t e)
{
  l4input_t *p = input_mem + input_queue_pos;

  if (p->time) {
    printf("Ringbuffer overflow, don't type/move too fast!\n");
    return 1;
  }

  e.time = 1;	/* we misuse the time field for a status */
  *p = e;

  input_queue_pos++;
  if (input_queue_pos == NR_INPUT_OBJS)
    input_queue_pos = 0;
  return 0;
}
 
static void propagate_event(l4input_t e)
{
  if (!enqueue_event(e))
    generate_irq();
}

static void loop(SDL_Surface *screen, void *fbmem)
{
  SDL_Event e;
  SDL_TimerID timer_id;

  if ((timer_id = SDL_AddTimer(refresh_rate, timer_call_back, NULL)) == NULL) {
    fprintf(stderr, "%s: Adding of timer failed!", PROGNAME);
    exit(1);
  }

  while (SDL_WaitEvent(&e)) {
    l4input_t l4e = { .time = 0, .code = 0, .type = 0, .value = 0 };

    switch (e.type) {
      case SDL_KEYUP:
      case SDL_KEYDOWN:
	l4e.value = e.type == SDL_KEYDOWN;
	l4e.type = EV_KEY;
	l4e.code = map_keycode(e.key.keysym.sym);

	//printf("%s: sdlkey = %d  l4e.code = %d\n", __func__, e.key.keysym.sym, l4e.code);

	propagate_event(l4e);

	break;

      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
	l4e.value = e.type == SDL_MOUSEBUTTONDOWN;
	l4e.type = EV_KEY;

	switch (e.button.button) {
	  case SDL_BUTTON_LEFT:
	          l4e.code = BTN_LEFT;
	          break;
          case SDL_BUTTON_RIGHT:
		  l4e.code = BTN_RIGHT;
		  break;
	  default:
		  l4e.code = 0;
	}

	propagate_event(l4e);

	break;

      case SDL_MOUSEMOTION:
	{
	  int ret = 1;

	  l4e.type = EV_REL;
	  if (e.motion.xrel) {
	    l4e.code = 0; // y motion
	    l4e.value = e.motion.xrel;
	    ret = enqueue_event(l4e);
	  }
	  if (e.motion.yrel) {
	    l4e.code = 1; // x motion
	    l4e.value = e.motion.yrel;
	    ret |= enqueue_event(l4e);
	  }
	  if (!ret)
	    generate_irq();
	}

	break;

      case SDL_ACTIVEEVENT: /* Window (non-)active */
	break;

      case SDL_QUIT:
	exit(0);

      case SDL_USEREVENT:
	/* Redraw screen */
	if (SDL_LockSurface(screen) < 0)
	  break;
	memcpy(screen->pixels, fbmem, fb_size);
	SDL_UnlockSurface(screen);
	SDL_Flip(screen);
	break;

      default:
	printf("%s: Unknown event: %d\n", PROGNAME, e.type);
	break;
    }

  }

}

int main(int argc, char **argv)
{
  int c;
  void *fbmem;
  SDL_Surface *screen;
  unsigned int sdl_video_flags = 0; /* SDL_DOUBLEBUF; */

  refresh_rate = DFL_REFRESH_INTERVAL;
  mouse_visible = DFL_MOUSE_VISIBLE;

  while ((c = getopt(argc, argv, "f:x:y:s:d:mF")) != -1) {
    switch (c) {
      case 'f':
	physmem_fd = atoi(optarg);
	break;
      case 's':
	physmem_fb_start = atol(optarg);
	break;
      case 'x':
	width = atoi(optarg);
	break;
      case 'y':
	height = atoi(optarg);
	break;
      case 'd':
	depth = atoi(optarg);
	break;
      case 'r':
	refresh_rate = atoi(optarg);
	break;
      case 'm':
	mouse_visible = !mouse_visible;
	break;
      case 'F':
	sdl_video_flags |= SDL_FULLSCREEN;
	break;
      default:
	usage(*argv);
	break;
    }
  }

  if (!physmem_fd || !physmem_fb_start || !width || !height || !depth ||
      !refresh_rate) {
    fprintf(stderr, "%s: Invalid arguments!\n", PROGNAME);
    usage(*argv);
    exit(1);
  }

  printf("Frame buffer resolution of UX-con: %dx%d@%d, refresh: %dms\n",
         width, height, depth, refresh_rate);

  depth_bytes = ((depth + 7) >> 3);
  fb_size = width * height * depth_bytes;

  physmem_size = ((fb_size & ~(SUPERPAGESIZE - 1))) + SUPERPAGESIZE;

  if ((fbmem = mmap(NULL, physmem_size + INPUTMEM_SIZE, 
	            PROT_READ | PROT_WRITE, MAP_SHARED,
	            physmem_fd, physmem_fb_start)) == MAP_FAILED) {
    fprintf(stderr, "%s: mmap failed: %s\n",
	    PROGNAME, strerror(errno));
    exit(1);
  }

  input_mem = (l4input_t *)((char *)fbmem + physmem_size);

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
    fprintf(stderr, "%s: Can't init SDL: %s\n",
	    PROGNAME, SDL_GetError());
    exit(1);
  }
  atexit(SDL_Quit);

  screen = SDL_SetVideoMode(width, height, depth, sdl_video_flags);
  if (screen == NULL) {
    fprintf(stderr, "%s: Couldn't init video mode: %s\n",
	    PROGNAME, SDL_GetError());
    exit(1);
  }

  SDL_WM_SetCaption(WM_WINDOW_TITLE, WM_ICON_TITLE);

  SDL_ShowCursor(mouse_visible);

  loop(screen, fbmem);

  return 0;
}
