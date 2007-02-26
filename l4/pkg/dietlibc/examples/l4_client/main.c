#include <l4/names/libnames.h>
#include <l4/log/l4log.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char* argv[])
{
    int fd, res;
    FILE *f1;
    char test[100];

    LOG_init("vfs_wr_c");

    if (errno != 0)
        perror("Initial error check");

    // is called on init. of the backend now
    /*
      LOGL("1");
      // should be called from crt0 startup
      l4xvfs_init();
      LOGL("2");
    */

    LOGL("3");
    // checking normal file io
    fd = open("/home/mp26/test", O_RDONLY);
    if (fd < 0)
    {
        LOG("error");
        exit(1);
    }

    res = read(fd, &test, 99);
    LOG("got %d bytes", res);
    test[100] = 0;
    LOG(test);

    LOGL("4");
    close(fd);

    LOG("5");
    fd = open("/home/mp26/test", O_RDWR | O_APPEND);
    if (fd < 0)
    {
        LOG("error");
        exit(2);
    }

    test[0]='ß';
    test[1]='ß';
    test[2]='ß';
    res = write(fd, &test, 10);
    LOG("put %d bytes", res);

    LOGL("6");
    close(fd);

    // checking buffered file io
    LOGL("7");
    f1 = fopen("/home/mp26/test", "r");
    if (fd < 0)
    {
        LOG("error");
        exit(3);
    }

    memset(test, 0, 100);
    res = fread(test, 10, 5, f1);
    LOG("got %d bytes", res);

    test[50] = 0;
    LOG(test);

    LOGL("8");
    fclose(f1);

    return 0;
}
