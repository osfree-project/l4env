/*
 * \brief   DDEUSB Server - stub driver + L4env server
 * \date    2009-04-07
 * \author  Dirk Vogt <dvogt@os.inf.tu-dresden.de>
 */

/*
 * This file is part of the DDEUSB package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


#include <linux/slab.h>
#include "usb_local.h"
#include <asm/io.h>
#include <l4/thread/thread.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/dde/ddekit/pgtab.h>

//#define DDEUSB_DEBUG 1

#include "debug.h"

#define DDEUSB_CACHE_URBS 1
#define DDEUSB_DEBUG_URB_CACHE 1

static struct ddeusb_client clients[DDEUSB_MAX_CLIENTS];
static DEFINE_MUTEX(client_lock);
static DEFINE_MUTEX(device_lock);
static LIST_HEAD(device_list);

#ifdef DDEUSB_CACHE_URBS
#define DDEUSB_PRECACHE_URBS 50

/* a simple URB cache... */
static DEFINE_MUTEX(urbcache_lock);
static LIST_HEAD(nonisourb_cache_list);
static LIST_HEAD(isourb_cache_list);

static void ddeusb_completion_handler(struct urb *urb);
static void ddeusb_rescan_devices(void);
static void ddeusb_rescan_devices_work_function(struct work_struct* work);
static void ddeusb_isostream_destroy(struct ddeusb_iso_stream * stream);

static inline struct urb * ddeusb_alloc_urb(int nof)
{
	struct urb * ret=NULL;
	struct urb *pos;

	mutex_lock(&urbcache_lock);

	/*
	 * is it an iso urb?
	 */
	if(nof) {
		
		/*
		 * are there free urbs ? 
		 */
		if(!list_empty(&isourb_cache_list)) 
		{
			/*
			 * do we have a suitable urb cached? 
			 */
			list_for_each_entry (pos, &isourb_cache_list, urb_list)
			{
				struct ddeusb_urb_completion_handler_context* ctx = pos->context;
				if (ctx->number_of_frames == nof)
				{
					ret = pos;
					list_del(&ret->urb_list);
					goto DONE;
				}
			}
		}

	} else {
		
		/*
		 * it's not a iso urb... do we have free urbs?
		 */
		if (!list_empty(&nonisourb_cache_list)) 
		{
			ret = list_entry(nonisourb_cache_list.next,struct urb, urb_list);
			list_del(&ret->urb_list);
			goto DONE;
		}

	}

	/* we did not found a suitable urb so create a new! */
	ret = usb_alloc_urb(nof,GFP_KERNEL);

	if(!ret) 
	{
		LOG("OUT OF MEM!");
		goto DONE;
	}

	ret->context= (struct ddeusb_urb_completion_handler_context*) 
	              kmalloc(sizeof(struct ddeusb_urb_completion_handler_context),
				  GFP_KERNEL);

	if(!ret->context)
	{
		usb_free_urb(ret);
		LOG("OUT OF MEM!");
		goto DONE;
	}

	((struct ddeusb_urb_completion_handler_context*) ret->context)->number_of_frames=nof;

#ifdef DDEUSB_DEBUG_URB_CACHE
	static int buffered_urbs;
	DEBUG_MSG("number of urbs in cache: %d, %d ", ++buffered_urbs, nof);
#endif


DONE:

	mutex_unlock(&urbcache_lock);
	return ret;
}

static inline void ddeusb_free_urb(struct urb * urb)
{
	struct ddeusb_urb_completion_handler_context* ctx = urb->context;
	
	mutex_lock(&urbcache_lock);

	if(ctx->number_of_frames)
	{
		list_add(&urb->urb_list, &isourb_cache_list);
	} 
	else 
	{
		list_add(&urb->urb_list, &nonisourb_cache_list);
	}

	mutex_unlock(&urbcache_lock);
}

#endif /* DDEUSB_CACHE_URBS */

void ddeusb_init_client_list (void)
{
	int i;
	for ( i=0 ; i < DDEUSB_MAX_CLIENTS; i++){
		DEBUG_MSG("init client[%d]",i);
		memset(&clients[i],0,sizeof (struct ddeusb_client));
		mutex_init(&clients[i].lock);
		clients[i].th= L4_INVALID_ID;
		clients[i].status=CLIENT_UNUSED;
	}
}

static inline void ddeusb_get_client(struct ddeusb_client *client) 
{
	atomic_inc(&client->ref);
}


static inline void ddeusb_put_client(struct ddeusb_client *client) 
{
	atomic_dec(&client->ref);
}

/*
 * searches in the client list for a client with a specific task_id.
 *
 * returns pointer an ddeusb_client structure if client found otherwise NULL.
 */

inline struct ddeusb_client * ddeusb_get_client_by_task_id(l4_threadid_t t){
	int i;
	
	/*
	 * lock the client DB
	 */

	mutex_lock(&client_lock);

	/*
	 * search client in client DB
	 */

	for (i=0 ; i < DDEUSB_MAX_CLIENTS; i++)
	{
		if (clients[i].status == CLIENT_ACTIVE 
			&& l4_task_equal(t, clients[i].th))
		{
			/*
			 * found it:
			 * - inc ref
			 * - unlock client DB
			 * - return client
			 */
			 
 			atomic_inc(&clients[i].ref);
			mutex_unlock(&client_lock);
			return &clients[i];
		}
	}

	/*
	 * did not find it
	 */
	mutex_unlock(&client_lock);

	return NULL;
}



/*
 * if a client client deregisters from server following things have do be done:
 * - don't accept any calls anymore from this client
 * - unbind the driver from any device (if bound)
 * - cleanup the client structure
 */
int
ddeusb_core_deregister_component (CORBA_Object _dice_corba_obj,
                                  CORBA_Server_Environment *_dice_corba_env)
{
	struct ddeusb_client *client;
	int ret = 0;
	int i;
	/* we are working on the client list so lock it */

	/* is the client registered ? */

	if( (client = ddeusb_get_client_by_task_id(*_dice_corba_obj)))
	{

		DEBUG_MSG("client %s unregistering", client->name);


		/*
		 * don't accept any calls anymore from this client 
		 */
		mutex_lock(&client->lock);
		client->status = CLIENT_CLEANUP;
		mutex_lock(&client->lock);

		/* the rest is done in a workqueue refcount */

		schedule_work(&client->unregister_work);

		ddeusb_put_client(client);

	}
	
	return 0;
}

