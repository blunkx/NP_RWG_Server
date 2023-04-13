
#ifndef _NP_SINGLE_PROC_H_
#define _NP_SINGLE_PROC_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "npshell.h"

typedef enum BROADCAST_TYPE_T
{
    LOG_IN,
    LOG_OUT,
    USER_PIPE,
    YELL_BR
} BROADCAST_TYPE_E;

#endif