/*
 * \brief   DDEUSB26 library - Virtual host controller for DDELinux 
 *          based USB drivers
 * \date    2009-04-07
 * \author  Dirk Vogt <dvogt@os.inf.tu-dresden.de>
 */

/*
 * This file is part of the DDEUSB package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */



#include <asm/io.h>
#include <linux/kthread.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/usb.h>


#include <l4/dde/ddekit/memory.h>


#include <l4/dm_mem/dm_mem.h>


#include "../../../common/contrib/drivers/usb/core/hcd.h"
#include "../../../common/contrib/drivers/usb/core/usb.h"
	
//#define DDEUSB_DEBUG 1

#include "ddeusb26_local.h"

MODULE_AUTHOR("Dirk Vogt <dvogt@os.inf.tu-dresden.de>");
MODULE_DESCRIPTION("DDEUSB stub driver");
MODULE_LICENSE("GPL");



static int urb_counter = 0;

static struct kmem_cache * priv_cache;

static const char driver_name[] = "ddeusb_vhcd";
static const char driver_desc[] = "DDEUSB Virtual Host Controller";

struct usb_hcd  *ddeusb_vhcd_hcd = NULL;
static u64 ddeusb_dma_mask = 0;



/* Context: in_interrupt()
 * Now HUBD is used. so we have to tell hubd that a device connected.
 */
static int ddeusb_register_device(int dev_id, int speed, unsigned short bus_mA, unsigned long long dma_mask)
{
    unsigned long flags;
	int i;
	int ret= -ENOMEM;

	struct ddeusb_vhcd_priv_data *data = NULL;
	
	DEBUG_MSG("Registering new device to vhcd");	
	/* check if hcd is allready setup */
	if(!ddeusb_vhcd_hcd) 
	{
		return -ENODEV;
	}

	data =  (struct ddeusb_vhcd_priv_data *) ddeusb_vhcd_hcd->hcd_priv;

	/*find an unused port */
    spin_lock_irqsave(&data->lock, flags);
	for(i=0; i < DDEUSB_VHCD_PORT_COUNT ; i++ ) 
	{
		if (data->gadget[i].id == -1)
		{
			/* found unused port */
			data->gadget[i].id = dev_id;
			DEBUG_MSG("registered: %p", dev_id);	
			switch(speed)
			{
				case USB_SPEED_HIGH:
					data->port_status[i] |= USB_PORT_STAT_HIGH_SPEED;
					break;
				case USB_SPEED_LOW:
					data->port_status[i] |= USB_PORT_STAT_LOW_SPEED;
					break;
				default:
				break;	
			}
			data->port_status [i] |= USB_PORT_STAT_CONNECTION | (1 << USB_PORT_FEAT_C_CONNECTION);
			ret=0;
			break;
			ddeusb_dma_mask &= dma_mask;
		}

	}
	data->gadget[i].speed=speed;
    spin_unlock_irqrestore(&data->lock, flags);
    return ret;
}



/*
 * context: in_interrupt()
 */
static void ddeusb_disconnect_device(int irq_ddeusb_id) {

    unsigned long flags;
    
	DEBUG_MSG("unregistering %d", irq_ddeusb_id);	
	struct ddeusb_vhcd_priv_data *data = NULL;
	int i;
	/* check if hcd is allready setup */
	if(!ddeusb_vhcd_hcd) 
	{
		return;
	}

	data =  (struct ddeusb_vhcd_priv_data *) ddeusb_vhcd_hcd->hcd_priv;
	/*find an unused port */

    spin_lock_irqsave(&data->lock, flags);
	
    for(i=0; i < DDEUSB_VHCD_PORT_COUNT ; i++ ) 
	{
		if (data->gadget[i].id == irq_ddeusb_id)
		{
			/* found unused port */
			data->gadget[i].id= -1;

			DEBUG_MSG("unregistered: %p", irq_ddeusb_id);	
			
			data->port_status[i] &= ~USB_PORT_STAT_CONNECTION ;
			data->port_status[i] |= (1 << USB_PORT_FEAT_C_CONNECTION);

			break;
		}

	}

    spin_unlock_irqrestore(&data->lock, flags);
}

/*
 * context: in_interrupt()
 */
