#include <log.h>

#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h> 
#include <mqueue.h>
#include <unistd.h>

#define STOP_SIG "!>kill"

/* Message */
typedef struct mq_msg {
    log_level_t level;
    char msg[MAX_MSG_SIZE + 1];
} mq_msg_t;

/* Process ID */
pid_t pid;
/* Current log level */
log_level_t cur_log_lvl;
/* Message queue descriptor */
mqd_t mq;
/* Message queue attributes */
struct mq_attr attr;
/* Current message */
mq_msg_t cur_msg;

/* Initialize message queue listner for the child process */
void init_listener() {
    while (1) {
        ssize_t len = mq_receive(mq, (char *)&cur_msg, sizeof(cur_msg), NULL);
        if (len > 0) {
            // Check stop signal
            if (strcmp(STOP_SIG, cur_msg.msg) == 0) { exit(EXIT_SUCCESS); }
            
            time_t timestamp;
            struct tm *timeinfo;

            time(&timestamp);
            timeinfo = localtime(&timestamp);
            FILE *f = cur_msg.level < LL_WARNING ? stderr : stdout;

            fprintf(f, "[%u][%02d:%02d:%02d] ", (uint32_t)timestamp,
                                                timeinfo->tm_hour,
                                                timeinfo->tm_min,
                                                timeinfo->tm_sec);
            fprintf(f, "%s\n", cur_msg.msg);
            fflush(f);
		    usleep(0.725 * 1e6); // TODO: Read about these magic numbers
        }
    }
}

/* Initializes logger */
bool init_log() {
    cur_log_lvl = LL_DEBUG;
    // Initialize a message queue
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(cur_msg);
    attr.mq_curmsgs = 0;
    mq = mq_open(MQ_NAME, O_CREAT | O_RDONLY, 0644, &attr);
    if (mq == (mqd_t)-1) {
        printf("Can't create a message queue\n");
        return -1;
    }
    // Create a logger process
    pid = fork();
    if (pid == -1) {
        printf("Can't create the logger process\n");
        return -1;
    }

    if (pid == 0) {
        signal(SIGINT, SIG_IGN); // Ignore SIGINT
        init_listener(); // Listen the message queue
    } else {
        mq = mq_open(MQ_NAME, O_WRONLY); // Connect to the message queue
        if (mq == (mqd_t)-1) {
            printf("Can't connect to the message queue\n");
            return -1;
        }
    }

    return 0;
}

void shutdown_log() {
    if (pid > 0) { 
        memset(cur_msg.msg, 0, sizeof(cur_msg.msg));
        strcpy(cur_msg.msg, STOP_SIG);
        mq_send(mq, (const char *)&cur_msg, sizeof(cur_msg), 0);
        wait(NULL);
    }
    if (mq == (mqd_t)-1) { mq_close(mq); }
}

/* Destructor */
void __attribute__((destructor)) finish_log() {
    // shutdown_log();
}

/* Sets log level */
void set_log_level(log_level_t lvl) {
    cur_log_lvl = lvl;
}

/* Returns log level */
log_level_t get_log_level() {
    return cur_log_lvl;
}

/* Sends log message to the message queue */
void trace(const char *format, ...) {
    if (pid != 0) {
        cur_msg.level = LL_DEBUG;
        va_list args;
        va_start(args, format);
        vsnprintf(cur_msg.msg, MAX_MSG_SIZE, format, args);
        va_end(args);
        mq_send(mq, (const char *)&cur_msg, sizeof(cur_msg), 0);
    }
}
