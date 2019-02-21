#include "tasks.h"
#include "map.h"
#include "config.client.h"

sl_t get_mx_record(sl_t domain)
{
    u_char nsbuf[256];
    char dispbuf[256];
    ns_msg msg;
    ns_rr rr;

    int rc = res_query(domain.s,  C_IN, T_MX, nsbuf, sizeof (nsbuf));
    if (rc < 0) 
        return SLNULL;

    ns_initparse(nsbuf, rc, &msg);
    rc = ns_msg_count(msg, ns_s_an);

    ns_parserr(&msg, ns_s_an, 0, &rr);
    ns_sprintrr(&msg, &rr, NULL, NULL, dispbuf, sizeof (dispbuf));

    int len = strlen(dispbuf);
    int pos = len - 1;
    
    while (len >= 0 && dispbuf[pos] != ' ')
        pos -= 1;

    if (pos < 0 || dispbuf[pos] != ' ')
        return SLNULL;

    return sl_create(FMT_SL, ARG_SL(SL(dispbuf + pos + 1, len - pos - 2)));
}

short get_event_by_state(smtp_state_e state)
{
    switch (state)
    {
        case E_SMTP_READ_CONNECTED_RES:
        case E_SMTP_READ_HELO_RES:
        case E_SMTP_READ_FROM_RES:
        case E_SMTP_READ_TO_RES:
        case E_SMTP_READ_DATA_RES:
        case E_SMTP_READ_RESULT:
            return POLLIN;
    
        case E_SMTP_WRITE_HELO:
        case E_SMTP_WRITE_FROM:
        case E_SMTP_WRITE_TO:
        case E_SMTP_WRITE_DATA:
        case E_SMTP_WRITE_FILE:
        case E_SMTP_WRITE_END_TRANSFER:
            return POLLOUT;

        case E_SMTP_WAIT:
            return POLLOUT;

        default: 
            return 0;
    }

    return 0;
}

int socket_open_to(const sl_t address, uint16_t port)
{
    int fd = -1;
    ASSERT(address.l);

    struct addrinfo   hints;
    struct addrinfo * res = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    sl_t s_port = sl_create("%hu", port);
    LOG_D(FMT_SL " " FMT_SL, ARG_SL(address), ARG_SL(s_port));
    int gai_code = getaddrinfo(address.s, s_port.s, &hints, &res);
    if (gai_code)
    {
        LOG_E("getaddrinfo(...) failed: %s", gai_strerror(gai_code));
        
        sl_free     (&s_port);
        freeaddrinfo(res);
        return -1;
    }

    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0)
    {
        LOG_E("socket(...) failed: %s", strerror(errno));

        sl_free     (&s_port);
        freeaddrinfo(res);
        return -1;
    }
    LOG_D("socket number %d", fd);

    int soc_flags = 1;
    if (0 > setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                       &soc_flags, sizeof(soc_flags)))
    {
        LOG_E("setsockopt(...) failed: %s", strerror(errno));

        close       (fd);
        sl_free     (&s_port);
        freeaddrinfo(res);
        return -1;
    }
  
    int flags;
    if ((flags = fcntl(fd, F_GETFL, 0)) < 0) 
    { 
        LOG_E("fcntl(...) failed: %s", strerror(errno));

        close       (fd);
        sl_free     (&s_port);
        freeaddrinfo(res);
        return -1;
    } 

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) 
    { 
        LOG_E("fcntl(...) failed: %s", strerror(errno));

        close       (fd);
        sl_free     (&s_port);
        freeaddrinfo(res);
        return -1;
    } 

    int res_connect = connect(fd, res->ai_addr, res->ai_addrlen);
    if (res_connect != 0 && errno != EINPROGRESS)
    {
        LOG_E("connect(...) failed: %s", strerror(errno));

        close       (fd);
        sl_free     (&s_port);
        freeaddrinfo(res);
        return -1;
    }
    LOG_I(FMT_SL ":%hu - connection established", ARG_SL(address), port);

    sl_free     (&s_port);
    freeaddrinfo(res);
    return fd;
}

domain_task_t * domain_task_create()
{
    domain_task_t * dt = malloc(sizeof(*dt));
    ASSERT(dt && "Error memory alloc");

    dt->socket_id = 0;
    dt->task_list = NULL;
    dt->current   = NULL;
    dt->file      = NULL;
    dt->buff      = SLNULL;

    dt->state = E_SMTP_NO_CONNECTED;
    return dt;
}

typedef enum
{
    E_OK,
    E_CONNECTION_CLOSE,
    E_ERROR,
} res_rw_e;