static void ddeusb_d_urb_complete (ddeusb_urb *irq_d_urb)
{
	struct urb *urb=NULL;
	struct ddeusb_vhcd_urbpriv *urb_priv = (struct ddeusb_vhcd_urbpriv *)irq_d_urb->priv;

    /* check if hcd is allready setup */
    if(!ddeusb_vhcd_hcd)
    {
		return;
    }

    urb=urb_priv->urb;

    
	//TODO: check if we realy have to give back urb...

    if(!urb) {
        /* it looks like the urb has been unlinked, so 
		 * there is nothing to do for us here. */
		return;
	}
	urb->status         = irq_d_urb->status;
	urb->actual_length  = irq_d_urb->actual_length;
	urb->start_frame    = irq_d_urb->start_frame;
	urb->interval       = irq_d_urb->interval;
	urb->error_count    = irq_d_urb->error_count;
    urb->transfer_flags = irq_d_urb->transfer_flags;	
    
   
    DEBUG_MSG("complete urb %d", ((struct ddeusb_vhcd_urbpriv *) urb->hcpriv)->urb_id);

    kmem_cache_free(priv_cache, urb_priv);
	//kfree(urb_priv);
    
	if (urb->setup_packet ) {
		memcpy(urb->setup_packet, irq_d_urb->setup_packet, 8);
	}
  
    if (urb->number_of_packets) {
        memcpy(urb->iso_frame_desc, irq_d_urb->iso_desc,
				urb->number_of_packets * sizeof(struct usb_iso_packet_descriptor));
	}

    urb->hcpriv=0;

	usb_hcd_giveback_urb(ddeusb_vhcd_hcd, urb);

    libddeusb_free_d_urb(irq_d_urb);
    usb_put_urb(urb);
}


static int ddeusb_vhcd_reset(struct usb_hcd *hcd) {
        WARN_UNIMPL;
        return 0;
}


static int ddeusb_vhcd_start(struct usb_hcd *hcd) {
	int i;
	
    struct ddeusb_vhcd_priv_data *data= (struct ddeusb_vhcd_priv_data *) hcd->hcd_priv;
	DEBUG_MSG("START");	
	sema_init(&data->rcv_buf_free, DDEUSB_VHCD_BUF_SIZE);

	for (i=0; i<DDEUSB_VHCD_PORT_COUNT;i++)
	{
		data->gadget[i].id=-1;
	}


	for (i = 0;  i < DDEUSB_VHCD_BUF_SIZE  ; i++) {
			data->rcv_buf[i] = NULL;
	}
	spin_lock_init(&data->lock);
	
	/*
	 * We let the usbcore do the mapping not the driver..
	 */
	
    hcd->self.uses_dma=0;
	
	hcd->state  = HC_STATE_RUNNING;
	
	ddeusb_vhcd_hcd = hcd;
	
	return 0;
}

/******************************************************************************/
/*                                                                            */
/*           power management functions are non implemented                   */
/*                                                                            */
/******************************************************************************/
static int ddeusb_vhcd_suspend(struct usb_hcd *hcd,  pm_message_t message) {
	WARN_UNIMPL;
	return 0;
}

static int ddeusb_vhcd_resume(struct usb_hcd *hcd) {
	WARN_UNIMPL;
	return 0;
}

static int ddeusb_vhcd_stop(struct usb_hcd *hcd) {
	WARN_UNIMPL;
	return 0;
}
static void ddeusb_vhcd_shutdown(struct usb_hcd *hcd) {
	WARN_UNIMPL;
}

/******************************************************************************/


/* get frame number should return the current usr frame */
/* not many drivers use it, so not supported yet        */
static int ddeusb_vhcd_get_frame_number(struct usb_hcd *hcd) {
	WARN_UNIMPL;
	return 0;
}



