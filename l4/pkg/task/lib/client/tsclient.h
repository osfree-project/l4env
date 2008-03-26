#ifndef GUARD_CLIENT_H
#define GUARD_CLIENT_H

#define TASKSERVER_PARENT_TID   3
#define RMGR_DEFAULT_ID         4

#define DEBUG                   0
#define DEBUG_MSG(msg, ...)     LOGd(DEBUG, "\033[35m"msg"\033[0m", ##__VA_ARGS__)

#endif
