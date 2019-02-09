#include "session.h"

void set_parameter (session *sessions, char *paramter, char *value) {
    map_set(&sessions->parameters, paramter, value);
}

char *get_parameter (session *sessions, char *paramter) {
    map_str_t paramters = sessions->parameters;
    char **r = map_get(&paramters, paramter);
    if (!r) {
        return NULL;
    } else {
        return *r;
    }
}
