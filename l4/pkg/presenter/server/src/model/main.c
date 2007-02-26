/*
 * \brief   startup of presenter server
 * \date    2003-04-11
 * \author  Jens Syckor <js712688@inf.tu-dresden.de>
 * 
 * This file describes the startup of presenter. It
 * initialises all needed modules and starts the server.
*/

#include <l4/dm_phys/dm_phys.h>

#include "presenter-server.h"
#include "util/arraylist.h"
#include "model/slide.h"
#include "util/presenter_conf.h"
#include "model/presenter.h"
#include "model/presentation.h"
#include "util/module_names.h"
#include "controller/presenter_server.h"
#include <l4/presenter/presenter_lib.h>

char LOG_tag[9]=PRESENTER_NAME;

l4_ssize_t l4libc_heapsize = 6*1024*1024;

/*** PROTOTYPES FROM 'MODULES' ***/
extern int init_presenter(struct presenter_services *);
extern int init_presentation(struct presenter_services *);
extern int init_slide(struct presenter_services *);
extern int init_keygenerator(struct presenter_services *);
extern int init_arraylist(struct presenter_services *);
extern int init_presmanager(struct presenter_services *);
extern int init_dataprovider(struct presenter_services *);
extern int init_presenter_server(struct presenter_services *);
extern int init_presenter_view(struct presenter_services *);
extern int init_display(struct presenter_services *);
extern int init_presenter_encapl4x(struct presenter_services *);

/*** PROTOTYPES FROM CONTRIB POOL.C ***/
extern long  pool_add(char *name, void *structure);
extern void *pool_get(char *name);

#ifdef DEBUG
int _DEBUG = 1;
#else
int _DEBUG = 0;
#endif

struct presenter_server_services *presenter_server;

struct presenter_services presenter_serv = {
    pool_get,
    pool_add,
};

int main(int argc, char **argv) {
    char *default_path;

    default_path = argv[1];

    /*** init modules ***/

    init_keygenerator(&presenter_serv);

    init_arraylist(&presenter_serv);

    init_dataprovider(&presenter_serv);

    init_presenter(&presenter_serv);

    init_presenter_encapl4x(&presenter_serv);

    init_slide(&presenter_serv);

    init_presentation(&presenter_serv);

    init_presmanager(&presenter_serv);

    init_display(&presenter_serv);

    init_presenter_view(&presenter_serv);

    init_presenter_server(&presenter_serv);

    presenter_server = presenter_serv.get_module(PRESENTER_SERVER_MODULE);

    LOGd(_DEBUG,"enter presenter_server start");

    /*** start living... ***/
    presenter_server->start(default_path);

    return 0;

}
