/*
 * \brief   DDEUSB client library - IPC wrapper and D_URB management
 * \date    2009-04-07
 * \author  Dirk Vogt <dvogt@os.inf.tu-dresden.de>
 */

/*
 * This file is part of the DDEUSB package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */





#include "libddeusb_local.h"

/* the thread ID of the our notification server */
static l4_threadid_t libddeusb_main_server_thread = L4_INVALID_ID;

/* the thread ID of the DDEUSB server */
static l4_threadid_t ddeusb_server_thread = L4_INVALID_ID;

/* the client's  callback functions */
static struct libddeusb_callback_functions *cbfuncs = NULL;


/* all shared DDEUSB URBs (array!!) */
static ddeusb_urb *_d_urbs = NULL;
static int  _d_urbs_length = 0; 


/* the following are the lists for DDEUSB URB management */
/* all free DDEUSB URBs */
static ddeusb_urb *free_d_urbs = NULL;

 
/* for now a global lock should be enough */
static l4lock_t list_lock;

void*  (*libddeusb_malloc) (l4_size_t)=NULL;
void  (*libddeusb_free) (void * ) =NULL;

static void libddeusb_memcpy(const void *_dst,const  void *_src, l4_size_t size) {
    char *src = (char *)_src;
    char *dst = (char *)_dst;
    
    while (size--) {
		*dst++=*src++;
    }

}

/* tries to get the thread_id of the DDEUSB server thread.
 * will sleep until thread_id found.
 * TODO: add a timeout
 */
inline l4_threadid_t * libddeusb_get_server_thread(void) {
	if (l4_thread_equal(ddeusb_server_thread,L4_INVALID_ID ))
	{
		while (l4_thread_equal(ddeusb_server_thread,L4_INVALID_ID )) {
			names_query_name(USB_NAMESERVER_NAME, &ddeusb_server_thread);
			l4_sleep(100);
		}
	}
	return &ddeusb_server_thread;
}

/* subscribe for device id IPC wrapper */

int L4_CV libddeusb_subscribe_for_device_id (ddeusb_usb_device_id *id) 
{
	CORBA_Environment env= dice_default_environment;
	l4_threadid_t * server = libddeusb_get_server_thread(); 
	if (!l4_thread_equal(*server,L4_INVALID_ID)) {
		/* we have to be registered at core before we register */
		while (l4_thread_equal(libddeusb_main_server_thread, L4_INVALID_ID))
			l4_sleep(100);
		return ddeusb_core_subscribe_for_device_id_call (server, id, &env);
	}
	return -1;
}


void L4_CV libddeusb_unlink_d_urb(ddeusb_urb *d_urb) {
    /* TODO: implement me! */
}


/* 
 * At first we have to check what kind of DDEUSB URB it is. In case of an 
 * fallback urb we have to add the urb to the wait_complete list, and submit
 * it. Otherwise it just has to be submitted.
 */
