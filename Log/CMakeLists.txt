cmake_minimum_required(VERSION 3.10)
project(mymuduo_logger)

file(GLOB LOGGER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_library(logger SHARED ${LOGGER_SRC})

# target_link_libraries(${LIBS})

message(STATUS "logger build complete!")