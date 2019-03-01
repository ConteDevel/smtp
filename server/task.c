#include "task.h"

int task_run(int index, int srv_sock, int (*condition)()) {
    LOG_D("Task %d", index);
	
	struct pollfd fds[MAX_CLIENTS];
	// int	nfds = 1, cur_size = 0;

	memset(fds, 0 , sizeof(fds));
	fds[0].fd = srv_sock;
	fds[0].events = POLLIN;

    while (condition()) {
        // int rc = poll(fds, nfds, TIMEOUT);
		// if (rc < 0) {
		// 	LOG_E("Can't call poll()");
		// 	return EXIT_FAILURE;
		// }
		// if (rc > 0) {

		// }
    }

    return EXIT_SUCCESS;
}