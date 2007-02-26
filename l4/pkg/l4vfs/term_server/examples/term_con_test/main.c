/**
 * \file   l4vfs/term_server/examples/term_con_test/main.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>
#include <l4/util/util.h>
#include <l4/l4vfs/extendable.h>
#include <l4/l4vfs/name_server.h>
#include <l4/l4vfs/volume_ids.h>

#include <dice/dice.h> // for CORBA_*

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

char LOG_tag[9]="t_c_test";

#define BUF_SIZE 1024

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
    l4_threadid_t ns;
    int fd, fd2, ret, i;
    unsigned char buf[BUF_SIZE];

    // mount server now
    ns = l4vfs_get_name_server_threadid();
    l4_sleep(1500);
    while ((ret = l4vfs_attach_namespace(ns, TERM_CON_VOLUME_ID,
                                         "/", "/linux")) == 3)
    {
        l4_sleep(100);
    }
    LOG("attach_namespace() = %d", ret);

    fd = open("/linux/vc0", O_RDONLY);
    LOG("stdin fd = '%d' (0), errno = %d", fd, errno);
    fd = open("/linux/vc0", O_WRONLY);
    LOG("stdout fd = '%d' (0), errno = %d", fd, errno);
    fd = open("/linux/vc0", O_WRONLY);
    LOG("stderr fd = '%d' (0), errno = %d", fd, errno);

    ret = write(1, "test", 4);
    LOG("write ret = '%d' (4)", ret);
    ret = write(1, "test2", 5);
    LOG("write ret = '%d' (5)", ret);
    ret = write(1, "\nNow write something: ", 22);
    LOG("write ret = '%d' (22)", ret);
    ret = read(0, buf, 6);
    LOG("read ret = '%d' (6), errno = %d", ret, errno);
    buf[6] = '\0';
    write(1, "\nI read: '", 10);
    write(1, buf, 6);
    write(1, "'\n", 2);

    printf("(printf) I read '\033[1m%s\033[22m'\n", buf);

//    printf(":");
////    ret = fread(&i, 1, 1, stdin);
//    ret = fgetc(stdin);
//    printf("ret = %d, i = %d", ret, i);

    printf(":");
    ret = scanf("%d", &i);
    printf("i = %d, ret = %d\nNow press a key ...\n", i, ret);
    read(0, buf, 1);

    printf("\033[30mc\033[31mo\033[32ml\033[33mo\033[34mr"
           "\033[35mt\033[36m\033[37mest\033[0m\n");
    printf("\033[40mc\033[41mo\033[42ml\033[43mo\033[44mr"
           "\033[45mt\033[46m\033[47mest\033[0m\n");
    printf("\033[1mi\033[0mn\033[2mt\033[0me\033[1mn"
           "\033[0ms\033[2mi\033[0mt\033[1my\033[0m "
           "\033[2mt\033[0me\033[1ms\033[0mt\033[0m\n");
    printf("\033[27mi\033[7mn\033[27mv\033[7me\033[27mr"
           "\033[7ms\033[27me\033[7m \033[27mt\033[7me"
           "\033[27ms\033[7mt\033[0m\n");
    
    // test positioning
    printf("\033[1;1H.\033[2;2H.\033[3;3H.\033[4;4H."
           "\033[5;5H.\033[6;6H.\033[7;7H.\033[8;8H."
           "\033[9;9H.\033[10;10H.\033[11;11H.\033[12;12H."
           "\033[13;13H.\033[14;14H.\033[15;15H.\033[16;16H.");
    printf("Positioning Test\n");

    l4_sleep(500);

    // now for the brave ...
    fd2 = open("/linux/vc1", O_RDWR);
    LOG("fd2 = '%d' (3)", fd2);
    write(fd2, "testing vc2 1\n", 14);
    write(fd2, "testing vc2 2\n", 14);
    write(fd2, "testing vc2 3\n", 14);
    write(fd2, "testing vc2 4\n", 14);
    write(fd2, "\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10"
               "\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20"
               "\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30"
               "\n31\n32\n33\n34\n35\n36\n37\n38\n39\n40"
               "\n41\n42\n43\n44\n45\n46\n47\n48\n49\n50"
               "\n51\n52\n53\n54\n55\n56\n57\n58\n59\n60", 180);

    l4_sleep(500);

    fd2 = open("/linux/vc2", O_RDWR);
    LOG("fd2 = '%d' (4)", fd2);
    for (i = 0; i < 20; i++)
        write(fd2, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n", 31);

    write(fd2, "\033[30;30HNow press some keys", 27);

    read(fd2, buf, 1);
    write(fd2, "\033[?5h", 5);
    read(fd2, buf, 1);
    write(fd2, "\033[?5l", 5);
    read(fd2, buf, 1);
    write(fd2, "\033[?5h", 5);
    read(fd2, buf, 1);
    write(fd2, "\033[?5l", 5);
    read(fd2, buf, 1);

    write(fd2, "\033[3;3H\033[1J\033[10;10H\0330J", 21);
    read(fd2, buf, 1);
    write(fd2, "\033[2J", 4);
    write(fd2, "\033#8", 3);
    read(fd2, buf, 1);
    write(fd2, "Go back to vc0\n", 15);

    printf("\nNow press a key ...\n");
    read(0, buf, 1);

   /* 
    printf("\nNow all valid chars\n");
    for (i = 0; i < 256; i++)
        if ((i >= 0x20 && i <= 0x7e) || (i >= 0xa1 && i <= 0xfe))
            write(1, ((unsigned char *)&i), 1);
    printf("\nNow you can type something (10) ...\n");

    // echo
    for (i = 0; i < 10; i++)
    {
        ret = read(0, buf, 1);
        printf("value = '%hhd', char = '%c', ret = '%d'\n",
               buf[0], buf[0], ret);
    }
*/
    // now for the patient do something fancy ...
    // mount server now
    ns = l4vfs_get_name_server_threadid();
    ret = l4vfs_attach_namespace(ns, SIMPLE_FILE_SERVER_VOLUME_ID, "/",
                                 "/server");
    LOG("attach_namespace() = %d", ret);

    fd2 = open("/server/Mr_Pumpkin", O_RDONLY);
    LOG("open() = %d", fd2);

    while ((ret = read(fd2, buf, 10)))
    {
        write(1, buf, ret);
//        LOG("%c", buf[0]);
        l4_sleep(10);
    }

    return 0;
}
