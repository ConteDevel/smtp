#ifndef __LOG_LOG_H__
#define __LOG_LOG_H__

#include <libs.h>

#define MQ_NAME "/logmq"
#define MAX_MSG_SIZE 512

typedef enum 
{
    LL_FATAL = 0,
    LL_ERROR,
    LL_WARNING,
    LL_INFO,
    LL_DEBUG
} log_level_t;

/* Initializes logger */
uint32_t init_log     (void);
/* Returns log level */
log_level_t get_log_level(void);
/* Sets log level */
void     set_log_level(uint32_t log_level);
/* Sends a log message to the message queue */
void     trace        (const char * format, ...);

#define _LOG(_label, _format, ...) trace("%lu " _label "%s:%d " _format "%s\n", (unsigned long)pthread_self(), __FILE__, __LINE__, __VA_ARGS__)

#define LOG(...) _LOG(__VA_ARGS__, "")
#define LOG_F(...) { if (get_log_level() >= LL_FATAL)   LOG("FATAL: ", ##__VA_ARGS__); }
#define LOG_E(...) { if (get_log_level() >= LL_ERROR)   LOG("ERROR: ", ##__VA_ARGS__); }
#define LOG_W(...) { if (get_log_level() >= LL_WARNING) LOG("WARN:  ", ##__VA_ARGS__); }
#define LOG_I(...) { if (get_log_level() >= LL_INFO)    LOG("INFO:  ", ##__VA_ARGS__); }
#define LOG_D(...) { if (get_log_level() >= LL_DEBUG)   LOG("DEBUG: ", ##__VA_ARGS__); }

#define ASSERT(_condition)                                  \
{                                                           \
    const bool ___assert_condition_value = _condition;      \
    if (!___assert_condition_value)                         \
    {                                                       \
        LOG_F("Assert triggered!");                         \
        assert(#_condition && ___assert_condition_value);   \
    }                                                       \
}

#endif //__LOG_H__
