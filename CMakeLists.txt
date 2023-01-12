cmake_minimum_required(VERSION 3.9)
project(DyDjMatch)

if(NOT CMAKE_BUILD_TYPE)
        message("Defaulting CMAKE_BUILD_TYPE to Release")
        set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# src-directory of the djmatch project
include_directories(
    ${PROJECT_SOURCE_DIR}/src
)

# include argtable
set(ARGTABLE_PATH ${PROJECT_SOURCE_DIR}/src/extern/argtable3-3.0.3)
include_directories(${ARGTABLE_PATH})

# set default path for AlgoraCore, if needed
if(NOT DEFINED ALGORA_CORE_PATH)
    set(ALGORA_CORE_PATH ${PROJECT_SOURCE_DIR}/Algora/AlgoraCore/)
endif()
include_directories(
    ${ALGORA_CORE_PATH}/src/
)
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    link_directories(${ALGORA_CORE_PATH}/build/Debug/)
else()
    link_directories(${ALGORA_CORE_PATH}/build/Release/)
endif()
# set default path for AlgoraDyn, if needed
if(NOT DEFINED ALGORA_DYN_PATH)
    set(ALGORA_DYN_PATH ${PROJECT_SOURCE_DIR}/Algora/AlgoraDyn/)
endif()
include_directories(
    ${ALGORA_DYN_PATH}/src/
)
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    link_directories(${ALGORA_DYN_PATH}/build/Debug/)
else()
    link_directories(${ALGORA_DYN_PATH}/build/Release/)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g -flto")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread  -flto")
set(CMAKE_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -march=native")

set(LIBS
    AlgoraDyn
    AlgoraCore)
set(DJMATCH_SRC
    ${ARGTABLE_PATH}/argtable3.c
    ${PROJECT_SOURCE_DIR}/src/main.cpp)

add_executable(DyDjMatch
               ${DJMATCH_SRC}
               ${PROJECT_SOURCE_DIR}/src/algoraapp_info.h)
target_link_libraries(DyDjMatch PUBLIC ${LIBS})

# write git revision, date, timestamp to algoraapp_info.h
add_custom_target(generate_header ALL
                  COMMAND ./updateInfoHeader src/algoraapp_info.h
                  BYPRODUCTS ${PROJECT_SOURCE_DIR}/src/algoraapp_info.h
                  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                  COMMENT "Generate info header")
add_dependencies(DyDjMatch generate_header)

message(STATUS "CXX Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Linker flags: ${CMAKE_EXE_LINKER_FLAGS}")

if (CMAKE_BUILD_TYPE STREQUAL Release)
    message (STATUS "Release build!")
    message (STATUS "Release CXX Flags: ${CMAKE_CXX_FLAGS_RELEASE}")
endif()

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    message (STATUS "Debug build!")
    message (STATUS "Debug CXX Flags: ${CMAKE_CXX_FLAGS_DEBUG}")
endif()
