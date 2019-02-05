cmake_minimum_required(VERSION 2.7)
project(httpd C)

set(CMAKE_C_STANDARD 11)

include_directories(source/log)
include_directories(source/server)
include_directories(source/util)
LINK_LIBRARIES(event event_pthreads )
add_executable(httpd
        source/log/log.c
        source/server/server.c
        source/util/cJSON.c
        source/util/cJSON.h
        source/util/parse_json.c
        source/util/threadpool.c
        source/util/tool.c
        source/util/map.c
        source/main.c source/server/post/post_handler.c)