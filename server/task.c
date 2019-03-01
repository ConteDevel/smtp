#include "task.h"

typedef struct {
	int id;
	int srv_sock;
	struct pollfd fds[MAX_CLIENTS + 1];
	int nfds;
} task_t;

int handle_new_conn(task_t *task) {
	int new_sock;
	do {
		new_sock = accept(task->srv_sock, NULL, NULL);
		if (new_sock < 0) {
			if (errno != EWOULDBLOCK) {
				LOG_E("Task %d: accept() failed.", task->id);
				return -1;
			}
			break;
		}
		LOG_I("Task %d: New incoming connection %d", task->id, new_sock);
		task->fds[task->nfds].fd = new_sock;
		task->fds[task->nfds].events = POLLIN;
		++task->nfds;
	} while (new_sock != -1);

	return 0;
}

int read_data(task_t *task, int index) {
	char buffer[128];
	do {
		int rc = recv(task->fds[index].fd, buffer, sizeof(buffer), 0);
		if (rc < 0) {
			if (errno != EWOULDBLOCK) {
				LOG_E("Task %d: recv() failed.", task->id);
				return -1;
			}
			break;
		}
		if (rc == 0) {
			LOG_E("Task %d: Client %d closed connection.", 
				task->id, task->fds[index].fd);
			return -1;
		}
	} while (1);
	return 0;
}

void compress_array(task_t *task) {
	for (int i = 0; i < task->nfds; ++i) {
		if (task->fds[i].fd == -1) {
			for(int j = i; j < task->nfds; ++j) {
				task->fds[j].fd = task->fds[j + 1].fd;
			}
			--i;
			--task->nfds;
		}
	}
}

int handle_fds(task_t *task, int *compress) {
	int cur_size = task->nfds;
	for (int i = 0; i < cur_size; ++i) {
		if(task->fds[i].revents == 0) { continue; }

		if(task->fds[i].revents != POLLIN) {
			LOG_E("Task %d: revents = %d.", task->id, task->fds[i].revents);
			return -1;
		}

		if (task->fds[i].fd == task->srv_sock && handle_new_conn(task)) {
			LOG_E("Task %d: Can't handle incoming connections.", task->id);
			return -1;
		} else {
			if (read_data(task, i)) {
				LOG_I("Task %d: Close connection %d.", 
					task->id, task->fds[i].fd);
				close(task->fds[i].fd);
				task->fds[i].fd = -1;
				*compress = 1;
			}
		}
	}
	return 0;
}

int task_run(int id, int srv_sock, int (*condition)()) {
	task_t task;
	task.id = id;
	task.srv_sock = srv_sock;
	task.nfds = 1;
	int run = 1;
	int status = EXIT_SUCCESS;

	memset(task.fds, 0, sizeof(task.fds));
	task.fds[0].fd = srv_sock;
	task.fds[0].events = POLLIN;

    while (condition() && run) {
        int rc = poll(task.fds, task.nfds, TIMEOUT);
		if (rc < 0) {
			LOG_E("Task %d: Can't call poll().", id);
			status = EXIT_FAILURE;
			break;
		}

		int compress = 0;

		if (rc > 0 && handle_fds(&task, &compress)) {
			status = EXIT_FAILURE;
			run = 0;
		}

		if (compress) {
			compress = 0;
			compress_array(&task);
		}
    }

	for (int i = 0; i < task.nfds; ++i) {
		if(task.fds[i].fd >= 0) { close(task.fds[i].fd); }
	}

    return status;
}