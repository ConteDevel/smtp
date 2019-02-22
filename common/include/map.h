#ifndef __COMMON_MAP_H__
#define __COMMON_MAP_H__
#include "sl.h"

typedef struct map_entry_t
{
    sl_t   key;
    void * value;

    struct map_entry_t * next;
} map_entry_t;

typedef void (*map_value_deleter_f)(void *);
typedef void (*map_key_deleter_f)  (sl_t *);

typedef struct
{
    map_entry_t * first;
    int           size;

    map_key_deleter_f   key_deleter;
    map_value_deleter_f value_deleter;
} map_t;

map_t * map_create  ();
void    map_free    (map_t * p);
void    map_put     (map_t * p, sl_t key, void * value);
void *  map_get     (map_t * p, sl_t key);
void    map_remove  (map_t * p, sl_t key);

#define MAP_FOREACH(__map, __entry_name) \
    for (map_entry_t * __entry_name = __map->first; __entry_name != NULL; __entry_name = __entry_name->next) \



#endif // __COMMON_MAP_H__