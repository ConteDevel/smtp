#include <log.h>

uint32_t current_log_level = LL_DEBUG;

void init_logs()
{
    // do nothing, TODO
}

uint32_t get_log_level()
{
    return current_log_level;
}

void set_log_level(uint32_t log_level)
{
    current_log_level = log_level;
}

void trace(const char * format, ...)
{
    time_t      timestamp;
    struct tm * timeinfo;

    time(&timestamp);
    timeinfo = localtime(&timestamp);

    printf("[%u][%02d:%02d:%02d] ", (uint32_t)timestamp,
                                    timeinfo->tm_hour,
                                    timeinfo->tm_min,
                                    timeinfo->tm_sec);

    va_list args;
    va_start(args, format);
    vprintf (format, args);
    va_end  (args);
}

