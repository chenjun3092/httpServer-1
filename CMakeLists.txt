cmake_minimum_required(VERSION 2.7)
project(hello C)

set(CMAKE_C_STANDARD 11)

include_directories(source/log)
include_directories(source/server)
include_directories(source/util)
include_directories(/usr/local/openssl/include/openssl/)
LINK_LIBRARIES(event event_pthreads event_openssl)
LINK_LIBRARIES(ssl crypto)
add_executable(hello
        source/log/log.c
        source/log/log.h
        source/server/init_config.h
        source/server/server.c
        source/server/server.h
        source/server/server_main.c
        source/util/cJSON.c
        source/util/cJSON.h
        source/util/parse_json.c
        source/util/parse_json.h
        source/util/threadpool.c
        source/util/threadpool.h
        source/util/tool.c
        source/util/tool.h
        source/main.c source/server/https/test_https.c source/server/https/server_https.c)