/*
 * In this work_function all pending urbs of a client will be killed, every
 * binding of this client will be undone and the device will be reseted.
 * Last but not least the l4_thread_id will be nulled, which signals that this
 * client_structure is ready for reuse.
 */
void ddeusb_client_unregister_work_func(struct work_struct *work)
{
	struct ddeusb_client *client = container_of(work, struct ddeusb_client, unregister_work);
	int i;
	
	/* check if urblist is empty */
	/* if it is it will be in futute because client isn't acitve anymore */
	/* if not kill every pending urb */
	
	while (!list_empty(&client->urb_list))
	{
		struct urb *urb=NULL;

		mutex_lock(&client->lock);

		if (!list_empty(&client->urb_list))
		{
			struct ddeusb_client_urb_entry *ue = list_entry(client->urb_list.next,
			                                                struct ddeusb_client_urb_entry,
															list);
			urb = usb_get_urb( ue->urb);
		}

		mutex_unlock(&client->lock);

		if(urb)
		{
			usb_kill_urb(urb);
#ifdef DDEUSB_CACHE_URBS
			ddeusb_free_urb(urb);
#else
			usb_free_urb(urb);
#endif
		}

	}

	/*
	 * wait for synchronous calls 
	 */

	while(atomic_read(&client->ref)) {
		msleep_interruptible(10);
	}

	/* 
	 * now every urb is killed, so it is save to clean all client<->device
	 * references, i.e. to unbind the device 
	 * no need to lock here because no new devices will be added 
	 */

	for (i=0 ; i < DDEUSB_CLIENT_MAX_DEVS ; i++ )
	{
		struct ddeusb_dev *dev=NULL ;

		if ( (dev = client->dev[i]) )
		{
			/*
			 * now device should really be ready to unlink... 
			 */
			client->dev[i]=NULL;
			dev->client=NULL;

			struct usb_device *udev = interface_to_usbdev(dev->intf);

			usb_lock_device(udev);
			usb_reset_configuration(udev);
			usb_unlock_device(udev);

		}

	}

	/*
	 * reset the cient DB's Entry of this client
	 */
	mutex_lock(&client->lock);

	client->th=L4_INVALID_ID;
	client->status = CLIENT_UNUSED;
	
	mutex_unlock(&client->lock);

}




/**
 * compares two device ids and returns 1 if they are equal otherwise 0
 * ...stolen^W inspired from linux, because it doesn't want us to use his one
 * (declared static)
 */

static int ddeusb_id_equal(const struct usb_interface *interface,
                           const struct usb_device_id * id)
{
	struct usb_host_interface *intf;
	struct usb_device *dev;

	/*
	 * proc_connectinfo in devio.c may call us with id == NULL.
	 */
	if (id == NULL)
		return 0;

	intf = interface->cur_altsetting;
	dev = interface_to_usbdev(interface);

	if ((id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
	    id->idVendor != le16_to_cpu(dev->descriptor.idVendor))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
	    id->idProduct != le16_to_cpu(dev->descriptor.idProduct))
		return 0;

	/* 
	 * No need to test id->bcdDevice_lo != 0, since 0 is never greater than any
	 * unsigned number. 
	 */
	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_LO) &&
	    (id->bcdDevice_lo > le16_to_cpu(dev->descriptor.bcdDevice)))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_HI) &&
	    (id->bcdDevice_hi < le16_to_cpu(dev->descriptor.bcdDevice)))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_CLASS) &&
	    (id->bDeviceClass != dev->descriptor.bDeviceClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_SUBCLASS) &&
	    (id->bDeviceSubClass!= dev->descriptor.bDeviceSubClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_PROTOCOL) &&
	    (id->bDeviceProtocol != dev->descriptor.bDeviceProtocol))
		return 0;

	/* 
	 * The interface class, subclass, and protocol should never be
	 * checked for a match if the device class is Vendor Specific,
	 * unless the match record specifies the Vendor ID. 
	 * */

	if (dev->descriptor.bDeviceClass == USB_CLASS_VENDOR_SPEC &&
	    !(id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
	    (id->match_flags & (USB_DEVICE_ID_MATCH_INT_CLASS |
	                        USB_DEVICE_ID_MATCH_INT_SUBCLASS |
	                        USB_DEVICE_ID_MATCH_INT_PROTOCOL)))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_CLASS) &&
	    (id->bInterfaceClass != intf->desc.bInterfaceClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_SUBCLASS) &&
	    (id->bInterfaceSubClass != intf->desc.bInterfaceSubClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_PROTOCOL) &&
	    (id->bInterfaceProtocol != intf->desc.bInterfaceProtocol))
		return 0;

	return 1;
}


/**
 * add a new client to the client list
 *
 * the caller hast to hold the client list lock
 * returns pointer on new client structure on success otherwise NULL
 */
static inline struct ddeusb_client * ddeusb_get_new_client(l4_threadid_t th, const char * name)
{
	int i;
	mutex_lock(&client_lock);
	/*
	 * find a free seat for our client 
	 */
	for (i=0 ; i < DDEUSB_MAX_CLIENTS ; i++)
	{

		if(  clients[i].status== CLIENT_UNUSED)
		{
			memset(&clients[i],0,sizeof (struct ddeusb_client));
			mutex_init(&clients[i].lock);
			clients[i].name=name;
			clients[i].th = th;
			clients[i].status = CLIENT_ACTIVE;
			INIT_LIST_HEAD(&clients[i].ids);
			INIT_LIST_HEAD(&clients[i].urb_list);
			INIT_WORK(&clients[i].unregister_work,ddeusb_client_unregister_work_func);
			atomic_inc(&clients[i].ref);
			mutex_unlock(&client_lock);
			return &clients[i];
		}
	}
	mutex_unlock(&client_lock);
	return NULL;
}

/*
 * Called whenever a new client wants to register at our server.
 * We havce to check if this client is yet registered, if so return an error.
 * If not we can add the client if there is space left in the client list.
 */