int L4_CV libddeusb_submit_d_urb(ddeusb_urb *d_urb) {
	CORBA_Environment env= dice_default_environment;
    int ret;
    l4_threadid_t * server = libddeusb_get_server_thread(); 
	if (!l4_thread_equal(*server, L4_INVALID_ID)) {

		/* TODO: for now we use fallback urbs for configuration messages.
		 *       should be no problem to use d_urbs as long the config
		 *       word is copied (and thus not in shared mem).
		 *
		 *       however, some work has to be done on server side to
		 *       get in work\dots
		 */
		
		if (d_urb->type ==  DDEUSB_CTL) {
			
			return ddeusb_core_ddeusb_submit_urb_call(server, 
                                                   (int) d_urb, 
                                                   d_urb->dev_id,
                                                   d_urb->type, 
                                                   d_urb->direction,
                                                   d_urb->endpoint, 
                                                   d_urb->transfer_flags,
                                                   d_urb->interval,
                                                   d_urb->start_frame,
                                                   d_urb->number_of_packets,
                                                   (void *)d_urb->setup_packet,
                                                   d_urb->data,
                                                   d_urb->size,
                                                   d_urb->iso_desc,
                                                   d_urb->iso_desc_size,
                                                   &env);
		}


        switch (d_urb->d_urb_type)
        {
         
            case DDEUSB_D_URB_TYPE_FALLBACK:
                return ddeusb_core_ddeusb_submit_urb_call(server, 
                                                   (int) d_urb, 
                                                   d_urb->dev_id,
                                                   d_urb->type, 
                                                   d_urb->direction, 
                                                   d_urb->endpoint,
                                                   d_urb->transfer_flags,
                                                   d_urb->interval,
                                                   d_urb->start_frame,
                                                   d_urb->number_of_packets,
                                                   (void *)d_urb->setup_packet,
                                                   d_urb->data,
                                                   d_urb->size,
                                                   d_urb->iso_desc,
                                                   d_urb->iso_desc_size,
                                                   &env);
        

            case DDEUSB_D_URB_TYPE_AUTO:
       
				d_urb->d_urb_type = DDEUSB_D_URB_TYPE_PHYS;
		
#if 0  
                if(d_urb->size < DDEUSB_D_URB_COPY_BUF_SIZE)
                {
                    libddeusb_memcpy(d_urb->buf,d_urb->data,d_urb->size);

                    d_urb->d_urb_type = DDEUSB_D_URB_TYPE_COPY;  
                } 
                else 
                {
                    l4_threadid_t dummy_t;
                    l4_addr_t dummy_a;

                    ret=l4rm_lookup(d_urb->data,
                                    &dummy_a, &d_urb->size,
                                    &d_urb->ds, &d_urb->offset,
                                    &dummy_t);

                    l4dm_share(&d_urb->ds, libddeusb_get_server_id(), L4DM_RW);
                    
                    d_urb->d_urb_type= DDEUSB_D_URB_TYPE_SHM;
                 }
#endif

			case DDEUSB_D_URB_TYPE_SHM:
			case DDEUSB_D_URB_TYPE_COPY:
			case DDEUSB_D_URB_TYPE_PHYS:
				return ddeusb_core_ddeusb_ds_submit_urb_call(server, d_urb->index, &env);
            default: 
				return -1;	
	    }
    }
    return -1;
}


l4_threadid_t libddeusb_get_server_id() {
    l4_threadid_t * server = libddeusb_get_server_thread();
    return *server;
}


void L4_CV
ddeusb_gadget_register_device_component (CORBA_Object _dice_corba_obj,
                                         int dev_id,
                                         int speed,
                                         unsigned short bus_mA,
                                         unsigned long long dma_mask,
                                         CORBA_Server_Environment *_dice_corba_env){

	if (cbfuncs)
	         cbfuncs->reg_dev(dev_id, speed, bus_mA, dma_mask);
  	
}

void L4_CV
ddeusb_gadget_disconnect_component (CORBA_Object _dice_corba_obj,
                                    int dev_id,
                                    CORBA_Server_Environment *_dice_corba_env)
{
	if (cbfuncs)
		cbfuncs->discon_dev(dev_id);
}

void L4_CV
ddeusb_gadget_ds_urb_complete_component (CORBA_Object _dice_corba_obj,
                                    int id,
                                    CORBA_Server_Environment *_dice_corba_env)
{  
	ddeusb_urb *d_urb = &_d_urbs[id];


	if (d_urb->d_urb_type==DDEUSB_D_URB_TYPE_COPY && d_urb->direction) {
            libddeusb_memcpy(d_urb->data, d_urb->buf, d_urb->actual_length);
    }

	if (cbfuncs) {
		cbfuncs->complete_d_urb(&_d_urbs[id]);
	}
}

void L4_CV
ddeusb_gadget_urb_complete_component (CORBA_Object _dice_corba_obj,
                                      int urb_id,
                                      int status,
                                      int actual_length,
                                      int interval,
                                      const void * transfer_buffer,
                                      l4_size_t tb_length,
                                      const void * setup_packet,
                                      unsigned int transfer_flags,
                                      int error_count,
                                      int start_frame,
                                      const char *iso_frame_desc,
                                      l4_size_t iso_frame_desc_size,
                                      CORBA_Server_Environment *_dice_corba_env)
{
    
	
	/* get the d_urb */
	
    ddeusb_urb* d_urb     = (ddeusb_urb *) urb_id;

	d_urb->status         = status;
    d_urb->actual_length  = actual_length;
    d_urb->interval       = interval;
    d_urb->transfer_flags = transfer_flags;
    d_urb->error_count    = error_count;
    d_urb->start_frame    = start_frame;
	

    if (d_urb->direction) {
		libddeusb_memcpy(d_urb->data, transfer_buffer, tb_length);
		libddeusb_memcpy(d_urb->setup_packet, setup_packet, 8);
	} 
	
	libddeusb_memcpy(d_urb->iso_desc, iso_frame_desc, iso_frame_desc_size);
    
    if (cbfuncs) {
		cbfuncs->complete_d_urb(d_urb);
	}
	
}



