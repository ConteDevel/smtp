#include "settings.h"

int load_settings(settings_t *settings, int argc, char **argv) {
    settings->address = SERVER_ADDR;
    settings->port = SERVER_PORT;
    settings->jobs = 1;
    settings->maildir = MAILDIR;

    //tOptions opts;
    //optionProcess(&opts, argc, argv);

    return 0;
}