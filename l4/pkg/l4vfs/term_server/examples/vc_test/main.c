/**
 * \file   l4vfs/term_server/examples/vc_test/main.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Björn Döbel  <doebel@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/log/l4log.h>
#include <l4/l4vfs/basic_name_server.h>
#include <l4/l4vfs/extendable.h>
#include <l4/l4vfs/name_server.h>
#include <l4/l4vfs/volume_ids.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/util/util.h>
#include <dice/dice.h> // for CORBA_*

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

char LOG_tag[9]="vc_test";

#ifdef DEBUG
    int _DEBUG = 1;
#else
    int _DEBUG = 0;
#endif

#define BUF_SIZE 1024

//#define RESOLVE_TEST
//#define GETDENTS_TEST
//#define REV_RESOLVE_TEST
#define TERM_TEST
//#define READ_TEST
//#define SIMPLE_TEST
//#define PRINT_TEST
//#define MOVE_TEST
#define ANIMATION_TEST
//#define CURSOR_TEST
//#define CUR_STORE_TEST
//#define FOPEN_TEST

void *CORBA_alloc(unsigned long size)
{
    return malloc(size);
}

void CORBA_free(void * prt)
{
    free(prt);
}

int main(int argc, char* argv[])
{
    l4_threadid_t ns, myself;
    int ret, ret2, i __attribute__((unused)), j __attribute__((unused));
    l4_uint32_t count __attribute__((unused)), count2 __attribute((unused));
    object_id_t res __attribute__((unused)), base;
    struct dirent *dirents __attribute__((unused));
    struct dirent *iter __attribute__((unused));
    char *path __attribute__((unused));
    char buf[100] __attribute__((unused));
    FILE *file __attribute__((unused));

    myself = l4_myself();

    // mount server now
    ns = l4vfs_get_name_server_threadid();
    l4_sleep(1500);

    LOG("VC_CLIENT");
    
    while ((ret = l4vfs_attach_namespace(ns, VC_SERVER_VOLUME_ID,
                                         "/", "/test3")) == 3)
    {
        LOG("ret = %d - server not found", ret);
        l4_sleep(1000);        
    }

    LOG("attached namespace to /test3: %d", ret);

    base.volume_id=L4VFS_NAME_SERVER_VOLUME_ID;
    base.object_id=0;

#ifdef RESOLVE_TEST
    // resolve an existing vc
    res = l4vfs_resolve( ns, base, "/test3/a/vc1" );
    LOG("resolved /test3/a/vc1: %d.%d", res.volume_id, res.object_id );
    
    // resolve an existing vc with some slashes in it
    res = l4vfs_resolve( ns, base, "/test3/a////vc1" );
    LOG("resolved /test3/a////vc1: %d.%d", res.volume_id, res.object_id );
    
    // resolve an existing vc with some slashes in it
    res = l4vfs_resolve( ns, base, "/test3/a//.//vc1" );
    LOG("resolved /test3/a//.//vc1: %d.%d", res.volume_id, res.object_id );
    
    // try to resolve a non existing vc
    res = l4vfs_resolve( ns, base, "/test3/a/vc10" );
    LOG("resolved /test3/a/vc10: %d.%d", res.volume_id, res.object_id );

    // resolve .. --> will be resolved by the name_server
    res = l4vfs_resolve( ns, base, "/test3/a/.." );
    LOG("resolved /test3/a/..: %d.%d", res.volume_id, res.object_id );

    // resolve vc1/.. --> should be illegal
    res = l4vfs_resolve( ns, base, "/test3/a/vc1/.." );
    LOG("resolved /test3/a/vc1/..: %d.%d", res.volume_id, res.object_id );
    
    // resolve vc1/.//../// --> should be illegal
    res = l4vfs_resolve( ns, base, "/test3/a/vc1/.//..///" );
    LOG("resolved /test3/a/vc1/.//..///: %d.%d", res.volume_id, res.object_id );
    
    // resolve .
    res = l4vfs_resolve( ns, base, "/test3/a/." );
    LOG("resolved /test3/a/.: %d.%d", res.volume_id, res.object_id );
#endif

#ifdef GETDENTS_TEST
    // open an existing vc
    ret = open( "/test3/a/vc0", O_RDONLY );
    LOG("opened /test3/a/vc0 : %d", ret );

    // getdents() of a terminal --> error!
    ret2 = getdents( ret, dirents, 4);
    LOG("getdents: %d", ret2);

    // close the v
    close(ret);    
    LOG("Closed /test3/a/vc0");

    // open root of vc_server
    ret = open( "test3/a/", O_RDONLY );
    LOG("opened /test/a/: %d", ret);

    // getdents() --> should work
    dirents = malloc(100);
    count = 100;
    ret2 = getdents( ret, dirents, count);
    LOG("getdents: %d", ret2);

    LOG("Received the following filenames and ids:");
    iter = dirents;
    i=0;
    while (i<ret2)
    {
	    LOG("name: %s, id = %d", iter->d_name, iter->d_ino);
	    i+=iter->d_reclen;
	    iter = (void *)iter + iter->d_reclen;
    }
    close(ret);

    // open root of vc_server
    ret = open( "test3/a/", O_RDONLY );
    LOG("opened /test/a/: %d", ret);
    
    // getdents() with a receive buffer not enough for all
    // dirents --> should work iteratively
    dirents = malloc(50);
    count =50;

    do
    {
	    ret2 = getdents( ret, dirents, count);
	    LOG("getdents: %d", ret2);
	
	    LOG("Received the following filenames and ids:");
	    iter = dirents;
	    i=0;
	    while (i<ret2)
	    {
		    LOG("name: %s, id = %d", iter->d_name, iter->d_ino);
		    i+=iter->d_reclen;
		    iter = (void *)iter + iter->d_reclen;
	    }
	    if (ret2==0)
		    LOG("end of directory reached.");
    }while (ret2);

    // close root
    close(ret);
    LOG("closed");
#endif

#ifdef REV_RESOLVE_TEST
    res.volume_id = 1000;
    res.object_id = 0;
    base.volume_id = NAME_SERVER_VOLUME_ID;
    base.object_id = 0;
   
    // rev resolve root
    path = l4vfs_rev_resolve( ns, res, &base );
    LOG("Root of vc server: %s", path);
    res.object_id = 1;
    // rev resolve valid terminal
    path = l4vfs_rev_resolve( ns, res, &base );
    LOG("first terminal: %s", path);
    // rev resolve invalid terminal
    res.object_id = 682;
    path = l4vfs_rev_resolve( ns, res, &base );
    LOG("invalid terminal: %s", path);
#endif

#ifdef TERM_TEST
    // open fd=0 --> stdin
    ret = open("/test3/vc0", O_RDONLY);
    // open fd=1 --> stdout - we need that for printf to work
    ret2 = open("/test3/vc0", O_WRONLY);
    LOG("opened vc0: in: %d, out: %d, errno=%d", ret, ret2, errno);

/*    file = fopen("/server/Mr_Pumpkin", "r");
    if (file != NULL)
        LOG("fopened Mr_Pumpkin: %p", file);
    else
        LOG("failed to open Mr_Pumpkin.");*/
    
