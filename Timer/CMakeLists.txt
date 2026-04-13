cmake_minimum_required(VERSION 3.10)
project(mymuduo_timer)

file(GLOB TIMER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
add_library(timer SHARED ${TIMER_SRC})

# target_link_libraries(${LIBS})

message(STATUS "timer build complete!")