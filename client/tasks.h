#ifndef __CLIENT_TASKS_H__
#define __CLIENT_TASKS_H__
#include "config.client.h"
#include <utils.h>

typedef enum 
{
    E_SMTP_NO_CONNECTED = 0,
    E_SMTP_READ_CONNECTED_RES,

    E_SMTP_WRITE_HELO,
    E_SMTP_READ_HELO_RES,

    E_SMTP_WAIT,

    E_SMTP_WRITE_FROM,
    E_SMTP_READ_FROM_RES,

    E_SMTP_WRITE_TO,
    E_SMTP_READ_TO_RES,

    E_SMTP_WRITE_DATA,
    E_SMTP_READ_DATA_RES,

    E_SMTP_WRITE_FILE,
    E_SMTP_WRITE_END_TRANSFER,
    E_SMTP_READ_RESULT,
} smtp_state_e;

short get_event_by_state(smtp_state_e state);

typedef struct task_t
{
    sl_t            path;
    sl_t            from;
    sl_t            to;
    sl_t            domain;

    sl_t            error_desc;
    struct task_t * next_task;
} task_t;

void task_free(task_t * task);

typedef struct
{
    smtp_state_e state;

    int      socket_id;
    task_t * current;
    task_t * task_list;
    FILE   * file;

    sl_t buff;
} domain_task_t;

typedef struct
{
    task_t * new_tasks;
    sem_t    new_tasks_lock;

    task_t * finished_tasks;
    sem_t    finished_tasks_lock;

    pthread_t thread;
    bool      is_worked;
} worker_t;

void worker_init (worker_t * p);
void worker_clear(worker_t * p);

#endif // __CLIENT_TASKS_H__