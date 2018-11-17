#ifndef __LOG_LOG_H__
#define __LOG_LOG_H__

#include <libs.h>

enum log_level_e
{
    LL_FATAL = 0,
    LL_ERROR,
    LL_WARNING,
    LL_INFO,
    LL_DEBUG
};

void     init_logs    (void);
uint32_t get_log_level(void);
void     set_log_level(uint32_t log_level);
void     trace        (const char * format, ...);

#define _LOG(_label, _format, ...) trace(_label "%s:%d " _format "%s\n", __FILE__, __LINE__, __VA_ARGS__)

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