static int ddeusb_vhcd_urb_enqueue(struct usb_hcd *hcd,
                                        struct usb_host_endpoint *ep,

                                   struct urb *urb,
                                   gfp_t mem_flags)
{
    struct ddeusb_vhcd_priv_data *data= (struct ddeusb_vhcd_priv_data *) hcd->hcd_priv;
	int number_of_iso_packets=0;
	int ret=0;
	unsigned int transfer_flags = 0 ;
    struct ddeusb_vhcd_urbpriv * priv=NULL; 
    l4_threadid_t dummy_t;
    l4_addr_t dummy_a;

	struct usb_device * udev = urb->dev;

	if (data->gadget[udev->portnum-1].id ==-1) 
	{
		/*
		 * Device is not connected!
		 */
		DEBUG_MSG("Trying to send URB to non existent dev...");
		return -ENODEV;
	}


    ddeusb_urb * d_urb = libddeusb_alloc_d_urb(DDEUSB_D_URB_TYPE_AUTO);

	if (!HC_IS_RUNNING(hcd->state)) 
	{
		DEBUG_MSG("HC is not running\n");
		return  -ENODEV;
	}	
	
	
	/* we have to trap some control messages, i.e. USB_REQ_SET_ADDRESS... */
	/* TODO: we don't have to do it here, but in the server */

	if (usb_pipedevice(urb->pipe) == 0) {
		__u8 type = usb_pipetype(urb->pipe);
		struct usb_ctrlrequest *ctrlreq = (struct usb_ctrlrequest *) urb->setup_packet;

		if (type != PIPE_CONTROL || !ctrlreq ) {
			DEBUG_MSG("invalid request to devnum 0\n");
			ret = -EINVAL;
			goto no_need_xmit;
		}

		switch (ctrlreq->bRequest) {
			
			case USB_REQ_SET_ADDRESS:
				data->gadget[udev->portnum].udev = urb->dev;
				DEBUG_MSG("SetAddress Request (%d) to port %d\n",
						ctrlreq->wValue, urb->dev->portnum);

				spin_lock (&urb->lock);
				if (urb->status == -EINPROGRESS) {
					/* This request is successfully completed. */
					/* If not -EINPROGRESS, possibly unlinked. */
					urb->status = 0;
				}
                spin_unlock (&urb->lock);

				goto no_need_xmit;

			case USB_REQ_GET_DESCRIPTOR:
				if (ctrlreq->wValue == (USB_DT_DEVICE << 8))
					DEBUG_MSG("Get_Descriptor to device 0 (get max pipe size)\n");
				goto out;

			default:
				/* NOT REACHED */
				DEBUG_MSG("invalid request to devnum 0 bRequest %u, wValue %u\n",
						ctrlreq->bRequest, ctrlreq->wValue);
				ret =  -EINVAL;
				goto no_need_xmit;
		}

	}


out:
	if (urb->status != -EINPROGRESS) 
	{
		DEBUG_MSG("URB already unlinked!, status %d\n", urb->status);
		return urb->status;
	}

	
	/*    is it an iso urb? */
	if (usb_pipeisoc(urb->pipe))
		number_of_iso_packets =  urb->number_of_packets;

	/* TODO: cache the privs */	
	priv = ( struct ddeusb_vhcd_urbpriv *) kmem_cache_alloc (priv_cache, GFP_KERNEL);	
	//priv = ( struct ddeusb_vhcd_urbpriv *) kmalloc (sizeof (struct ddeusb_vhcd_urbpriv), GFP_KERNEL);	

	urb->hcpriv=(void *)priv;

	priv->urb=urb;

    transfer_flags = urb->transfer_flags;
    // don't free the urb, we need it yet
    usb_get_urb(urb);

    ret=0;
    
    if(!ret)
    {
        d_urb->type              = usb_pipetype(urb->pipe);
        d_urb->dev_id            = data->gadget[urb->dev->portnum-1].id;
        d_urb->endpoint          = usb_pipeendpoint(urb->pipe);
        d_urb->direction         = 0 || usb_pipein(urb->pipe);
        d_urb->interval          = urb->interval;
        d_urb->transfer_flags    = urb->transfer_flags;
        d_urb->number_of_packets = urb->number_of_packets;
        d_urb->priv              = priv;
        d_urb->size              = urb->transfer_buffer_length; 
        d_urb->data			     = urb->transfer_buffer;
		d_urb->phys_addr	     = d_urb->data?virt_to_phys(d_urb->data):0;
		

		if(urb->setup_packet) {
            memcpy(d_urb->setup_packet, urb->setup_packet, 8);
        }

        if(urb->number_of_packets)
            memcpy(d_urb->iso_desc, urb->iso_frame_desc, urb->number_of_packets*sizeof(struct usb_iso_packet_descriptor));

        ret= libddeusb_submit_d_urb(d_urb);
        DEBUG_MSG("tried to send urb : %d",ret);
    }

	if (ret)
	{
	    DEBUG_MSG("URB SUBMIT FAILED (%d).",ret);
		/* s.t. went wrong. */	
//      spin_lock_irqsave(&data->lock, flags);  
//      data->rcv_buf[i]=NULL;
//      spin_unlock_irqrestore(&data->lock, flags);
//      down(&data->rcv_buf_free);
        //kmem_cache_free(priv_cache, urb->hcpriv);
        usb_put_urb(urb);
        urb->status = ret;
        urb->hcpriv=NULL;


		/*
		 * free the d_urb 
		 */

		libddeusb_free_d_urb(d_urb);
		return ret;
	}
    
	DEBUG_MSG("sent urb with id %d", priv->urb_id);

    return 0;

no_need_xmit:
    usb_hcd_giveback_urb(hcd, urb);
	return 0;
}

