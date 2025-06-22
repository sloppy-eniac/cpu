cmake_minimum_required(VERSION 3.28)
project(cpu)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# OpenSSL 찾기
find_package(OpenSSL REQUIRED)

# 수동으로 라이브러리 경로 설정
set(LIBWEBSOCKETS_INCLUDE_DIR "/opt/homebrew/Cellar/libwebsockets/4.3.5/include")
set(LIBWEBSOCKETS_LIBRARY "/opt/homebrew/Cellar/libwebsockets/4.3.5/lib/libwebsockets.dylib")
set(JSON_C_INCLUDE_DIR "/opt/homebrew/Cellar/json-c/0.18/include")
set(JSON_C_LIBRARY "/opt/homebrew/Cellar/json-c/0.18/lib/libjson-c.dylib")

include_directories(.)
include_directories(assets)
include_directories(docs)
include_directories(include)
include_directories(src)
include_directories(tests)
include_directories(${LIBWEBSOCKETS_INCLUDE_DIR})
include_directories(${JSON_C_INCLUDE_DIR})

# OpenSSL include 경로 추가
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

# 소스 파일들 추가
add_executable(
    cpu
    docs/comment_guideline.md
    docs/folder_structure.md
    include/memory.h
    src/cpu.c
    src/memory.c
    src/register.c
    src/alu.c
    src/cache.c
    src/instruction.c
    src/interpreter.c
    src/flags.c
    src/websocket_server.c
    src/main.c
)

# 라이브러리 링크
target_link_libraries(cpu 
    ${LIBWEBSOCKETS_LIBRARY}
    ${JSON_C_LIBRARY}
    ${OPENSSL_LIBRARIES}
    pthread
)

# 명시적 라이브러리 경로 추가
target_link_directories(cpu PRIVATE 
    "/opt/homebrew/Cellar/libwebsockets/4.3.5/lib"
    "/opt/homebrew/Cellar/json-c/0.18/lib"
)

target_include_directories(cpu PRIVATE 
    ${LIBWEBSOCKETS_INCLUDE_DIRS}
    ${JSON_C_INCLUDE_DIRS}
)

target_compile_options(cpu PRIVATE 
    ${LIBWEBSOCKETS_CFLAGS_OTHER}
    ${JSON_C_CFLAGS_OTHER}
)