#ifdef SIMPLE_TEST
    printf("Hello World!\n");
    return;
#endif //simple test

#ifdef PRINT_TEST
    printf("\033[1m");
    for (j=0; j<5; j++)
    {
        for (count=0; count < 7; count++)
        {
                printf("\033[%d;40m\033[%dCHello, crude world! (Stage 1)\n", 
                                                    count+31,
                                                    count*5 );
        }
    }
    /* loop is the same as:
    printf("\033[31;40mHello, crude world!\n");
    printf("\033[32;40m     Hello, crude world!\n");
    printf("\033[33;40m          Hello, crude world!\n");
    printf("\033[34;40m               Hello, crude world!\n");
    printf("\033[35;40m                    Hello, crude world!\n");
    printf("\033[36;40m                         Hello, crude world!\n");
    printf("\033[37;40m                              Hello, crude world!\n");*/

    printf("\n\n\n");
#endif

#ifdef MOVE_TEST
    // 10 forw., 5 backw.
    printf("\033[10C\033[5DHello, crude world (Stage 2)\n");
    // 10 forw., 7 backw.
    printf("\033[10C\033[7DHello, crude world (Stage 2)\n");
    // 10 forw., 9 backw.
    printf("\033[10C\033[9DHello, crude world (Stage 2)\n");

    // 5 up, color yellow
    printf("\033[5A\033[33;40mHello, crude world! (Stage 3)\n");
    // 5 down
    printf("\033[5BHello, crude world! (Stage 3)");

    // color green on black
    printf("\033[32;40m");
    // first col, 3 lines down
    printf("\033[3EStage 4: 1");
    // 1st col., 5 lines up
    printf("\033[5FStage 4: 2");
    // 3 tabs
    printf("\033[3IStage 4: 3");
    // 2 lines down
    printf("\033[2B");
    // absolute col: 10, 40, 70
    printf("\033[10G10\033[40G40\033[70G70");
  
