cmake_minimum_required(VERSION 3.0.0)

project(cppasta)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(FetchContent)
include(cmake/wall.cmake)

if (CPPASTA_ENABLE_ASAN)
  include(cmake/asan.cmake)
endif()

set(SRC
  io.cpp
  math.cpp
  random.cpp
  strings.cpp
  unicode.cpp
)
if (UNIX)
  set(UNIX_SRC
    fd.cpp
  )
  set(SRC ${SRC} ${UNIX_SRC})
endif (UNIX)
list(TRANSFORM SRC PREPEND src/)

add_library(cppasta ${SRC})
target_include_directories(cppasta PUBLIC include)
set_wall(cppasta)

option(CPPASTA_BUILD_TESTS "Build tests" OFF)
if(CPPASTA_BUILD_TESTS)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.6.0
  )
  FetchContent_MakeAvailable(Catch2)

  add_executable(tests tests/unicode.cpp tests/sparsevector.cpp)
  target_link_libraries(tests PRIVATE cppasta)
  target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
endif()