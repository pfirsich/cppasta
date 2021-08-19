cmake_minimum_required(VERSION 3.0.0)

project(cppasta)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(cmake/wall.cmake)

if (CPPASTA_ENABLE_ASAN)
  include(cmake/asan.cmake)
endif()

set(SRC
  io.cpp
  math.cpp
  random.cpp
  strings.cpp
  utf8.cpp
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
