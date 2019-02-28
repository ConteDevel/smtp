#include "pch.h"
#include "settings.h"
#include "socket.h"
#include "worker.h"

#include <signal.h>     /** sigaction **/
#include <sys/wait.h> 

static int srv_sock;
static settings_t settings;
static pid_t *pids = 0;

/* Releases all captured resources and shuts down server */
void shutdown_server(int status) {
	LOG_I("Shutting down server...");
	close(srv_sock);
	settings_destroy(&settings);
	if (pids) { free(pids); }
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
    struct sigaction sa;
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
		LOG_F("Can't load configuration");
		exit(EXIT_FAILURE);
	}
	settings_log(&settings);
	
	if (socket_init(&srv_sock, settings.address, settings.port)) {
	    LOG_F("Can't initialize socket");
        exit(EXIT_FAILURE);
	}

	if (settings.jobs > 1) {
		pids = (pid_t *)malloc((settings.jobs - 1) * sizeof(pids));
		/* Start children. */
		for (int i = 0; i < (settings.jobs - 1); ++i) {
			if ((pids[i] = fork()) < 0) {
				LOG_F("Can't create a worker");
				shutdown_server(EXIT_FAILURE);
			} else if (pids[i] == 0) {
				LOG_I("Worker %d", (i + 1));
				exit(EXIT_SUCCESS);
			}
		}
	}

	LOG_I("Worker %d", settings.jobs);

	if (settings.jobs > 1) {
		/* Wait for children to exit. */
		int n = settings.jobs - 1;
		while (n > 0) {
			wait(NULL);
			--n;
		}
	}

	shutdown_server(EXIT_SUCCESS);
}
