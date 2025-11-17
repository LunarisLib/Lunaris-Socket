cmake_minimum_required(VERSION 3.19)

project(lunaris-socket
    VERSION 1.0
    DESCRIPTION "A simple cross-platform socket library in C++"
    LANGUAGES CXX
)

add_library(${PROJECT_NAME}
    src/socket.cpp
    src/tcp_socket.cpp
    src/udp_socket.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build tests" ON)

if (BUILD_TESTS)
    add_subdirectory(tests)
endif()