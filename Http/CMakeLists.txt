cmake_minimum_required(VERSION 3.10)
project(mymuduo_http)

file(GLOB HTTP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_library(http SHARED ${HTTP_SRC})

# target_link_libraries(${LIBS})

message(STATUS "http build complete!")