/*TODO: */
static int ddeusb_vhcd_urb_dequeue(struct usb_hcd *hcd, struct urb *urb)
{
	return 0;
}

/* See hub_configure in hub.c */
static inline void hub_descriptor(struct usb_hub_descriptor *desc)
{
	memset(desc, 0, sizeof(*desc));
	desc->bDescriptorType = 0x29;
	desc->bDescLength = 9;
	desc->wHubCharacteristics = (__force __u16)
		(__constant_cpu_to_le16 (0x0001));
	desc->bNbrPorts = DDEUSB_VHCD_PORT_COUNT;
	desc->bitmap [0] = 0xff;
	desc->bitmap [1] = 0xff;
}

static void    ddeusb_vhcd_endpoint_disable (struct usb_hcd *hcd,
                        struct usb_host_endpoint *ep)
{
	WARN_UNIMPL;
}


/*
 * hubd is polling this funtion. it returns wich on wich port events occured
 */
static int ddeusb_vhcd_hub_status_data (struct usb_hcd *hcd, char *buf) {
    
    struct ddeusb_vhcd_priv_data *data = (struct ddeusb_vhcd_priv_data *) hcd->hcd_priv;
	int		ret = 0;
    unsigned int flags;
	unsigned long	*event_bits = (unsigned long *) buf;
	int		port;
	int		changed = 0;


	*event_bits = 0;
	
	spin_lock_irqsave(&data->lock, flags);
	if (!test_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags))
	{
		DEBUG_MSG("hw accessible flag in on?\n");
		goto done;
	}

	/* check pseudo status register for each port */
	for(port = 0; port < DDEUSB_VHCD_PORT_COUNT; port++)
	{
		if ((data->port_status[port] & PORT_C_MASK)) 
		{
			/* The status of a port has been changed, */
			DEBUG_MSG("port %d has changed\n", port);

			*event_bits |= 1 << ( port + 1);
			changed = 1;
		}
	}

	if (changed)
		ret = 1 + (DDEUSB_VHCD_PORT_COUNT / 8);
	else
		ret = 0;

done:
	spin_unlock_irqrestore(&data->lock, flags);
	return ret;	
}




