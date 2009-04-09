/**
 *  \file    libddeusb.h
 *  \date    2008-10-23
 *  \author  Dirk Vogt <dvogt os.inf.tu-dresden.de>
 *  
 *  This is the interface to libddeusb. This library has three purposes:
 * 
 *  - IPC-abstraction,
 *  - Notification server implementation,
 *  - and managing the shared memory region containing the arguments of 
 *    the ddeusb_submit_d_ urb() function call.
 *
 *   
 **/

/*
 *   Copyright (C) 2007-2008  Dirk Vogt  <dvogt@os.inf.tu-dresden.de>
 *   
 *   This file is part of the DDEUSB package, which is distributed under
 *   the  terms  of the  GNU General Public Licence 2.  Please see the
 *   COPYING file for details.
 */


#ifndef L4_USB_LIBDDEUSB_H
#define L4_USB_LIBDDEUSB_H

#include <l4/sys/types.h>

#include <l4/usb/usb_types.h>

/**
 * \struct libddeusb_callback_functions
 * \brief Notification server callback functions
 *
 *        This Library implements a notification server, which is interfaced
 *        be the client be providing several callback functions, defined in
 *        this struct. The callback functions are passed to libddeusb by
 *        filling a struct of this type with pointers to the callback functions
 *        and calling  libddeusb_register_callbacks().
 *
 **/
struct libddeusb_callback_functions
{
	/**
     * \brief new device notification
     *        
     *        This notification is called whenever a new USB device was
     *        forwarded to the client.
     *
     *
     *        \param dev_id The device_id of the new device.
     *        \param speed The speed of the forwarded device (according to the.
     *        usb spec)
     *        \param power The power consumption of the device (obsolete).
     *        \param dma_mask The DMA-mask of the host controller (not used yet...).
     *               
     */
    L4_CV int (*reg_dev) (int dev_id, 
                          int speed, 
                          unsigned short bus_mA, 
                          unsigned long long dma_mask );

    /**
     * \brief device disconnect notification
     *
     *        This notification informs the client, that the USB device with
     *        the given device id is disconnected from the bus or revoked by
     *        the server. After this notification no more USB transaction for
     *        this device are allowed from the DDEUSB-server.
     *
     *
     *        \param dev_id The device ID of the revoked device.
     **/

    L4_CV void (*discon_dev) (int dev_id) ;
#if 0    
    /** 
     * \brief complete urb notification (DEPRECATED)
     * 
     * This notification informs the client about the completion of the URB
     * with URB ID \a urb_id.
     */


    L4_CV void (*complete_urb) (int urb_id, 
                                int status,
                                int actual_length,
                                int interval,
                                const char * transfer_buffer,
                                l4_size_t tb_size,
                                const char  setup_packet[8],
                                unsigned int transfer_flags,
                                int error_count,
                                int start_frame,
                                const char *iso_frame_desc,
                                l4_size_t iso_frame_desc_size);
#endif 

     /** 
     * \brief complete urb notification
     * 
     * This notification informs the client about the completion of \a d_urb. 
     * .
     */

	L4_CV void (*complete_d_urb) (ddeusb_urb *d_urb);
};

/**
 * \brief DDEUSB server loop
 *        
 *        This functions tries to register the client at the ddeusb
 *        server, and then starts the notification server.
 *        
 *        \param name the name with which the client will register at the
 *               server.
 *        \param _malloc memory allocation function for the notification server
 *        \param _free memory freeing function for the notification server
 *        \param d_urbs shared memory location of the d_urbs
 *        \param count number of d_urbs in shared memory location (i.e. size of shared. mem)
 *
 *        \return will not return if successfull, otherwise negative error number.   
 **/
int L4_CV libddeusb_server_loop(char *            name,
                                void *(*malloc)(l4_size_t),
                                void (*free)(void *)  ,
                                ddeusb_urb *      d_urbs, 
                                int               count);
/*****************************************************************************/


/**
 * \brief Subscribe for specific USB device id.
 *
 * This function is used by the client to subscribe to a given USB device ID \a
 * id.
 *
 * \param id The USB device ID the client wants to subscribe for.
 *
 * \return 0 on success, otherwise negative error number.
 */
int L4_CV  libddeusb_subscribe_for_device_id (ddeusb_usb_device_id * id);
/*****************************************************************************/

#if 0
/** 
 * \brief submits a URB to the server.
 */
int L4_CV  libddeusb_submit_urb(int urb_id,
                           int dev_id,
                           int type,
                           int endpoint,
                           int direction,
                           unsigned int transfer_flags,
                           int interval,
                           int start_frame,
                           int number_of_packets,
                           char * setup_packet,
                           void *data,
                           l4_size_t size,
                           void * iso_frame_desc,
                           l4_size_t iso_frame_desc_size);

#endif


/**
 * Submits a DDEUSB URB to the server
 * \param d_urb the DDEUSB URB to submit
 */
int L4_CV libddeusb_submit_d_urb(ddeusb_urb *d_urb);
/*****************************************************************************/


#if 0
 /*
 * \brief cancels the transmission of the urb with the given URB ID
 *
 * \param urb_id the ID of the URB to cancel
 */
void L4_CV libddeusb_unlink_urb(int urb_id);
/*****************************************************************************/
#endif

/**
 *  \brief cancels the transmission of the urb with the given DDEUSB URB
 *
 *  \param d_urb the DDEUSB URB to cancel
 */
void L4_CV libddeusb_unlink_d_urb(ddeusb_urb *d_urb);

/**
 * \brief frees a DDEUSB URB
 * 
 * This function frees a DDUESB URB, previously allocated with 
 * libddeusb_get_free_d_urb().
 */
void L4_CV libddeusb_free_d_urb(ddeusb_urb *d_urb); 
/*****************************************************************************/

/**
 * \brief  allocates a DDEUSB URB
 *
 * \return Pointer to allocated ddeusb_urb on success,  otherwise NULL.
 **/
ddeusb_urb * L4_CV libddeusb_alloc_d_urb(int d_urb_type);
/*****************************************************************************/


/**
 * \brief Sets the callback functions of the notification server.
 */ 
void L4_CV libddeusb_register_callbacks(struct libddeusb_callback_functions *callbacks);
/*****************************************************************************/


/**
 * \brief Returns the thread ID of the DDEUSB server
 *
 * \return the thread id of the DDEUSB server
 */
l4_threadid_t libddeusb_get_server_id(void);
/*****************************************************************************/

#endif
