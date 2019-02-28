#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include "pch.h"
#include "checkopt.h"

#include <libconfig.h>

typedef struct settings {
    char *address;
    int port;
    int jobs;
    char *maildir;
} settings_t;

int load_settings(settings_t *settings, int argc, char **argv);

#endif // __SETTINGS_H__