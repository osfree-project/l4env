/*
 * \brief   Interface of DOpE client library
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef __DOPE_INCLUDE_DOPELIB_H_
#define __DOPE_INCLUDE_DOPELIB_H_

#define EVENT_TYPE_UNDEFINED    0
#define EVENT_TYPE_COMMAND      1
#define EVENT_TYPE_MOTION       2
#define EVENT_TYPE_PRESS        3
#define EVENT_TYPE_RELEASE      4
#define EVENT_TYPE_KEYREPEAT    5

typedef struct command_event {
	long  type;                     /* must be EVENT_TYPE_COMMAND */
	char *cmd;                      /* command string */
} command_event;

typedef struct motion_event {
	long type;                      /* must be EVENT_TYPE_MOTION */
	long rel_x,rel_y;               /* relative movement in x and y direction */
	long abs_x,abs_y;               /* current position inside the widget */
} motion_event;

typedef struct press_event {
	long type;                      /* must be EVENT_TYPE_PRESS */
	long code;                      /* code of key/button that is pressed */
} press_event;

typedef struct release_event {
	long type;                      /* must be EVENT_TYPE_RELEASE */
	long code;                      /* code of key/button that is released */
} release_event;

typedef struct keyrepeat_event {
	long type;                      /* must be EVENT_TYPE_KEYREPEAT */
	long code;                      /* code of key/button that is pressed */
} keyrepeat_event;

typedef union dopelib_event_union {
	long type;
	command_event   command;
	motion_event    motion;
	press_event     press;
	release_event   release;
	keyrepeat_event keyrepeat;
} dope_event;


/*** INITIALISE DOpE LIBRARY ***/
extern long  dope_init(void);


/*** DEINITIALISE DOpE LIBRARY ***/
extern void  dope_deinit(void);


/*** REGISTER DOpE CLIENT APPLICATION ***
 *
 * \param appname  name of the DOpE application
 * \return         DOpE application id
 */
extern long  dope_init_app(const char *appname);


/*** UNREGISTER DOpE CLIENT APPLICATION ***
 *
 * \param app_id  DOpE application to unregister
 * \return        0 on success
 */
extern long  dope_deinit_app(long app_id);


/*** EXECUTE DOpE COMMAND ***
 *
 * \param app_id  DOpE application id
 * \param cmd     command to execute
 * \return        0 on success
 */
extern int dope_cmd(long app_id, const char *cmd);


/*** EXECUTE DOpE FORMAT STRING COMMAND ***
 *
 * \param app_id  DOpE application id
 * \param cmdf    command to execute specified as format string
 * \return        0 on success
 */
extern int dope_cmdf(long app_id, const char *cmdf, ...);


/*** EXECUTE DOpE COMMAND AND REQUEST RESULT ***
 *
 * \param app_id    DOpE application id
 * \param dst       destination buffer for storing the result string
 * \param dst_size  size of destination buffer in bytes
 * \param cmd       command to execute
 * \return          0 on success
 */
extern int dope_req(long app_id, char *dst, int dst_size, const char *cmd);


/*** REQUEST RESULT OF A DOpE COMMAND SPECIFIED AS FORMAT STRING ***
 *
 * \param app_id    DOpE application id
 * \param dst       destination buffer for storing the result string
 * \param dst_size  size of destination buffer in bytes
 * \param cmd       command to execute - specified as format string
 * \return          0 on success
 */
extern int dope_reqf(long app_id, char *dst, int dst_size, const char *cmdf, ...);


/*** BIND AN EVENT TO A DOpE WIDGET ***
 *
 * \param app_id      DOpE application id
 * \param var         widget to bind an event to
 * \param event_type  identifier for the event type
 * \param callback    callback function to be called for incoming events
 * \param arg         additional argument for the callback function
 */
extern void dope_bind(long app_id,const char *var, const char *event_type,
                      void (*callback)(dope_event *,void *), void *arg);


/*** BIND AN EVENT TO A DOpE WIDGET SPECIFIED AS FORMAT STRING ***
 *
 * \param app_id      DOpE application id
 * \param varfmt      widget to bind an event to (format string)
 * \param event_type  identifier for the event type
 * \param callback    callback function to be called for incoming events
 * \param arg         additional argument for the callback function
 * \param ...         format string arguments
 */
extern void dope_bindf(long id, const char *varfmt, const char *event_type,
                       void (*callback)(dope_event *,void *), void *arg,...);


/*** ENTER DOPE EVENTLOOP ***
 *
 * \param app_id  DOpE application id
 */
extern void dope_eventloop(long app_id);


/*** RETURN NUMBER OF PENDING EVENTS ***
 *
 * \param app_id  DOpE application id
 * \return        number of pending events
 */
int dope_events_pending(int app_id);


/*** PROCESS ONE SINGLE DOpE EVENT ***
 *
 * This function processes exactly one DOpE event. If no event is pending, it
 * blocks until an event is available. Thus, for non-blocking operation, this
 * function should be called only if dope_events_pending was consulted before.
 *
 * \param app_id  DOpE application id
 */
extern void dope_process_event(long app_id);


/*** INJECT ARTIFICIAL EVENT INTO EVENT QUEUE ***
 *
 * This function can be used to serialize events streams from multiple
 * threads into one DOpE event queue.
 */
extern void dope_inject_event(long app_id, dope_event *ev,
                              void (*callback)(dope_event *,void *), void *arg);


/*** REQUEST KEY OR BUTTON STATE ***
 *
 * \param app_id   DOpE application id
 * \param keycode  keycode of the requested key
 * \return         1 if key is currently pressed
 */
extern long dope_get_keystate(long app_id, long keycode);


/*** REQUEST CURRENT ASCII KEYBOARD STATE ***
 *
 * \param app_id   DOpE application id
 * \param keycode  keycode of the requested key
 * \return         ASCII value of the currently pressed key combination
 */
extern char dope_get_ascii(long app_id, long keycode);

#endif /* __DOPE_INCLUDE_DOPELIB_H_ */