res_rw_e write_to_socket(domain_task_t * dt)
{
    int rc = send(dt->socket_id, dt->buff.s, dt->buff.l, 0);
    if (rc < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        LOG_E("Error on send(...) %s", strerror(errno));
        return E_ERROR;
    }
    else if (rc == 0)
    {
        LOG_W("Connection was closed");
        return E_CONNECTION_CLOSE;
    }
    else
    {
        LOG_D("Sended: " FMT_SL, rc, dt->buff.s);
        if (rc != dt->buff.l)
        {
            sl_t tmp = SL(dt->buff.s + rc, dt->buff.l - rc);
            tmp = sl_dup(tmp);
            sl_free(&dt->buff);
            dt->buff = tmp;
        }
        else
        {
            sl_free(&dt->buff);
            dt->buff = SLNULL;
        }

        return E_OK;
    }   
}

res_rw_e read_from_socket(domain_task_t * dt)
{
    const int buffer_size = 1024;
    char buffer[buffer_size];

    int rc = recv(dt->socket_id, buffer, buffer_size, 0);
    if (rc < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        LOG_E("Error on recv(...) %s", strerror(errno));
        return E_ERROR;
    }
    else if (rc == 0)
    {
        LOG_W("Connection was closed");
        return E_CONNECTION_CLOSE;
    }   

    sl_t sl_buff = SL(buffer, rc);
    LOG_D("Readed: " FMT_SL, ARG_SL(sl_buff));
    sl_t new_str = sl_combine(&dt->buff, &sl_buff);

    sl_free(&dt->buff);
    dt->buff = new_str;
    
    //if (dt->buff.s[dt->buff.l - 2] == '\r' && dt->buff.s[dt->buff.l - 2] == '\n')
    return E_OK;
}

typedef enum
{
    E_RES_IN_PROGRESS,
    E_RES_FINISHED,
    E_RES_FAIL,
} handle_res_e;

#define SWITCH_TO(__state) \
{\
    LOG_D("Switch state %d -> %d", dt->state, __state);\
    dt->state = __state;\
}

handle_res_e handle_read_task(domain_task_t * dt)
{
    res_rw_e rr = read_from_socket(dt);
    if (rr == E_ERROR || rr == E_CONNECTION_CLOSE)
        return E_RES_FAIL;

    switch (dt->state)
    {
        case E_SMTP_READ_CONNECTED_RES:
            if (dt->buff.s[dt->buff.l - 2] == '\r' && dt->buff.s[dt->buff.l - 1] == '\n')
            {
                sl_free(&dt->buff);
                dt->buff  = SLNULL;
                SWITCH_TO(E_SMTP_WRITE_HELO);
            }
            break;

        case E_SMTP_READ_HELO_RES:
            if (dt->buff.s[dt->buff.l - 2] == '\r' && dt->buff.s[dt->buff.l - 1] == '\n')
            {
                int code;
                int rc = sscanf(dt->buff.s, "%d", &code);
                if (rc != 1)
                {
                    LOG_W("Wrong response for HELO cmd: " FMT_SL, ARG_SL(dt->buff));
                    return E_ERROR;
                }

                if (code != 250)
                {
                    LOG_W("Wrong response code for HELO cmd: code %d, resp = " FMT_SL, code, ARG_SL(dt->buff));
                    return E_ERROR;
                }

                sl_free(&dt->buff);
                dt->buff  = SLNULL;
                SWITCH_TO(E_SMTP_WAIT);
            }
            break;


        case E_SMTP_READ_FROM_RES:
            if (dt->buff.s[dt->buff.l - 2] == '\r' && dt->buff.s[dt->buff.l - 1] == '\n')
            {
                int code;
                int rc = sscanf(dt->buff.s, "%d", &code);
                if (rc != 1)
                {
                    LOG_W("Wrong response for FROM cmd: " FMT_SL, ARG_SL(dt->buff));
                    return E_ERROR;
                }

                if (code != 250)
                {
                    LOG_W("Wrong response code for FROM cmd: code %d, resp = " FMT_SL, code, ARG_SL(dt->buff));
                    return E_ERROR;
                }

                sl_free(&dt->buff);
                dt->buff  = SLNULL;
                SWITCH_TO(E_SMTP_WRITE_TO);
            }
            break;

        case E_SMTP_READ_TO_RES:
            if (dt->buff.s[dt->buff.l - 2] == '\r' && dt->buff.s[dt->buff.l - 1] == '\n')
            {
                int code;
                int rc = sscanf(dt->buff.s, "%d", &code);
                if (rc != 1)
                {
                    LOG_W("Wrong response for TO cmd: " FMT_SL, ARG_SL(dt->buff));
                    return E_ERROR;
                }

                if (code != 250)
                {
                    LOG_W("Wrong response code for TO cmd: code %d, resp = " FMT_SL, code, ARG_SL(dt->buff));
                    return E_ERROR;
                }

                sl_free(&dt->buff);
                dt->buff  = SLNULL;
                SWITCH_TO(E_SMTP_WRITE_DATA);
            }
            break;

        case E_SMTP_READ_DATA_RES:
            if (dt->buff.s[dt->buff.l - 2] == '\r' && dt->buff.s[dt->buff.l - 1] == '\n')
            {
                int code;
                int rc = sscanf(dt->buff.s, "%d", &code);
                if (rc != 1)
                {
                    LOG_W("Wrong response for DATA cmd: " FMT_SL, ARG_SL(dt->buff));
                    return E_ERROR;
                }

                if (code != 354)
                {
                    LOG_W("Wrong response code for DATA cmd: code %d, resp = " FMT_SL, code, ARG_SL(dt->buff));
                    return E_ERROR;
                }

                sl_free(&dt->buff);
                dt->buff  = SLNULL;
                SWITCH_TO(E_SMTP_WRITE_FILE);
            }
            break;

        case E_SMTP_READ_RESULT:
            if (dt->buff.s[dt->buff.l - 2] == '\r' && dt->buff.s[dt->buff.l - 1] == '\n')
            {
                int code;
                int rc = sscanf(dt->buff.s, "%d", &code);
                if (rc != 1)
                {
                    LOG_W("Wrong response for transfer data: " FMT_SL, ARG_SL(dt->buff));
                    return E_ERROR;
                }

                if (code != 250)
                {
                    LOG_W("Wrong response code for for transfer data: code %d, resp = " FMT_SL, code, ARG_SL(dt->buff));
                    return E_ERROR;
                }

                sl_free(&dt->buff);
                dt->buff  = SLNULL;
                SWITCH_TO(E_SMTP_WAIT);
                return E_RES_FINISHED;
            }
            break;

        default:
            break;
    }

    return E_RES_IN_PROGRESS;
}

