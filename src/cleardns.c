#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "loader.h"
#include "logger.h"
#include "sundry.h"
#include "system.h"
#include "assets.h"
#include "adguard.h"
#include "crontab.h"
#include "constant.h"
#include "dnsproxy.h"
#include "overture.h"
#include "structure.h"

struct {
    char *config;
    uint8_t debug;
    uint8_t verbose;
} settings;

void init(int argc, char *argv[]) { // return config file
    settings.config = strdup(CONFIG_FILE);
    settings.debug = FALSE;
    settings.verbose = FALSE;

    if (getenv("CONFIG") != NULL) {
        free(settings.config);
        settings.config = strdup(getenv("CONFIG"));
    }
    if (getenv("DEBUG") != NULL && !strcmp(getenv("DEBUG"), "TRUE")) {
        settings.debug = TRUE;
    }
    if (getenv("VERBOSE") != NULL && !strcmp(getenv("VERBOSE"), "TRUE")) {
        settings.verbose = TRUE;
    }

    for (int i = 0; i < argc; ++i) {
        if (!strcmp(argv[i], "--debug")) {
            settings.debug = TRUE;
        }
        if (!strcmp(argv[i], "--verbose")) {
            settings.verbose = TRUE;
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
            free(settings.config);
            settings.config = strdup(argv[++i]); // use custom config file
        }
    }
    log_debug("Config file -> %s", settings.config);
}

void cleardns() { // cleardns service
    if (settings.verbose || settings.debug) {
        LOG_LEVEL = LOG_DEBUG; // enable debug log level
    }
    create_folder(EXPOSE_DIR);
    create_folder(WORK_DIR);
    chdir(EXPOSE_DIR);
    load_config(settings.config); // configure parser
    free(settings.config);
    if (settings.debug) { // debug mode enabled
        loader.diverter->debug = TRUE;
        loader.domestic->debug = TRUE;
        loader.foreign->debug = TRUE;
        if (loader.crond != NULL) {
            loader.crond->debug = TRUE;
        }
        if (loader.filter != NULL) {
            loader.filter->debug = TRUE;
        }
    }

    log_info("Start loading process");
    process_list_init();
    assets_load(loader.resource);
    process_list_append(dnsproxy_load("Domestic", loader.domestic, "domestic.json"));
    process_list_append(dnsproxy_load("Foreign", loader.foreign, "foreign.json"));
    process_list_append(overture_load(loader.diverter, "overture.json"));
    overture_free(loader.diverter);
    dnsproxy_free(loader.domestic);
    dnsproxy_free(loader.foreign);
    assets_free(loader.resource);
    if (loader.crond != NULL) {
        process_list_append(crontab_load(loader.crond)); // free crontab struct later
    }
    if (loader.filter != NULL) {
        process_list_append(adguard_load(loader.filter, ADGUARD_DIR));
        adguard_free(loader.filter);
    }

    for (char **script = loader.script; *script != NULL; ++script) { // running custom script
        log_info("Run custom script -> `%s`", *script);
        run_command(*script);
    }
    string_list_free(loader.script);

    process_list_run(); // start all process
    if (loader.crond != NULL) { // assets not disabled
        kill(getpid(), SIGALRM); // send alarm signal to cleardns
        crontab_free(loader.crond);
    }
    process_list_daemon(); // daemon all process
}

int main(int argc, char *argv[]) {

    char **demo = string_list_init();

    string_list_append(&demo, "item 1");
    string_list_append(&demo, "item 2");
    string_list_append(&demo, "item 3");
    string_list_append(&demo, "item 4");
    string_list_append(&demo, "item 5");

    char *tmp = string_list_dump(demo);
    log_warn("dump -> %s", tmp);

    log_info("string list -> %p -> %p", &demo, demo);
    log_info("string list 1 -> %p -> %p -> `%s`", &demo[0], demo[0], demo[0]);
    log_info("string list 2 -> %p -> %p -> `%s`", &demo[1], demo[1], demo[1]);
    log_info("string list 3 -> %p -> %p -> `%s`", &demo[2], demo[2], demo[2]);
    log_info("string list 4 -> %p -> %p -> `%s`", &demo[3], demo[3], demo[3]);
    log_info("string list 5 -> %p -> %p -> `%s`", &demo[4], demo[4], demo[4]);
    log_info("string list 6 -> %p -> %p -> `%s`", &demo[5], demo[5], demo[5]);

    rust_test_multi(demo);

    log_info("string list 1 -> %p -> %p -> `%s`", &demo[0], demo[0], demo[0]);
    log_info("string list 2 -> %p -> %p -> `%s`", &demo[1], demo[1], demo[1]);
    log_info("string list 3 -> %p -> %p -> `%s`", &demo[2], demo[2], demo[2]);
    log_info("string list 4 -> %p -> %p -> `%s`", &demo[3], demo[3], demo[3]);
    log_info("string list 5 -> %p -> %p -> `%s`", &demo[4], demo[4], demo[4]);
    log_info("string list 6 -> %p -> %p -> `%s`", &demo[5], demo[5], demo[5]);

    string_list_free(demo);

    log_warn("test end");
    exit(0);

    init(argc, argv);
    log_info("ClearDNS server start (%s)", VERSION);
    cleardns();
    return 0;
}
