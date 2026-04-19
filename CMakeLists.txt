cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_COMPILER clang)

project(cmonkey LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_C_FLAGS_DEBUG
    "-g -O0 -fno-omit-frame-pointer -fsanitize=address,undefined")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG
    "-fsanitize=address,undefined")

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")


# find_package(Curses REQUIRED)

set (SRC_FILES
    src/wordbank.c
    src/wc_imp.c
)

add_executable(main
    src/main.c
    ${SRC_FILES}
)
target_include_directories(main PRIVATE include external)
target_link_libraries(main PRIVATE ${CURSES_LIBRARIES})


add_executable(tests
    tests/test_main.c
    tests/test_json_wordbank_loader.c
    ${SRC_FILES}
)
target_include_directories(tests PRIVATE include tests external)
target_link_libraries(tests)
target_link_libraries(tests PRIVATE ${CURSES_LIBRARIES})