int
ddeusb_core_register_component (CORBA_Object _dice_corba_obj,
                                const char* name,
                                const l4_threadid_t * gadget_server,
                                CORBA_Server_Environment *_dice_corba_env)
{
	int ret;

	DEBUG_MSG("Client is trying to register...\n");

	if (!ddeusb_get_client_by_task_id(*_dice_corba_obj)) 
	{
		/* 
		 * the client does not exist yet in our database
		 * so we create a new one
		 */
		if ((ddeusb_get_new_client(*gadget_server, name))) 
		{
			
			/*
			 * success 
			 */

			DEBUG_MSG("New client registered: %s calling from %x.%x\n",name,
			          _dice_corba_obj->id.task, _dice_corba_obj->id.lthread);

			ret = 0;

		}
		else 
		{
			/* 
			 * there was no space left in the client list 
			 */

			DEBUG_MSG("Clientlist full\n");
			ret = -ENOMEM;
		}

		/*
		 * no put_client here... will be done in deregister function! 
		 */
	}
	else
	{
		DEBUG_MSG("Client tries to register twice\n");
		ret = -EACCES;
	}
	return ret;
}

/*
 * Called whenever a clients who is registered at the server wants to register for an
 * USB deviceID
 *
 * one can implement several policies here (e.g. one can not register for an id if another
 * client had done it before, or only special tasks can register for special IDs).
 * for now a client, if it is registered at server side,  allways can register for an id.
 */

int ddeusb_core_subscribe_for_device_id_component (CORBA_Object _dice_corba_obj,
                                                   const ddeusb_usb_device_id* id,
                                                   CORBA_Server_Environment *_dice_corba_env)
{
	struct ddeusb_client *client         = NULL;
	struct ddeusb_client_idlist_entry *e = NULL;
	struct usb_device_id * _id           = NULL;

	int ret = 0;

	/* 
	 * is the client registered ? 
	 */

	if( (client = ddeusb_get_client_by_task_id(*_dice_corba_obj)))
	{

		DEBUG_MSG("client %s subsribing for id", client->name);

		e = (struct ddeusb_client_idlist_entry *) kmalloc(
		     sizeof(struct ddeusb_client_idlist_entry),GFP_KERNEL);

		if (e) 
		{
			_id = (struct usb_device_id *) 
				                         kmalloc(sizeof(struct usb_device_id), 
						                 GFP_KERNEL);
		}

		if(!e || ! _id)
		{
			/* 
			 * seems we are out of mem 
			 */

			if (e)
				kfree(e);
			if (_id)
				kfree(_id);
			LOG_Error("OUT OF MEMORY");
			

			ddeusb_put_client(client);
			return -ENOMEM;
		}

		memcpy(_id,id,sizeof(struct usb_device_id));

		e->id=_id;
		
		mutex_lock(&client->lock);
		
		list_add(&e->list,&client->ids);

		mutex_unlock(&client->lock);
		
		ddeusb_rescan_devices();

		ddeusb_put_client(client);
		return 0;
	}
	else
	{
		/* client is not registered... */
		return -EACCES;
		DEBUG_MSG("not registered client tries to register for device_id");
	}

	/* client is not registered at serverside */
}

/*
 * this function ensures, that a rescan for matching client<->gadget pairs
 * is pending.
 */
static void ddeusb_rescan_devices(){
	
	static DECLARE_WORK(w, ddeusb_rescan_devices_work_function);
	static DEFINE_MUTEX(l);
	
	mutex_lock(&l);
	
	if (!work_pending(&w))
	{
		schedule_work(&w);
	}
	
	mutex_unlock(&l);
}

/* 
 * forwards device to client...
 * client has to be locked
 * returns 1 on success
 */
static int ddeusb_forward_device_to_client(struct ddeusb_client *client,
                                           struct ddeusb_dev *dev) {
	int devno;	
	CORBA_Environment env = dice_default_environment;
	struct usb_device *udev = 0;
	
	
	/*
	 * find a free device slot
	 */

	for (devno = 0 ; devno < DDEUSB_CLIENT_MAX_DEVS; devno++)
	{
		if (client->dev[devno] == NULL )
			break;
	}

	if (devno == DDEUSB_MAX_CLIENTS)
	{
		/* 
		 * no free device slot found 
		 */ 

		return 0;
	}

	/* 
	 * now we bind the client to the device
	 */

	udev        = interface_to_usbdev(dev->intf);
	dev->client = client;
	dev->dev_id = dev->client->dev_counter++;

	ddeusb_gadget_register_device_send(&client->th,
	                                    devno,
										udev->speed,
										udev->bus_mA,
										*udev->bus->controller->dma_mask,
										&env);

	client->dev[devno]=dev;
	
	return 1;
}


static int ddeusb_rescan_single_client(struct ddeusb_client* client,
                                       struct  ddeusb_dev *dev) {


	int ret=0;
	/*
	 * lock the client structure
	 */

	mutex_lock(&client->lock);

	/*
	 * we only need to check if client is active
	 */
	
	if (client->status == CLIENT_ACTIVE )
	{
		
		struct ddeusb_client_idlist_entry * pos;

		/*
		 * ...check for every device id the client has subscribed for ... 
		 */
		list_for_each_entry(pos, &client->ids, list)
		{

			DEBUG_MSG("checking id");

			/*
			 * ... if it equals with the deivce id of the device 
			 */
			if (ddeusb_id_equal(dev->intf,pos->id))
			{
				
				/*
				 * found matching pair! 
				 */

				DEBUG_MSG("matched pair!");
				
				int devno;
				
				/* and try to forward device to client */		
				
				ret = ddeusb_forward_device_to_client(client, dev);
			}
		}
	
	}
	
	mutex_unlock(&client->lock);

	return ret;
}

/*
 * this is the workqueue function for rescanning all devices which are not yet bound
 * to a client. it needs no data so it releases is corresponding workstruct and it
 * can be resceduled. this allows us to have only one global rescan work an we don't
 * need to put this work more often into the workqueue then necessary.
 * (i.e. if we made some changes that may affect the bindings AND the rescanwork
 * is pending, our changes will be applied.)
 **/

static void ddeusb_rescan_devices_work_function(struct work_struct* work)
{
	struct ddeusb_dev *dev=NULL;


	work_release(work);

	mutex_lock(&device_lock);

	DEBUG_MSG("rescaning for matches");

	/*
	 * iterate over every device we have in our database
	 */

	list_for_each_entry(dev,&device_list,list)
	{
		DEBUG_MSG("checking dev %p, from list %p", dev, &device_list);

		/*
		 * if this device is unbound 
		 */

		if(dev->client == NULL)
		{
			DEBUG_MSG("checking unbound dev");
			
			int i;
			
			/*
			 * check every client... 
			 */
			for( i=0 ; i < DDEUSB_MAX_CLIENTS; i++)
			{
				if (ddeusb_rescan_single_client(&clients[i], dev)) 
				{
					break;
				}
			} /* for( i=0 ; i < DDEUSB_MAX_CLIENTS; i++) */

		} /* if(dev->client == NULL) */
	} /* list_for_each_entry(dev,&device_list,list) */

	mutex_unlock(&device_lock);
}


