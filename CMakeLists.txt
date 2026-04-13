cmake_minimum_required(VERSION 3.10)
project(mymuduo)

# C++ 11 standard
set(CMAKE_CXX_STANDARD 11)
list(APPEND CMAKE_CXX_FLAGS "-Wall -g")

set(LIB ${PROJECT_SOURCE_DIR}/lib)

set(INC ${PROJECT_SOURCE_DIR}/include)
message(STATUS "include path:${INC}")

set(HTTP ${PROJECT_SOURCE_DIR}/Http)
message(STATUS "HTTP path:${HTTP}")

set(LOGGER ${PROJECT_SOURCE_DIR}/Log)
message(STATUS "LOGGER path:${LOGGER}")

set(POOL ${PROJECT_SOURCE_DIR}/Pool)
message(STATUS "POOL path:${POOL}")

set(SRC ${PROJECT_SOURCE_DIR}/src)
message(STATUS "SRC path:${SRC}")

set(TIMER ${PROJECT_SOURCE_DIR}/Timer)
message(STATUS "TIMER path:${TIMER}")




include_directories(${INC})

# output path
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)



set(LIBS
    pthread
)

add_subdirectory(${HTTP})
add_subdirectory(${LOGGER})
# add_subdirectory(${POOL})
add_subdirectory(${SRC})
add_subdirectory(${TIMER})


message(STATUS "build complete!")