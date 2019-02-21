#include "map.h"

map_t * map_create()
{
    map_t * p = malloc(sizeof(*p));
    ASSERT(p && "Error memory alloc");

    p->first         = NULL;
    p->size          = 0;
    p->key_deleter   = NULL;
    p->value_deleter = NULL;

    return p;
}

map_entry_t * map_entry_create(sl_t key, void * value)
{
    map_entry_t * p = malloc(sizeof(*p));
    ASSERT(p && "Error memory alloc");

    p->key   = key;
    p->value = value;

    p->next = NULL;

    return p;
}

void map_entry_contain_free(map_entry_t * p, map_t * map)
{
    if (map->key_deleter)
        map->key_deleter(&p->key);

    if (map->value_deleter)
        map->value_deleter(p->value);
}

void map_entry_free(map_entry_t * p, map_t * map)
{
    map_entry_contain_free(p, map);

    free(p);
}

void map_free(map_t * map)
{
    ASSERT(map);

    map_entry_t * ptr = map->first;
    while (ptr != NULL)
    {
        map_entry_t * cur = ptr;
        ptr = ptr->next;

        map_entry_free(cur, map);
    }

    free(map);
}

map_entry_t * map_entry_get(map_t * map, sl_t key)
{
    ASSERT(map);

    map_entry_t * p = map->first;
    while (p != NULL)
    {
        if (!sl_cmp(p->key, key))
            break;

        p = p->next;
    }

    return p;
}

void map_put(map_t * map, sl_t key, void * value)
{
    ASSERT(key.l > 0);
    ASSERT(value);

    map_entry_t * entry = map_entry_get(map, key);
    if (entry)
    {
        map_entry_contain_free(entry, map);
        entry->key   = key;
        entry->value = value;
    }
    else
    {
        entry = map_entry_create(key, value);
        entry->next = map->first;
        map->first  = entry;
        map->size  += 1;
    }
}

void * map_get(map_t * map, sl_t key)
{
    map_entry_t * entry = map_entry_get(map, key);
    if (entry)
        return entry->value;

    return NULL;
}

void map_remove(map_t * map, sl_t key)
{
    ASSERT(map);

    map_entry_t  * p    =  map->first;
    map_entry_t ** prev = &map->first;
    while (p != NULL)
    {
        if (!sl_cmp(p->key, key))
            break;

        prev = &p->next;
        p    =  p->next;
    }

    if (p)
    {
        *prev = p->next;
        map_entry_free(p, map);
        map->size -= 1;
    }
}