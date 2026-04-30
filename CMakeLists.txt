cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_COMPILER clang)

project(cmonkey LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_EXTENSIONS ON)

# Default to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Debug: sanitizers + no optimisation
set(CMAKE_C_FLAGS_DEBUG
"-g -O0 -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG
"-fsanitize=address -fsanitize=undefined")

# DebugNoSan: debug info, no sanitizers - clean for codelldb
set(CMAKE_C_FLAGS_DEBUGNOSAN
"-g -O0 -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS_DEBUGNOSAN "")

set(VALID_BUILD_TYPES Debug Release DebugNoSan)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${VALID_BUILD_TYPES})

# Release: full optimisation
set(CMAKE_C_FLAGS_RELEASE                           # this flag doesn't work in debug with Asan
"-O3 -DNDEBUG")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")


set (SRC_FILES
    src/wc_imp.c
    src/wordbank.c
    src/buffer.c
    src/draw.c
    src/config.c
    src/cmonkey.c
)

add_executable(main
    src/main.c
    ${SRC_FILES}
)
target_include_directories(main PRIVATE include external)
target_link_libraries(main PRIVATE)


add_executable(tests
    tests/test_main.c
    tests/test_json_wordbank_loader.c
    tests/test_buf_draw.c
    tests/test_timer.c
    ${SRC_FILES}
)
target_include_directories(tests PRIVATE include tests external)
target_link_libraries(tests)
target_link_libraries(tests PRIVATE)


