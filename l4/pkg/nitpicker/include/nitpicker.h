/*
 * \brief   Nitpicker client library
 * \date    2007-06-05
 * \author  Thomas Zimmermann <tzimmer@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2007 Thomas Zimmermann <tzimmer@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nitpicker package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef _NITPICKER_NITPICKER_H_
#define _NITPICKER_NITPICKER_H_


#include <l4/dm_generic/dm_generic.h>


EXTERN_C_BEGIN


/*
 * This enum describes the screens pixel format.
 */
typedef enum {
	RGB16 = 1,   /**< RGB format, 5:6:5 bits per component */
	RGBA32       /**< RGBA format, 8 bits per component */
} nitpicker_pixel_format_t;


/*
 * This structure describes a connection to a nitpicker server.
 * Its fields are private.
 */
struct nitpicker;


/*
 * function callback type for the event handler
 */
typedef L4_CV void (*nitpicker_event_func)(unsigned long token,
                                           int type,
                                           int keycode,
                                           int rx, int ry,
                                           int ax, int ay);


/*** OPEN A CONNECTION TO NITPICKER SERVER ***
 *
 * \return Pointer to a connection description or NULL on failure
 */
L4_CV struct nitpicker *nitpicker_open(void);


/*** CLOSE A CONNECTION ***
 *
 * \param[in] np  Pointer to the connection to close
 */
L4_CV void nitpicker_close(struct nitpicker *np);


/*** CHECK IF A CONNECTION IS VAILD ***
 *
 * \param[in]  np  Connection to Nitpicker service
 * \return         1 if valid, otherwise 0
 */
L4_CV int nitpicker_is_valid(const struct nitpicker *np);


/*** RETRIEVE SOME INFORMATION ABOUT THE SCREEN CONFIGURATION ***
 *
 * \param[in]  np      Connection
 * \param[out] width   Screen width
 * \param[out] height  Screen height
 * \param[out] format  Pixel format
 * \return             0 on success or negative error code
 */
L4_CV int nitpicker_get_screen_info(const struct nitpicker   *np,
                                    unsigned int             *width,
                                    unsigned int             *height,
                                    nitpicker_pixel_format_t *format);


/*** DONATE SOME MEMORY TO NITPICKER ***
 *
 * \param[in]  np          Connection
 * \param[in]  max_views   Maximum number of views supported
 * \param[in]  max_buffers Maximum number of buffers supported
 * \param[out] ds          Associated dataspace
 * \return                 0 on success or negative error code
 */
L4_CV int nitpicker_donate_memory(const struct nitpicker *np,
                                  unsigned int            max_views,
                                  unsigned int            max_buffers,
                                  l4dm_dataspace_t       *ds);


/*** DONATE A DATASPACE TO NITPICKER ***
 *
 * \param[in] np           Connection
 * \param[in] max_views    Maximum number of views supported
 * \param[in] max_buffers  Maximum number of buffers supported
 * \param[in] ds           Dataspace to donate
 * \return                 0 on success or negative error code
 */
L4_CV int nitpicker_donate_dataspace(const struct nitpicker *np,
                                     unsigned int            max_views,
                                     unsigned int            max_buffers,
                                     const l4dm_dataspace_t *ds);


/*** REMOVE A DATASPACE FROM NITPICKER ***
 *
 * \param[in] np  Connection
 * \param[in] ds  Dataspace to remove
 */
L4_CV void nitpicker_remove_dataspace(struct nitpicker *np, l4dm_dataspace_t *ds);


/*** ENTER NITPICKER MAINLOOP ***
 *
 * \param[in] np  Connection
 * \param[in] cb  Pointer to an event callback function
 *
 * This function is the mainloop of nitpicker clients. It is not intended
 * to return. You can pass NULL as event callback, if you're not interested
 * in any input events. This function is also threads save. It is possible
 * to have more then one thread running this function simulaneously. Each
 * thread may use a different event callback.
 */
L4_CV void nitpicker_mainloop(nitpicker_event_func cb);

EXTERN_C_END

#endif
