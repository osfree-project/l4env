#ifndef L4_USB_USB_TYPES_H
#define  L4_USB_USB_TYPES_H
/** 
 * \file usb_types.h
 **/

/* usb_types.h */
#include  <l4/sys/types.h>
#include <l4/sys/l4int.h>
#include <l4/dm_generic/types.h>

#define USB_MAXINTERFACES	32

/** isochronous transfer */
#define DDEUSB_ISO 0
/** interrupt transfer */
#define DDEUSB_INT 1
/** control transfer */
#define DDEUSB_CTL 2
/** bulk transfer */


#define DDEUSB_BLK 3

/* receive */
#define DDEUSB_RCV 0

#define  __u16 unsigned short
#define __u8 unsigned char


/** \struct ddeusb_usb_device_id_s 
 * ...a one to one copy of the Linux USB device ID struct */
typedef struct ddeusb_usb_device_id_s {
	/* which fields to match against? */
	__u16		match_flags;

	/* Used for product specific matches; range is inclusive */
	__u16		idVendor;
	__u16		idProduct;
	__u16		bcdDevice_lo;
	__u16		bcdDevice_hi;

	/* Used for device class matches */
	__u8		bDeviceClass;
	__u8		bDeviceSubClass;
	__u8		bDeviceProtocol;

	/* Used for interface class matches */
	__u8		bInterfaceClass;
	__u8		bInterfaceSubClass;
	__u8		bInterfaceProtocol;

	/* not matched against */
	unsigned long	driver_info;

} ddeusb_usb_device_id;

/** 
 * \struct ddeusb_usb_iso_packet_descriptor_s
 */
typedef struct ddeusb_usb_iso_packet_descriptor_s {
	unsigned int offset;
	unsigned int length;		/* expected length */
	unsigned int actual_length;
	unsigned int status;
} ddeusb_usb_iso_packet_descriptor;



/* the transfer buffer transfer mechanisms: */
/** use the fall-back method */
#define DDEUSB_D_URB_TYPE_FALLBACK 0
/** use dynamical shared memory */
#define DDEUSB_D_URB_TYPE_SHM 1
/** use the buffer in DDEUSB URB */
#define DDEUSB_D_URB_TYPE_COPY 2
/** transmit physical addresses*/
#define DDEUSB_D_URB_TYPE_PHYS 3
/** let libddeusb device how to transmit transfer buffer */ 
#define DDEUSB_D_URB_TYPE_AUTO 4

/** the max size if the transfer buffer in a DDEUSB URB */
#define DDEUSB_D_URB_COPY_BUF_SIZE (16384)
/** THE max size of the iso frame descriptors in a DDEUSB URB */
#define DDEUSB_D_URB_ISODESC_SIZE (1024)

/**
 * \struct ddeusb_urb_s
 * \brief The DDEUSB URB data structure.
 * 
 * The DDEUSB URB data-structure is the corresponding data structure for a USB IO request package (or
 * Linux URB).
 **/
typedef struct ddeusb_urb_s {
    
    /** (IN) The client-side ID of the device */
    int dev_id;                             
    
    /** (IN) The USB transfer type of this URB */
    int type; 

    /** (IN) The number of the target endpoint of this transaction */
    int endpoint;

    /** (IN) The direction of this transaction */
    int direction;

    /** (CLIENT PRIV) The id of this transaction, perhaps it would be better 
     * to put this into the client's private datastructure, because it is not
     * needed by the server */
    unsigned int urb_id;

    /** (IN+OUT) The intervall of the USB transfer (in case of periodic 
     *  transfers) */
    int interval;
    
    /** (IN) The number of isochronous packets */
    int number_of_packets;

    /** (IN+OUT) The start frame of isochronous packets */
    int start_frame;

    /** (IN) The buffer transfer type to use:  DDEUSB_D_URB_TYPE_SHM, 
     *  DDEUSB_D_URB_TYPE_COPY, or DDEUSB_D_URB_TYPE_PHYS */
    int d_urb_type;

    /** (IN+OUT)
     *  Pointer to the transferbuffer of the urb in case of 
     *  DDEUSB_D_URB_TYPE_FALLBACK
     */
    void* data;
    
    /** (IN+OUT)This buffer should be used for copying transfer buffers 
     *  in shared memory in case of DDEUSB_D_URB_TYPE_COPY)*/
    char buf[DDEUSB_D_URB_COPY_BUF_SIZE];


    /** (IN) the physical address of the buffer, in case DDEUSB_D_URB_TYPE_PHYS */
    l4_addr_t phys_addr;

    /** (IN) the dataspace containing the transfer buffer, in case of
     *  DDEUSB_D_URB_TYPE_SHM */
    l4dm_dataspace_t ds;
    
    /** (IN) the offset of the buffer in the dataspace, in case of 
     *  DDEUSB_D_URB_TYPE_SHM */
    l4_offs_t offset;

    /** (IN) the size of the transfer buffer (in all cases) */
    l4_size_t size; 

    /** (IN+OUT) the transfer flags (according to those of the Linux URB */
    unsigned int transfer_flags;

    /** (IN+OUT) the setup packed, in case of config transfer */
    unsigned char setup_packet[8];
                                                

    /** (RETURN) the status of this transfer */
    int status;

    /** (RETURN) the actual number of bytes transfered */
    int actual_length;
    
    /** (RETURN) the number of errors that ocurred during this transfer */
    int error_count;

    /** (IN+OUT) The iso framedesciptors */
    char iso_desc[DDEUSB_D_URB_ISODESC_SIZE];

    /** size of iso_frame_descriptors in case of DDEUSB_D_URB_TYPE_FALLBACK */
    l4_size_t iso_desc_size;

    // client-data 
    
    /** (CLIENT PRIV) the pointer to private client data */
    void *priv; 

    // libddeusb
    struct ddeusb_urb_s * next;             
    int index; 
} ddeusb_urb;

#endif
