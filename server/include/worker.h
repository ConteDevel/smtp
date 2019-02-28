#ifndef __WORKER_H__
#define __WORKER_H__

#include "pch.h"

#include <sys/wait.h> 

typedef struct {
    pid_t *pids;
    size_t count;
} workers_t;

/* Initializes workers structure */
int workers_init(workers_t *workers, const size_t count);
/* Binds workers with task and executes it */
int workers_run(workers_t *workers, int (*runnable)(int));
/* Waits until all workers return a execution status */
void workers_wait(workers_t *workers);
/* Releases workers structure */
void workers_destroy(workers_t *workers);

#endif //__WORKER_H__