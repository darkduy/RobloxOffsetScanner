cmake_minimum_required(VERSION 3.15)
project(RobloxOffsetScanner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0A00 -DNOMINMAX -DWIN32_LEAN_AND_MEAN)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(SOURCES
    src/main.cpp
    src/memory_scanner.cpp
    src/dynamic_finder.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE psapi)
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /O2 /MT)
    target_link_options(${PROJECT_NAME} PRIVATE /OPT:REF /OPT:ICF)
endif()