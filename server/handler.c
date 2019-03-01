#include "handler.h"

int handlers_init(handlers_t *handlers, const size_t count) {
    handlers->pids = (pid_t *)calloc(count, sizeof(pid_t));
    handlers->count = count;
    if (!handlers->pids) { return -1; }
    return 0;
}

int handlers_run(handlers_t *handlers, int (*runnable)(int), void (*on_finish)(int, int)) {
    for (int i = 0; i < handlers->count; ++i) {
        if ((handlers->pids[i] = fork()) < 0) {
            return -1;
        } else if (handlers->pids[i] == 0) {
            if (runnable(i)) {
                LOG_F("Handler %d: Can't continue a task.", i);
                on_finish(i, EXIT_FAILURE);
            }
            on_finish(i, EXIT_SUCCESS);
        }
    }
    return 0;
}

int handlers_wait(handlers_t *handlers) {
    int n = handlers->count;
    int result = 0;
    
    while (n > 0) {
        int status = 0;
        wait(&status);
        --n;
        if (status) { result = -1; }
    }

    return result;
}

void handlers_destroy(handlers_t *handlers) {
    if (handlers->pids) { free(handlers->pids); }
    handlers->count = 0;
}