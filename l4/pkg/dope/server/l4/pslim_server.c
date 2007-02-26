/*
 * \brief	DOpE pSLIM server module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 *
 * The  pSLIM server  provides an IDL based  interface
 * to DOpE clients that  are using  pSLIM widgets. The
 * operations are applied immediately by instrumenting
 * the graphics operations of the pSLIM widget.
 */

#include <stdio.h>	/* !!! should be kicked out !!! */

/*** FLICK/L4-SPECIFIC ***/
#include <l4/names/libnames.h>
#include <l4/lock/lock.h>
#include "pslim-server.h"

/*** DOPE SPECIFIC ***/
#include "../include/dope-config.h"
#include "../include/memory.h"
#include "../include/thread.h"
#include "../include/pslim.h"
#include "../include/pslim_server.h"

#define STRBUF_SIZE			256*1024

#define NOTIFY_TIMEOUT		L4_IPC_NEVER
#define BE_NOTIFIED_TIMEOUT	L4_IPC_NEVER

#define WAIT_TIMEOUT		L4_IPC_TIMEOUT(0,1,0,0,0,0)	
#define FIRSTWAIT_TIMEOUT	L4_IPC_NEVER
#define REPLY_TIMEOUT		L4_IPC_NEVER
#define RECEIVE_TIMEOUT		L4_IPC_NEVER
#define REPRCV_TIMEOUT		L4_IPC_NEVER

static struct memory_services *mem;
static struct thread_services *thread;

static s16 thread_started;

static u8 *ident_tab;		/* pslim server identifier table */
static s32 max_idents=100;	/* max. nb of concurrent pslim servers */

int init_pslim_server(struct dope_services *d);



/**********************************/
/*** FUNCTIONS FOR INTERNAL USE ***/
/**********************************/

//static char strbuf[STRBUF_SIZE];	/* receive buffer for strdopes */

static void pslim_server_thread(void *pslim_widget) {
	CORBA_Environment dice_env = dice_default_environment;
	
	int i;
//	l4_msgdope_t result;
//	sm_request_t request;
//  	l4_ipc_buffer_t ipc_buf;
	char ident_buf[10];
//
//	flick_init_request(&request, &ipc_buf);
//	flick_server_set_timeout(&request, WAIT_TIMEOUT);
//	flick_server_set_rcvstring(&request, 0, STRBUF_SIZE, strbuf);
//	flick_set_number_rcvstring(&request, 1);
//
	l4thread_set_prio(l4thread_myself(),l4thread_get_prio(l4thread_myself())-5);
//
//	/* pass this_vc reference as implicit arg */
//	flick_server_set_local(&request, pslim_widget);

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
	
//	/* IDL server loop */
//	while (1) {
//		result = flick_server_wait(&request);
//
//		while (!L4_IPC_IS_ERROR(result)) {
//			/* dispatch request */
//			ret = dope_pslim_server(&request);
//
//			switch(ret) {
//			case DISPATCH_ACK_SEND:
//				/* reply and wait for next request */
//				result = flick_server_reply_and_wait(&request);
//				break;
//				
//			default:
//				/* wait for next request */
//				result = flick_server_wait(&request);
//				break;
//			}
//		} /* !L4_IPC_IS_ERROR(result) */
//	}
//	printf("pSLIMserver(server_thread): entering serverloop of widget %x\n",
//					(int)dice_env.user_data);
//
	printf("Hello I am here!\n");
	printf("l4_myself = %x.%x\n", l4_myself().id.task, l4_myself().id.lthread);
	dope_pslim_server_loop(&dice_env);
//	dope_pslim_server_loop(NULL);
}



/******************************************************/
/*** FUNCTIONS THAT ARE CALLED BY THE SERVER THREAD ***/
/******************************************************/


/*** PSLIM CORE FUNCTIONS ***/


CORBA_long dope_pslim_fill_component(CORBA_Object *_dice_corba_obj,
                                     pslim_rect_t *rect,
                                     pslim_color_t color,
                                     CORBA_Environment *_dice_corba_env) {
//l4_int32_t dope_pslim_server_fill(sm_request_t *request, const pslim_rect_t *rect, 
//								pslim_color_t color, sm_exc_t *_ev) {
	PSLIM *p = (PSLIM *)_dice_corba_env->user_data;
//	printf("pSLIMserver(fill): fill function called for widget %x\n",(int)p);
//(PSLIM *) flick_server_get_local(request); 

//	printf("pSLIMserver(fill): xywh = (%d,%d,%d,%d)\n",
//					(int)rect->x,(int)rect->y,(int)rect->w,(int)rect->h);
//	printf("address of pslim_fill = %x\n",(int)p->pslim->fill);
	return p->pslim->fill(p,rect,color);
//	printf("pSLIMserver(fill): fill function finished.\n");
//	return 0;
}


