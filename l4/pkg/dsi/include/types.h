/* $Id$ */
/*****************************************************************************/
/**
 * \file  dsi/include/types.h
 * \brief DROPS Stream Interface public types.
 *
 * \date   07/01/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _DSI_TYPES_H
#define _DSI_TYPES_H

/* DROPS/L4 includes */
#include <l4/sys/types.h>
#include <l4/l4rm/l4rm.h>
#include <l4/thread/thread.h>
#include <setjmp.h>

/*****************************************************************************
 * generic types                                                             *
 *****************************************************************************/

/*!\brief	jitter constrained periodic stream 
 * \ingroup	general
 */
typedef struct dsi_jcp_stream
{
  l4_uint32_t bw;          //!< bandwidth, byte/s
  l4_uint32_t tau;         //!< jitter, microseconds
  l4_uint32_t size;        //!< packet size
} dsi_jcp_stream_t;

/*!\brief	low level stream configuration
 * \ingroup	general
 *
 * for now it is specified explicitly, at some stage it should be
 * calculated from jitter constrained periodic stream definition 
 */
typedef struct dsi_stream_cfg
{
  l4_uint32_t num_packets; //!< number of control packets
  l4_uint32_t max_sg;      //!< max. length of packet scatter gather list
} dsi_stream_cfg_t;

/*****************************************************************************
 * component types
 *****************************************************************************/

/*****************************************************************************
 * packet
 *****************************************************************************/

/*!\brief	packet semaphore type 
 * \ingroup	component
 */
typedef volatile l4_int8_t dsi_semaphore_t;

/*!\brief	packet descriptor 
 * \ingroup	component
 */
typedef struct dsi_packet
{
  l4_uint32_t      no;      //!< packet number
  l4_uint16_t      flags;   //!< packet flags
  dsi_semaphore_t  tx_sem;  //!< send semaphore counter (__get_send_packet())
  dsi_semaphore_t  rx_sem;  /**< receive semaphore counter 
			       (__get_receive_packet()) */
  l4_uint64_t      expires; //!< expire time
  
  l4_uint32_t      sg_len;  //!< length of scatter gather list
  l4_uint32_t      sg_list; /*!< pointer to scatter gather list, it is an index
			      to the scatter gather element array */
  l4_uint32_t      sg_idx;  /*!< index in scatter gather list. Used in 
			      dsi_packet_add_data() to store the index of the
			      last element in the list, in
			      dsi_packet_get_data() to store the index of
			      the next data area to get */
} dsi_packet_t;

/* packet flags */
#define DSI_PACKET_RELEASE_CALLBACK 0x0001  /* send notify message if packet
					     * release by receiver */
#define DSI_PACKET_MAP              0x0002  /* explicitly map packet data */
#define DSI_PACKET_COPY             0x0004  /* copy packet data */

/* internal flags */
#define DSI_PACKET_TX_WAITING       0x8000  /* sender is waiting for packet */
#define DSI_PACKET_TX_PENDING       0x4000  /* sender wakeup notification 
					     * pending for this packet */
#define DSI_PACKET_RX_WAITING       0x2000  /* receiver is waiting for 
					     * packet */
#define DSI_PACKET_RX_PENDING       0x1000  /* receiver wakeup notification is
					     * pending for this packet */

#define DSI_PACKETS_USER_MASK       0x0FFF  /* flags defined by user */

/*****************************************************************************
 * sgatter gather element
 *****************************************************************************/

/*!\brief	scatter gather element 
 * \ingroup	component
 */
typedef struct dsi_sg_elem
{
  l4_addr_t    addr;	   /*!< data area start address, it is relative to the
                                start address of the data dataspace */
  l4_size_t    size;       //!< data area size
  l4_uint32_t  flags;      //!< data area flags

  l4_uint32_t  next;       //!< index of next element, DSI_SG_ELEM_LAST end
} dsi_sg_elem_t;

/* scatter gather list flags */
#define DSI_SG_ELEM_UNUSED    0x00000000  /* internal, sg element unused */
#define DSI_SG_ELEM_USED      0x80000000  /* internal, sg element used */
#define DSI_SG_ELEM_USER_MASK 0x0FFFFFFF  /* flags defined by user */

#define DSI_DATA_AREA_GAP     0x00000001  /* empty data area, it describes a
					   * gap in the data stream */
#define DSI_DATA_AREA_MAP     0x00000002  /* map data area before usage */
#define DSI_DATA_AREA_EOS     0x00000004  /* empty data area, the area marks 
					   * the end of the stream */
#define DSI_DATA_AREA_PHYS    0x00000008  /* chunk contains physical address */

