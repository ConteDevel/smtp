#include "pch.h"
#include "settings.h"
#include "socket.h"
#include "task.h"
#include "handler.h"

#include <signal.h>     /** sigaction **/

static int srv_sock;
static settings_t settings;
static handlers_t handlers;
static volatile sig_atomic_t gotsig = 0;

/* Releases all captured resources and shuts down handler */
void shutdown_handler(int index, int status) {
	if (status == EXIT_FAILURE) {
		LOG_E("Handler %d: Unexpected behavior", index);
	}

	LOG_I("Handler %d: Shutting down...", index);
	close(srv_sock);
	settings_destroy(&settings);
	handlers_destroy(&handlers);
	LOG_I("Handler %d: OK!", index);
	exit(status);
}

/* Releases all captured resources and shuts down server */
void shutdown_server(int status) {
	if (status == EXIT_FAILURE) {
		LOG_E("Unexpected behavior");
	}
	
	LOG_I("Shutting down server...");
	close(srv_sock);
	settings_destroy(&settings);
	handlers_destroy(&handlers);
	LOG_I("OK!");
	shutdown_log();
	exit(status);
}

/* Handles signals */
void handle_signal_action(int signo) {
    gotsig = 1;
}

/** Subscribes to events when thread was terminated or no reader of the pipe **/
int setup_signals() {
    struct sigaction sa = {0};
    sa.sa_handler = handle_signal_action;
	//sa.sa_flags = 0;
    //sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, 0) != 0) {
        LOG_F("Can't subscribe to SIGINT");
        return -1;
    }
    if (sigaction(SIGPIPE, &sa, 0) != 0) {
        LOG_F("Can't subscribe to SIGPIPE");
        return -1;
    }
    return 0;
}

int condition() {
	return gotsig == 0;
}

int runnable(int index) {
	if (task_run(index, srv_sock, condition)) {
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/* Entry point */
int main(int argc, char **argv) {
	if (setup_signals()) {
		LOG_F("Can't setup signals");
		return -1;
	}

	if (init_log()) {
		printf("Can't initialize logger.\n");
		return -1;
	}

	if (settings_init(&settings, argc, argv)) {
		LOG_F("Can't load configuration.");
		exit(EXIT_FAILURE);
	}
	settings_log(&settings);
	
	if (socket_init(&srv_sock, settings.address, settings.port)) {
	    LOG_F("Can't initialize socket.");
        exit(EXIT_FAILURE);
	}

	if (settings.jobs > 1) {
		if (handlers_init(&handlers, settings.jobs - 1)) {
			LOG_E("Can't create tasks, run server in the single process.");
		} else if (handlers_run(&handlers, runnable, shutdown_handler)) {
			LOG_F("Can't run one or few tasks.");
			shutdown_server(EXIT_FAILURE);
		}
	}

	if (runnable(settings.jobs - 1)) {
		LOG_F("Task %d: Can't continue work.", settings.jobs - 1);
		shutdown_server(EXIT_FAILURE);
	}

	if (handlers.count > 0 && handlers_wait(&handlers)) {
		shutdown_server(EXIT_FAILURE);
	}

	shutdown_server(EXIT_SUCCESS);
}
