#include "worker.h"

int workers_init(workers_t *workers, const size_t count) {
    workers->pids = (pid_t *)calloc(count, sizeof(pid_t));
    workers->count = count;
    if (!workers->pids) { return -1; }
    return 0;
}

int workers_run(workers_t *workers, int (*runnable)(int)) {
    for (int i = 0; i < workers->count; ++i) {
        if ((workers->pids[i] = fork()) < 0) {
            return -1;
        } else if (workers->pids[i] == 0) {
            if (runnable(i)) {
                LOG_F("Worker %d: Can't continue a task.", i);
                return -1;
            }
            exit(EXIT_SUCCESS);
        }
    }
    return 0;
}

void workers_wait(workers_t *workers) {
    int n = workers->count;
    while (n > 0) {
        wait(NULL);
        --n;
    }
}

void workers_destroy(workers_t *workers) {
    if (workers->pids) { free(workers->pids); }
    workers->count = 0;
}