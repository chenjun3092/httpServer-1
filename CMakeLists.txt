cmake_minimum_required(VERSION 2.7)
project(httpd C)

set(CMAKE_C_STANDARD 11)

include_directories(source/log)
include_directories(source/server)
include_directories(source/util)
LINK_LIBRARIES(event event_pthreads)
link_directories(/usr/local/lib)
LINK_LIBRARIES(curl ssl crypto event_openssl)
add_executable(httpd
        source/server/get/get_handler.c
        source/log/log.c
        source/server/server.c
        source/util/cJSON.c
        source/util/parse_json.c
        source/util/threadpool.c
        source/util/tool.c
        source/util/map.c
        source/util/list.c
        source/server/https/https-common.c
        source/main.c source/server/post/post_handler.c
        source/module/markdown_src/markdown_convert/markdown_tohtml.c
        source/module/markdown_src/markdown_parser/markdown_paragraph.c
        source/module/markdown_src/markdown_parser/markdown_parser.c
        source/module/markdown_src/markdown_parser/markdown_sentence.c
        source/module/markdown_src/markdown_token/markdown_token.c
        source/module/markdown_src/main.c
        source/module/markdown_src/markdown_token/markdown_headnodes.c
        source/module/markdown_src/markdown_convert/markdown_html2pdf.c
        source/module/markdown_src/markdown_convert/markdown_time.c
        source/module/markdown_src/markdown_translate/markdown_translate.c source/server/http_component/cookie.c source/server/http_component/cookie.h source/server/http_util.c source/server/http_util.h source/server/mtime.c source/server/mtime.h source/server/http_component/session.c source/server/http_component/session.h)