void L4_CV libddeusb_register_callbacks(struct libddeusb_callback_functions *callbacks)
{
	
	cbfuncs = callbacks;	
}

/* link_durbs together in list and store the index of the each DDEUSB URB 
 * in the _d_urb array, to find it fast in the case of URB completion */
static L4_CV void init_d_urbs (ddeusb_urb * d_urbs, int count) 
{
    int i;

    /* init lock */
    list_lock = L4LOCK_UNLOCKED;
    
    l4lock_lock(&list_lock);
    
    _d_urbs        = d_urbs;
    _d_urbs_length = count;

    /* first element */
    free_d_urbs = d_urbs;
    

    d_urbs[0].index = 0;
    
    for(i=1;i<count-1;i++) {
        d_urbs[i-1].next=&d_urbs[i];
		d_urbs[i].index=i;
    }
    d_urbs[i].next=NULL;
    l4lock_unlock(&list_lock);
}

/* allocates a new DDEUSB URB of type type.
 * if the type is not DDEUSB_D_URB_TYPE_FALLBACK the function will try to get
 * a URB out of the shared memory region, if this is not possible or the type
 * should be DDEUSB_D_URB_TYPE_FALLBACK it will allocate a new one
 **/
ddeusb_urb * libddeusb_alloc_d_urb(int type) {
    ddeusb_urb * ret = NULL;

    switch(type)
    {
        default:
        
            l4lock_lock(&list_lock);

			ret = free_d_urbs;

			if (ret) {
                free_d_urbs = ret->next;
                ret->next   = NULL; 
            }
        
            l4lock_unlock(&list_lock);

                
            if (ret) {
				ret->d_urb_type = type;
                break;
			}

        case DDEUSB_D_URB_TYPE_FALLBACK:
        
            ret = libddeusb_malloc(sizeof(ddeusb_urb));
            ret->d_urb_type = DDEUSB_D_URB_TYPE_FALLBACK;
			LOG("allocated new d_urb at %p\n", ret);

    }
    
	return ret;
}

/*
 * frees a DDEUSB URB. (see above...)
 */
void L4_CV libddeusb_free_d_urb(ddeusb_urb *d_urb) {
    switch(d_urb->d_urb_type)
    {
        default:
            l4lock_lock(&list_lock);
            
            if (free_d_urbs) {
                d_urb->next= free_d_urbs; 
            }
            
            free_d_urbs = d_urb;

            l4lock_unlock(&list_lock);
        
            return;

        case DDEUSB_D_URB_TYPE_FALLBACK:
            libddeusb_free(d_urb);
    }
}


/* TODO: split this in init and loop */
int L4_CV libddeusb_server_loop(char * name, void* (*_malloc)(l4_size_t), void (*_free)(void *) , ddeusb_urb *d_urbs, int count) {
	
	CORBA_Environment env     = dice_default_environment;
	CORBA_Server_Environment srv_env = dice_default_server_environment;
    int ret;

    l4dm_dataspace_t ds;
    l4_addr_t dummy_a;
    l4_offs_t offset;
    l4_threadid_t dummy_t;
    l4_size_t dummy_s;
    l4_size_t size = sizeof(ddeusb_urb) * count;

    LOG("d_urbs @ %p", d_urbs);
    init_d_urbs(d_urbs,count);

    libddeusb_malloc = _malloc;
    libddeusb_free = _free;

    srv_env.malloc = (dice_malloc_func ) _malloc;
    srv_env.free   = (dice_free_func  ) _free;
	l4_threadid_t * server = libddeusb_get_server_thread();
	l4_threadid_t me = l4_myself();
	
	ddeusb_core_register_call (server, name, &me, &env);

    ret=l4rm_lookup(d_urbs,&dummy_a,&dummy_s,&ds,&offset,&dummy_t);
    
    l4dm_share(&ds,*server,L4DM_RW);
    
    ddeusb_core_ddeusb_setup_control_ds_call(server,&ds,offset,size,&env);
	
    libddeusb_main_server_thread = me;
	
	ddeusb_gadget_server_loop(&srv_env);

	return 0;
}