static int ddeusb_vhcd_hub_control (struct usb_hcd *hcd,
                               u16 typeReq, u16 wValue, u16 wIndex,
                                char *buf, u16 wLength) 
{
   struct ddeusb_vhcd_priv_data *data =  (struct ddeusb_vhcd_priv_data *)  hcd->hcd_priv;
    unsigned int flags;	
    int ret = 0;
	int	port;	
	
    if (!test_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags))
		return -ETIMEDOUT;

	if (wIndex > DDEUSB_VHCD_PORT_COUNT)
	{
		DEBUG_MSG("invalid port number %d\n", wIndex);
		return -ENODEV;
	}
	
    /*
  	 * NOTE:
  	 * wIndex shows the port number and begins from 1.
  	 */	

	port = ((__u8 ) (wIndex & 0x00ff)) -1;
	DEBUG_MSG("port = %d \n", port);

	spin_lock_irqsave(&data->lock, flags);
	switch (typeReq)
	{
		case ClearHubFeature:
			DEBUG_MSG("unimplemented hub control request: ClearHubFeature");
			break;
		case ClearPortFeature:
			switch (wValue) 
			{
				case USB_PORT_FEAT_SUSPEND:
					DEBUG_MSG("unimplemented hub control request: ClearPortFeature USB_PORT_FEAT_SUSPEND");
					break;

 				case USB_PORT_FEAT_C_RESET:
 
					DEBUG_MSG("ClearPortFeature USB_PORT_FEAT_C_RESET");
 					
					switch (data->gadget[port].speed) {
 						case USB_SPEED_HIGH:
 							data->port_status[port] |= USB_PORT_STAT_HIGH_SPEED;
 							break;
						case USB_SPEED_LOW:
							data->port_status[port] |= USB_PORT_STAT_LOW_SPEED;
							break;
						default:
							break;
					}	
				case USB_PORT_FEAT_POWER:
					DEBUG_MSG("ClearPortFeature USB_PORT_FEAT_POWER");
					//data->port_status[port] = 0;
					
					/* FALLS THROUGH */

				default:
					DEBUG_MSG(" ClearPortFeature: default %x\n", wValue);
					data->port_status[port] &= ~(1 << wValue);
					break;
			}
			break;
		case GetHubDescriptor:
			DEBUG_MSG("hub_control_request: GetHubDescriptor");
			/* return the hub descriptor */ 
			hub_descriptor ((struct usb_hub_descriptor *) buf);
			
			break;
		case GetHubStatus:
			*(u32 *) buf = __constant_cpu_to_le32 (0);
			break;
		case GetPortStatus:
			if (wIndex > DDEUSB_VHCD_PORT_COUNT || wIndex < 1) {
				DEBUG_MSG(" invalid port number %d\n", wIndex);
				ret = -EPIPE;
			}


			if ((data->port_status[port] & (1 << USB_PORT_FEAT_RESET)) != 0
					/* && time_after (jiffies, dum->re_timeout) */) 
			{
				data->port_status[port] |= (1 << USB_PORT_FEAT_C_RESET);
				data->port_status[port] &= ~(1 << USB_PORT_FEAT_RESET);
				
				if (data->gadget[port].id)
				{
					DEBUG_MSG(" enabled port %d ddeusb device id %p)\n", port, data->gadget[port].id);
					data->port_status[port] |= USB_PORT_STAT_ENABLE;
				}
				DEBUG_MSG(" enable rhport %d (status)\n", port);
				data->port_status[port] |= USB_PORT_STAT_ENABLE;
		
			}
			((__le16 *) buf)[0] = cpu_to_le16 (data->port_status[port]);
			((__le16 *) buf)[1] = cpu_to_le16 (data->port_status[port] >> 16);

			break;
		case SetHubFeature:
			DEBUG_MSG("unimplemented hub control request: SetHubFeature");
			ret = -EPIPE;
			break;
		case SetPortFeature:
			switch (wValue) {
				case USB_PORT_FEAT_SUSPEND:
		            DEBUG_MSG("unimplemented hub control request: SetPortFeature USB_PORT_FEAT_SUSPEND");
					break;
				case USB_PORT_FEAT_RESET:
					/* if it's already running, disconnect first */
					if (data->port_status[port] & USB_PORT_STAT_ENABLE) 
					{
						data->port_status[port] &= ~(USB_PORT_STAT_ENABLE
								| USB_PORT_STAT_LOW_SPEED
								| USB_PORT_STAT_HIGH_SPEED);

					}
				default:
				data->port_status[port] |= (1 << wValue);
					break;
			}
			break;
		

		default:	
			DEBUG_MSG("no such request.");
	}
	spin_unlock_irqrestore(&data->lock, flags);
	return ret;
}



static int ddeusb_vhcd_bus_suspend (struct usb_hcd * hcd) {
	WARN_UNIMPL;
	return 0;
}
static int ddeusb_vhcd_bus_resume (struct usb_hcd * hcd) {
	WARN_UNIMPL;
	return 0;
}
static int ddeusb_vhcd_start_port_reset(struct usb_hcd *hcd, unsigned port_num)
{
	WARN_UNIMPL;
	return 0;
}
static void ddeusb_vhcd_hub_irq_enable(struct usb_hcd *hcd)
{
	WARN_UNIMPL;
}


static struct hc_driver ddeusb_vhcd_driver= {
	.description=driver_name,	/* "ehci-hcd" etc */
	.product_desc=driver_desc,	/* product/vendor string */
	
	.hcd_priv_size= sizeof(struct ddeusb_vhcd_priv_data),

	.flags = HCD_USB2,

	.reset = ddeusb_vhcd_reset,
	.start = ddeusb_vhcd_start,

	.suspend = ddeusb_vhcd_suspend,

	.resume = ddeusb_vhcd_resume,

	.shutdown = ddeusb_vhcd_shutdown,
 
	.get_frame_number = ddeusb_vhcd_get_frame_number,
	.urb_enqueue = ddeusb_vhcd_urb_enqueue,
	.urb_dequeue = ddeusb_vhcd_urb_dequeue,

