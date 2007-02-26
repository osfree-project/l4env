/* Mostly copied from Linux. See the DDE_LINUX define for diffs */

#ifndef _WRAPPER_H_
#define _WRAPPER_H_

#ifndef DDE_LINUX
#define mem_map_reserve(p)	set_bit(PG_reserved, &((p)->flags))
#define mem_map_unreserve(p)	clear_bit(PG_reserved, &((p)->flags))
#else /* DDE_LINUX */
#define mem_map_reserve(p)
#define mem_map_unreserve(p)
#endif /* DDE_LINUX */

#endif /* _WRAPPER_H_ */
