#ifndef __TASK_H__
#define __TASK_H__

#include "pch.h"
#include "socket.h"

#include <sys/poll.h>

#define TIMEOUT (3 * 60 * 1000)

int task_run(int id, int srv_sock, int (*exp)());

#endif //__TASK_H__