void ddeusb_set_interface_work(struct work_struct * work)
{
	CORBA_Environment env = dice_default_environment;
	struct ddeusb_dev *dev =  container_of(work, struct ddeusb_dev, work);
	int ret,i;
	DEBUG_MSG("SET INTERFACE CMD!");

	DEBUG_MSG("cancel all pending urbs!\n");

	schedule_timeout_interruptible(msecs_to_jiffies(1000));

	DEBUG_MSG("Set Interface: %d altsetting: %d\n",dev->interface, dev->alternate);
	ret = usb_set_interface(interface_to_usbdev(dev->intf),dev->interface, dev->alternate);
	DEBUG_MSG("Set Interface returned %d\n",ret);


	/*
	 * success isn't enough because usb device may return -EPIPE 
	 * (see usb_set_interface())
	 */

	ret = usb_control_msg(interface_to_usbdev(dev->intf),
	                      usb_sndctrlpipe(interface_to_usbdev(dev->intf), 0),
						  USB_REQ_SET_INTERFACE,
						  USB_RECIP_INTERFACE,
						  dev->alternate,
						  dev->interface, 
						  NULL, 0, 5000);
	

	/*
	 * returning the status to the urb should be enough...
	 */

	ddeusb_gadget_urb_complete_send(&dev->client->th,
	                                dev->urb_id,
	                                ret,
	                                0,
	                                0,
	                                0,
	                                0,
	                                "_unused_",
	                                0,
	                                0,
	                                0,
	                                0,
	                                0,
	                                &env);

	ddeusb_put_client(dev->client);
}

void ddeusb_clear_halt_work(struct work_struct *work)
{
	DEBUG_MSG("CLEAR_HALT CMD!");
	CORBA_Environment env = dice_default_environment;
	struct ddeusb_dev *dev =  container_of(work, struct ddeusb_dev, work);
	int ret ;

	ret = usb_clear_halt(interface_to_usbdev(dev->intf),dev->pipe);

	/* returning the status to the client should be enough...*/
	ddeusb_gadget_urb_complete_send (&dev->client->th,
	                                 dev->urb_id,
	                                 ret,
	                                 0,
	                                 0,
	                                 0,
	                                 0,
	                                 "_unused_",
	                                 0,
	                                 0,
	                                 0,
	                                 0,
	                                 0,
	                                 &env);

	ddeusb_put_client(dev->client);
}



/*
 * this function prepares a fallback urb
 */
static int
ddeusb_prepare_fallback_urb (struct urb *urb,
		struct usb_device *dev,
		const ddeusb_urb * d_urb,
		const char * iso_frame_desc /* in */,
		l4_size_t iso_frame_desc_size /* in */)
{
	DEBUG_MSG("");
	
	/*
	 * it is a fallback urb, so we have to allocate the transferbuffer. 
	 */
	if (d_urb->size > 0) {
		urb->transfer_buffer =
			usb_buffer_alloc(dev, d_urb->size, GFP_KERNEL, &urb->transfer_dma);
	} else {
		urb->transfer_buffer = NULL;
	}

	/*
	 * since we have allocated the buffer with usb_buffer_alloc the memory
	 * is allready dmqa mapped. 
	 */
	urb->transfer_flags &= URB_NO_TRANSFER_DMA_MAP;

	/*
	 * if it is an outgoing transfer, we have to copy the buffer into the
	 * URB's transfer buffer  
	 */
	if (!d_urb->direction && (d_urb->size>0)) { // out transfer
		memcpy(urb->transfer_buffer, d_urb->data, d_urb->size);
	}

	/*
	 * copy iso_frame_desc if it is a iso URB 
	 */
	if (iso_frame_desc_size) {
		memcpy(urb->iso_frame_desc, iso_frame_desc, iso_frame_desc_size);
	}

	return 0;
}


/*
 * this function does the shm d_urb specific stuff
 * perhaps a good place to check if client may do DMA?
 */
static int
ddeusb_prepare_shm_urb (struct urb *urb,
		const struct usb_device *dev,
		const ddeusb_urb * d_urb,
		const char * iso_frame_desc /* in */,
		l4_size_t iso_frame_desc_size /* in */)
{

	int ret;
	if (d_urb->number_of_packets) {
		memcpy(urb->iso_frame_desc, d_urb->iso_desc,
				d_urb->number_of_packets *sizeof(struct usb_iso_packet_descriptor));
	}

	ret= l4rm_attach(&d_urb->ds, d_urb->size, d_urb->offset, L4DM_RW | L4RM_MAP,
			&urb->transfer_buffer);

	l4dm_mem_ds_lock(&d_urb->ds,
			d_urb->offset,
			d_urb->size);

	//TODO: check ret
	//TODO: check dma_cap;

	l4_size_t phys_size;
	l4_addr_t phys_addr;

	ret = l4dm_mem_ds_phys_addr(&d_urb->ds, d_urb->offset, d_urb->size,
			&phys_addr, &phys_size);

	ddekit_pgtab_set_region(urb->transfer_buffer, phys_addr, d_urb->size/L4_PAGESIZE, -1);

	return ret;
}


/*
 * this function does the phys d_urb specific stuff
 * perhaps a good place to check if client may do DMA?
 */

static int
ddeusb_prepare_phys_urb (struct urb *urb,
		const struct usb_device *dev,
		const ddeusb_urb * d_urb,
		const char * iso_frame_desc /* in */,
		l4_size_t iso_frame_desc_size /* in */)
{
	if (d_urb->number_of_packets) {
		memcpy(urb->iso_frame_desc, d_urb->iso_desc,
				d_urb->number_of_packets *sizeof(struct usb_iso_packet_descriptor));
	}



	((struct ddeusb_urb_completion_handler_context* )urb->context)->is_dma = 1;
	urb->transfer_buffer = (void *)1;         // TODO: check if HC supports DMA else return error;
	urb->transfer_dma = (dma_addr_t) d_urb->phys_addr;
	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	return 0;
}

