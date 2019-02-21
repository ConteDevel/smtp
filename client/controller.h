#ifndef __CLIENT_CONTROLLER_H__
#define __CLIENT_CONTROLLER_H__

#include "tasks.h"
#include <map.h>

typedef struct 
{
    worker_t   worker;
    map_t    * assigned_domains;
} worker_wrapper_t;

typedef struct 
{
    worker_wrapper_t * worker_ptr;
    int                worker_cnt;
    bool               is_worked;
} controller_t;

void controller_init (controller_t * p, int worker_cnt);
void controller_clear(controller_t * p);
void controller_run  (controller_t * p, sl_t mail_dir);

#endif // __CLIENT_CONTROLLER_H__