/*
#             (C) 2008 Hans de Goede <j.w.r.degoede@hhs.nl>

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __LIBV4LCONVERT_H
#define __LIBV4LCONVERT_H

/* These headers are not needed by us, but by linux/videodev2.h,
   which is broken on some systems and doesn't include them itself :( */
//#include <sys/time.h>
//#include <linux/types.h>
//#include <linux/ioctl.h>
/* end broken header workaround includes */
#include <linux/videodev2.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if __GNUC__ >= 4
#define LIBV4L_PUBLIC __attribute__ ((visibility("default")))
#else
#define LIBV4L_PUBLIC
#endif

struct v4lconvert_data;

LIBV4L_PUBLIC struct v4lconvert_data *v4lconvert_create(int fd);
LIBV4L_PUBLIC void v4lconvert_destroy(struct v4lconvert_data *data);

/* With regards to dest_fmt just like VIDIOC_TRY_FMT, except that the try
   format will succeed and return the requested V4L2_PIX_FMT_foo in dest_fmt if
   the cam has a format from which v4lconvert can convert to dest_fmt.
   The real format to which the cam should be set is returned through src_fmt
   when not NULL. */
LIBV4L_PUBLIC int v4lconvert_try_format(struct v4lconvert_data *data,
  struct v4l2_format *dest_fmt, /* in / out */
  struct v4l2_format *src_fmt /* out */
);

/* Just like VIDIOC_ENUM_FMT, except that the emulated formats are added at
   the end of the list */
LIBV4L_PUBLIC int v4lconvert_enum_fmt(struct v4lconvert_data *data, struct v4l2_fmtdesc *fmt);

/* Is conversion necessary or can the app use the data directly? */
LIBV4L_PUBLIC int v4lconvert_needs_conversion(struct v4lconvert_data *data,
  const struct v4l2_format *src_fmt,   /* in */
  const struct v4l2_format *dest_fmt); /* in */

/* return value of -1 on error, otherwise the amount of bytes written to
   dest */
LIBV4L_PUBLIC int v4lconvert_convert(struct v4lconvert_data *data,
  const struct v4l2_format *src_fmt,  /* in */
  const struct v4l2_format *dest_fmt, /* in */
  unsigned char *src, int src_size, unsigned char *dest, int dest_size);

/* get a string describing the last error*/
LIBV4L_PUBLIC const char *v4lconvert_get_error_message(struct v4lconvert_data *data);

/* Just like VIDIOC_ENUM_FRAMESIZE, except that the framesizes of emulated
   formats can be enumerated as well. */
LIBV4L_PUBLIC int v4lconvert_enum_framesizes(struct v4lconvert_data *data,
  struct v4l2_frmsizeenum *frmsize);

/* Just like VIDIOC_ENUM_FRAMEINTERVALS, except that the intervals of emulated
   formats can be enumerated as well. */
LIBV4L_PUBLIC int v4lconvert_enum_frameintervals(struct v4lconvert_data *data,
  struct v4l2_frmivalenum *frmival);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