/*
 * this function does the copy d_urb specific stuff
 * perhaps a good place to check if client may do DMA?
 */

static int
ddeusb_prepare_copy_urb (struct urb *urb,
		const struct usb_device *dev,
		const ddeusb_urb * d_urb,
		const char * iso_frame_desc /* in */,
		l4_size_t iso_frame_desc_size /* in */)
{
	if (d_urb->number_of_packets) {
		memcpy(urb->iso_frame_desc, d_urb->iso_desc,
				d_urb->number_of_packets *sizeof(struct usb_iso_packet_descriptor));
	}

	urb->transfer_buffer = (void *)d_urb->buf;
	return 0;
}



/*
 * this function submits an urb  
 *  - first setup the common URB values
 *  - then setup the D_URB specific stuff
 *  - then the do the transfertype specific stuff
 *  - submit the urb
 */
	int
ddeusb_submit_urb (struct ddeusb_client *client,
		const ddeusb_urb * d_urb,
		const char * iso_frame_desc /* in */,
		l4_size_t iso_frame_desc_size /* in */,
		int d_urb_id)
{
	struct urb *urb = NULL;
	struct ddeusb_urb_completion_handler_context *context=NULL;
	struct ddeusb_dev *ddev= NULL;
	struct usb_device *dev= NULL;
	int ret;
	unsigned int pipe = 0;
	int target_endp;
	int target_dir;
	struct usb_ctrlrequest * req = NULL;

	mutex_lock(&client->lock);
	ddev= client->dev[d_urb->dev_id];
	mutex_unlock(&client->lock);


	if (!ddev) {
		
		/*
		 * client has no device with that ID 
		 */

		DEBUG_MSG("Device does not exist.");
		ret = -ENODEV;
		goto SUERROR;

	}

#ifdef DDEUSB_CACHE_URBS
	urb = ddeusb_alloc_urb(d_urb->number_of_packets);
#else
	urb = usb_alloc_urb(d_urb->number_of_packes,GFP_KERNEL);
#endif

	if (!urb) {
		LOG_Error("Out of mem...");
		ret = -ENOMEM;
		goto SUERROR;
	}

#ifndef DDEUSB_CACHE_URBS
	context = (struct ddeusb_urb_completion_handler_context *) kmalloc(sizeof(struct ddeusb_urb_completion_handler_context), GFP_KERNEL);
#else
	context = (struct ddeusb_urb_completion_handler_context *) urb->context;
#endif /* DDEUSB_CACHE_URBS */

	if (!context) {
		LOG_Error("Out of mem...");
		ret = -ENOMEM;
		goto SUERROR1;
	}

	if(!ddev->intf)	{
		LOG_Error("Device disconnected");
		ret = -ENOMEM;
		goto SUERROR1;

	}

	dev = interface_to_usbdev(ddev->intf);

	/*
	 * set up the context of the URB
	 */

	context->urb_id         = d_urb->urb_id;
	context->d_urb_id       = d_urb_id;
	context->client         = client;
	context->dev            = ddev;
	context->transfer_flags = d_urb->transfer_flags;

	/* 
	 * set up the common URB data 
	 */

	urb->dev                     = dev;
	urb->transfer_flags          = d_urb->transfer_flags
		& ~URB_NO_TRANSFER_DMA_MAP
		& ~URB_NO_SETUP_DMA_MAP;
	urb->transfer_buffer         = NULL;
	urb->setup_packet            = NULL;
	urb->transfer_buffer_length  = d_urb->size;
	urb->interval                = d_urb->interval;
	urb->context                 = context;
	urb->complete                = ddeusb_completion_handler;
	urb->start_frame             = d_urb->start_frame;
	urb->number_of_packets       = d_urb->number_of_packets;
	urb->actual_length           = 0;

	/*
	 * setup the specific D_URB_TYPE specific URB data 
	 */

	if (d_urb_id == -1) {

		/*
		 * FALLBACK URB 
		 */

		ddeusb_prepare_fallback_urb(urb, dev,  d_urb, iso_frame_desc, iso_frame_desc_size);
	} 
	else
	{

		/*
		 * it is a URB in shared dataspace 
		 */

			switch(d_urb->d_urb_type) {

			case DDEUSB_D_URB_TYPE_SHM:

				ret = ddeusb_prepare_shm_urb(urb, dev,  d_urb, iso_frame_desc, iso_frame_desc_size);

				break;

			case DDEUSB_D_URB_TYPE_PHYS:
				ret = ddeusb_prepare_phys_urb(urb, dev,  d_urb, iso_frame_desc, iso_frame_desc_size);
				break;

			case DDEUSB_D_URB_TYPE_COPY:

				ret = ddeusb_prepare_copy_urb(urb, dev,  d_urb, iso_frame_desc, iso_frame_desc_size);

				break;

			default:
				break;        //TODO: ERROR
		}
	}

	switch (d_urb->type)
	{

		/***************************************************/
		/*   INTERRUPT URB                                 */
		/***************************************************/

		/* interrupt urbs are straight forward...      */
		/* just submit with standard completionhandler */

		case DDEUSB_INT:
			if (d_urb->direction)
				urb->pipe = usb_rcvintpipe(dev,d_urb->endpoint);
			else
				urb->pipe = usb_sndintpipe(dev,d_urb->endpoint);
			break;

		/***************************************************/
		/*   CONTROL URB                                   */
		/***************************************************/

		/* there are three control messages we have to be aware of:*
		 * - set interface (because we have to activate the new    *
		 *   altsetting also on our side)                          *
		 * - clear halt                                            *
		 * -                                                       *
		 * this two calls are blocking so we have to put them      *
		 * into a workqueue.                                       */

		case DDEUSB_CTL:

			if (d_urb->direction) {

				urb->pipe = usb_rcvctrlpipe(dev,d_urb->endpoint);
			}
			else {

				urb->pipe = usb_sndctrlpipe(dev,d_urb->endpoint);
			}

			urb->setup_packet = usb_buffer_alloc(dev, 8, GFP_KERNEL, &urb->setup_dma);

			urb->transfer_flags |= URB_NO_SETUP_DMA_MAP;

			memcpy(urb->setup_packet, d_urb->setup_packet, 8);

			/* 
			 * now look what kind of ctl-msg it is 
			 */

			req = (struct usb_ctrlrequest *) urb->setup_packet;

			/*
			 * is it a set interface command? 
			 */

			if ((req->bRequest == USB_REQ_SET_INTERFACE) &&
					(req->bRequestType == USB_RECIP_INTERFACE))
			{
				/*
				 * if so, we have to schedule the setinterface work of this device 
				 * */
				__u16 alternate;
				__u16 interface;
				ddev->alternate = le16_to_cpu(req->wValue);
				ddev->interface = le16_to_cpu(req->wIndex);
				ddev->urb_id=context->urb_id;
				INIT_WORK(&ddev->work,ddeusb_set_interface_work);
				schedule_work(&ddev->work);
				ret=0;
				goto DONE;
			}


			/*
			 * is it a clear halt cmd? 
			 */
			if( (req->bRequest == USB_REQ_CLEAR_FEATURE) &&
					(req->bRequestType == USB_RECIP_ENDPOINT) &&
					(req->wValue == USB_ENDPOINT_HALT))
			{
				target_endp = le16_to_cpu(req->wIndex) & 0x000f;
				target_dir = le16_to_cpu(req->wIndex) & 0x0080;
				if(target_dir)
					ddev->pipe=usb_rcvctrlpipe (urb->dev, target_endp);
				else
					ddev->pipe=usb_sndctrlpipe (urb->dev, target_endp);

				ddev->urb_id=context->urb_id;
				INIT_WORK(&ddev->work,ddeusb_clear_halt_work);
				schedule_work(&ddev->work);
				ret=0;
				goto DONE;
			}

			break;

		/******************************************************/
		/*    BULK URBS                                       */
		/******************************************************/

		case DDEUSB_BLK:
			if (d_urb->direction)
				urb->pipe = usb_rcvbulkpipe(dev,d_urb->endpoint);
			else
				urb->pipe = usb_sndbulkpipe(dev,d_urb->endpoint);
			break;



		/******************************************************/
		/*    ISOCHRONOUS URBS                                */
		/******************************************************/
		case DDEUSB_ISO:
			if (d_urb->direction) {
#ifndef DDEUSB_ISO_BUFFERING
				urb->pipe = usb_rcvisocpipe(dev,d_urb->endpoint);
#else
				if(urb->transfer_buffer && d_urb->dma_addr)
					usb_buffer_free(dev, size, urb->transfer_buffer, urb->transfer_dma);
				ddeusb_iso_submit_urb(urb);
				ddeusb_put_client(client);
				return 0;
#endif
			} else {
#ifndef DDEUSB_ISO_BUFFERING
				urb->pipe = usb_sndisocpipe(dev,d_urb->endpoint);
#else
				DEBUG_MSG("sending buffered iso urbs is not supported yet! BACK TO WORK! \n");
				BUG();
#endif
			}
			break;
		default:
			DEBUG_MSG("Unsupported URB type");
			ret = -EINVAL;
			goto SUERROR2;
	}

	/*
	 * Add the URB the the Clients URB list
	 */

	mutex_lock(&client->lock);
	list_add(&context->list,&client->urb_list);
	mutex_unlock(&client->lock);

	/*
	 * finally submit the URB
	 */

	ret = usb_submit_urb(urb, GFP_KERNEL);
	if (ret)
	{
		DEBUG_MSG("error submiting urb: %d\n",ret);
		
		/*
		 * Remove the URB from the client's URB list
		 */
		
		mutex_lock(&client->lock);
		list_del(&context->list);
		mutex_unlock(&client->lock);
		
		goto SUERROR3;
	}
	DEBUG_MSG("Submited");
	return ret;


DONE:
SUERROR3:
	if(urb->transfer_buffer && (d_urb_id==-1))
		usb_buffer_free(dev, urb->transfer_buffer_length, urb->transfer_buffer, urb->transfer_dma);
	if(urb->setup_packet)
		usb_buffer_free(dev, 8, urb->setup_packet,urb->setup_dma);

SUERROR2:
	if(context){
#ifndef DDEUSB_CACHE_URBS
		kfree(context);
#endif /* DDEUSB_CACHE_URBS*/
	}
SUERROR1:
	if(urb) {
#ifdef DDEUSB_CACHE_URBS
		usb_free_urb(urb);
#else
		ddeusb_free_urb();
#endif
	}
SUERROR:
	ddeusb_put_client(client);
	return ret ;

}



