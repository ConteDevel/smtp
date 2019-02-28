#include "pch.h"
#include "settings.h"
#include "socket.h"

static int srv_sock;
static settings_t settings;

void shutdown_server(int status) {
	LOG_I("Shutting down server...");
	close(srv_sock);
	LOG_I("Success!");
	exit(status);
}

int main(int argc, char **argv) {
	if (init_log()) {
		printf("Can't initialize logger.\n");
		return -1;
	}

	if (load_settings(&settings, argc, argv)) {
		LOG_F("Can't load configuration");
		exit(EXIT_FAILURE);
	}
	// struct pollfd fds[MAX_CLIENTS + 2]; // Max clients + STDIN + STDOUT
	
	if (make_socket(&srv_sock)) {
	    LOG_F("Can't initialize socket");
        exit(EXIT_FAILURE);
	}
	
	//while (1) {
        LOG_W("Hello, world!");
    //}
    
    shutdown_server(EXIT_SUCCESS);
}
