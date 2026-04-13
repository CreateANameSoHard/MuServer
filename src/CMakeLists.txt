cmake_minimum_required(VERSION 3.10)
project(mymuduo_SRC)

file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
add_library(src SHARED ${SRC})

# target_link_libraries(${LIBS})

message(STATUS "src build complete!")