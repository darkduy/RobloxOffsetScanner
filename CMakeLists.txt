cmake_minimum_required(VERSION 3.15)
project(RobloxOffsetScanner VERSION 2.1.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "RobloxOffsetScanner uses Win32 process APIs and must be built on Windows.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/memory_scanner.cpp
    src/memory_scanner.h
    src/dynamic_finder.cpp
    src/dynamic_finder.h
    src/utils.h
)

target_include_directories(${PROJECT_NAME} PRIVATE src)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    _WIN32_WINNT=0x0A00
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)
target_link_libraries(${PROJECT_NAME} PRIVATE psapi)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive- /EHsc)
    target_link_options(${PROJECT_NAME} PRIVATE /OPT:REF /OPT:ICF)
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
