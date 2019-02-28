#include "pch.h"
#include "settings.h"
#include "socket.h"
#include "worker.h"

#include <signal.h>     /** sigaction **/

static int srv_sock;
static settings_t settings;
static workers_t workers;

/* Releases all captured resources and shuts down server */
void shutdown_server(int status) {
	LOG_I("Shutting down server...");
	close(srv_sock);
	settings_destroy(&settings);
	workers_destroy(&workers);
	LOG_I("Success!");
	shutdown_log();
	exit(status);
}

/* Handles signals */
void handle_signal_action(int sig_number)
{
    if (sig_number == SIGINT) {
        LOG_D("SIGINT was catched!\n");
        shutdown_server(EXIT_SUCCESS);
    } else if (sig_number == SIGPIPE) {
        LOG_D("SIGPIPE was catched!\n");
        shutdown_server(EXIT_SUCCESS);
    }
}

/** Subscribes to events when thread was terminated or no reader of the pipe **/
int setup_signals(void (*handler)(int))
{
    struct sigaction sa = {0};
    sa.sa_handler = handler;
    if (sigaction(SIGINT, &sa, 0) != 0) {
        LOG_E("Can't subscribe to SIGINT");
        return -1;
    }
    if (sigaction(SIGPIPE, &sa, 0) != 0) {
        LOG_E("Can't subscribe to SIGPIPE");
        return -1;
    }
    return 0;
}

int runnable(int index) {
	LOG_D("Worker %d", index);
	return EXIT_SUCCESS;
}

/* Entry point */
int main(int argc, char **argv) {
	// if (setup_signals(handle_signal_action)) {
	// 	LOG_F("Can't setup signals");
	// 	return -1;
	// }

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
		if (workers_init(&workers, settings.jobs - 1)) {
			LOG_E("Can't create workers, run server in the single process.");
		} else if (workers_run(&workers, runnable)) {
			LOG_F("Can't run one or few workers.");
			shutdown_server(EXIT_FAILURE);
		}
	}

	if (runnable(settings.jobs - 1)) {
		LOG_F("Worker %d: Can't continue a task.", settings.jobs - 1);
		shutdown_server(EXIT_FAILURE);
	}

	if (workers.count > 0) {
		workers_wait(&workers);
	}

	shutdown_server(EXIT_SUCCESS);
}