#define DSI_SG_ELEM_LAST      0xFFFFFFFF

/*****************************************************************************
 * socket
 *****************************************************************************/

/**
 * \brief	control area header 
 * \ingroup	component
 */
typedef struct dsi_ctrl_header
{
  /* control area config */
  l4_uint32_t num_packets;	///< number of packets
  l4_uint32_t num_sg_elems;	///< number of scatter/gather list elements
  l4_uint32_t max_sg_len;	///< maximum length of sg-list
} dsi_ctrl_header_t;

/*!\brief	socket id
 * \ingroup	app
 *
 * It is used as a global reference to a socket e.g. in 
 * synchronisation messages between sender and receiver component  
 */ 
typedef l4_int32_t dsi_socketid_t;

/*!\brief	socket reference
 * \ingroup	app
 *
 * It is given to the application by the component to 
 * describe a socket 
 */
typedef struct dsi_socket_ref
{
  /* socket descriptor */
  dsi_socketid_t socket;   //!< reference to socket

  /* thread ids */
  l4_threadid_t  work_th;  //!< work thread
  l4_threadid_t  sync_th;  //!< synchronisation thread
  l4_threadid_t  event_th; ///< event signalling thread
} dsi_socket_ref_t;

/*!\brief	socket type 
 * \ingroup	component
 */
typedef struct dsi_socket dsi_socket_t;

/*!\brief	prototype for synchronization callback function
 * \ingroup	component
 */
typedef void(* dsi_sync_callback_fn_t) (dsi_socket_t * socket, 
					l4_uint32_t packet_no,
					l4_uint32_t flags);

/* flags */
#define DSI_SYNC_NO_SEND_PACKET      0x00000001
#define DSI_SYNC_NO_RECEIVE_PACKET   0x00000002

/*!\brief prototype for release packet callback function
 * 
 * \ingroup component
 */
typedef void(* dsi_release_callback_fn_t) (dsi_socket_t * socket,
					   dsi_packet_t * packet);

/**
 * Event handling
 * max. number of events, events are specified as a bit field, we need
 * 2 bits in the IPC to the components signalling thread to encode
 * the command, therefore we have 30 bits left to specify the event mask
 */
#define DSI_MAX_EVENTS  30

/**
 * Event client descriptor
 */
typedef struct dsi_event_client
{
  l4_threadid_t             id;      ///< L4 thread id of client
  l4_uint32_t               events;  ///< event mask
  struct dsi_event_client * next;    ///< client list
} dsi_event_client_t;

/*!\brief	socket descriptor
 * \ingroup	component
 *
 * It is the component-local socket reference.
 *
 */ 
struct dsi_socket
{
  dsi_ctrl_header_t *       header;             //!< header
  dsi_packet_t *            packets;            //!< packet array
  dsi_sg_elem_t *           sg_lists;           //!< sg elem array

  volatile l4_uint32_t      flags;              //!< flags

  void *                    data_area;          //!< start address of data area
  l4_size_t                 data_size;          //!< size of data area
  l4_size_t                 data_map_size;      ///< size of data map area (log2)

  l4dm_dataspace_t          data_ds;            //!< dataspace for data
  l4dm_dataspace_t          ctrl_ds;            //!< control dataspace

  /* callbacks and abort helpers */
  dsi_sync_callback_fn_t    sync_callback;      //!< sync event callback
  dsi_release_callback_fn_t release_callback;   //!< release notification callback

  /* thread ids */
  l4_threadid_t             work_th;            //!< thread id work thread
  l4_threadid_t             sync_th;            //!< synchronisation thread
  l4thread_t                sync_id;            ///< sync thread (thread lib id)

  dsi_socketid_t            socket_id;		//!< my socket id

  dsi_socket_ref_t          remote_socket;      //!< remote socket reference

  l4_uint32_t		    num_sg_elems;	//!< number of sg_elems in array
  l4_uint32_t		    num_packets;	//!< number of packets in array

  /* internal counter */
  l4_uint32_t               packet_count;       //!< packet counter
  l4_uint32_t               next_packet;        /*!< index of next
                                                     send/receive packet */
  l4_uint32_t               next_sg_elem;       /*!< index of next scatter
                                                     gather list element (used
                                                     in  dsi_packet_add_data()
                                                     to find next empty
                                                     element) */
  void *                    next_buf;           /**< next receive buffer if
						 **  copy packet data */

  void *	            priv;		//!< to be used by upper layers

  /* events */
  l4_uint32_t               events[DSI_MAX_EVENTS]; ///< event counter 
  l4_uint32_t               waiting;             /**< bit field indicating for
                                                  **  which event a client is 
						  **  waiting */
  dsi_event_client_t *      clients;             ///< client wait queue
  
