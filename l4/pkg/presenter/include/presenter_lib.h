#ifndef ___PRESENTER_INCLUDE_PRESENTER_H_
#define ___PRESENTER_INCLUDE_PRESENTER_H_

#define PRESENTER_NAME "PRESENT"

/** INITIALIZE PRESENTER LIBRARY **/
extern long presenter_init(void);

/** DEINITIALIZE PRESENTER LIBRARY **/
extern long presenter_deinit(void);

/** LOAD A PRESENTATION FROM A SPECIAL PATH **/
extern int presenter_load(char *pathname);

/** SHOW A PRESENTATION IN A DOpE WIDGET **/
extern int presenter_show(int presentation_id);

#endif
