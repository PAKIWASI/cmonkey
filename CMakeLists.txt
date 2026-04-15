cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_COMPILER clang)

project(cmonkey LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")

# Default to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Debug configuration with sanitizers
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined -lncurses")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-fsanitize=address -fsanitize=undefined")

# Release configuration
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -lncurses")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")


set(SRC_FILES
    src/json_wordbank_loader.c
    src/wc_imp.c
)

add_executable(main
    src/main.c
    ${SRC_FILES}
)


target_include_directories(main PRIVATE include external)


