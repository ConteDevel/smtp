#ifndef __TASK_H__
#define __TASK_H__

#include "pch.h"

#define TIMEOUT (3 * 60 * 1000)

int task_run(int index, int srv_sock, int (*exp)());

#endif //__TASK_H__