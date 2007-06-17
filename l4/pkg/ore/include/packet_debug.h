/****************************************************************
 * (c) 2007 Technische Universitaet Dresden                     *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

/* Packet debugging functions. These functions can be used for
 * tracing network packets that travel through ORe or some other
 * network client.
 */

#ifndef __PACKET_DEBUG_H
#define __PACKET_DEBUG_H

/* Print information about a specific packet.
 *
 * \param packet    pointer to the beginning of the packet's
 *                  ethernet header
 */
void packet_debug(unsigned char *packet);

#endif
