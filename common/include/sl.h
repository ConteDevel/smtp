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
    int    l;
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

#endif // __SL_H__