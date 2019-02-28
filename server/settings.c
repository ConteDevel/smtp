#include "settings.h"

/* Merges the server settings with configuration file */
int load_config(settings_t *settings) {
    config_t cfg;
    config_init(&cfg);
    if(!config_read_file(&cfg, SERVER_CONFIG)) {
        LOG_E("Can't read config: %s:%d - %s\n", config_error_file(&cfg),
            config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return -1;
    }

    const char *str;
    int num;
    if(config_lookup_string(&cfg, CONFIG_ADDRESS, &str) 
        && _strrpl(str, &settings->address)) {
        config_destroy(&cfg);
        return -1;
    }
    if(config_lookup_int(&cfg, CONFIG_PORT, &num)) {
        settings->port = num;
    }
    if(config_lookup_int(&cfg, CONFIG_JOBS, &num)) {
        settings->jobs = num;
    }
    if(config_lookup_string(&cfg, CONFIG_MAILDIR, &str) 
        && _strrpl(str, &settings->maildir)) {
        config_destroy(&cfg);
        return -1;
    }

    config_destroy(&cfg);
    return 0;
}

/* Loads server settigns */
int settings_init(settings_t *settings, int argc, char **argv) {
    settings->address = calloc(strlen(SERVER_ADDR), sizeof(char));
    if (!settings->address) { return -1; }
    settings->address = strcpy(settings->address, SERVER_ADDR);
    settings->port = SERVER_PORT;
    settings->jobs = 1;
    settings->maildir = calloc(strlen(MAILDIR), sizeof(char));
    if (!settings->maildir) { return -1; }
    settings->maildir = strcpy(settings->maildir, MAILDIR);

    //tOptions opts;
    //optionProcess(&opts, argc, argv);
    if (access(SERVER_CONFIG, F_OK ) != -1 && load_config(settings)) {
        return -1;
    }

    return 0;
}

/* Releases settigns */
void settings_destroy(settings_t *settings) {
    if (settings) {
        if (settings->address) { free(settings->address); }
        if (settings->maildir) { free(settings->maildir); }
    }
}

void settings_log(settings_t *settings) {
    if (settings) {
        LOG_I("Configuration { address: '%s:%d', jobs: %d, maildir: '%s'}",
            settings->address, settings->port, 
            settings->jobs, settings->maildir);
    }
}