/* ov772x Camera
 *
 * Copyright (C) 2008 Renesas Solutions Corp.
 * Kuninori Morimoto <morimoto.kuninori@renesas.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __OV772X_H__
#define __OV772X_H__

#include <media/soc_camera.h>

/* for flags */
#define OV772X_FLAG_VFLIP     0x00000001 /* Vertical flip image */
#define OV772X_FLAG_HFLIP     0x00000002 /* Horizontal flip image */

struct ov772x_camera_info {
	unsigned long          buswidth;
	unsigned long          flags;
	struct soc_camera_link link;
};

#endif /* __OV772X_H__ */
