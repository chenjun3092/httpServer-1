cmake_minimum_required(VERSION 2.7)
project(httpd C)

set(CMAKE_C_STANDARD 11)

include_directories(source/log)
include_directories(source/server)
include_directories(source/util)
include_directories(/usr/local/openssl/include/openssl/)
LINK_LIBRARIES(event event_pthreads )
add_executable(httpd
        source/log/log.c
        source/log/log.h
        source/server/init_config.h
        source/server/server.c
        source/server/server.h
        source/util/cJSON.c
        source/util/cJSON.h
        source/util/parse_json.c
        source/util/parse_json.h
        source/util/threadpool.c
        source/util/threadpool.h
        source/util/tool.c
        source/util/tool.h
        source/util/map.c
        source/util/map.h
        source/main.c)