#ifndef __HANDLER_H__
#define __HANDLER_H__

#include "pch.h"

#include <sys/wait.h> 

typedef struct {
    pid_t *pids;
    size_t count;
} handlers_t;

/* Initializes handlers structure */
int handlers_init(handlers_t *handlers, const size_t count);
/* Binds handlers with task and executes it */
int handlers_run(handlers_t *handlers, int (*runnable)(int), void (*on_finish)(int, int));
/* Waits until all handlers return a execution status */
int handlers_wait(handlers_t *handlers);
/* Releases handlers structure */
void handlers_destroy(handlers_t *handlers);

#endif //__HANDLER_H__