#endif // move test


    #ifdef ANIMATION_TEST    
        count = l4vfs_attach_namespace( ns, SIMPLE_FILE_SERVER_VOLUME_ID,
                            "/", "/server" );
        LOG("attached SFS: %d", count);

        count2 = open("/server/Mr_Pumpkin", O_RDONLY);
        LOG("opened file: %d", count2);

        while( (count = read( count2, buf, 10 )) )
        {
                write(1, buf, count);
                l4_sleep(10);
        }
    #endif // animation

    #ifdef READ_TEST
        while(1)
        {
            count = read( ret, &buf, 1 );
            write( ret2, buf, 1 );
        }
                
    #endif // read

    while(1);
    close(ret);
    close(ret2);
#endif // term test

#ifdef CURSOR_TEST
    // open fd=0 --> stdin
    ret = open("/test3/a/vc0", O_RDONLY);
    // open fd=1 --> stdout - we need that for printf to work
    ret2 = open("/test3/a/vc0", O_WRONLY);
    LOG("opened vc0: in: %d, out: %d", ret, ret2);
    printf("\033[1m");

    printf("\033[?25h\033[?8h\033[12h\n");
    printf("Enter a number (cursor on, autorepeat on, echo on): \n");
    LOG("printf");
    scanf("%d", &count);
    printf("%d\n", count);

    printf("\033[?8l\033[?25l");
    printf("Enter another one (cursor off, autorepeat off, echo on): \n");
    scanf("%d", &count2);
    printf("%d\n", count2);

    printf("\033[?25h\033[?8h\033[12l");
    printf("And a last one (cursor on, autorepeat on, echo off): \n");
    scanf("%d", &count);
    printf("%d\n", count);
#endif

#ifdef CUR_STORE_TEST
    // open fd=0 --> stdin
    ret = open("/test3/a/vc0", O_RDONLY);
    // open fd=1 --> stdout - we need that for printf to work
    ret2 = open("/test3/a/vc0", O_WRONLY);
    LOG("opened vc0: in: %d, out: %d", ret, ret2);

    // bold
    printf("\033[1m");
    // cursor visible
    printf("\033[?25h");

    while(1)
    {
        int x,y;
        char *buf = "H e l l o ,   w o r l d ! ";

        y = 12;
        x = (int)((80 - strlen(buf))/2);

        // goto (x,y) and store cursor position
        printf("\033[%dB\033%dC\0337",y,x);
        
        for (count=31; count<38; count++)
        {
            // set color
            printf("\033[%dm", count);
            for (count2=0; count2<strlen(buf); count2++)
            {
                printf("%c", buf[count2]);
                l4_sleep(100);
            }

            printf("\nHello");
            printf("\033[20l\nHello\nHello\nHello");
            printf("\033[20h\nHello");
            
            // restore cursor pos
            printf("\0338");
        }

        // reset to defaults
        printf("\033c");
    }

#endif

    return 0;
}
