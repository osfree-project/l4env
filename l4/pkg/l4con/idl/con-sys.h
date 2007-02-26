/* $Id$ */

/*	con/idl/con/con-sys.h
 *
 *	con IPC interface constants
 */

#ifndef _CON_CON_SYS_H
#define _CON_CON_SYS_H

/* IPC function numbers */

/* `simple' if */
#define req_con_if_openqry		0x0000
#define req_con_if_direct_openqry	0x000e
#define req_con_if_dsi_openqry		0x000f
#define req_con_if_close_all		0x0010
#define req_con_if_screenshot		0x0011

/* vc if */
#define req_con_vc_smode		0x0001
#define req_con_vc_gmode		0x0002
#define req_con_vc_close		0x0003
#define req_con_vc_ev_sflt		0x00e1
#define req_con_vc_ev_gflt		0x00e2
#define req_con_vc_graph_smode		0x00da
#define req_con_vc_graph_gmode		0x00db
#define req_con_vc_graph_mapfb		0x00dc
#define req_con_vc_pslim_set		0x00f1
#define req_con_vc_pslim_bmap		0x00f2
#define req_con_vc_pslim_fill		0x00f3
#define req_con_vc_pslim_copy		0x00f4
#define req_con_vc_pslim_cscs		0x00f5
#define req_con_vc_puts 		0x00f6
#define req_con_vc_puts_attr 		0x00f7
#define req_con_vc_direct_setfb		0x00f8
#define req_con_vc_direct_update	0x00f9
#define req_con_vc_stream_cscs		0x00fa

/* dsi_vc if */
#define req_con_dsi_vc_open		0x1001
#define req_con_dsi_vc_connect		0x1002
#define req_con_dsi_vc_rawset_open	0x1003
#define req_con_dsi_vc_rawcscs_open	0x1004
#define req_con_dsi_vc_bmap_open	0x1005
#define req_con_dsi_vc_smode	        0x1006
#define req_con_dsi_vc_graph_gmode	0x1007

#endif /* !_CON_CON_SYS_H */

