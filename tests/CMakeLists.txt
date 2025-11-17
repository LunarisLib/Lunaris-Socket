# Enable CTest
enable_testing()

# Dumb VS Code warning fix
file(WRITE ${CMAKE_BINARY_DIR}/DartConfiguration.tcl "")

# Gather all test source files
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Loop through each file
foreach(test_src ${TEST_SOURCES})
    # Extract filename without extension
    get_filename_component(test_name ${test_src} NAME_WE)

    # Create an executable for this test
    add_executable(${test_name} ${test_src})

    # Link the library under test
    target_link_libraries(${test_name} PRIVATE lunaris-socket)
    
    # Tell the test executable AND the linked library to compile with TEST_DEBUG
    target_compile_definitions(${test_name} PRIVATE TEST_DEBUG)

    # Add the test to CTest (command is just the executable)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()