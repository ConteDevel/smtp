#include "pch.h"
#include "socket.h"

int main() {
	if (init_log()) {
		printf("Can't initialize logger.\n");
		return -1;
	}
	
	int sock;
	// struct pollfd fds[MAX_CLIENTS + 2]; // Max clients + STDIN + STDOUT
	
	if (make_socket(&sock)) {
	    LOG_F("make_socket()\n");
        exit(EXIT_FAILURE);
	}
	
	while (1) {
        LOG_W("Hello, world!");
    }
    
    close(sock);

    return 0;
}
