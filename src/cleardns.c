#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "loader.h"
#include "logger.h"
#include "constant.h"
#include "dnsproxy.h"
#include "overture.h"
#include "adguard.h"
#include "system.h"
#include "sundry.h"
#include "structure.h"
#include "assets.h"
#include "crontab.h"

char* init(int argc, char *argv[]) { // return config file
    char *config = string_init(CONFIG_FILE);
    for (int i = 0; i < argc; ++i) {
        if (!strcmp(argv[i], "--debug")) {
            LOG_LEVEL = LOG_DEBUG; // enable debug mode
        }
        if (!strcmp(argv[i], "--version")) {
            printf("ClearDNS version %s\n", VERSION); // show version
            exit(0);
        }
        if (!strcmp(argv[i], "--help")) {
            printf("\n%s\n", HELP_MSG); // show help message
            exit(0);
        }
        if (!strcmp(argv[i], "--config")) {
            if (i + 1 == argc) {
                log_error("Option `--config` missing value");
                exit(1);
            }
            free(config);
            config = string_init(argv[++i]); // use custom config file
        }
    }
    log_debug("Config file -> %s", config);
    return config;
}

int main(int argc, char *argv[]) { // ClearDNS service
    char *config_file = init(argc, argv);

    LOG_LEVEL = LOG_DEBUG;

    log_info("ClearDNS server start (%s)", VERSION);
    create_folder(WORK_DIR);
    extract_assets();
    load_config(config_file);
    free(config_file);

    if (LOG_LEVEL == LOG_DEBUG) { // debug mode enabled
        loader.filter->debug = TRUE;
        loader.diverter->debug = TRUE;
        loader.domestic->debug = TRUE;
        loader.foreign->debug = TRUE;
    }

    process_list_init();
    process_list_append(assets_load(loader.assets));
    process_list_append(dnsproxy_load("Domestic", loader.domestic, "domestic.json"));
    process_list_append(dnsproxy_load("Foreign", loader.foreign, "foreign.json"));
    process_list_append(overture_load(loader.diverter, "overture.json"));
    overture_free(loader.diverter);
    dnsproxy_free(loader.domestic);
    dnsproxy_free(loader.foreign);
    assets_free(loader.assets);
    if (loader.filter != NULL) {
        process_list_append(adguard_load(loader.filter, ADGUARD_DIR));
        adguard_free(loader.filter);
    }

    for (char **script = loader.script; *script != NULL; ++script) { // run custom script
        log_info("Run custom script -> `%s`", *script);
        run_command(*script);
    }
    string_list_free(loader.script);

    process_list_run();
    kill(1, SIGALRM); // send alarm signal to itself
    process_list_daemon();
    return 0;
}