handle_res_e handle_write_task(domain_task_t * dt)
{
    #define WRITE_AND_SWITCH_ON_FINISH_TO(__next)   \
        res_rw_e rr = write_to_socket(dt);                      \
        if (rr == E_ERROR || rr == E_CONNECTION_CLOSE)          \
            return E_RES_FAIL;                                  \
        else if (dt->buff.s == NULL)                            \
        {                                                       \
            SWITCH_TO(__next);                                 \
        }                                                       

    ASSERT(dt->state != E_SMTP_NO_CONNECTED);

    switch (dt->state)
    {  
        case E_SMTP_WRITE_HELO:
        {
            if (dt->buff.s == NULL)
            {
                dt->buff = sl_dup(SLLIT("EHLO some.host.ru\r\n"));
                dt->buff.l -= 1;
            }

            WRITE_AND_SWITCH_ON_FINISH_TO(E_SMTP_READ_HELO_RES);
            return E_RES_IN_PROGRESS;
        }
        break;

        case E_SMTP_WRITE_FROM:
        {
            if (dt->buff.s == NULL)
            {
                dt->buff = sl_create("MAIL FROM: <" FMT_SL ">\r\n", 
                                    ARG_SL(dt->current->from));
                dt->buff.l -= 1;
            }

            WRITE_AND_SWITCH_ON_FINISH_TO(E_SMTP_READ_FROM_RES);
            return E_RES_IN_PROGRESS;
        }
        break;

        case E_SMTP_WRITE_TO:
        {
            if (dt->buff.s == NULL)
            {
                dt->buff = sl_create("RCPT TO: <" FMT_SL ">\r\n", 
                                    ARG_SL(dt->current->to));
                dt->buff.l -= 1;
            }

            WRITE_AND_SWITCH_ON_FINISH_TO(E_SMTP_READ_TO_RES);
            return E_RES_IN_PROGRESS;
        }
        break;

        case E_SMTP_WRITE_DATA:
        {
            if (dt->buff.s == NULL)
            {
                dt->file = fopen(dt->current->path.s, "r");
                if (!dt->file)
                {
                    LOG_E("Cannot open " FMT_SL " file", ARG_SL(dt->current->path));
                    return E_RES_FAIL;
                }
    
                dt->buff = sl_create("DATA\r\n");
                dt->buff.l -= 1;
            }

            WRITE_AND_SWITCH_ON_FINISH_TO(E_SMTP_READ_DATA_RES);
            return E_RES_IN_PROGRESS;
        }
        break;

        case E_SMTP_WRITE_FILE:
        {
            if (dt->buff.s != NULL)
            {
                res_rw_e rr = write_to_socket(dt);            
                if (rr == E_ERROR || rr == E_CONNECTION_CLOSE)
                    return E_RES_FAIL;   

                return E_RES_IN_PROGRESS;
            }
            else
            {
                while (!feof(dt->file) && dt->buff.s == NULL)
                {
                    const int buff_size = 256;
                    char buff[buff_size];

                    int rc = fread(buff, sizeof(char), buff_size, dt->file);
                    if (!feof(dt->file) && ferror(dt->file))
                    {
                        LOG_E("Error on read from " FMT_SL, ARG_SL(dt->current->path));
                        return E_RES_FAIL;
                    }

                    dt->buff = sl_dup(SL(buff, rc));

                    res_rw_e rr = write_to_socket(dt);            
                    if (rr == E_ERROR || rr == E_CONNECTION_CLOSE)
                        return E_RES_FAIL;   
                }

                if (dt->buff.s == NULL)
                {
                    SWITCH_TO(E_SMTP_WRITE_END_TRANSFER);
                    fclose(dt->file);
                    dt->file = NULL;
                }

                return E_RES_IN_PROGRESS;
            }
        }
        break;

        case E_SMTP_WRITE_END_TRANSFER:
        {
            if (dt->buff.s == NULL)
            {
                dt->buff = sl_create("\r\n.\r\n");
            }

            WRITE_AND_SWITCH_ON_FINISH_TO(E_SMTP_WAIT);

            if (dt->buff.s == NULL)
                SWITCH_TO(E_SMTP_READ_RESULT);
                // return E_RES_FINISHED;

            return E_RES_IN_PROGRESS;
        }
        break;

        default:
            break;
    }

    return E_RES_IN_PROGRESS;
}

