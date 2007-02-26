/*
 * \brief   DOpE pSLIM server module
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * The  pSLIM server  provides an IDL based  interface
 * to DOpE clients that  are using  pSLIM widgets. The
 * operations are applied immediately by instrumenting
 * the graphics operations of the pSLIM widget.
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <stdio.h>   /* !!! should maybe be kicked out !!! */

/*** FLICK/L4-SPECIFIC ***/
#include <l4/names/libnames.h>
#include <l4/lock/lock.h>
#include "pslim-server.h"

/*** DOPE SPECIFIC ***/
#include "../include/dopestd.h"
#include "../include/thread.h"
#include "../include/pslim.h"
#include "../include/pslim_server.h"

static struct thread_services *thread;

static s16 thread_started;

static u8 *ident_tab;       /* pslim server identifier table */
static s32 max_idents=100;  /* max. nb of concurrent pslim servers */

int init_pslim_server(struct dope_services *d);



/**********************************/
/*** FUNCTIONS FOR INTERNAL USE ***/
/**********************************/

static void pslim_server_thread(void *pslim_widget) {
	CORBA_Environment dice_env = dice_default_environment;

	int i;
	char ident_buf[10];
	l4thread_set_prio(l4thread_myself(),l4thread_get_prio(l4thread_myself())-5);

	/* find free identifier for this pslim server */
	for (i=0;(i<max_idents) && (ident_tab[i]);i++);
	if (i<max_idents-1) {
		sprintf(ident_buf,"DpE%d",i);
		ident_tab[i]=1;
	} else {
		/* if there are not enough identifiers, exit the server thread */
		thread_started=1;
		return;
	}

	printf("PSLIMserver(server_thread): ident_buf=%s\n",ident_buf);
	if (!names_register(ident_buf)) return;
	((PSLIM *)pslim_widget)->pslim->reg_server((PSLIM *)pslim_widget,ident_buf);

	dice_env.user_data = pslim_widget;
	thread_started=1;

//  printf("Hello I am here!\n");
//  printf("l4_myself = %x.%x\n", l4_myself().id.task, l4_myself().id.lthread);
	dope_pslim_server_loop(&dice_env);
}



/******************************************************/
/*** FUNCTIONS THAT ARE CALLED BY THE SERVER THREAD ***/
/******************************************************/


/*** PSLIM CORE FUNCTIONS ***/


long dope_pslim_fill_component(CORBA_Object _dice_corba_obj,
                                     const pslim_rect_t *rect,
                                     pslim_color_t color,
                                     CORBA_Environment *_dice_corba_env) {
	PSLIM *p = (PSLIM *)_dice_corba_env->user_data;
	return p->pslim->fill(p,rect,color);
}


long dope_pslim_copy_component(CORBA_Object _dice_corba_obj,
                                     const pslim_rect_t *rect,
                                     short dx,
                                     short dy,
                                     CORBA_Environment *_dice_corba_env) {
	PSLIM *p = (PSLIM *)_dice_corba_env->user_data;
	return p->pslim->copy(p,rect,dx,dy);
}


long dope_pslim_bmap_component(CORBA_Object _dice_corba_obj,
                                     const pslim_rect_t *rect,
                                     pslim_color_t fg_color,
                                     pslim_color_t bg_color,
                                     const unsigned char *bmap,
                                     long bmap_size,
                                     long bmap_type,
                                     CORBA_Environment *_dice_corba_env) {
	PSLIM *p = (PSLIM *)_dice_corba_env->user_data;
	return p->pslim->bmap(p,rect,fg_color,bg_color,(char*)bmap,bmap_type);
}


long dope_pslim_set_component(CORBA_Object _dice_corba_obj,
                                    const pslim_rect_t *rect,
                                    const char* pmap,
                                    int pmap_size,
                                    CORBA_Environment *_dice_corba_env) {
	PSLIM *p = (PSLIM *)_dice_corba_env->user_data;
	return p->pslim->set(p,rect,(char*)pmap);
}



long dope_pslim_cscs_component(CORBA_Object _dice_corba_obj,
                                     const pslim_rect_t *rect,
                                     const char* yuv,
                                     int yuv_size,
                                     long yuv_type,
                                     char scale,
                                     CORBA_Environment *_dice_corba_env) {
	PSLIM *p = (PSLIM *)_dice_corba_env->user_data;
	return p->pslim->cscs(p,rect,(char*)yuv,yuv_type,scale);
}


/*** PSLIM EXTENSIONS ***/

long dope_pslim_puts_component(CORBA_Object _dice_corba_obj,
                                     const char* s,
                                     short x,
                                     short y,
                                     pslim_color_t fg_color,
                                     pslim_color_t bg_color,
                                     CORBA_Environment *_dice_corba_env) {
	PSLIM *p = (PSLIM *)_dice_corba_env->user_data;
	return p->pslim->puts(p,s,x,y,fg_color,bg_color);
}


long dope_pslim_puts_attr_component(CORBA_Object _dice_corba_obj,
                                          const char* s,
                                          int strattr_size,
                                          short x,
                                          short y,
                                          CORBA_Environment *_dice_corba_env) {
	PSLIM *p = (PSLIM *)_dice_corba_env->user_data;
	*(((char *)s) + strattr_size)=0;
	*(((char *)s) + strattr_size + 1)=0;
	return p->pslim->puts_attr(p,s,x,y);
}



/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

/*** START SERVING ***/
static THREAD *start(PSLIM *pslim_widget) {
	THREAD *new;

	thread_started=0;

	printf("pSLIMserver(start): pslim_widget at addr. %x\n",(int)pslim_widget);

	INFO(printf("PSLIMServer(start): creating server thread\n");)
	new=thread->create_thread(&pslim_server_thread,(void *)pslim_widget);
	while (!thread_started) l4_sleep(1);
	return new;
}



/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct pslim_server_services services = {
	start,
};



/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_pslim_server(struct dope_services *d) {
	s32 i;

	thread = d->get_module("Thread 1.0");

	/* init identifier flag table */
	ident_tab = malloc(sizeof(s8)*max_idents);
	for (i=0;i<max_idents;i++) ident_tab[i]=0;

	d->register_module("PSLIMServer 1.0",&services);
	return 1;
}