/*
 * this function prepares the shared dataspace
 * if the client wants to use D_URBS this function has to be called
 * before submitting any D_URBS
 */
int
ddeusb_core_ddeusb_setup_control_ds_component (CORBA_Object _dice_corba_obj,
                                               const l4dm_dataspace_t *ds /* in */,
                                               l4_offs_t offset /* in */,
                                               l4_size_t size /* in */,
                                               CORBA_Server_Environment *_dice_corba_env)
{
	struct ddeusb_client *client;
	l4_size_t phys_size;
	l4_addr_t phys_addr;
	int ret;

	if( !(client =  ddeusb_get_client_by_task_id(*_dice_corba_obj)))
	{
		/*
		 * client is not registered...
		 */
		DEBUG_MSG("client not registered...");

		return -EACCES;
	}

	ret = l4rm_attach(ds, size,offset, L4DM_RW | L4RM_MAP,
	                  (void**) &client->d_urbs);


	ret = l4dm_mem_ds_phys_addr(ds, offset, size,
	                            &client->d_urbs_phys, &phys_size);

	ddekit_pgtab_set_region(client->d_urbs, client->d_urbs_phys,
	                        size/L4_PAGESIZE, -1);

	LOG("Attached control data space (size :%x, offset: %lx) \
         returned %d (mapped to %p)"                                                                   ,size,offset,ret,(void*)client->d_urbs);

	return ret;
}


/*
 * This function submits a d_urb
 */
int ddeusb_core_ddeusb_ds_submit_urb_component (CORBA_Object _dice_corba_obj,
                                                int id,
                                                CORBA_Server_Environment *_dice_corba_env)
{
	struct ddeusb_client *client;
	int ret;
	DEBUG_MSG(" ");
	if( !(client =  ddeusb_get_client_by_task_id(*_dice_corba_obj)))
	{
		/*
		 * client is not registered...
		 */
		DEBUG_MSG("client not registered...");
		return -EACCES;
	}

	return ddeusb_submit_urb (client,&client->d_urbs[id], 0, 0,id);

}

/*
 * this function submits a fallback URB
 */
