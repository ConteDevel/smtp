#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "pch.h"

#include <sys/ioctl.h>  /* ioctl, FIONBIO */
#include <sys/types.h>  /* some BSD implementations require it */
#include <sys/socket.h> /* socket, setsockopt */
#include <netinet/in.h> /* super set of <arpa/inet.h> */
#include <arpa/inet.h>  /* inet_addr */

#define MAX_CLIENTS SOMAXCONN

/* Creates a new TCP-socket to listen incomming connections */
int socket_init(int *sock, const char *address, const int port);

#endif //__SOCKET_H__
