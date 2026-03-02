cmake_minimum_required(VERSION 3.19)

project(lunaris-socket
    VERSION "2.0.0"
    DESCRIPTION "A simple socket library"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE PROJECT_SOURCES
     CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

list(
    FILTER PROJECT_SOURCES
    EXCLUDE REGEX ".*/platform_.*\\.cpp$"
)

if (WIN32)
    list(APPEND PROJECT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/platform_windows.cpp")
else()    
    list(APPEND PROJECT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/platform_linux.cpp")
endif()

message(STATUS "Files used for compilation:")
foreach(file IN LISTS PROJECT_SOURCES)
    message(STATUS " - ${file}")
endforeach()

add_library(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        $<$<CONFIG:Debug>:LUNARIS_DEBUG>
        $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=0x0A00>
)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>
)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 mswsock advapi32)
endif()

option(BUILD_TESTS "Build tests" ON)

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

