cmake_minimum_required(VERSION 2.7)
project(httpServ C)

set(CMAKE_C_STANDARD 11)

include_directories(source/log)
include_directories(source/server)
include_directories(source/util)
LINK_LIBRARIES(event event_pthreads)

add_executable(httpServ
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
        source/main.c)