CORBA_long dope_pslim_copy_component(CORBA_Object *_dice_corba_obj,
                                     pslim_rect_t *rect,
                                     CORBA_short dx,
                                     CORBA_short dy,
                                     CORBA_Environment *_dice_corba_env) {
//l4_int32_t dope_pslim_server_copy(sm_request_t *request, const pslim_rect_t *rect, 
//								l4_int16_t dx, l4_int16_t dy, sm_exc_t *_ev) {
	PSLIM *p = (PSLIM *)_dice_corba_env->user_data;
//	(PSLIM *) flick_server_get_local(request); 
	return p->pslim->copy(p,rect,dx,dy);
}


CORBA_long dope_pslim_bmap_component(CORBA_Object *_dice_corba_obj,
                                     pslim_rect_t *rect,
                                     pslim_color_t fg_color,
                                     pslim_color_t bg_color,
                                     CORBA_byte *bmap,
									 CORBA_long bmap_size,
                                     CORBA_long bmap_type,
                                     CORBA_Environment *_dice_corba_env) {
//l4_int32_t dope_pslim_server_bmap(sm_request_t *request, const pslim_rect_t *rect, 
//								pslim_color_t fg_color, pslim_color_t bg_color, 
//								l4_strdope_t bmap, l4_uint8_t bmap_type, sm_exc_t *_ev){
	PSLIM *p = (PSLIM *)_dice_corba_env->user_data;
//	PSLIM *p = (PSLIM *) flick_server_get_local(request);   
	return p->pslim->bmap(p,rect,fg_color,bg_color,bmap,bmap_type);
}


CORBA_long dope_pslim_set_component(CORBA_Object *_dice_corba_obj,
                                    pslim_rect_t *rect,
                                    CORBA_char_ptr pmap,
									CORBA_long pmap_size,
                                    CORBA_Environment *_dice_corba_env) {
//l4_int32_t dope_pslim_server_set(sm_request_t *request, const pslim_rect_t *rect, 
//								l4_strdope_t pmap, sm_exc_t *_ev) {
	PSLIM *p = (PSLIM *)_dice_corba_env->user_data;
//	PSLIM *p = (PSLIM *) flick_server_get_local(request);   
//	printf("pSLIMserver(set): xywh = %d,%d,%d,%d, pmap = %x\n",
//			(int)rect->x, (int)rect->y, (int)rect->w, (int)rect->h, (int)pmap);
	return p->pslim->set(p,rect,pmap);	
}



CORBA_long dope_pslim_cscs_component(CORBA_Object *_dice_corba_obj,
                                     pslim_rect_t *rect,
                                     CORBA_char_ptr yuv,
									 CORBA_long yuv_size,
                                     CORBA_long yuv_type,
                                     CORBA_char scale,
                                     CORBA_Environment *_dice_corba_env) {
//l4_int32_t dope_pslim_server_cscs(sm_request_t *request, const pslim_rect_t *rect, 
//								l4_strdope_t yuv, l4_uint8_t yuv_type, char scale, 
//								sm_exc_t *_ev) {
	PSLIM *p = (PSLIM *)_dice_corba_env->user_data;
//	PSLIM *p = (PSLIM *) flick_server_get_local(request);   
	return p->pslim->cscs(p,rect,yuv,yuv_type,scale);	
}


/*** PSLIM EXTENSIONS ***/

CORBA_long dope_pslim_puts_component(CORBA_Object *_dice_corba_obj,
                                     CORBA_char_ptr s,
                                     CORBA_short x,
                                     CORBA_short y,
                                     pslim_color_t fg_color,
                                     pslim_color_t bg_color,
                                     CORBA_Environment *_dice_corba_env) {
//l4_int32_t dope_pslim_server_puts(sm_request_t *request, l4_strdope_t s, 
//								l4_int16_t x, l4_int16_t y, pslim_color_t fg_color, 
//								pslim_color_t bg_color, sm_exc_t *_ev) {
	PSLIM *p = (PSLIM *)_dice_corba_env->user_data;
//	PSLIM *p = (PSLIM *) flick_server_get_local(request);   
	return p->pslim->puts(p,s,x,y,fg_color,bg_color);	
}


CORBA_long dope_pslim_puts_attr_component(CORBA_Object *_dice_corba_obj,
                                          CORBA_char_ptr s,
										  CORBA_long strattr_size,
                                          CORBA_short x,
                                          CORBA_short y,
                                          CORBA_Environment *_dice_corba_env) {
//l4_int32_t dope_pslim_server_puts_attr(sm_request_t *request, l4_strdope_t s, 
//									 l4_int16_t x, l4_int16_t y, sm_exc_t *_ev) {
	PSLIM *p = (PSLIM *)_dice_corba_env->user_data;
//	PSLIM *p = (PSLIM *) flick_server_get_local(request); 
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
	
	DOPEDEBUG(printf("PSLIMServer(start): creating server thread\n");)
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

	mem    = d->get_module("Memory 1.0");
	thread = d->get_module("Thread 1.0");

	/* init identifier flag table */
	ident_tab = mem->alloc(sizeof(s8)*max_idents);
	for (i=0;i<max_idents;i++) ident_tab[i]=0;

	d->register_module("PSLIMServer 1.0",&services);
	return 1;
}
