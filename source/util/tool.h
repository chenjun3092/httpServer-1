
#ifndef HTTPSERV_TOOL_H
#define HTTPSERV_TOOL_H

#include <map.h>

int hexit (char c);

void encode_str (char *to, int tosize, const char *from);

void decode_str (char *to, char *from);

const char *get_file_type (const char *name);

int rand_num ();


#endif //HTTPSERV_TOOL_H
