/* include original header */
#include_next <linux/kernel.h>

#undef KERN_EMERG
#undef KERN_ALERT
#undef KERN_CRIT
#undef KERN_ERR
#undef KERN_WARNING
#undef KERN_NOTICE
#undef KERN_INFO
#undef KERN_DEBUG

#define KERN_EMERG ""
#define KERN_ALERT ""
#define KERN_CRIT ""
#define KERN_ERR ""
#define KERN_WARNING ""
#define KERN_NOTICE ""
#define KERN_INFO ""
#define KERN_DEBUG ""
