#ifndef __LOG_H__
#define __LOG_H__

#include "libs.h"

#define MQ_NAME "/logmq"
#define MAX_MSG_SIZE 512
#define TIME_FORMAT "%d.%m.%Y %H:%M:%S"
#define TIME_SIZE 20

/* Log levels */
typedef enum log_lvl {
    LL_DEBUG = 0,
    LL_INFO,
    LL_WARN,
    LL_ERROR,
    LL_FATAL
} log_lvl_t;

/* Message */
typedef struct mq_msg {
    log_lvl_t level;
    char msg[MAX_MSG_SIZE + 1];
} mq_msg_t;

/* Initializes logger */
uint32_t init_log();

/* Sets log level */
void set_log_lvl(log_lvl_t lvl);
/* Returns log level */
log_lvl_t get_log_lvl();

/* Sends log message to the message queue */
void trace(const char *format, ...);

#endif //__LOG_H__
