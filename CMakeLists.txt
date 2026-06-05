cmake_minimum_required(VERSION 3.15)
project(RobloxOffsetScanner VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Windows specific
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0A00)
    add_definitions(-DNOMINMAX)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    
    # Static runtime
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Executable
add_executable(RobloxOffsetScanner
    src/main.cpp
    src/scanner.cpp
    src/dynamic_scanner.cpp
)

target_include_directories(RobloxOffsetScanner PRIVATE src)

# Link libraries
if(WIN32)
    target_link_libraries(RobloxOffsetScanner
        psapi
        ntdll
        dbghelp
    )
endif()

# Compiler options
if(MSVC)
    target_compile_options(RobloxOffsetScanner PRIVATE /W4 /O2 /MT)
    target_link_options(RobloxOffsetScanner PRIVATE /OPT:REF /OPT:ICF)
else()
    target_compile_options(RobloxOffsetScanner PRIVATE -Wall -Wextra -O3)
endif()

# Install
install(TARGETS RobloxOffsetScanner RUNTIME DESTINATION .)
