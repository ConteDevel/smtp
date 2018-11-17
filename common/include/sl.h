#ifndef __SL_H__
#define __SL_H__

#include <libs.h>
#include <log.h>

/*
    String-Length pair
    String may not-null terminated
*/
typedef struct
{
    char * s;
    int       l;
} sl_t;

#define SL(_string, _length) (sl_t){ .s = _string, .l = _length }
#define SLLIT(_literal) SL(_literal, sizeof(_literal))
#define SLNULL SL(NULL, 0)
#define SLEMPTY SL("", 0)
#define FMT_SL "%.*s"
#define ARG_SL(_sl) (_sl).l, (_sl).s

int  sl_cmp     (const sl_t     lhs, 
                 const sl_t     rhs);
sl_t sl_dup     (const sl_t     str);
sl_t sl_create  (const char *   format, 
                 ...);
sl_t sl_combine (const sl_t *   lhs,
                 const sl_t *   rhs);
void sl_free    (sl_t *         str);
void sl_clear   (sl_t *         str);

inline sl_t sl_combine(const sl_t * lhs, const sl_t * rhs)
{
    ASSERT(lhs);
    ASSERT(rhs);

    const int lenght = lhs->l + rhs->l;

    char * str = malloc((lenght > 0 ? lenght : 1) * sizeof(char));
    ASSERT(str);

    if (lhs->l > 0)
        memcpy(str,          lhs->s, lhs->l);

    if (rhs->l > 0)
        memcpy(str + lhs->l, rhs->s, rhs->l);

    return SL(str, lhs->l + rhs->l);
}

inline sl_t sl_create(const char * format, ...)
{
    const uint32_t default_size = 128;

    uint32_t current_size = default_size;
    char *            str = (char *)malloc(current_size);
    ASSERT(str);

    va_list args;
    va_start(args, format);
    int res = vsnprintf(str, current_size, format, args);
    va_end(args);

    ASSERT(res >= 0);

    while(res == current_size)
    {
        free(str);
        current_size *= 2;
        str           = (char *)malloc(current_size);
        ASSERT(str);

        va_list args;
        va_start(args, format);
        res = vsnprintf(str, current_size, format, args);
        va_end(args);
        ASSERT(res >= 0);
    }


    return SL(str, res + 1);
}

inline void sl_free(sl_t * str)
{
    ASSERT(str);
    
    if (str->s)
        free(str->s);

    sl_clear(str);
}

inline void sl_clear(sl_t * str)
{
    ASSERT(str);

    str->s = NULL;
    str->l = 0;
}

inline int sl_cmp(const sl_t lhs, const sl_t rhs)
{
    if (lhs.l < rhs.l)
        return -1;
    else if (lhs.l > rhs.l)
        return 1;
    else 
        return strncmp(lhs.s, rhs.s, lhs.l);
}

inline sl_t sl_dup(const sl_t str)
{
    const int buffsize = str.l != 0 ? str.l : 1;
    char * str_copy = (char *)malloc(buffsize * sizeof(char));
    if (str_copy == NULL)
    {
        LOG_F("Error memory allocation, abort");
        ASSERT(false);
    }

    memcpy(str_copy, str.s, str.l);
    return SL(str_copy, str.l);
}

#endif // __SL_H__