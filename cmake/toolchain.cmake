cmake_minimum_required(VERSION 3.25)

#  include vcpkg location for toolchain access
include(/usr/vcpkg/scripts/buildsystems/vcpkg.cmake)

list(APPEND COMPILER_OPTS

    # common cxx options
    -Wall
    -Wextra
    -Wuninitialized
    -Wpedantic
    -Wunreachable-code
    $<$<CONFIG:RELEASE>:-Os>
    $<$<CONFIG:DEBUG>:-g>
    $<$<CONFIG:DEBUG>:-O0>
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)