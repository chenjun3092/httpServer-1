#include <stdlib.h>
#include <zconf.h>
#include <init_config.h>
#include <log.h>
#include "server.h"

const char *jpath;
extern server_config_package *p;


int main (int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "ex: httpserv json_path");
        exit(1);
    }
    jpath = argv[1];
    server_init(jpath);

    return 0;
}