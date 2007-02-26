#include <l4/log/l4log.h>
#include <l4/sys/ktrace.h>

void LOG_outstring_fiasco_tbuf(const char *s)
{
    char buf[35];
    unsigned int i;

    while (*s)
      {
        for (i = 0; ; )
          {
            if (i == sizeof(buf) - 2)
              {
                buf[i++] = '~';
                break;
              }
            if (*s == '\n')
              {
                buf[i++] = '\\';
                s++;
                break;
              }
            if (*s == 0)
              break;
            buf[i++] = *s++;
          }
        buf[i] = 0;
        fiasco_tbuf_log(buf);
      }
}