int
ddeusb_core_ddeusb_submit_urb_component  (CORBA_Object _dice_corba_obj,
                                          int urb_id,
                                          int dev_id,
                                          int type,
                                          int direction,
                                          int endpoint,
                                          unsigned int transfer_flags,
                                          int interval,
                                          int start_frame,
                                          int number_of_packets,
                                          const void *setup_packet,
                                          const void *data /* in */,
                                          l4_size_t size /* in */,
                                          const char * iso_frame_desc /* in */,
                                          l4_size_t iso_frame_desc_size /* in */,
                                          CORBA_Server_Environment *_dice_corba_env)
{

	/* it is ok to have the d_urb only on the stack because ddeusb_submit_urb
	 * will copy all essential data */
	ddeusb_urb d_urb;

	struct ddeusb_client *client = NULL;

	if( !(client =  ddeusb_get_client_by_task_id(*_dice_corba_obj)))
	{
		/* client is not registered...*/
		DEBUG_MSG("client not registered...");
		return -EACCES;
	}

	memcpy(d_urb.setup_packet, setup_packet, 8);

	d_urb.urb_id            = urb_id;
	d_urb.dev_id            = dev_id;
	d_urb.type              = type;
	d_urb.endpoint          = endpoint;
	d_urb.direction         = direction;
	d_urb.transfer_flags    = transfer_flags,
	d_urb.interval          = interval;
	d_urb.start_frame       = start_frame;
	d_urb.number_of_packets = number_of_packets;
	d_urb.data              = data;
	d_urb.size              = size;

	return ddeusb_submit_urb(client, &d_urb, iso_frame_desc,
	                         iso_frame_desc_size, -1);

}

static void ddeusb_completion_handler(struct urb *urb)
{

	CORBA_Environment env = dice_default_environment;
	struct      ddeusb_client *client=NULL;
	ddeusb_urb *d_urb=NULL;
	struct ddeusb_urb_completion_handler_context * context =
	        (struct ddeusb_urb_completion_handler_context *)urb->context;
	int i=0, tb_size=0, sp_size=8;


	tb_size = context->is_dma ? 0 : urb->actual_length;

	DEBUG_MSG("sending completion event to %X.%X, urb-nr: %d, status: %d",
	          context->client->th.id.task, context->client->th.id.lthread,
	          context->urb_id, urb->status);

	mutex_lock(&context->client->lock);

	list_del(&context->list);

	mutex_unlock(&context->client->lock);

	if(context->client->status==CLIENT_ACTIVE) {

		client = context->client;
		/* was it a fallback URB ? */
		if (context->d_urb_id>=0) {

			/* it was not a fall-back urb*/

			d_urb = &client->d_urbs[context->d_urb_id];

			d_urb->status         = urb->status;
			d_urb->actual_length  = urb->actual_length;
			d_urb->interval       = urb->interval;
			d_urb->error_count    = urb->error_count;
			d_urb->start_frame    = urb->start_frame;
			d_urb->phys_addr      = urb->transfer_dma;

			memcpy(d_urb->iso_desc, urb->iso_frame_desc,
			       urb->number_of_packets * sizeof(struct usb_iso_packet_descriptor));

			if (urb->setup_packet) {

				memcpy(d_urb->setup_packet, urb->setup_packet, 8);
			}

			if (d_urb->d_urb_type == DDEUSB_D_URB_TYPE_SHM) {

				ddekit_pgtab_clear_region(urb->transfer_buffer,  -1);
				l4rm_detach(urb->transfer_buffer);
				l4dm_mem_ds_unlock(&d_urb->ds,
				                   d_urb->offset,
				                   d_urb->size);
			}

			ddeusb_gadget_ds_urb_complete_send (&context->client->th,
			                                    context->d_urb_id, &env);

		} else {

			/* it was a fall-back urb */
			ddeusb_gadget_urb_complete_send
			(&context->client->th, context->urb_id,
			 urb->status, urb->actual_length, urb->interval,
			 urb->transfer_buffer,
			 urb->transfer_buffer_length,
			 urb->setup_packet ? (void *)urb->setup_packet : "_unused_",
			 urb->transfer_flags, urb->error_count, urb->start_frame,
			 (char *)&urb->iso_frame_desc,
			 urb->number_of_packets*sizeof(struct usb_iso_packet_descriptor),
			 &env);

			if (urb->transfer_buffer && !context->is_dma ) {

				if (urb->transfer_buffer) {
					usb_buffer_free(urb->dev, urb->transfer_buffer_length,
					                urb->transfer_buffer, urb->transfer_dma);
				}

			}
		}
	}


	if(urb->setup_packet)
		usb_buffer_free(urb->dev, sp_size,urb->setup_packet, urb->setup_dma);

	ddeusb_put_client(context->client);

#ifndef DDEUSB_CACHE_URBS
	kfree(context);
	usb_free_urb(urb);
#else
	ddeusb_free_urb(urb);
#endif /* DDEUSB_CACHE_URBS*/
}


int
ddeusb_core_unlink_urb_component (CORBA_Object _dice_corba_obj,
                                  int urb_id,
                                  CORBA_Server_Environment *_dice_corba_env)
{
	struct ddeusb_client *client = NULL;

	/* we need to search for the client */
	DEBUG_MSG("unlink...");
	if( !(client =  ddeusb_get_client_by_task_id(*_dice_corba_obj)))
	{
		/* client is not registered...*/
		return -ENODEV;
	}
	else
	{
		struct urb *urb = NULL;
		/* we found the client */
		mutex_lock(&client->lock);
		struct ddeusb_client_urb_entry *e;
		list_for_each_entry(e,&client->urb_list,list)
		{
			if(((struct ddeusb_urb_completion_handler_context *)e->urb->context)->urb_id == urb_id)
			{
				urb = e->urb;
				break;
			}
		}
		mutex_unlock(&client->lock);
		if(urb)
			usb_unlink_urb(urb);
	}
	return 0;
}


/*
 * we have registered ourself for all usb_device_ids so our probefunction will
 * be called for every new usb_device. we'll tell usbcore, that we care about
 * the new device and put it in our device_db where it can wait for an
 * ddeusb_gadget_driver which serves this device.
 *
 * so there is not much to do for this funtion:
 *
 * (1) store the interface and the device id in our db
 * (2) look if there is yet an client who wants to serve this device. if so
 *     register the device at this client.
 * 
 * READY.
 *
 */

