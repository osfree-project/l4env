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

#ifndef _NITPICKER_BUFFER_H_
#define _NITPICKER_BUFFER_H_


#include "nitpicker.h"


EXTERN_C_BEGIN


/*
 * This structure describes a nitpicker buffer.
 * All of its fields are private.
 */
struct nitpicker_buffer;


/*** CREATE A NITPICKER BUFFER ***
 *
 * \param[in] np      Nitpicker connection.
 * \param[in] width   Width of the new buffer.
 * \param[in] height  Height of the new buffer.
 * \return            Pointer to a buffer description or NULL on failure.
 */
struct nitpicker_buffer* nitpicker_buffer_create(struct nitpicker *np,
                                                 unsigned int      width,
                                                 unsigned int      height);


/*** DESTROY A BUFFER ***
 *
 * \param[in] npb  Buffer to destroy
 */
void nitpicker_buffer_destroy(struct nitpicker_buffer *npb);


/*** CHECK IF A BUFFER IS VAILD ***
 *
 * \param[in] npb  Buffer
 * \return         1 if buffer is valid, 0 otherwise
 */
int nitpicker_buffer_is_valid(const struct nitpicker_buffer *npb);


/*** REFRESH A BUFFER'S VISUAL APPEARENCE ***
 *
 * \param[in] npb  Buffer
 * \return         0 on success or negative error code
 */
int nitpicker_buffer_refresh(struct nitpicker_buffer *npb);


/*** REFRESH A BUFFERS VISUAL APPEARENCE IN A SPECIFIED AREA ***
 *
 * \param[in] npb     Buffer
 * \param[in] x       Area's most left coordinate
 * \param[in] y       Area's most upper coordinate
 * \param[in] width   Area's width
 * \param[in] height  Area's height
 * \return            0 on success or negative error code
 */
int nitpicker_buffer_refresh_at(struct nitpicker_buffer *npb,
                                unsigned int x, unsigned int y,
                                unsigned int width, unsigned int height);


/*** RETURN A BUFFERS SIZE ***
 *
 * \param[in]  npb     Buffer
 * \param[out] width   Area's width
 * \param[out] height  Area's height
 * \return             0 on success or negative error code
 */
int nitpicker_buffer_get_size(struct nitpicker_buffer *npb,
                              unsigned int *width, unsigned int *height);


/*** RETURN A BUFFERS PIXEL FORMAT ***
 *
 * \param[in]  npb     Buffer
 * \param[out] format  Buffer's pixel format
 * \return             0 on success or negative error code
 */
int nitpicker_buffer_get_format(struct nitpicker_buffer *npb,
                                nitpicker_pixel_format_t *format);


/*** RETURN A BUFFERS MEMORY ADDRESS ***
 *
 * \param[in] npb  Buffer
 * \return         Buffer's memory
 *
 * This function returns the lowest address of the buffer's memory.
 * You can use it to get the offset for drawing into the buffer.
 */
void* nitpicker_buffer_get_address(struct nitpicker_buffer *npb);


EXTERN_C_END


#endif
