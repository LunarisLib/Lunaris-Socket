cmake_minimum_required(VERSION 3.19)

project(lunaris-example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(lunaris-socket REQUIRED)

add_executable(example main.cpp)

get_target_property(INCLUDES lunaris::lunaris-socket INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "Includes: ${INCLUDES}")

target_link_libraries(example
    PRIVATE
        lunaris::lunaris-socket
)

# Use CMAKE_PREFIX_PATH to point find package to install folder on call!