static int ddeusb_probe(struct usb_interface * intf, const struct usb_device_id *id)
{
	struct ddeusb_dev *dev=NULL;
	/* we don't care about hubs: */
	/* they should not get here anyway... */

	if ((id->match_flags == USB_DEVICE_ID_MATCH_DEV_CLASS && id->bDeviceClass == USB_CLASS_HUB)
	    || (id->match_flags == USB_DEVICE_ID_MATCH_INT_CLASS && id->bInterfaceClass == USB_CLASS_HUB))
		return -ENODEV;

	DEBUG_MSG("Adding new device to device database.");

	/* is the device containing this interface yet given to a client? */
	mutex_lock(&device_lock);

	list_for_each_entry(dev,&device_list,list)
	{
		if(interface_to_usbdev(dev->intf)==interface_to_usbdev(intf)) {

			DEBUG_MSG("device yet given to client: nothing to do...\n");
#if 0
			usb_set_intfdata(intf,dev);
			usb_get_intf(intf);
			usb_get_dev(interface_to_usbdev(dev->intf));
#endif
			mutex_unlock(&device_lock);
			return 0;
		}
	}

	mutex_unlock(&device_lock);


	/* (1) store this device in our device_db */
	dev = (struct ddeusb_dev *) kmalloc(sizeof(struct ddeusb_dev), GFP_KERNEL);

	if (!dev){
		LOG_Error("ERROR: out of mem");
		return -ENOMEM;
	}


	dev->intf=intf;


	dev->client=NULL;

	DEBUG_MSG("created dev %p", dev);

	mutex_lock(&device_lock);
	list_add_tail(&dev->list, &device_list);
	mutex_unlock(&device_lock);

	usb_set_intfdata(intf,dev);

	usb_get_intf(intf);
	usb_get_dev(interface_to_usbdev(dev->intf));

	ddeusb_rescan_devices();

	return 0;
}

/*
 * this function is called by usbcore, whenever a device is disconnected
 *
 * !!assumption!! : interfaces are not disconnected seperatly
 *
 * what we have to do:
 *
 * if (client is registered for this device)
 *   inform client
 *
 * remove device from device_db
 *
 * ready.
 *
 */

static void ddeusb_disconnect (struct usb_interface *intf)
{

	struct ddeusb_dev * dev = (struct ddeusb_dev *) usb_get_intfdata(intf);

	DEBUG_MSG("Disconnect");
	
	if(!dev) {
		DEBUG_MSG(" device does not exist anymore");
		/* 
		 * device is yet removed (it had multiple interfaces...)
		 */
		return;
	}
	struct usb_device *udev = interface_to_usbdev(dev->intf);
	
	
	CORBA_Environment env = dice_default_environment;
	
	struct ddeusb_client_urb_entry *urb_e =NULL;
	
	LIST_HEAD(urbs);

	if (!dev)
	{
		DEBUG_MSG("Device doesn't exist? Why twice Why twice??");
		return;
	}
	
	mutex_lock(&device_lock);

	/* if a client client is bound to this device -> unbind */
	if (dev->client)
	{
		int i;
		mutex_lock(&dev->client->lock);

		for (i=0 ; i < DDEUSB_CLIENT_MAX_DEVS ; i++)
		{
			
			if (dev->client->dev[i] == dev )
			{

				DEBUG_MSG("found device in client struct, sending disconnect notification");

				/*
				 * here we only can delete the client->dev binding,
				 * (but no new urbs will be generated for this device) 
				 */
				
				
				ddeusb_gadget_disconnect_send(&dev->client->th,
				                             i,&env);

				dev->client->dev[i] = 0;

				struct ddeusb_urb_completion_handler_context *c;

				list_for_each_entry(c,&dev->client->urb_list,list)
				{
					struct urb *urb= container_of((void *)c,struct urb,hcpriv);
					
					if(urb->dev==udev)
					{
						urb_e = (struct ddeusb_client_urb_entry *)
						        kmalloc(sizeof(struct ddeusb_client_urb_entry), GFP_KERNEL);
						usb_get_urb(urb);
						urb_e->urb = urb;
						list_add(&urb_e->list,&urbs);
					}

				}
			}
		}
		mutex_unlock(&dev->client->lock);
	}
	else 
	{
		DEBUG_MSG("device not bound to a device.");
	}

	/* remove dev from list, so no new client can bind to it */
	list_del(&dev->list);
	mutex_unlock(&device_lock);

	/* now, that we are not holding any locks anymore, we can to the
	 * blocking work, i.e. killling urbs belonging to this device.*/

	while(!list_empty(&urbs))
	{
		DEBUG_MSG("Deleting pending urb");
		urb_e = list_entry(urbs.next,struct ddeusb_client_urb_entry,list);
		usb_kill_urb(urb_e->urb);
#ifndef DDEUSB_CACHE_URBS
		usb_free_urb(urb_e->urb);
#else
		list_del(&urb_e->list);
#endif /* DDEUSB_CACHE_URBS */
		kfree(urb_e);

	}


	/* look if a synchronous call is pending... */
	/* a new synchorous call can not be startet because
	 * (1) the client -> device ptr is nulled
	 * (2) a new client can't bind to this device, because it isn't in the list
	 *     anymore.
	 */

	while(work_pending(&dev->work))
	{
		/* we have to wait */
		schedule();
	}

	/* free our device struct */

	kfree(dev);
	usb_set_intfdata(intf, NULL);
	usb_put_intf(intf);
	usb_put_dev(udev);
	/* done. */

	DEBUG_MSG("Disconnect");
}

/*
 * ... we take everthing!
 */

static struct usb_device_id ddeusb_id_table [] = {
	{ .driver_info = 1 },
	{ }     /* Terminating entry */
};

struct usb_driver ddeusb_usb_driver = {
	.name           = "ddeusb",
	.probe          = ddeusb_probe,
	.disconnect     = ddeusb_disconnect,
	.id_table       = ddeusb_id_table,
};


static int __init ddeusb_init_driver(void) {
	int i=0;
	struct urb *urbs[DDEUSB_PRECACHE_URBS];
	usb_register(&ddeusb_usb_driver);
	for(i=0; i<DDEUSB_PRECACHE_URBS ; i++) {
		urbs[i]=ddeusb_alloc_urb(0);
	}

	for(i=0; i<DDEUSB_PRECACHE_URBS ; i++) {
		ddeusb_free_urb(urbs[i]);
	}


	return 0;
}

module_init(ddeusb_init_driver);

