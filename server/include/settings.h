#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include "pch.h"
#include "checkopt.h"

#include <libconfig.h>
#include <unistd.h>     /* access */

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 587
#define SERVER_CONFIG "server.conf"
#define MAILDIR "maildir"

#define CONFIG_ADDRESS "address"
#define CONFIG_PORT "port"
#define CONFIG_JOBS "jobs"
#define CONFIG_MAILDIR "maildir"

typedef struct settings {
    char *address;
    int port;
    int jobs;
    char *maildir;
} settings_t;

/* Loads server settigns */
int settings_init(settings_t *settings, int argc, char **argv);
/* Releases settigns */
void settings_destroy(settings_t *settings);

void settings_log(const settings_t *settings);

#endif // __SETTINGS_H__