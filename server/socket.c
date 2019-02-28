#include "socket.h"

/** Makes a socket address reusable **/
int make_address_reusable(int sock) {
    /* Make address reusable */
    int reused = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reused, sizeof(reused))) {
        LOG_F("setsockopt(SO_REUSEADDR)");
        return -1;
    }
#ifdef SO_REUSEPORT
    /* Make port reusable */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reused, sizeof(reused))) {
        LOG_F("setsockopt(SO_REUSEPORT)");
        return -1;
    }
#endif
    return 0;
}

/* Creates a new TCP-socket to listen incomming connections */
int make_socket(int *sock)
{
    /* Initialize TCP-socket */
    *sock = socket(AF_INET, SOCK_STREAM, 0);
    if (*sock < 0) {
        LOG_F("socket()");
        return -1;
    }
    /* Make socket address reusable */
    if (make_address_reusable(*sock)) {
        LOG_F("make_address_reusable()\n");
        return -1;
    }
    /* Setup address */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    addr.sin_port = htons(SERVER_PORT);
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        LOG_F("inet_addr()");
        return -1;
    }
    /* Bind to this address */
    if (bind(*sock, (struct sockaddr *)&addr, sizeof(struct sockaddr)) != 0) {
        LOG_F("bind()");
        return -1;
    }

    if (listen(*sock, MAX_CLIENTS) != 0) {
        LOG_F("listen()");
        return -1;
    }

    return 0;
}
