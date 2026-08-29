# Lunaris Socket Library

This is a pseudo-raw socket library that you can add on your project! It works on both Windows and Linux.

There are some tests to check if it builds correctly and they can be disabled with `BUILD_TESTS OFF`.

## How to add the project to your project

### Using FetchContent

You can create a file like `cmake/installLibrary.cmake` and put in there:

```cmake
include(FetchContent)

FetchContent_Declare(
    lunaris-socket
    GIT_REPOSITORY https://github.com/LunarisLib/Lunaris-Socket.git
    GIT_TAG        (put version here)
)
FetchContent_MakeAvailable(lunaris-socket)
```

This will allow you to download and link the library like:

```cmake
# ...

include(cmake/installLibrary.cmake) # does the FetchContent

target_link_libraries(YourProjectName PRIVATE
    lunaris::lunaris-socket
)
```

### Using find_package()

If you get the install version with the lib and headers and want to avoid recompiling the library yourself, you can do

```cmake
# ...

find_package(lunaris-socket REQUIRED)

target_link_libraries(YourProjectName PRIVATE
    lunaris::lunaris-socket
)
```

The find_package will try to find the `lunaris-socket-config.cmake` or similar files that should be available to download in the Release tab.