cmake_minimum_required(VERSION 3.28)
project(cpu)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# OpenSSL 찾기
find_package(OpenSSL REQUIRED)

# pkg-config를 사용하여 라이브러리 찾기
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBWEBSOCKETS REQUIRED libwebsockets)
pkg_check_modules(JSON_C REQUIRED json-c)

include_directories(.)
include_directories(assets)
include_directories(docs)
include_directories(include)
include_directories(src)
include_directories(tests)
include_directories(${LIBWEBSOCKETS_INCLUDE_DIRS})
include_directories(${JSON_C_INCLUDE_DIRS})

# OpenSSL include 경로 추가
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

# 소스 파일들 추가
add_executable(
    cpu
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
    ${LIBWEBSOCKETS_LIBRARIES}
    ${JSON_C_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    pthread
)

target_link_directories(cpu PRIVATE
    ${LIBWEBSOCKETS_LIBRARY_DIRS}
    ${JSON_C_LIBRARY_DIRS}
)

target_include_directories(cpu PRIVATE
    ${LIBWEBSOCKETS_INCLUDE_DIRS}
    ${JSON_C_INCLUDE_DIRS}
)

target_compile_options(cpu PRIVATE
    ${LIBWEBSOCKETS_CFLAGS_OTHER}
    ${JSON_C_CFLAGS_OTHER}
)
