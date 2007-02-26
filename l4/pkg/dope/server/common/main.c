/*
 * \brief	DOpE entry function
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This file describes the startup of DOpE. It
 * initialises all needed modules and calls the
 * eventloop (see eventloop.c) of DOpE.
 */


#include "dope-config.h"

/*** PROTOTYPES FROM STARTUP.C (IN SYSTEM DEPENDENT DIRECTORY) ***/
extern void native_startup(int argc, char **argv);

/*** PROTOTYPES FROM POOL.C ***/
extern long  pool_add(char *name,void *structure);
extern void *pool_get(char *name);

/*** PROTOTYPES FROM 'MODULES' ***/
extern int init_memory      (struct dope_services *);
extern int init_keymap      (struct dope_services *);
extern int init_clipping    (struct dope_services *);
extern int init_screen      (struct dope_services *);
extern int init_input       (struct dope_services *);
extern int init_basegfx     (struct dope_services *);
extern int init_widman      (struct dope_services *);
extern int init_button      (struct dope_services *);
extern int init_background  (struct dope_services *);
extern int init_container   (struct dope_services *);
extern int init_window      (struct dope_services *);
extern int init_winman      (struct dope_services *);
extern int init_userstate   (struct dope_services *);
extern int init_conv_fnt    (struct dope_services *);
extern int init_fontman     (struct dope_services *);
extern int init_cache       (struct dope_services *);
extern int init_image16data (struct dope_services *);
extern int init_image8idata (struct dope_services *);
extern int init_scrollbar   (struct dope_services *);
extern int init_frame       (struct dope_services *);
extern int init_grid        (struct dope_services *);
extern int init_redraw      (struct dope_services *);
extern int init_rtman       (struct dope_services *);
extern int init_hashtable   (struct dope_services *);
extern int init_thread      (struct dope_services *);
extern int init_tokenizer   (struct dope_services *);
extern int init_script      (struct dope_services *);
extern int init_appman      (struct dope_services *);
extern int init_server      (struct dope_services *);
extern int init_terminal    (struct dope_services *);
extern int init_pslim_server(struct dope_services *);
extern int init_pslim       (struct dope_services *);
extern int init_winlayout   (struct dope_services *);
extern int init_messenger   (struct dope_services *);
extern int init_timer       (struct dope_services *);
extern int init_vscreen     (struct dope_services *);
extern int init_vscr_server (struct dope_services *);
extern int init_sharedmem   (struct dope_services *);

/*** PROTOTYPES FROM EVENTLOOP.C ***/
extern void eventloop(struct dope_services *);

struct dope_services dope = {
	pool_get,
	pool_add,
};

int main(int argc,char **argv) {
	DOPEDEBUG(char *dbg="Main(init): ");

	printf("DOpE(main) started!\n");
	native_startup(argc, argv);
	printf("DOpE(main) native init finished. init DOpE modules...\n");

	/*** init modules ***/
	DOPEDEBUG(printf("%sMemory\n",dbg));
	init_memory(&dope);

	DOPEDEBUG(printf("%sSharedMemory\n",dbg));
	init_sharedmem(&dope);
	
	DOPEDEBUG(printf("%sTimer\n",dbg));
	init_timer(&dope);

	DOPEDEBUG(printf("%sKeymap\n",dbg));
	init_keymap(&dope);

	DOPEDEBUG(printf("%sThread\n",dbg));
	init_thread(&dope);
	
	DOPEDEBUG(printf("%sCache\n",dbg));
	init_cache(&dope);

	DOPEDEBUG(printf("%sHashTable\n",dbg));
	init_hashtable(&dope);

	DOPEDEBUG(printf("%sTokenizer\n",dbg));
	init_tokenizer(&dope);

	DOPEDEBUG(printf("%sApplication Manager\n",dbg));
	init_appman(&dope);

	DOPEDEBUG(printf("%sMessenger\n",dbg));
	init_messenger(&dope);

	DOPEDEBUG(printf("%sScript\n",dbg));
	init_script(&dope);
	
	DOPEDEBUG(printf("%sClipping\n",dbg));
	init_clipping(&dope);
	
	DOPEDEBUG(printf("%sScreen\n",dbg));
	init_screen(&dope);

	DOPEDEBUG(printf("%sInput\n",dbg));
	init_input(&dope);
	
	DOPEDEBUG(printf("%sConvertFNT\n",dbg));
	init_conv_fnt(&dope);
	
	DOPEDEBUG(printf("%sFontManager\n",dbg));
	init_fontman(&dope);
	
	DOPEDEBUG(printf("%sImage16Data\n",dbg));
	init_image16data(&dope);
	
	DOPEDEBUG(printf("%sImage8iData\n",dbg));
	init_image8idata(&dope);
	
	DOPEDEBUG(printf("%sBaseGFX\n",dbg));
	init_basegfx(&dope);
	
	DOPEDEBUG(printf("%sWindowManager\n",dbg));
	init_winman(&dope);
	
	DOPEDEBUG(printf("%sRedrawManager\n",dbg));
	init_redraw(&dope);	

	DOPEDEBUG(printf("%sRealtimeManager\n",dbg));
	init_rtman(&dope);	
	
	DOPEDEBUG(printf("%sUserState\n",dbg));
	init_userstate(&dope);
	
	DOPEDEBUG(printf("%sWidgetManager\n",dbg));
	init_widman(&dope);
	
	DOPEDEBUG(printf("%sButton\n",dbg));
	init_button(&dope);
	
	DOPEDEBUG(printf("%sBackground\n",dbg));
	init_background(&dope);
	
	DOPEDEBUG(printf("%sScrollbar\n",dbg));
	init_scrollbar(&dope);

	DOPEDEBUG(printf("%sFrame\n",dbg));
	init_frame(&dope);
	
	DOPEDEBUG(printf("%sTerminal\n",dbg));
	init_terminal(&dope);

	DOPEDEBUG(printf("%sContainer\n",dbg));
	init_container(&dope);

	DOPEDEBUG(printf("%sGrid\n",dbg));
	init_grid(&dope);
	
	DOPEDEBUG(printf("%sWinLayout\n",dbg));
	init_winlayout(&dope);

	DOPEDEBUG(printf("%sWindow\n",dbg));
	init_window(&dope);

	DOPEDEBUG(printf("%sPSLIMServer\n",dbg));
	init_pslim_server(&dope);

	DOPEDEBUG(printf("%sPSLIM\n",dbg));
	init_pslim(&dope);

	DOPEDEBUG(printf("%sVScreenServer\n",dbg));
	init_vscr_server(&dope);
	
	DOPEDEBUG(printf("%sVScreen\n",dbg));
	init_vscreen(&dope);

	DOPEDEBUG(printf("%sServer\n",dbg));
	init_server(&dope);

	/*** start living... ***/	
	DOPEDEBUG(printf("%sstarting eventloop\n",dbg));
	eventloop(&dope);

	return 0;
}