	.endpoint_disable = ddeusb_vhcd_endpoint_disable,

	.hub_status_data = ddeusb_vhcd_hub_status_data,
	.hub_control = ddeusb_vhcd_hub_control,
	.bus_suspend = ddeusb_vhcd_bus_suspend,
	
	.bus_resume = ddeusb_vhcd_bus_resume,
	.start_port_reset = ddeusb_vhcd_start_port_reset,
	.hub_irq_enable = ddeusb_vhcd_hub_irq_enable,
};

static int ddeusb_vhcd_pd_probe(struct platform_device *pdev)
{
	int ret,err;
	struct usb_hcd* hcd;
    
	
	/* check if it is our device... */
	if(strcmp(pdev->name,driver_name)) {
		DEBUG_MSG("this device is not my business...\n");
		return -ENODEV;
	}
	
	/* create the hcd */
	hcd = usb_create_hcd(&ddeusb_vhcd_driver, &pdev->dev, pdev->dev.bus_id);
	if (!hcd) {
		DEBUG_MSG("create hcd failed\n");
		return -ENOMEM;
	}
	

	/* make it known to the world! */
	ret = usb_add_hcd(hcd, 0, 0);



	return ret;	
}

static int ddeusb_vhcd_pd_remove(struct platform_device *pdev)
{
	WARN_UNIMPL;
	return 0;
}

static int ddeusb_vhcd_pd_suspend(struct platform_device *pdev,pm_message_t state)
{
	WARN_UNIMPL;
	return 0;
	
}

static int ddeusb_vhcd_pd_resume(struct platform_device *pdev)
{
    WARN_UNIMPL;
	return 0;
}

void ddeusb_vhcd_pd_release(struct device *dev)
{
    WARN_UNIMPL;
}

static struct platform_driver ddeusb_vhcd_pdriver ={
	.probe	= ddeusb_vhcd_pd_probe,
	.remove	= ddeusb_vhcd_pd_remove,
	.suspend = ddeusb_vhcd_pd_suspend,
	.resume	= ddeusb_vhcd_pd_resume,
	.driver	= {
		.name = (char *) driver_name,
		.owner = THIS_MODULE,
	},
};

static struct platform_device ddeusb_vhcd_pdev = {
	/* should be the same name as driver_name */
	.name = (char *) driver_name,
	.id = -1,
	.dev = {
		.release = ddeusb_vhcd_pd_release,
		.dma_mask = 0
	},
};


static struct libddeusb_callback_functions callbacks= 
{
	.reg_dev      = ddeusb_register_device,
	.discon_dev   = ddeusb_disconnect_device,
	.complete_d_urb = ddeusb_d_urb_complete 
};


static int server_thread(void * unused) {
    ddeusb_urb *du;
    libddeusb_register_callbacks(&callbacks);
    du = (ddeusb_urb*)ddekit_simple_malloc(50*sizeof(ddeusb_urb));
    if(du)
        libddeusb_server_loop("name?" ,ddekit_simple_malloc,  ddekit_simple_free, du,50);
    BUG();
}


int __init ddeusb_vhcd_init(void)
{
	int ret;
	if (usb_disabled())
		return -ENODEV;
	
	info("driver %s, %s\n", driver_name, driver_desc);
	
    DEBUG_MSG("...registering platformdevice...\n");
	ret = platform_device_register(&ddeusb_vhcd_pdev);
	if (ret < 0)
		goto err_platform_device_register;
	
	DEBUG_MSG("...registering platformdriver...\n");
	ret = platform_driver_register(&ddeusb_vhcd_pdriver);
	if (ret < 0)
		goto err_driver_register;

	DEBUG_MSG("init returns\n");
	
	/* creating cache for privs */
    priv_cache  = kmem_cache_create("privcache", sizeof(struct ddeusb_vhcd_urbpriv),0,0,0,0);

	
	/* we'll start the notification thread here but:
	 * we have to be shure that this is called before any
	 * other usb(gadget) driver, because this would lead into
	 * deadlock */
    kthread_run(server_thread,(void *)NULL, "vhcd_srvth");

return ret;
	
	/* error occurred */

err_driver_register:

err_platform_device_register:
	platform_device_unregister(&ddeusb_vhcd_pdev);
	return ret;
}
fs_initcall(ddeusb_vhcd_init);