handle_res_e handle_task(domain_task_t * dt)
{
    ASSERT(dt->state != E_SMTP_NO_CONNECTED);

    switch (dt->state)
    {
        case E_SMTP_READ_CONNECTED_RES:
        case E_SMTP_READ_HELO_RES:
        case E_SMTP_READ_FROM_RES:
        case E_SMTP_READ_TO_RES:
        case E_SMTP_READ_DATA_RES:
        case E_SMTP_READ_RESULT:
        {
            return handle_read_task(dt);
        }
        break;
    
        case E_SMTP_WRITE_HELO:
        case E_SMTP_WRITE_FROM:
        case E_SMTP_WRITE_TO:
        case E_SMTP_WRITE_DATA:
        case E_SMTP_WRITE_FILE:
        case E_SMTP_WRITE_END_TRANSFER:
        {
            return handle_write_task(dt);
        }

        case E_SMTP_WAIT:
        {
            SWITCH_TO(E_SMTP_WRITE_FROM);
            return E_RES_IN_PROGRESS;
        }
        break;

        default:
            break;
    }

    return E_RES_FAIL;
}

void finish_task(worker_t * worker, task_t * task, sl_t error)
{
    task->error_desc = error;
    sem_wait(&worker->finished_tasks_lock);
    task->next_task = worker->finished_tasks;
    worker->finished_tasks = task;
    sem_post(&worker->finished_tasks_lock);
}

