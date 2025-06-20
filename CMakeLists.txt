cmake_minimum_required(VERSION 3.28)
project(cpu)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)
include_directories(assets)
include_directories(docs)
include_directories(include)
include_directories(src)
include_directories(tests)

add_executable(
        cpu
        docs/comment_guideline.md
        docs/folder_structure.md
        include/memory.h
        
        )
