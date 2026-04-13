cmake_minimum_required(VERSION 3.10)
project(mymuduo_pool)

file(GLOB POOL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
add_library(pool SHARED ${POOL_SRC})

# target_link_libraries(${LIBS})

message(STATUS "pool build complete!")