void do_assigned_works(worker_t * worker, map_t * tasks)
{
    struct pollfd * fds = malloc(sizeof(*fds) * tasks->size);
    int         fds_cnt = 0;
    ASSERT(fds && "Error memory alloc");

    MAP_FOREACH(tasks, entry)
    {
        domain_task_t * dt = entry->value;

        if (dt->current == NULL)
        {
            dt->current   = dt->task_list;
            dt->task_list = dt->task_list->next_task;

            ASSERT(dt->current);
        }

        if (dt->socket_id == 0)
        {
            sl_t mx = get_mx_record(dt->current->domain);
            if (mx.s == NULL)
            {
                LOG_E("Cannot get mx for domain " FMT_SL, ARG_SL(dt->current->domain));
                continue;
            }

            int socket_id = socket_open_to(mx, CONF_CL_TARGET_PORT);
            if (socket_id < 0)
            {
                LOG_E("Cannot open socket to " FMT_SL " (domain " FMT_SL "):%d, skip it", 
                      ARG_SL(mx),
                      ARG_SL(dt->current->domain),
                      CONF_CL_TARGET_PORT);
                sl_free(&mx);
                continue;
            }
            sl_free(&mx);

            dt->socket_id = socket_id;
            SWITCH_TO(E_SMTP_READ_CONNECTED_RES);
        }

        fds[fds_cnt].fd      = dt->socket_id;
        fds[fds_cnt].events  = get_event_by_state(dt->state);
        fds[fds_cnt].revents = 0;
        fds_cnt += 1;
    }

    do
    {
        int rc = poll(fds, fds_cnt, CONF_CL_TIMEOUT_MSEC);
        if (rc < 0)
        {
            LOG_E("poll(...) error: %s", strerror(errno));
            break;
        }

        if (rc == 0)
        {
            //LOG_W("poll(...) timeout, end");
            break;
        }

        for (int i = 0; i < fds_cnt; ++i)
        {
            if (fds[i].revents != 0)
            {
                domain_task_t * dt = NULL;
                sl_t domain = SLNULL;
                MAP_FOREACH(tasks, entry)
                {
                    if (((domain_task_t*)entry->value)->socket_id == fds[i].fd)
                    {
                        dt     = entry->value;
                        domain = entry->key;
                        break;
                    }
                }

                ASSERT(dt);
                handle_res_e res = handle_task(dt);
                if (res == E_RES_FAIL)
                {
                    close(dt->socket_id);
                    dt->socket_id = 0;
                    SWITCH_TO(E_SMTP_NO_CONNECTED);    

                    finish_task(worker, dt->current, SLLIT("Error in handled letter"));  
                    dt->current = NULL;  
                }
                else if (res == E_RES_FINISHED)
                {
                    finish_task(worker, dt->current, SLNULL);
                    dt->current = NULL;  
                }

                if (dt->current == NULL && dt->task_list == NULL)
                {
                    if (dt->socket_id != 0)
                    {
                        close(dt->socket_id);
                        dt->socket_id = 0;
                    }

                    LOG_I("Finished send letters to " FMT_SL, ARG_SL(domain));

                    if (dt->file)
                        fclose(dt->file);

                    sl_free(&dt->buff);

                    free(dt);
                    map_remove(tasks, domain);
                }
            }  
        }
    } while (false);

    free(fds);
}

void distribute_task_for_domains(map_t * tasks, task_t * first)
{
    task_t * p      = first;
    task_t * p_next = NULL;
    while (p != NULL)
    {
        p_next = p->next_task;

        domain_task_t * dt = map_get(tasks, p->domain);
        if (dt)
        {
            p->next_task  = dt->task_list;
            dt->task_list = p;
        }
        else
        {
            dt = domain_task_create();
            p->next_task  = dt->task_list;
            dt->task_list = p;
            map_put(tasks, sl_dup(p->domain), dt);
        }

        p = p_next;
    }
}

void * worker_run(void * p_void)
{
    worker_t * worker = (worker_t *) p_void;

    LOG_I("Thread %lu is running", worker->thread);

    map_t * domain_tasks = map_create();
    domain_tasks->key_deleter = sl_free;

    while (worker->is_worked)
    {
        sem_wait(&worker->new_tasks_lock);
        task_t * first = worker->new_tasks;
        worker->new_tasks = NULL;
        sem_post(&worker->new_tasks_lock);

        distribute_task_for_domains(domain_tasks, first);

        do_assigned_works(worker, domain_tasks);

        sleep(0);
    }

    map_free(domain_tasks);

    LOG_I("Thread %lu is stopped", worker->thread);
    return NULL;
}

void worker_init(worker_t * p)
{
    ASSERT(p);

    p->new_tasks      = NULL;
    p->finished_tasks = NULL;

    int rc;

    rc = sem_init(&p->new_tasks_lock, 0, 1);
    ASSERT(rc == 0);

    rc = sem_init(&p->finished_tasks_lock, 0, 1);
    ASSERT(rc == 0);

    rc = pthread_create(&p->thread, NULL, worker_run, (void *)p);
    ASSERT(rc == 0);

    p->is_worked = true;
}

void worker_clear(worker_t * worker)
{
    ASSERT(worker);

    task_t * p;

    p = worker->new_tasks;
    while (p != NULL)
    {
        task_t * next = p->next_task;
        task_free(p);
        free(p);
        p = next;
    }

    p = worker->finished_tasks;
    while (p != NULL)
    {
        task_t * next = p->next_task;
        task_free(p);
        free(p);
        p = next;
    }
}

void task_free(task_t * task)
{
    ASSERT(task);

    sl_free(&task->path);
    sl_free(&task->domain);
    sl_free(&task->from);
    sl_free(&task->to);
}