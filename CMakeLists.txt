cmake_minimum_required(VERSION 3.10)
project(mymuduo)

set(CMAKE_CXX_STANDARD 11)
set(INC ${PROJECT_SOURCE_DIR}/include)

list(APPEND CMAKE_CXX_FLAGS "-Wall -g")

include_directories(${INC})
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

file(GLOB SRC ${PROJECT_SOURCE_DIR}/src/*.cc)

add_library(mymuduo SHARED ${SRC})
message(STATUS "build complete!")