  /* packet_get abort data */
  l4_uint32_t		    abort_stack[20];	// more than enought
  jmp_buf		    packet_get_abort_env;/*!< used when aborting a
						      dsi_packet_get() */
  
};

/* socket flags */
#define DSI_SOCKET_USED             0x00000000 /* internal */
#define DSI_SOCKET_UNUSED           0x80000000 /* internal */

#define DSI_SOCKET_FREE_SYNC        0x40000000 /* internal, shutdown sync 
						* thread on close */   
#define DSI_SOCKET_FREE_CTRL        0x20000000 /* internal, release control 
						* dataspace on close */
#define DSI_SOCKET_FREE_DATA        0x10000000 /* internal, release data 
						* dataspace on close */

#define DSI_SOCKET_SEND             0x01000000 /* send socket */
#define DSI_SOCKET_RECEIVE          0x02000000 /* receive socket */

#define DSI_SOCKET_BLOCKING_IN_GET  0x00010000 /* currently blocking in
						  packet_get() */
#define DSI_SOCKET_BLOCK_ABORT      0x00020000 /* Aborting from packet_get()
						  requested */

#define DSI_SOCKET_BLOCK            0x00000001 /* block if no packet descriptor 
						* is available, wait for 
						* partner to commit packet.
						* Specify either NONBLOCK or
						* BLOCK. */
#define DSI_SOCKET_SYNC_CALLBACK    0x00000002 /* call callback function if 
						* synchronization occurred 
						* (but only if blocked) */
#define DSI_SOCKET_RELEASE_CALLBACK 0x00000004 /* notify sender about packet 
						* release by receiver */
#define DSI_SOCKET_MAP              0x00000010 /* map packet data */
#define DSI_SOCKET_COPY             0x00000020 /* copy packet data */

#define DSI_SOCKET_USER_FLAGS       0x0000FFFF /* flags which can be set at 
						* runtime */

/*****************************************************************************
 * application types                                                         *
 *****************************************************************************/

/*!\brief	component descriptor 
 * \ingroup	app
 */
typedef struct dsi_component
{
  dsi_socket_ref_t socketref;	//!< socket reference

  /* component interface functions to connect/start/stop/close sockets */
  void		   *priv;	/*!< Service pointer for component functions.
  				     Can be set by component creator as a
  				     hint for the connect, start, stop
  				     and close callbacks */
  int(*connect)(struct dsi_component*, dsi_socket_ref_t * remote);
  				//!< interface function for connect action
  int(*start)(struct dsi_component*);
  				//!< interface function for start action
  int(*stop)(struct dsi_component*);
  				//!< interface function for stop action
  int(*close)(struct dsi_component*);
  				//!< interface function for close action
} dsi_component_t;

/*!\brief	application stream descriptor
 * \ingroup	app
 *
 * Used by in-the-middle-applications to store send- and
 * receive-components. Used by dsi_stream_create(), dsi_stream_start(),
 * dsi_stream_stop() and dsi_stream_close().
 *
 */
typedef struct dsi_stream
{
  /* send/receive component */
  dsi_component_t   sender;	     //!< send component
  dsi_component_t   receiver;	     //!< receive component

  /* data/control dataspaces */
  l4dm_dataspace_t  data;	     //!< data dataspace
  l4dm_dataspace_t  ctrl;	     //!< ctrl dataspace

  l4_uint32_t       flags;	     //!< stream flags

  void *            __private;	     //!< private data
} dsi_stream_t;

/* stream flags */
#define DSI_STREAM_USED        0x00000000   /* internal */
#define DSI_STREAM_UNUSED      0x80000000   /* internal */

/* component flags */
#define DSI_SEND_COMPONENT     0x00000001   ///< operation on sender
#define DSI_RECEIVE_COMPONENT  0x00000002   ///< operation on receiver

/**
 * dsi_event_select socket list
 */
typedef struct dsi_select_socket
{
  dsi_stream_t *  stream;      ///< stream descriptor
  l4_uint32_t     component;   /**< component (either \c DSI_SEND_COMPONENT
                                *   or \c DSI_RECEIVE_COMPONENT) */
  l4_uint32_t     events;      ///< event mask
} dsi_select_socket_t;

/* some predefined events */
#define DSI_EVENT_EOS          0x00000001   ///< end of stream
#define DSI_EVENT_DROPPED      0x00000002   ///< dropped packet
#define DSI_EVENT_ERROR        0x00000004   ///< error in component

#endif /* !_DSI_TYPES_H */
