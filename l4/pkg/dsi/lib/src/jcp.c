/* $Id: */
/*****************************************************************************/
/*! \file dsi/lib/src/jcp.c
 *
 * \date   11/28/2000
 * \author Jork Loeser <jork_loeser@inf.tu-dresden.de>
 *
 * \brief  Jitter-Constrained-Periodic-Streams stuff
 */
/*****************************************************************************/

#include <l4/dsi/dsi.h>


/*!\brief Convert jcp to stream-config
 *
 * \ingroup general
 *
 * \param  jcp	 JCP-params bw, tau, packetsize
 * \retval s_cfg contains number of packets and max_sg=1
 */
void dsi_jcp_2_config(dsi_jcp_stream_t *jcp, dsi_stream_cfg_t *s_cfg){
    int pps;		// packets per second
    
    pps=(jcp->bw+jcp->size-1)/jcp->size+1;
    s_cfg->num_packets = ((double)jcp->tau)*((double)pps)/1000000. + 2;
    s_cfg->max_sg = 1;
}

