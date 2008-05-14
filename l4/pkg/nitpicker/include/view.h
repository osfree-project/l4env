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

#ifndef _NITPICKER_VIEW_H_
#define _NITPICKER_VIEW_H_


#include "buffer.h"


EXTERN_C_BEGIN

/*
 * This enum describes the view's position on the stack
 * when the order is changed.
 */
typedef enum {
    TOP = 1,   /**< Move view to top of the view stack */
    BOTTOM,    /**< Move view to bottom of the view stack */
    INFRONT,   /**< Move view in front of a given neighbor */
    BEHIND     /**< Move view to behind a given neighbor */
} nitpicker_stack_position_t;


/*
 * This structure describes a view on a nitpicker buffer.
 * Its fields are private.
 */
struct nitpicker_view;


/*** CREATE A NITPICKER VIEW ***
 *
 * \param[in] np  Nitpicker buffer
 * \return        Pointer to a view description or NULL on failure
 */
L4_CV struct nitpicker_view* nitpicker_view_create(struct nitpicker_buffer *npb);


/*** DESTROY A VIEW ***
 *
 * param[in] npv  View to destroy
 */
L4_CV void nitpicker_view_destroy(struct nitpicker_view *npv);


/*** CHECK IF A BUFFER IS VAILD ***
 *
 * \param[in] npv  View
 * \return         0 on success or negative error code
 */
L4_CV int nitpicker_view_is_valid(const struct nitpicker_view *npv);


/*** SET A VIEWS POSITION AND SIZE ON THE ASSOCIATED BUFFER ***
 *
 * \param[in] npv        View
 * \param[in] buffer_x   X offset in the buffer
 * \param[in] buffer_y   Y offset in the buffer
 * \param[in] x, y       View position (top left corner)
 * \param[in] w, h       View size
 * \param[in] do_redraw  Mark view to be redrawn
 * \return               0 on success or negative error code
 */
L4_CV int nitpicker_view_set_view_port(struct nitpicker_view *npv,
                                       int buffer_x, int buffer_y,
                                       int x, int y,
                                       unsigned int width,
                                       unsigned int height,
                                       int do_redraw);


/*** SET A VIEW'S POSITION IN THE VIEW STACK ***
 *
 * \param[in] npv        View
 * \param[in] neighbor   View's neighbor
 * \param[in] stack_pos  Describes the view's new position
 * \param[in] do_redraw  Mark view to be redrawn
 * \return               0 on success or negative error code
 */
L4_CV int nitpicker_view_stack(struct nitpicker_view *npv,
                               struct nitpicker_view *neighbor,
                               nitpicker_stack_position_t stack_pos, int do_redraw);


/*** SET A VIEW'S TITLE ***
 *
 * \param[in] npv    View
 * \param[in] title  New title
 * \return           0 on success or negative error code
 */
L4_CV int nitpicker_view_set_title(struct nitpicker_view *npv, const char *title);


/*** SET A VIEW AS THE BACKGROUND VIEW ***
 *
 * \param[in] npv  View
 * \return         0 on success or negative error code
 */
L4_CV int nitpicker_view_make_background(struct nitpicker_view *npv);


EXTERN_C_END


#endif
