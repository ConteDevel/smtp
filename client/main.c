#include <utils.h>
#include "controller.h"
#include "config.client.h"


int socket_open_to_2(const sl_t address, uint16_t port)
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

typedef struct
{
    sl_t sended_data;
    int  send_cnt;

    sl_t recived_data;
} socket_info_t;

socket_info_t * socket_info_new()
{
    socket_info_t * ptr = malloc(sizeof(socket_info_t));
    ASSERT(ptr);

    sl_clear(&ptr->sended_data);
    
    ptr->recived_data = sl_dup(SLEMPTY);
    ptr->send_cnt = 0;

    return ptr;
}

void socket_info_delete(socket_info_t * ptr)
{
    ASSERT(ptr);

    sl_free(&ptr->recived_data);
    free  (ptr);
}

typedef struct 
{
    struct pollfd  fds  [CONF_CL_SOCKETS_MAX_COUNT];
    socket_info_t* infos[CONF_CL_SOCKETS_MAX_COUNT];
    int            fds_cnt;
    bool           need_compress;
} sockets_t;

void sockets_clear(sockets_t * sockets)
{
    ASSERT(sockets);

    memset(sockets->fds,   0, sizeof(sockets->fds[0]) * CONF_CL_SOCKETS_MAX_COUNT);
    memset(sockets->infos, 0, sizeof(sockets->fds[0]) * CONF_CL_SOCKETS_MAX_COUNT);

    sockets->fds_cnt       = 0;
    sockets->need_compress = false;
}

int sockets_add(sockets_t * sockets, int fd, socket_info_t * info)
{
    ASSERT(sockets);
    ASSERT(info);
    ASSERT(fd > 0);

    if (sockets->fds_cnt >= CONF_CL_SOCKETS_MAX_COUNT)
    {
        LOG_E("Socket was not add - already max sockets");
        return -1;
    }

    sockets->fds  [sockets->fds_cnt].fd     = fd;
    sockets->fds  [sockets->fds_cnt].events = POLLOUT;
    sockets->infos[sockets->fds_cnt]        = info;
    sockets->fds_cnt++;

    LOG_I("Socket was add to %d pos, fd = %d, %d", sockets->fds_cnt - 1, fd, sockets->fds[0].events);
    return 0;
}

void sockets_remove_by_index(sockets_t * sockets, int index)
{
    ASSERT(sockets);
    ASSERT(index >= 0 && index < sockets->fds_cnt);
    ASSERT(sockets->fds[index].fd > 0);
    ASSERT(sockets->infos[index]);

    close(sockets->fds[index].fd);
    sockets->fds[index].fd = -1;

    socket_info_delete(sockets->infos[index]);
    sockets->infos[index] = NULL;

    LOG_D("Socket remove indx = %d", index);
}


void sockets_destroy(sockets_t * ptr)
{
    ASSERT(ptr);

    for (int i = 0; i < ptr->fds_cnt; ++i)
    {
        if (ptr->fds[i].fd > 0)
            sockets_remove_by_index(ptr, i);
    }

    sockets_clear(ptr);
}

void do_send()
{
    ASSERT(CONF_CL_NUM_WORK_THREADS > 0);

    sockets_t sockets;
    sockets_clear(&sockets);

    {
        int fd = socket_open_to_2(SLLIT("google.com"), 80);
        ASSERT(0 < fd);
        socket_info_t * info = socket_info_new();
        info->sended_data    = SLLIT("GET /index.html HTTP/1.1\r\n\r\n");

        sockets_add(&sockets, fd, info);
    }

    const int timeout = CONF_CL_TIMEOUT_MSEC;
    while (true)
    {
        int rc = poll(sockets.fds, sockets.fds_cnt, timeout);
        if (rc < 0)
        {
            LOG_E("poll(...) error: %s", strerror(errno));
            break;
        }

        if (rc == 0)
        {
            LOG_W("poll(...) timeout, end");
            break;
        }

        for (int i = 0; i < sockets.fds_cnt; ++i)
        {
            if (sockets.fds[i].fd <= 0)
                continue;

            if (sockets.fds[i].revents == 0)
                continue;

            if (sockets.fds[i].revents == POLLOUT)
            {
                rc = send(sockets.fds[i].fd, 
                          sockets.infos[i]->sended_data.s + sockets.infos[i]->send_cnt,
                          sockets.infos[i]->sended_data.l - sockets.infos[i]->send_cnt,
                          0);

                if (rc < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    LOG_E("Error on send(...) %s", strerror(errno));
                    sockets_remove_by_index(&sockets, i);
                    continue;
                }
                else if (rc == 0)
                {
                    LOG_W("Connection was closed");
                    sockets_remove_by_index(&sockets, i);
                    continue;
                }
                else
                {
                    sockets.infos[i]->send_cnt += rc;
                    if (sockets.infos[i]->send_cnt == sockets.infos[i]->sended_data.l)
                    {
                        sockets.fds[i].events = POLLIN;
                        LOG_I("End send, start recv");
                    }
                }           
            }
            else if (sockets.fds[i].revents == POLLIN)
            {
                const int buffer_size = 1024;
                char buffer[buffer_size];

                rc = recv(sockets.fds[i].fd, buffer, buffer_size, 0);
                if (rc < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    LOG_E("Error on recv(...) %s", strerror(errno));
                    sockets_remove_by_index(&sockets, i);
                    continue;
                }
                else if (rc == 0)
                {
                    LOG_W("Connection was closed, print data:\n" FMT_SL, 
                          ARG_SL(sockets.infos[i]->recived_data));
                    sockets_remove_by_index(&sockets, i);
                    continue;
                }    
                else
                {
                    sl_t sl_buff = SL(buffer, rc);
                    sl_t new_str = sl_combine(&sockets.infos[i]->recived_data, &sl_buff);

                    sl_free(&sockets.infos[i]->recived_data);
                    sockets.infos[i]->recived_data = new_str;
                }            
            }
            else 
            {
                LOG_W("Unexpected revents %d", sockets.fds[i].revents);
                continue;
            }
        }
    }

    sockets_destroy(&sockets);
}

controller_t controller;

void sigint_handler(int dummy) 
{
    controller.is_worked = false;
}

int main(int argc, char const *argv[])
{
    init_log();

    sl_t mail_dir = SLLIT("/home/lordneznay/smtp/maildir");

    controller_init(&controller, CONF_CL_NUM_WORK_THREADS);

    signal(SIGINT, sigint_handler);

    controller_run(&controller, mail_dir);

    controller_clear(&controller);
    //do_send();

    return 0;
}
